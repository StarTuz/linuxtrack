#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "glwidget.h"
#include "objreader.h"
#include <QtGui>
#include <QtOpenGL>
#include <cmath>
#include <iostream>
#include <set>

#ifndef DARWIN
#include <GL/glu.h>
#else
#include <glu.h>
#endif

#include "pathconfig.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QFileInfo>

// --- Shader Sources for Modern OpenGL (Qt6) ---
static const char *vertexShaderSource = R"(
    #version 300 es
    in vec3 aPos;
    in vec3 aNormal;
    in vec2 aTexCoord;
    
    uniform mat4 mvp;
    uniform mat3 normalBox;
    
    out vec2 TexCoord;
    out vec3 Normal;
    
    void main() {
        gl_Position = mvp * vec4(aPos, 1.0);
        TexCoord = aTexCoord;
        Normal = normalize(normalBox * aNormal);
    }
)";

static const char *fragmentShaderSource = R"(
    #version 300 es
    precision mediump float;
    in vec2 TexCoord;
    in vec3 Normal;
    
    uniform sampler2D texture1;
    uniform bool useTexture;
    
    out vec4 fragColor;
    
    void main() {
        vec3 lightDir = normalize(vec3(0.5, 0.5, 1.0));
        float diff = max(dot(Normal, lightDir), 0.3);
        vec4 col = vec4(1.0, 1.0, 1.0, 1.0); 
        
        if (useTexture) {
            col = texture(texture1, TexCoord);
        }
        
        fragColor = vec4(col.rgb * diff, col.a);
    }
)";
#endif

// --- Threading ---
ReaderThread::ReaderThread() : QThread() {}

void ReaderThread::run() {
  read_obj();
  emit done();
}

void GLWidget::objectsRead() {
  std::cerr << "GLWidget: objectsRead signal received in thread "
            << QThread::currentThreadId() << "\n";
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  objectsLoaded = true;
  if (glInitialized && program && program->isLinked()) {
    std::cerr << "GLWidget: context initialized, building buffers now.\n";
    if (!isValid()) {
      std::cerr << "GLWidget: not valid yet, deferring.\n";
      return;
    }
    makeCurrent();
    if (makeObjects()) {
      std::cerr << "Buffers built successfully! Vertex count: " << vertexCount
                << "\n";
      update();
    } else {
      std::cerr << "Failed to build buffers!\n";
    }
  } else {
    std::cerr << "OpenGL context not ready yet, will build in initializeGL.\n";
  }
#endif
  emit ready();
}

GLWidget::GLWidget(QWidget *parent)
    : GLWIDGET_BASE(parent), rt(std::make_unique<ReaderThread>())
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      ,
      program(nullptr), texture(nullptr), vertexCount(0), objectsLoaded(false),
      glInitialized(false)
#endif
{
  xRot = 0;
  yRot = 0;
  zRot = 0;
  xTrans = 0;
  yTrans = 0;
  zTrans = 0;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  trolltechPurple = QColor::fromRgbF(0.1, 0.1, 0.15); // Dark blue-gray
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_OpaquePaintEvent);
  setMinimumSize(320, 240);
#else
  trolltechPurple = QColor::fromCmykF(0.0, 0.0, 0.0, 0.0);
#endif

  connect(rt.get(), SIGNAL(done()), this, SLOT(objectsRead()));
  rt->start();
}

GLWidget::~GLWidget() {
  if (rt->isRunning()) {
    rt->wait();
  }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  makeCurrent();
  program.reset();
  delete texture;
  texture = nullptr;
  vbo.destroy();
  vao.destroy();

  // Collect unique textures to avoid double-deletion
  std::set<QOpenGLTexture *> uniqueTextures;
  for (const auto &cmd : drawCommands) {
    if (cmd.texture) {
      uniqueTextures.insert(cmd.texture);
    }
  }
  for (auto tex : uniqueTextures) {
    delete tex;
  }
  drawCommands.clear();
  doneCurrent();
#else
  makeCurrent();
  std::vector<GLuint>::iterator i;
  for (i = objects.begin(); i != objects.end(); ++i) {
    glDeleteLists(*i, 1);
  }
#endif
}

QSize GLWidget::minimumSizeHint() const { return QSize(50, 50); }
QSize GLWidget::sizeHint() const { return QSize(400, 400); }

void GLWidget::setXRotation(float angle) {
  if (angle != xRot) {
    xRot = angle;
  }
}
void GLWidget::setYRotation(float angle) {
  if (angle != yRot) {
    yRot = angle;
  }
}
void GLWidget::setZRotation(float angle) {
  if (angle != zRot) {
    zRot = angle;
  }
}
void GLWidget::setXTrans(float val) { xTrans = val; }
void GLWidget::setYTrans(float val) { yTrans = val; }
void GLWidget::setZTrans(float val) { zTrans = val; }

