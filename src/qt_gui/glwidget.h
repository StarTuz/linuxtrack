#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
// Qt6: Use modern OpenGL with shaders (required for Wayland/EGL)
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#define GLWIDGET_BASE QOpenGLWidget
#else
// Qt5: Use legacy fixed-function OpenGL
#include <QGLWidget>
#define GLWIDGET_BASE QGLWidget
#endif

#include <QThread>

class ReaderThread : public QThread {
  Q_OBJECT
public:
  ReaderThread();
  void run();
signals:
  void done();

private:
};

class GLWidget : public GLWIDGET_BASE
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    ,
                 protected QOpenGLFunctions
#endif
{
  Q_OBJECT

public:
  GLWidget(QWidget *parent = 0);
  ~GLWidget();

  QSize minimumSizeHint() const;
  QSize sizeHint() const;
signals:
  void ready();
public slots:
  void setXRotation(float angle);
  void setYRotation(float angle);
  void setZRotation(float angle);
  void setXTrans(float pos);
  void setYTrans(float pos);
  void setZTrans(float pos);
  void objectsRead();

protected:
  void initializeGL();
  void paintGL();
  void resizeGL(int width, int height);

private:
  bool makeObjects();
  void normalizeAngle(int *angle);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  // Modern OpenGL Resources (Qt6)
  QOpenGLVertexArrayObject vao;
  QOpenGLBuffer vbo;
  QOpenGLShaderProgram *program;
  QOpenGLTexture *texture;

  // Transform Matrices
  QMatrix4x4 projection;

  // Vertex count for drawing
  int vertexCount;

  struct DrawCommand {
    int offset;
    int count;
    QOpenGLTexture *texture;
    bool isGlass;
  };
  std::vector<DrawCommand> drawCommands;

  // Synchronization flags for thread/GL race condition
  bool objectsLoaded;
  bool glInitialized;
#else
  // Legacy OpenGL Resources (Qt5)
  std::vector<GLuint> objects;
#endif

  float xRot;
  float yRot;
  float zRot;
  float xTrans;
  float yTrans;
  float zTrans;

  QColor trolltechPurple;

  ReaderThread *rt;
};

#endif
