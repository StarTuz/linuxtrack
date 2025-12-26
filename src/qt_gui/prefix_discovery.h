#ifndef PREFIX_DISCOVERY_H
#define PREFIX_DISCOVERY_H

#include <QDir>
#include <QList>
#include <QString>
#include <QStringList>

struct DetectedPrefix {
  QString name;
  QString path;
  QString type;
};

class PrefixDiscovery {
public:
  static QList<DetectedPrefix> discover();
  static QStringList discoverXPlane();

private:
  static void scanSteam(QList<DetectedPrefix> &list);
  static void scanLutris(QList<DetectedPrefix> &list);
  static void scanBottles(QList<DetectedPrefix> &list);
  static void scanStardardWine(QList<DetectedPrefix> &list);

  static QString getSteamName(const QString &manifestPath);
};

#endif
