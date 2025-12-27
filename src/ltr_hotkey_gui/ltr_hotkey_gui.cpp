/*
 * ltr_hotkey_gui - Native hotkey configuration GUI for Linuxtrack
 */

#include "ltr_hotkey_gui.h"

#include <QApplication>
#include <QCloseEvent>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

HotkeyGUI::HotkeyGUI(QWidget *parent)
    : QMainWindow(parent), daemonProcess(nullptr), statusTimer(nullptr) {
  setWindowTitle("Linuxtrack - Native Hotkey Controller");
  setFixedSize(420, 200);

  settings = new QSettings("linuxtrack", "ltr_hotkey_gui");

  setupUI();
  setupTray();
  loadSettings();
  updateHotkeyLabels();

  // Check daemon status periodically
  statusTimer = new QTimer(this);
  connect(statusTimer, &QTimer::timeout, this, &HotkeyGUI::checkDaemonStatus);
  statusTimer->start(2000);
  checkDaemonStatus();
}

HotkeyGUI::~HotkeyGUI() {
  stopDaemon();
  delete settings;
}

void HotkeyGUI::setupUI() {
  QWidget *central = new QWidget(this);
  setCentralWidget(central);

  QVBoxLayout *mainLayout = new QVBoxLayout(central);

  // Hotkey configuration group
  QGroupBox *hotkeyGroup = new QGroupBox("Hotkey Configuration", this);
  QGridLayout *gridLayout = new QGridLayout(hotkeyGroup);

  // Recenter key
  recenterLabel = new QLabel("Recenter: F12", this);
  recenterBtn = new QPushButton("Redefine", this);
  gridLayout->addWidget(recenterLabel, 0, 0);
  gridLayout->addWidget(recenterBtn, 0, 1);

  // Pause key
  pauseLabel = new QLabel("Toggle Pause: Pause", this);
  pauseBtn = new QPushButton("Redefine", this);
  gridLayout->addWidget(pauseLabel, 1, 0);
  gridLayout->addWidget(pauseBtn, 1, 1);

  mainLayout->addWidget(hotkeyGroup);

  // Status
  QHBoxLayout *statusLayout = new QHBoxLayout();
  statusLabel = new QLabel("Status: Not running", this);
  statusLabel->setStyleSheet("font-weight: bold;");
  statusLayout->addWidget(statusLabel);
  mainLayout->addLayout(statusLayout);

  // Bottom buttons
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  startStopBtn = new QPushButton("Start Daemon", this);
  quitBtn = new QPushButton("Quit", this);
  buttonLayout->addStretch();
  buttonLayout->addWidget(startStopBtn);
  buttonLayout->addWidget(quitBtn);
  mainLayout->addLayout(buttonLayout);

  // Connections
  connect(recenterBtn, &QPushButton::clicked, this,
          &HotkeyGUI::onRecenterRedefine);
  connect(pauseBtn, &QPushButton::clicked, this, &HotkeyGUI::onPauseRedefine);
  connect(startStopBtn, &QPushButton::clicked, this,
          &HotkeyGUI::onStartStopClicked);
  connect(quitBtn, &QPushButton::clicked, this, &HotkeyGUI::onQuitClicked);
}

void HotkeyGUI::setupTray() {
  trayIcon = new QSystemTrayIcon(this);
  trayIcon->setIcon(
      QIcon::fromTheme("input-gaming", QIcon(":/icons/linuxtrack.png")));
  trayIcon->setToolTip("Linuxtrack Hotkey Controller");

  QMenu *trayMenu = new QMenu(this);
  trayMenu->addAction("Show", this, [this]() {
    show();
    raise();
    activateWindow();
  });
  trayMenu->addSeparator();
  trayMenu->addAction("Recenter Now", this,
                      [this]() { QProcess::execute("ltr_recenter", {}); });
  trayMenu->addSeparator();
  trayMenu->addAction("Quit", this, &HotkeyGUI::onQuitClicked);
  trayIcon->setContextMenu(trayMenu);

  connect(trayIcon, &QSystemTrayIcon::activated, this,
          &HotkeyGUI::onTrayActivated);
}

void HotkeyGUI::closeEvent(QCloseEvent *event) {
  // Hide to tray instead of closing
  hide();
  trayIcon->show();
  event->ignore();
}

