/*
 * ltr_hotkey_gui - Native hotkey configuration GUI for Linuxtrack
 *
 * Provides a small configuration window similar to Controller.exe
 * for Wine/Proton, but for native Linux games.
 */

#ifndef LTR_HOTKEY_GUI_H
#define LTR_HOTKEY_GUI_H

#include <QKeySequence>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTimer>

class QLabel;
class QPushButton;
class QSettings;
class QProcess;

class HotkeyGUI : public QMainWindow {
  Q_OBJECT

public:
  HotkeyGUI(QWidget *parent = nullptr);
  ~HotkeyGUI();

protected:
  void closeEvent(QCloseEvent *event) override;

private slots:
  void onRecenterRedefine();
  void onPauseRedefine();
  void onQuitClicked();
  void onStartStopClicked();
  void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
  void checkDaemonStatus();

private:
  void setupUI();
  void setupTray();
  void loadSettings();
  void saveSettings();
  void updateHotkeyLabels();
  void startDaemon();
  void stopDaemon();
  bool isDaemonRunning();

  // UI elements
  QLabel *recenterLabel;
  QLabel *pauseLabel;
  QLabel *statusLabel;
  QPushButton *recenterBtn;
  QPushButton *pauseBtn;
  QPushButton *startStopBtn;
  QPushButton *quitBtn;

  // System tray
  QSystemTrayIcon *trayIcon;

  // Settings
  QSettings *settings;
  QKeySequence recenterKey;
  QKeySequence pauseKey;
  QKeySequence quitKey;

  // Daemon process
  QProcess *daemonProcess;
  QTimer *statusTimer;
};

#endif // LTR_HOTKEY_GUI_H