void GLWidget::initializeGL() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  std::cerr << "GLWidget: initializeGL start" << std::endl;
  initializeOpenGLFunctions();
  std::cerr << "GLWidget: initializeOpenGLFunctions() finished" << std::endl;

  const char *glVersion = (const char *)glGetString(GL_VERSION);
  std::cerr << "GLWidget initializeGL: GL Version: "
            << (glVersion ? glVersion : "NULL") << std::endl;

  // Create Shader Program
  program = std::make_unique<QOpenGLShaderProgram>();

  if (!program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                        vertexShaderSource)) {
    std::cerr << "Vertex Shader FAILED: " << program->log().toStdString()
              << "\n";
  }
  if (!program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                        fragmentShaderSource)) {
    std::cerr << "Fragment Shader FAILED: " << program->log().toStdString()
              << "\n";
  }
  if (!program->link()) {
    std::cerr << "Shader Link FAILED: " << program->log().toStdString() << "\n";
  } else {
    std::cerr << "Shader program linked successfully!\n";
  }

  QColor c = trolltechPurple;
  glClearColor(c.redF(), c.greenF(), c.blueF(), c.alphaF());
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  projection.setToIdentity();
  projection.perspective(55.0f, 1.0f, 0.1f, 500.0f);

  glInitialized = true;

  if (objectsLoaded) {
    if (makeObjects()) {
      std::cerr << "initializeGL: Objects built successfully.\n";
    } else {
      std::cerr << "initializeGL: makeObjects failed!\n";
    }
  } else {
    std::cerr << "initializeGL: Waiting for objects to load...\n";
  }

  update();
#else
  qglClearColor(trolltechPurple.darker());
  makeObjects();
  glShadeModel(GL_FLAT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glEnable(GL_TEXTURE_2D);
#endif
}

void GLWidget::resizeGL(int width, int height) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  projection.setToIdentity();
  projection.perspective(55.0f, (float)width / height, 0.1f, 500.0f);
#else
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(55.0, (double)width / height, 0.1, 45.0);
  glMatrixMode(GL_MODELVIEW);
#endif
}

