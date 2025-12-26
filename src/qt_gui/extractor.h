#ifndef EXTRACTOR__H
#define EXTRACTOR__H

#include "ui_extractor.h"

#include <QDialog>
#include <QProcess>
#include <QThread>
#include <QWidget>

#include "downloading.h"
#include "hashing.h"
#include "progress.h"
#include "wine_launcher.h"
#include <map>
#include <stdint.h>

typedef std::multimap<uint16_t, BlockId> targets_t;
typedef std::multimap<uint16_t, BlockId>::iterator targets_iterator_t;

class TirFwExtractThread : public QThread {
  Q_OBJECT
public:
  TirFwExtractThread() : targets(NULL), gameDataFound(false) {};
  virtual void start(targets_t &t, const QString &p, const QString &d);
  void run();
  void stop() { quit = true; };
  bool haveEverything() { return everything; };
signals:
  void progress(const QString &msg);

private:
  void analyzeFile(const QString fname);
  bool findCandidates(QString name);
  bool allFound();
  targets_t *targets;
  QString path;
  QString destPath;
  bool gameDataFound;
  bool tirviewsFound;
  bool quit;
  bool everything;
};

class Extractor : public QDialog {
  Q_OBJECT
public:
  Extractor(QWidget *parent = 0);
  ~Extractor();

protected:
  Ui::Form ui;
  targets_t targets;
  WineLauncher *wine;
  QString winePrefix;
  Downloading *dl;
  Progress *progressDlg;
  QString destPath;
  bool readSpec();
  bool readSources(const QString &sources);
  QString findSrc(const QString &name);
  virtual void commenceExtraction(QString file) { (void)file; };
  virtual void enableButtons(bool enable);
  virtual void browseDirPressed(const QString &dirName = QString()) {
    (void)dirName;
  };
  bool tryBlob(const QString &file);
signals:
  void finished(bool result);
public slots:
  void show();
protected slots:
  void on_BrowseInstaller_pressed();
  void on_BrowseDir_pressed() { browseDirPressed(); };
  void on_AnalyzeSourceButton_pressed() {};
  void on_DownloadButton_pressed();
  void on_QuitButton_pressed();
  void on_HelpButton_pressed();
  void progress(const QString &msg);
  void threadFinished() {};
  void wineFinished(bool result) { (void)result; };
  void downloadDone(bool ok, QString fileName);
};

class TirFwExtractor : public Extractor {
  Q_OBJECT
public:
  TirFwExtractor(QWidget *parent = 0);
  ~TirFwExtractor();

private:
  void commenceExtraction(QString file);
  void enableButtons(bool enable);

public:
  void browseDirPressed(const QString &dirName = QString()) override;

private:
  TirFwExtractThread *et;
  bool haveSpec;
  bool wineInitialized;
  QString installerFile;

private slots:
  void threadFinished();
  void wineFinished(bool result);
  void on_AnalyzeSourceButton_pressed();
  void on_QuitButton_pressed();
};

// mfc42u.dll is needed by TIRViews.dll
class Mfc42uExtractor : public Extractor {
  Q_OBJECT
public:
  Mfc42uExtractor(QWidget *parent = 0);
  ~Mfc42uExtractor();

private:
  void commenceExtraction(QString file);
  void enableButtons(bool enable);
  int stage;
  QProcess *cabextract;
private slots:
  void wineFinished(bool result);
  void cabextractFinished(int exitCode, QProcess::ExitStatus status);
};

#endif
