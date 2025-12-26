#include "extractor.h"
#include "prefix_discovery_dialog.h"
#include "utils.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <iostream>
#include <ltr_gui_prefs.h>
#include <plugin_install.h>
#include <zlib.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

PluginInstall::PluginInstall(const Ui::LinuxtrackMainForm &ui, QObject *parent)
    : QObject(parent), state(DONE), gui(ui), inst(NULL), dlfw(NULL),
      dlmfc(NULL), poem1(PREF.getRsrcDirPath() +
                         QString::fromUtf8("/tir_firmware/poem1.txt")),
      poem2(PREF.getRsrcDirPath() +
            QString::fromUtf8("/tir_firmware/poem2.txt")),
      gameData(PREF.getRsrcDirPath() +
               QString::fromUtf8("/tir_firmware/gamedata.txt")),
      mfc42u(PREF.getRsrcDirPath() +
             QString::fromUtf8("/tir_firmware/mfc42u.dll")),
      tirViews(PREF.getRsrcDirPath() +
               QString::fromUtf8("/tir_firmware/TIRViews.dll")) {
#ifndef DARWIN
  // Since we now use surgical injection, we don't strictly need the legacy
  // installer exe to be present. We'll enable the buttons and let the
  // surgical logic handle the missing files later if needed.
#endif
  inst = new WineLauncher();
  gui.LinuxtrackWineButton->setEnabled(true);
  Connect();
}

void PluginInstall::close() {
  if (dlfw != NULL) {
    dlfw->close();
  }
  if (dlmfc != NULL) {
    dlmfc->close();
  }
}

PluginInstall::~PluginInstall() {
  if (dlfw != NULL) {
    dlfw->close();
    delete dlfw;
    dlfw = NULL;
  }
  if (dlmfc != NULL) {
    dlmfc->close();
    delete dlmfc;
    dlmfc = NULL;
  }
  delete inst;
}

void PluginInstall::Connect() {
  QObject::connect(gui.LinuxtrackWineButton, SIGNAL(pressed()), this,
                   SLOT(installWinePlugin()));
  QObject::connect(gui.TIRFWButton, SIGNAL(pressed()), this,
                   SLOT(on_TIRFWButton_pressed()));
  //  QObject::connect(gui.TIRViewsButton, SIGNAL(pressed()),
  //    this, SLOT(on_TIRViewsButton_pressed()));
  QObject::connect(inst, SIGNAL(finished(bool)), this, SLOT(finished(bool)));
}

void PluginInstall::on_TIRFWButton_pressed() {
  state = TIR_FW;
  tirFirmwareInstall();
}

/*
void PluginInstall::on_TIRViewsButton_pressed()
{
  state = MFC_ONLY;
  mfc42uInstall();
}
*/