void GLWidget::paintGL() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (!program || !program->isLinked()) {
    return;
  }
  if (drawCommands.empty()) {
    return;
  }

  program->bind();
  vao.bind();

  QMatrix4x4 model;
  model.rotate(-xRot, 1, 0, 0);
  model.rotate(-yRot, 0, 1, 0);
  model.rotate(-zRot, 0, 0, 1);
  model.translate(-xTrans, -yTrans, -zTrans);
  model.translate(0.0f, -0.7f, -2.3f);

  QMatrix4x4 view;
  QMatrix4x4 mvp = projection * view * model;

  program->setUniformValue("mvp", mvp);
  program->setUniformValue("normalBox", model.normalMatrix());

  for (const auto &cmd : drawCommands) {
    if (cmd.texture) {
      cmd.texture->bind();
      program->setUniformValue("useTexture", true);
    } else {
      program->setUniformValue("useTexture", false);
    }

    if (cmd.isGlass) {
      glEnable(GL_BLEND);
      glDepthMask(GL_FALSE);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glDrawArrays(GL_TRIANGLES, cmd.offset, cmd.count);

    if (cmd.isGlass) {
      glDepthMask(GL_TRUE);
      glDisable(GL_BLEND);
    }

    if (cmd.texture) {
      cmd.texture->release();
    }
  }

  vao.release();
  program->release();
#else
  // Legacy Qt5 rendering
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  glRotated(-xRot, 1.0, 0.0, 0.0);
  glRotated(-yRot, 0.0, 1.0, 0.0);
  glRotated(-zRot, 0.0, 0.0, 1.0);
  glTranslated(-xTrans, -yTrans, -zTrans);

  glPushMatrix();
  glTranslated(0.0, -0.7, -2.265);
  std::vector<GLuint>::const_iterator i;
  for (i = objects.begin(); i != objects.end(); ++i) {
    glCallList(*i);
  }
  glPopMatrix();
#endif
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
// Modern OpenGL makeObjects for Qt6
bool GLWidget::makeObjects() {
  QMutexLocker locker(&object_table_mutex);
  if (object_table.empty())
    return false;

  QVector<float> vertexData;
  drawCommands.clear();

  int currentVertexOffset = 0;

  for (size_t objIdx = 0; objIdx < object_table.size(); ++objIdx) {
    const auto &obj = object_table[objIdx];
    QOpenGLTexture *currentTex = nullptr;
    QString texPath = obj.texture;

    if (!texPath.isEmpty()) {
      QFileInfo checkFile(texPath);
      if (checkFile.exists() && checkFile.isFile()) {
        currentTex = new QOpenGLTexture(QImage(texPath).mirrored());
        if (currentTex) {
          currentTex->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
          currentTex->setMagnificationFilter(QOpenGLTexture::Linear);
          std::cerr << "Loaded texture: " << texPath.toStdString() << "\n";
        }
      } else {
        std::cerr << "Texture not found: " << texPath.toStdString() << "\n";
      }
    }

    for (const auto &tri_group : object_table[objIdx].tris_table) {
      int count = 0;
      for (int i = 0; i < tri_group.count; ++i) {
        int indices_idx = tri_group.offset + i;
        if (indices_idx < 0 || indices_idx >= (int)obj.vtx_indices.size())
          continue;

        int idx = obj.vtx_indices[indices_idx];
        if (idx < 0 || idx >= (int)obj.vtx_table.size()) {
          if (idx > 0 && (idx - 1) < (int)obj.vtx_table.size())
            idx--;
          else
            continue;
        }

        const vtx_t &v = obj.vtx_table[idx];
        vertexData << v.x << v.y << v.z;
        vertexData << v.nx << v.ny << v.nz;
        vertexData << v.s << v.t;
        count++;
      }

      DrawCommand cmd;
      cmd.offset = currentVertexOffset;
      cmd.count = count;
      cmd.texture = currentTex;
      cmd.isGlass = tri_group.glass;
      drawCommands.push_back(cmd);

      currentVertexOffset += count;
    }
  }

  vertexCount = currentVertexOffset;
  std::cerr << "Total vertices: " << vertexCount
            << ", Draw commands: " << drawCommands.size() << "\n";

  if (!vao.create()) {
    std::cerr << "Failed to create VAO\n";
  }
  vao.bind();

  if (!vbo.create()) {
    std::cerr << "Failed to create VBO\n";
  }
  vbo.bind();
  vbo.allocate(vertexData.constData(), vertexData.size() * sizeof(float));

  int stride = 8 * sizeof(float);

  if (program && program->isLinked()) {
    program->bind();
    int posLoc = program->attributeLocation("aPos");
    if (posLoc >= 0) {
      program->enableAttributeArray(posLoc);
      program->setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, stride);
    }
    int normLoc = program->attributeLocation("aNormal");
    if (normLoc >= 0) {
      program->enableAttributeArray(normLoc);
      program->setAttributeBuffer(normLoc, GL_FLOAT, 3 * sizeof(float), 3,
                                  stride);
    }
    int texLoc = program->attributeLocation("aTexCoord");
    if (texLoc >= 0) {
      program->enableAttributeArray(texLoc);
      program->setAttributeBuffer(texLoc, GL_FLOAT, 6 * sizeof(float), 2,
                                  stride);
    }
    program->release();
  }

  vao.release();
  vbo.release();

  return true;
}
#else
// Legacy OpenGL makeObjects for Qt5
bool textured;
object_t obj;

static void make_vortex(int index) {
  vtx_t vtx = obj.vtx_table[index];
  glNormal3f(vtx.nx, vtx.ny, vtx.nz);
  if (textured)
    glTexCoord2f(vtx.s, vtx.t);
  glVertex3f(vtx.x, vtx.y, vtx.z);
}

static void make_triangle(int index1, int index2, int index3) {
  make_vortex(index3);
  make_vortex(index2);
  make_vortex(index1);
}

bool GLWidget::makeObjects() {
  int triangles = 0;
  int cntr;
  int objectsNumber = object_table.size();
  bool textureChanged = false;
  QString currentTexture;

  for (cntr = 0; cntr < objectsNumber; ++cntr) {
    obj = object_table[cntr];
    if (currentTexture.compare(obj.texture) != 0) {
      textureChanged = true;
      currentTexture = obj.texture;
    }

    GLuint list = glGenLists(1);
    objects.push_back(list);
    glNewList(list, GL_COMPILE);

    int i;
    if (!obj.texture.isEmpty()) {
      if (textureChanged) {
        QImage img = QImage(QString(obj.texture))
                         .convertToFormat(QImage::Format_RGBA8888)
                         .mirrored();
        GLuint texId;
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(), 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        textureChanged = false;
      }
      textured = true;
    } else {
      textured = false;
    }

    glBegin(GL_TRIANGLES);
    for (i = 0; i < (int)obj.tris_table.size(); ++i) {
      triangles += obj.tris_table[i].count;
      for (int j = 0; j < obj.tris_table[i].count; ++j) {
        int index = obj.vtx_indices[obj.tris_table[i].offset + j];
        make_vortex(index);
      }
    }
    glEnd();
    glEndList();
  }
  return true;
}
#endif
