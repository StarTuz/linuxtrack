#include <QFile>
#include <QtGlobal>
#include <iostream>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QRegularExpression>
#else
#include <QRegExp>
#endif
#include <QTextStream>

#include "ltr_gui_prefs.h"
#include "objreader.h"
#include "pathconfig.h"

std::vector<object_t> object_table;
QMutex object_table_mutex;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
static void process_line(const QString &line, object_t &obj, bool &glass) {
  static QRegularExpression vt_line(
      "^\\s*VT\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)"
      "\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s*$");
  static QRegularExpression idx10_line(
      "^\\s*IDX10\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)"
      "\\s+"
      "(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s*$");
  static QRegularExpression idx_line("^\\s*IDX\\s+(\\S+)\\s*$");
  static QRegularExpression tris_line("^\\s*TRIS\\s+(\\S+)\\s+(\\S+)\\s*$");
  static QRegularExpression texture_line("^\\s*TEXTURE\\s+(.*)\\s*$");
  static QRegularExpression glass_line("^\\s*GLASS\\s*$");

  QRegularExpressionMatch match;
  match = vt_line.match(line);
  if (match.hasMatch()) {
    float x, y, z, nx, ny, nz, s, t;
    x = match.captured(1).toFloat();
    y = match.captured(2).toFloat();
    z = match.captured(3).toFloat();
    nx = match.captured(4).toFloat();
    ny = match.captured(5).toFloat();
    nz = match.captured(6).toFloat();
    s = match.captured(7).toFloat();
    t = match.captured(8).toFloat();
    obj.vtx_table.push_back((vtx_t){x, y, z, nx, ny, nz, s, t});
  } else if ((match = idx10_line.match(line)).hasMatch()) {
    for (int i = 1; i <= 10; ++i) {
      obj.vtx_indices.push_back(match.captured(i).toInt());
    }
  } else if ((match = idx_line.match(line)).hasMatch()) {
    obj.vtx_indices.push_back(match.captured(1).toInt());
  } else if ((match = tris_line.match(line)).hasMatch()) {
    obj.tris_table.push_back(
        (tri_t){match.captured(1).toInt(), match.captured(2).toInt(), glass});
    glass = false;
  } else if ((match = texture_line.match(line)).hasMatch()) {
    if (!match.captured(1).isEmpty()) {
      obj.texture = PrefProxy::getDataPath(match.captured(1));
    }
  } else if (glass_line.match(line).hasMatch()) {
    glass = true;
  }
}
#else
static void process_line(const QString &line, object_t &obj, bool &glass) {
  static QRegExp vt_line("^\\s*VT\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)"
                         "\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s*$");
  static QRegExp idx10_line(
      "^\\s*IDX10\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)"
      "\\s+"
      "(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s*$");
  static QRegExp idx_line("^\\s*IDX\\s+(\\S+)\\s*$");
  static QRegExp tris_line("^\\s*TRIS\\s+(\\S+)\\s+(\\S+)\\s*$");
  static QRegExp texture_line("^\\s*TEXTURE\\s+(.*)\\s*$");
  static QRegExp glass_line("^\\s*GLASS\\s*$");

  if (vt_line.indexIn(line) != -1) {
    float x, y, z, nx, ny, nz, s, t;
    x = vt_line.cap(1).toFloat();
    y = vt_line.cap(2).toFloat();
    z = vt_line.cap(3).toFloat();
    nx = vt_line.cap(4).toFloat();
    ny = vt_line.cap(5).toFloat();
    nz = vt_line.cap(6).toFloat();
    s = vt_line.cap(7).toFloat();
    t = vt_line.cap(8).toFloat();
    obj.vtx_table.push_back((vtx_t){x, y, z, nx, ny, nz, s, t});
  } else if (idx10_line.indexIn(line) != -1) {
    for (int i = 1; i <= 10; ++i) {
      obj.vtx_indices.push_back(idx10_line.cap(i).toInt());
    }
  } else if (idx_line.indexIn(line) != -1) {
    obj.vtx_indices.push_back(idx_line.cap(1).toInt());
  } else if (tris_line.indexIn(line) != -1) {
    obj.tris_table.push_back(
        (tri_t){tris_line.cap(1).toInt(), tris_line.cap(2).toInt(), glass});
    glass = false;
  } else if (texture_line.indexIn(line) != -1) {
    if (!texture_line.cap(1).isEmpty()) {
      obj.texture = PrefProxy::getDataPath(texture_line.cap(1));
    }
  } else if (glass_line.indexIn(line) != -1) {
    glass = true;
  }
}
#endif

void read_obj() {
  const char *obj_list[] = {"sphere.obj", "sparow_opaq.obj", "sparow_glass.obj",
                            nullptr};

  std::cerr << "read_obj: Starting to load objects...\n";
  std::vector<object_t> new_table;

  for (int i = 0; obj_list[i] != nullptr; ++i) {
    QString path = PrefProxy::getDataPath(QString::fromUtf8(obj_list[i]));
    std::cerr << "read_obj: Trying to load " << obj_list[i] << " from "
              << path.toStdString() << "\n";
    QFile f(path);

    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
      std::cerr << "read_obj: FAILED to open " << path.toStdString() << "\n";
      continue;
    }

    object_t obj;
    bool current_glass = false;
    QTextStream in(&f);
    while (!in.atEnd()) {
      process_line(in.readLine(), obj, current_glass);
    }
    f.close();
    std::cerr << "read_obj: Loaded " << obj_list[i] << " with "
              << obj.vtx_table.size() << " vertices\n";
    new_table.push_back(std::move(obj));
  }

  // Atomically update the global table
  object_table_mutex.lock();
  object_table = std::move(new_table);
  object_table_mutex.unlock();
  std::cerr << "read_obj: Done. Total objects loaded: " << object_table.size()
            << "\n";
}
