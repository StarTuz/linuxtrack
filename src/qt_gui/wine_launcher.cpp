#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "utils.h"
#include "wine_launcher.h"
#include <QApplication>
#include <iostream>
#include <sstream>

void WineLauncher::envSet(const QString var, const QString val) {
  env.insert(var, val);
  std::ostringstream s;
  s << "    " << var.toUtf8().constData() << "='" << val.toUtf8().constData()
    << "'\n";
  ltr_int_log_message(s.str().c_str());
}

WineLauncher::WineLauncher()
    : winePath(QString::fromUtf8("")), available(false) {
  std::ostringstream s;
  env = QProcessEnvironment::systemEnvironment();

  // Default Proton/Steam compatibility tunables (from user's surgical script)
  env.insert(QString::fromUtf8("PROTON_NO_FSYNC"), QString::fromUtf8("1"));
  env.insert(QString::fromUtf8("PROTON_NO_ESYNC"), QString::fromUtf8("1"));

  if (!check()) {
    envSet(QString::fromUtf8("WINEARCH"), QString::fromUtf8("win32"));
#ifdef DARWIN
    winePath =
        QApplication::applicationDirPath() + QString::fromUtf8("/../wine/bin/");
    QString libPath =
        QApplication::applicationDirPath() + QString::fromUtf8("/../wine/lib/");
    available = true;
    QString path = winePath + QString::fromUtf8(":") +
                   env.value(QString::fromUtf8("PATH"));
    s.str(std::string(""));
    s << "Using internal wine; adjusting env variables:\n";
    ltr_int_log_message(s.str().c_str());
    envSet(QString::fromUtf8("PATH"), path);
    envSet(QString::fromUtf8("WINESERVER"),
           winePath + QString::fromUtf8("wineserver"));
    envSet(QString::fromUtf8("WINELOADER"),
           winePath + QString::fromUtf8("wine"));
    envSet(QString::fromUtf8("WINEDLLPATH"),
           libPath + QString::fromUtf8("wine/fakedlls"));
    envSet(QString::fromUtf8("DYLD_LIBRARY_PATH"), libPath);
    envSet(QString::fromUtf8("DYLD_PRINT_ENV"), QString::fromUtf8("1"));
    envSet(QString::fromUtf8("DYLD_PRINT_LIBRARIES"), QString::fromUtf8("1"));
    envSet(QString::fromUtf8("WINEDEBUG"),
           QString::fromUtf8("+file,+seh,+tid,+process,+rundll,+module"));
#endif
  } else {
    available = true;
  }
  QObject::connect(&wine, &QProcess::finished, this,
                   (void (WineLauncher::*)(
                       int, QProcess::ExitStatus))&WineLauncher::finished);
  QObject::connect(&wine, &QProcess::errorOccurred, this, &WineLauncher::error);
}

WineLauncher::~WineLauncher() {
  QObject::disconnect(&wine, &QProcess::finished, this,
                      (void (WineLauncher::*)(
                          int, QProcess::ExitStatus))&WineLauncher::finished);
  QObject::disconnect(&wine, &QProcess::errorOccurred, this,
                      &WineLauncher::error);
  if (wine.state() != QProcess::NotRunning) {
    wine.waitForFinished(10000);
  }
  if (wine.state() != QProcess::NotRunning) {
    wine.kill();
  }
}

void WineLauncher::setEnv(const QString &var, const QString &val) {
  env.insert(var, val);
}

void WineLauncher::run(const QString &tgt) {
  wine.setProcessEnvironment(env);
  QString program = winePath + QString::fromUtf8("wine");
  QStringList args;
  args << tgt;
  std::ostringstream s;
  s << "Launching wine command: '" << program.toUtf8().constData() << " "
    << tgt.toUtf8().constData() << "'\n";
  ltr_int_log_message(s.str().c_str());
  wine.setProcessChannelMode(QProcess::MergedChannels);
  wine.start(program, args);
  wine.waitForFinished();
}

void WineLauncher::run(const QString &tgt, const QStringList &params) {
  wine.setProcessEnvironment(env);
  QString program = winePath + QString::fromUtf8("wine");
  QStringList args;
  args << tgt << params;
  std::ostringstream s;
  s << "Launching wine command: '" << program.toUtf8().constData() << " "
    << tgt.toUtf8().constData() << " "
    << params.join(QStringLiteral(" ")).toUtf8().constData() << "'\n";
  ltr_int_log_message(s.str().c_str());
  wine.setProcessChannelMode(QProcess::MergedChannels);
  wine.start(program, args);
  wine.waitForFinished();
}

void WineLauncher::finished(int exitCode, QProcess::ExitStatus exitStatus) {
  QString status;
  switch (exitStatus) {
  case QProcess::NormalExit:
    status = QString::fromUtf8("Normal exit");
    break;
  case QProcess::CrashExit:
    status = QString::fromUtf8("Crashed");
    break;
  default:
    status = QString::fromUtf8("Unknown exit status");
    break;
  }
  ltr_int_log_message("Wine finished with exitcode %d (%s).", exitCode,
                      status.toUtf8().constData());
  QString msg(QString::fromUtf8(wine.readAllStandardOutput().constData()));
  std::ostringstream s;
  s << msg.toUtf8().constData() << "\n";
  ltr_int_log_message(s.str().c_str());
  if (exitCode == 0) {
    emit finished(true);
  } else {
    emit finished(false);
  }
}

const QString errorStr(QProcess::ProcessError error) {
  QString reason;
  switch (error) {
  case QProcess::FailedToStart:
    reason = QString::fromUtf8("Failed To Start");
    break;
  case QProcess::Crashed:
    reason = QString::fromUtf8("Crashed");
    break;
  case QProcess::Timedout:
    reason = QString::fromUtf8("Timedout");
    break;
  case QProcess::WriteError:
    reason = QString::fromUtf8("Write Error");
    break;
  case QProcess::ReadError:
    reason = QString::fromUtf8("Read Error");
    break;
  default:
    reason = QString::fromUtf8("Unknown Error");
    break;
  }
  return reason;
}

void WineLauncher::error(QProcess::ProcessError error) {
  QString msg(QString::fromUtf8(wine.readAllStandardOutput().constData()));
  QString reason = errorStr(error);
  ltr_int_log_message("Error launching wine(%s)!", reason.toUtf8().constData());
  std::ostringstream s;
  s << msg.toUtf8().constData() << "\n";
  ltr_int_log_message(s.str().c_str());
  emit finished(false);
}

bool WineLauncher::wineAvailable() { return available; }

bool WineLauncher::check() {
  run(QString::fromUtf8("--version"));
  // run() already calls waitForFinished(), so we just check the result
  if (wine.state() == QProcess::NotRunning && wine.exitCode() == 0) {
    return true;
  }
  return false;
}

/*
void PluginInstall::installWinePlugin()
{
  gui.pushButton_2->setEnabled(false);
  QString prefix = QFileDialog::getExistingDirectory(nullptr, QString("Select
Wine Prefix..."), QDir::homePath()+"/.wine", QFileDialog::ShowDirsOnly); QString
installerPath = PREF.getDataPath("linuxtrack-wine.exe");

  QString program = "/bin/bash";
  QString arg = QString("WINEPREFIX=") + prefix + " wine " + installerPath;
  QStringList args;
  args << "-c" << arg;
  inst->start(program, args);
}

void Extractor::extractFirmware(QString file)
{
  QString cmd("bash -c \"wine ");
  cmd += QString("%1\"").arg(file);
  progress(QString("Initializing wine and running installer %1").arg(file));
  wine->start(cmd);
}
*/

#include "moc_wine_launcher.cpp"
