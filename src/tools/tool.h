#ifndef TOOL_H
#define TOOL_H

//#include "viewport.h"

//#include <QObject>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>

class Camera;

class Tool: public QObject
{
    Q_OBJECT
public:
    Tool();
  //  ~Tool();

    //virtual void draw(QOpenGLFunctions *f);
    virtual void draw(QOpenGLFunctions *f, QVector3D cameraPosition, QMatrix4x4& viewProjection);
    virtual void drawSelection(QOpenGLFunctions *f, QVector3D cameraPosition, QMatrix4x4& viewProjection);

    virtual void draw(QOpenGLFunctions *f, Camera &camera) {}
    virtual void drawSelection(QOpenGLFunctions *f, Camera &camera) {}

    virtual void initialize(); // per non caricare tutti i tool subito, controlla se inizializzato quando preso dalla lista?
    //virtual void terminate();
    bool initialized;

    virtual void activate();
    virtual void deactivate();

   // bool event(QEvent *event);
    virtual void keyPressEvent(QKeyEvent *event); //TODO vedere se fare tornare bool
    virtual void keyReleaseEvent(QKeyEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent* event);

    virtual void registerAction() {}
    virtual void registerWidget() {}

};


Q_DECLARE_METATYPE(Tool*)


#endif // TOOL_H