void PluginInstall::installWinePlugin() {
  QMessageBox::StandardButton reply;
  reply = QMessageBox::question(
      NULL, QString::fromUtf8("Linuxtrack-Wine Support"),
      QString::fromUtf8(
          "Do you want to surgically inject the Linuxtrack-Wine bridge into a "
          "prefix?\n\n"
          "Select 'Yes' to proceed with bridge injection (Fast & Modern).\n"
          "Select 'No' to run the Firmware/MFC harvesting wizard "
          "(Optional/Legacy)."),
      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

  if (reply == QMessageBox::Yes) {
    installLinuxtrackWine();
    state = LTR_W;
  } else if (reply == QMessageBox::No) {
    if (!isTirFirmwareInstalled()) {
      state = TIR_FW;
      tirFirmwareInstall();
    } else if (!isMfc42uInstalled()) {
      state = MFC;
      mfc42uInstall();
    } else {
      installLinuxtrackWine();
      state = LTR_W;
    }
  }
}

bool PluginInstall::isTirFirmwareInstalled() {
  return QFile::exists(poem1) && QFile::exists(poem2) &&
         QFile::exists(gameData) && QFile::exists(tirViews);
}

bool PluginInstall::isMfc42uInstalled() { return QFile::exists(mfc42u); }

void PluginInstall::installLinuxtrackWine() {
  if (dlfw != NULL) {
    dlfw->hide();
  }
  if (dlmfc != NULL) {
    dlmfc->hide();
  }
#ifndef DARWIN
  PrefixDiscoveryDialog dlg;
  if (dlg.exec() != QDialog::Accepted)
    return;
  QString prefix = dlg.getSelectedPath();
  if (prefix.isEmpty())
    return;

  // Install to both Program Files paths for 32-bit and 64-bit compatibility
  QStringList instDirs;
  instDirs << prefix + QString::fromUtf8("/drive_c/Program Files/Linuxtrack")
           << prefix +
                  QString::fromUtf8("/drive_c/Program Files (x86)/Linuxtrack");

  for (const QString &instDirBase : instDirs) {
    QDir().mkpath(instDirBase);
  }

  // Surgical Phase 1: Copy binaries directly to prefix
  // We rename .dll.so to .dll so Wine can load them as native Windows DLLs
  struct {
    const char *src;
    const char *dst;
    const char *subdir; // Subdirectory in wine_bridge
  } files[] = {{"NPClient.dll.so", "NPClient.dll", "client"},
               {"NPClient64.dll.so", "NPClient64.dll", "client"},
               {"FreeTrackClient.dll.so", "FreeTrackClient.dll", "ft_client"},
               {NULL, NULL, NULL}};

  // Build a list of potential source directories
  QStringList srcDirs;
  QString appDir = QApplication::applicationDirPath();
  srcDirs << appDir + QString::fromUtf8("/../lib/linuxtrack")
          << appDir + QString::fromUtf8("/../lib")
          << QString::fromUtf8("/opt/linuxtrack/lib/linuxtrack")
          << QString::fromUtf8("/opt/linuxtrack/lib")
          << appDir + QString::fromUtf8("/../share/linuxtrack");

  // Also check the source build directories (for development)
  srcDirs << appDir + QString::fromUtf8("/../../wine_bridge/client")
          << appDir + QString::fromUtf8("/../../wine_bridge/ft_client")
          << appDir + QString::fromUtf8("/../wine_bridge/client")
          << appDir + QString::fromUtf8("/../wine_bridge/ft_client");

  for (int i = 0; files[i].src != NULL; ++i) {
    QString srcPath;

    // Try to find the source file in various locations
    for (const QString &dir : srcDirs) {
      QString candidate =
          dir + QString::fromUtf8("/") + QString::fromUtf8(files[i].src);
      if (QFile::exists(candidate)) {
        srcPath = candidate;
        break;
      }
    }

    // Also try getLibPath as fallback
    if (srcPath.isEmpty()) {
      QString libPath = PREF.getLibPath(QString::fromUtf8(files[i].src));
      if (QFile::exists(libPath)) {
        srcPath = libPath;
      }
    }

    if (!srcPath.isEmpty()) {
      // Copy to both installation directories
      for (const QString &instDirBase : instDirs) {
        QString dstPath = instDirBase + QString::fromUtf8("/") +
                          QString::fromUtf8(files[i].dst);
        if (QFile::exists(dstPath))
          QFile::remove(dstPath);
        if (QFile::copy(srcPath, dstPath)) {
          ltr_int_log_message("Surgically injected %s to %s\n", files[i].src,
                              qPrintable(dstPath));
        } else {
          ltr_int_log_message("Failed to copy %s to %s\n", files[i].src,
                              qPrintable(dstPath));
        }
      }
    } else {
      ltr_int_log_message("Warning: Could not find source file %s\n",
                          files[i].src);
    }
  }

  // Also copy firmware if available - to both directories
  QString fwSrc =
      PREF.getRsrcDirPath() + QString::fromUtf8("/tir_firmware/TIRViews.dll");
  if (QFile::exists(fwSrc)) {
    for (const QString &instDirBase : instDirs) {
      QString fwDst = instDirBase + QString::fromUtf8("/TIRViews.dll");
      if (QFile::exists(fwDst))
        QFile::remove(fwDst);
      QFile::copy(fwSrc, fwDst);
    }
  }

  // Surgical Phase 2: Registry Updates via native wine reg call
  // This bypasses the need for the NSIS installer's UI and Windows runtimes.
  // Set keys for both 32-bit and 64-bit paths for maximum compatibility
  inst->setEnv(QString::fromUtf8("WINEPREFIX"), prefix);

  // NaturalPoint key - 64-bit path
  QStringList npArgs;
  npArgs << QString::fromUtf8("add")
         << QString::fromUtf8(
                "HKCU\\Software\\NaturalPoint\\NATURALPOINT\\NPClient Location")
         << QString::fromUtf8("/v") << QString::fromUtf8("Path")
         << QString::fromUtf8("/t") << QString::fromUtf8("REG_SZ")
         << QString::fromUtf8("/d")
         << QString::fromUtf8("C:\\Program Files\\Linuxtrack\\")
         << QString::fromUtf8("/f");
  inst->run(QString::fromUtf8("reg"), npArgs);

  // NaturalPoint key - 32-bit path (WoW6432Node for 32-bit apps on 64-bit Wine)
  QStringList np32Args;
  np32Args
      << QString::fromUtf8("add")
      << QString::fromUtf8(
             "HKCU\\Software\\NaturalPoint\\NATURALPOINT\\NPClient Location")
      << QString::fromUtf8("/v") << QString::fromUtf8("Path")
      << QString::fromUtf8("/t") << QString::fromUtf8("REG_SZ")
      << QString::fromUtf8("/d")
      << QString::fromUtf8("C:\\Program Files (x86)\\Linuxtrack\\")
      << QString::fromUtf8("/f") << QString::fromUtf8("/reg:32");
  inst->run(QString::fromUtf8("reg"), np32Args);

  // FreeTrack key - 64-bit
  QStringList ftArgs;
  ftArgs << QString::fromUtf8("add")
         << QString::fromUtf8("HKCU\\Software\\Freetrack\\FreetrackClient")
         << QString::fromUtf8("/v") << QString::fromUtf8("Path")
         << QString::fromUtf8("/t") << QString::fromUtf8("REG_SZ")
         << QString::fromUtf8("/d")
         << QString::fromUtf8("C:\\Program Files\\Linuxtrack\\")
         << QString::fromUtf8("/f");
  inst->run(QString::fromUtf8("reg"), ftArgs);

  // FreeTrack key - 32-bit
  QStringList ft32Args;
  ft32Args << QString::fromUtf8("add")
           << QString::fromUtf8("HKCU\\Software\\Freetrack\\FreetrackClient")
           << QString::fromUtf8("/v") << QString::fromUtf8("Path")
           << QString::fromUtf8("/t") << QString::fromUtf8("REG_SZ")
           << QString::fromUtf8("/d")
           << QString::fromUtf8("C:\\Program Files (x86)\\Linuxtrack\\")
           << QString::fromUtf8("/f") << QString::fromUtf8("/reg:32");
  inst->run(QString::fromUtf8("reg"), ft32Args);

  QMessageBox::information(
      NULL, QString::fromUtf8("Surgical Injection Complete"),
      QString::fromUtf8(
          "Linuxtrack support has been surgically injected into the prefix.\n\n"
          "Installed to:\n"
          "  C:\\Program Files\\Linuxtrack\\\n"
          "  C:\\Program Files (x86)\\Linuxtrack\\\n\n"
          "No Windows installers were required."));
#else
  if (isTirFirmwareInstalled() && isMfc42uInstalled()) {
    QMessageBox::information(
        NULL, QString::fromUtf8("Firmware extraction successfull"),
        QString::fromUtf8("Firmware extraction finished successfully!"
                          "\nNow you can install linuxtrack-wine.exe to the "
                          "Wine bottle/prefix of your choice."));
  }
#endif
  gui.LinuxtrackWineButton->setEnabled(true);
}

void PluginInstall::tirFirmwareInstall() {
  if (dlfw == NULL) {
    dlfw = new TirFwExtractor();
    QObject::connect(dlfw, SIGNAL(finished(bool)), this, SLOT(finished(bool)));
  }
  dlfw->show();
}

void PluginInstall::mfc42uInstall() {
  if (!isTirFirmwareInstalled()) {
    QMessageBox::warning(NULL, QString::fromUtf8("Mfc42u install"),
                         QString::fromUtf8("Install TrackIR firmware first!"));
    state = TIR_FW;
    tirFirmwareInstall();
    return;
  }
  if (dlmfc == NULL) {
    dlmfc = new Mfc42uExtractor();
    QObject::connect(dlmfc, SIGNAL(finished(bool)), this, SLOT(finished(bool)));
  }
  dlmfc->show();
}

void PluginInstall::finished(bool ok) {
  (void)ok;
  if (dlfw != NULL) {
    dlfw->hide();
  }
  if (dlmfc != NULL) {
    dlmfc->hide();
  }
  switch (state) {
  case TIR_FW:
    state = MFC;
    mfc42uInstall();
    break;
  case MFC:
    state = LTR_W;
    installLinuxtrackWine();
    break;
  case LTR_W:
  case TIR_FW_ONLY:
  case MFC_ONLY:
  default:
    state = DONE;
    enableButtons(true);
    break;
  }
}

void PluginInstall::enableButtons(bool ena) {
  gui.LinuxtrackWineButton->setEnabled(ena);
  gui.TIRFWButton->setEnabled(ena);
  // gui.TIRViewsButton->setEnabled(ena);
}

#include "moc_plugin_install.cpp"