void HotkeyGUI::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
  if (reason == QSystemTrayIcon::DoubleClick ||
      reason == QSystemTrayIcon::Trigger) {
    show();
    raise();
    activateWindow();
    trayIcon->hide();
  }
}

void HotkeyGUI::loadSettings() {
  recenterKey = QKeySequence(settings->value("recenter_key", "F12").toString());
  pauseKey = QKeySequence(settings->value("pause_key", "Pause").toString());
  quitKey = QKeySequence(settings->value("quit_key", "Ctrl+F12").toString());
}

void HotkeyGUI::saveSettings() {
  settings->setValue("recenter_key", recenterKey.toString());
  settings->setValue("pause_key", pauseKey.toString());
  settings->setValue("quit_key", quitKey.toString());
  settings->sync();
}

void HotkeyGUI::updateHotkeyLabels() {
  recenterLabel->setText(QString("Recenter: %1").arg(recenterKey.toString()));
  pauseLabel->setText(QString("Toggle Pause: %1").arg(pauseKey.toString()));
}

void HotkeyGUI::onRecenterRedefine() {
  QKeySequenceEdit *edit = new QKeySequenceEdit(recenterKey, this);
  edit->setWindowFlags(Qt::Dialog);
  edit->setWindowTitle("Press new Recenter key");
  edit->setFixedSize(200, 50);
  edit->show();
  edit->setFocus();

  connect(edit, &QKeySequenceEdit::editingFinished, this, [this, edit]() {
    if (!edit->keySequence().isEmpty()) {
      recenterKey = edit->keySequence();
      updateHotkeyLabels();
      saveSettings();
      // Restart daemon to pick up new key
      if (isDaemonRunning()) {
        stopDaemon();
        startDaemon();
      }
    }
    edit->deleteLater();
  });
}

void HotkeyGUI::onPauseRedefine() {
  QKeySequenceEdit *edit = new QKeySequenceEdit(pauseKey, this);
  edit->setWindowFlags(Qt::Dialog);
  edit->setWindowTitle("Press new Pause key");
  edit->setFixedSize(200, 50);
  edit->show();
  edit->setFocus();

  connect(edit, &QKeySequenceEdit::editingFinished, this, [this, edit]() {
    if (!edit->keySequence().isEmpty()) {
      pauseKey = edit->keySequence();
      updateHotkeyLabels();
      saveSettings();
      if (isDaemonRunning()) {
        stopDaemon();
        startDaemon();
      }
    }
    edit->deleteLater();
  });
}

void HotkeyGUI::onStartStopClicked() {
  if (isDaemonRunning()) {
    stopDaemon();
  } else {
    startDaemon();
  }
  checkDaemonStatus();
}

void HotkeyGUI::onQuitClicked() {
  stopDaemon();
  qApp->quit();
}

void HotkeyGUI::startDaemon() {
  if (daemonProcess && daemonProcess->state() != QProcess::NotRunning) {
    return;
  }

  daemonProcess = new QProcess(this);
  // TODO: Pass custom keys to daemon via arguments or config file
  daemonProcess->start("ltr_hotkeyd", QStringList());
}

void HotkeyGUI::stopDaemon() {
  // Kill any running ltr_hotkeyd
  QProcess::execute("killall", {"ltr_hotkeyd"});
  if (daemonProcess) {
    daemonProcess->terminate();
    daemonProcess->waitForFinished(1000);
    delete daemonProcess;
    daemonProcess = nullptr;
  }
}

bool HotkeyGUI::isDaemonRunning() {
  QProcess pgrep;
  pgrep.start("pgrep", {"-x", "ltr_hotkeyd"});
  pgrep.waitForFinished();
  return pgrep.exitCode() == 0;
}

void HotkeyGUI::checkDaemonStatus() {
  bool running = isDaemonRunning();
  if (running) {
    statusLabel->setText("Status: Running ✓");
    statusLabel->setStyleSheet("font-weight: bold; color: green;");
    startStopBtn->setText("Stop Daemon");
  } else {
    statusLabel->setText("Status: Not running");
    statusLabel->setStyleSheet("font-weight: bold; color: red;");
    startStopBtn->setText("Start Daemon");
  }
}

// MOC include for single-file builds
#include "moc_ltr_hotkey_gui.cpp"
