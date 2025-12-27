#include <QApplication>
#include <QSurfaceFormat>

#include "ltr_gui.h"
#include "utils.h"
#include <locale.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  ltr_int_check_root();
  ltr_int_log_message("Starting ltr_gui\n");
  setenv("LC_ALL", "C", 1);
  setlocale(LC_ALL, "C");

  // Force Qt6 to use software rendering for RHI to avoid OpenGL context
  // issues with bundled Qt in AppImage. The GLWidget uses its own OpenGL
  // context via QOpenGLWidget, so this only affects Qt's internal rendering.
  setenv("QSG_RHI_BACKEND", "software", 0); // Force software RHI
  setenv("QT_QPA_PLATFORM", "xcb", 0);      // Explicitly use xcb (X11)

  // Set default surface format for our GLWidget's OpenGL compatibility
  QSurfaceFormat format;
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  format.setVersion(3, 0);
  format.setProfile(QSurfaceFormat::CoreProfile);
  QSurfaceFormat::setDefaultFormat(format);

  QLocale::setDefault(QLocale::c());
  QApplication app(argc, argv);
  LinuxtrackGui gui;
  gui.show();
  return app.exec();
}
