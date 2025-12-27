#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <QDesktopServices>
#include <QHelpContentWidget>
#include <QHelpEngine>
#include <QSettings>
#include <QSplitter>

#include "help_view.h"
#include "help_viewer.h"
#include "ltr_gui_prefs.h"
#include <iostream>

HelpViewer *HelpViewer::hlp = nullptr;

HelpViewer &HelpViewer::getHlp() {
  if (hlp == nullptr) {
    hlp = new HelpViewer();
  }
  return *hlp;
}

void HelpViewer::ShowWindow() {
  std::cerr << "HelpViewer: ShowWindow() called" << std::endl;
  getHlp().show();
  std::cerr << "HelpViewer: ShowWindow() finished" << std::endl;
}

void HelpViewer::RaiseWindow() {
  getHlp().raise();
  getHlp().activateWindow();
}

void HelpViewer::ChangePage(QString name) {
  std::cerr << "HelpViewer: ChangePage(" << name.toStdString() << ") called"
            << std::endl;
  getHlp().ChangeHelpPage(name);
  std::cerr << "HelpViewer: ChangePage finished" << std::endl;
}

void HelpViewer::ChangeHelpPage(QString name) {
  QString tmp =
      QString::fromUtf8("qthelp://uglyDwarf.com.linuxtrack.1.0/doc/help/") +
      name;
  viewer->setSource(QUrl(tmp));
}

void HelpViewer::CloseWindow() { getHlp().close(); }

void HelpViewer::LoadPrefs(QSettings &settings) {
  HelpViewer &hv = getHlp();
  settings.beginGroup(QString::fromUtf8("HelpWindow"));
  hv.resize(
      settings.value(QString::fromUtf8("size"), QSize(800, 600)).toSize());
  hv.move(settings.value(QString::fromUtf8("pos"), QPoint(0, 0)).toPoint());
  settings.endGroup();
}

void HelpViewer::StorePrefs(QSettings &settings) {
  HelpViewer &hv = getHlp();
  settings.beginGroup(QString::fromUtf8("HelpWindow"));
  settings.setValue(QString::fromUtf8("size"), hv.size());
  settings.setValue(QString::fromUtf8("pos"), hv.pos());
  settings.endGroup();
}

HelpViewer::HelpViewer(QWidget *parent) : QWidget(parent) {
  std::cerr << "HelpViewer: Constructor start" << std::endl;
  ui.setupUi(this);
  setWindowTitle(QString::fromUtf8("Help viewer"));

  QString helpFile = PREF.getDataPath(QString::fromUtf8("/help/") +
                                      QString::fromUtf8(HELP_BASE) +
                                      QString::fromUtf8("/help.qhc"));
  std::cerr << "HelpViewer: Using help file: " << helpFile.toStdString()
            << std::endl;
  helpEngine = new QHelpEngine(helpFile);
  std::cerr << "HelpViewer: setupData()..." << std::endl;
  helpEngine->setupData();
  std::cerr << "HelpViewer: Getting contentWidget..." << std::endl;
  contents = helpEngine->contentWidget();
  if (contents == nullptr) {
    std::cerr << "HelpViewer: WARNING - contentWidget() is NULL!" << std::endl;
  }
  splitter = new QSplitter();
  std::cerr << "HelpViewer: Creating HelpViewWidget..." << std::endl;
  viewer = new HelpViewWidget(helpEngine, this);
  layout = new QHBoxLayout();
  layout->addWidget(splitter);
  if (contents != nullptr) {
    std::cerr << "HelpViewer: Adding contents to splitter..." << std::endl;
    splitter->addWidget(contents);
  }
  std::cerr << "HelpViewer: Adding viewer to splitter..." << std::endl;
  splitter->addWidget(viewer);
  ui.verticalLayout->insertLayout(0, layout);

  if (contents != nullptr) {
    std::cerr << "HelpViewer: Connecting contents signals..." << std::endl;
    QObject::connect(contents, SIGNAL(linkActivated(const QUrl &)), viewer,
                     SLOT(setSource(const QUrl &)));
    QObject::connect(contents, SIGNAL(clicked(const QModelIndex &)), this,
                     SLOT(itemClicked(const QModelIndex &)));
    QObject::connect(helpEngine->contentModel(), SIGNAL(contentsCreated()),
                     this, SLOT(helpInitialized()));
  }

  QObject::connect(viewer, SIGNAL(anchorClicked(const QUrl &)), this,
                   SLOT(followLink(const QUrl &)));
  viewer->setOpenLinks(false);
  splitter->setStretchFactor(1, 4);
  std::cerr << "HelpViewer: Constructor finished" << std::endl;
}

HelpViewer::~HelpViewer() {
  std::cerr << "HelpViewer: Destructor start" << std::endl;
  ui.verticalLayout->removeItem(layout);
  layout->removeWidget(contents);
  layout->removeWidget(viewer);
  delete (layout);
  // contents is owned by helpEngine
  delete (viewer);
  delete (helpEngine);
  std::cerr << "HelpViewer: Destructor finished" << std::endl;
}

void HelpViewer::itemClicked(const QModelIndex &index) {
  const QHelpContentItem *ci = helpEngine->contentModel()->contentItemAt(index);
  if (ci) {
    viewer->setSource(ci->url());
  }
}

void HelpViewer::helpInitialized() { contents->expandAll(); }

void HelpViewer::on_CloseButton_pressed() { close(); }

void HelpViewer::followLink(const QUrl &url) {
  if (QString::fromUtf8("http").compare(url.scheme(), Qt::CaseInsensitive) ==
      0) {
    QDesktopServices::openUrl(url);
  } else {
    viewer->setSource(url);
  }
}
