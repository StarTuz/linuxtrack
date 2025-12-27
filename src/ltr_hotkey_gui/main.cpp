/*
 * ltr_hotkey_gui - main.cpp
 */

#include "ltr_hotkey_gui.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName("ltr_hotkey_gui");
  app.setOrganizationName("linuxtrack");
  app.setQuitOnLastWindowClosed(false); // Stay in tray

  HotkeyGUI window;
  window.show();

  return app.exec();
}
