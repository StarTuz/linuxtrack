#ifndef OBJREADER__H
#define OBJREADER__H
#include <QString>

#include <vector>

typedef struct vtx_s {
  float x, y, z;
  float nx, ny, nz;
  float s, t;
} vtx_t;

typedef struct tri_s {
  int offset;
  int count;
  bool glass;
} tri_t;

typedef struct object_s {
  std::vector<vtx_t> vtx_table;
  std::vector<int> vtx_indices;
  std::vector<tri_t> tris_table;
  QString texture;
} object_t;

#include <QMutex>
extern std::vector<object_t> object_table;
extern QMutex object_table_mutex;

void read_obj();

#endif
