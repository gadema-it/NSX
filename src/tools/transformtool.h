#ifndef GIZMOTRANSFORM_H
#define GIZMOTRANSFORM_H

#include "tool.h"
#include "transform3d.h"
#include "geometry.h"

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

class QOpenGLShaderProgram;
class QOpenGLFunctions;

// NSX
class Object3D;
class Viewport;

// edit local transform of an object or component
class TransformTool: public Tool
{
   // Q_OBJECT
public:
    TransformTool();

    enum TransformMode {
        TRANSLATION,
        ROTATION,
        SCALE
    };

    enum components {
        X_AXIS,
        Y_AXIS,
        Z_AXIS,
        XY_AXIS,
        ALL_AXIS
    };

    enum selection_flag { // TODO refactor to only axis
        NO_SELECTION = 0,
        X_AXIS_SELECTED = 1,
        Y_AXIS_SELECTED = 2,
        Z_AXIS_SELECTED = 4,
        CIRCLE_SELECTED = 8,
        ALL_AXIS_SELECTED = X_AXIS_SELECTED | Y_AXIS_SELECTED | Z_AXIS_SELECTED,
        XY_AXIS_SELECTED = X_AXIS_SELECTED | Y_AXIS_SELECTED,
        YZ_AXIS_SELECTED = Y_AXIS_SELECTED | Z_AXIS_SELECTED,
        ZX_AXIS_SELECTED = Z_AXIS_SELECTED | X_AXIS_SELECTED
    };


    TransformMode transform_mode;

    QVector3D selectedColor;

    QVector<QVector3D> vertices;
    std::vector<GLushort> indices;

    QOpenGLVertexArrayObject vertexArrayObject;
    QOpenGLBuffer vertexBuffer;
    QOpenGLBuffer indexBuffer;
    QOpenGLShaderProgram* shaderProgram;

    QOpenGLBuffer rotate_vertex_buffer;

    int u_modelViewProj;
    int u_color;
    int u_point_size;

    boolean active = false;

    boolean translating = false; //TODO  -> dragging
    boolean dragging = false;

    boolean isCursorOver = false;
    boolean has_selection = false;
    boolean selecting = false;

    selection_flag componentSelected = NO_SELECTION;
    selection_flag hide = NO_SELECTION;

    QVector3D start_global_position;
    QVector3D start_local_position;
    Transform3D start_local_transform;
    Transform3D currentTransform; //TODO fix: sarebbe la transform del manipolatore
    //Object3D* currentObject; //3d object,

    std::vector<Vertex> currentVertices; //copy of //TODO id, position

    std::vector<Object3D*> currentObjects;
    std::vector<Transform3D> currentTransforms;

    QVector3D local_translation;

    QVector3D start_ball_rotation;
    QVector2D ball_center;

    QQuaternion start_rotation;

    Tool *selectTool;

    //TODO components

    void initialize();
    void showGizmo();
    void showGizmo(Transform3D position);
    void hideGizmo();
    void updateSize(QMatrix4x4 cameraView);


    void draw(QOpenGLFunctions *f, Camera &camera) override;
    void drawSelection(QOpenGLFunctions *f, Camera &camera) override;

    void activate() override;
    void deactivate() override;

    void startTranslation();
    void updateTranslation(QPoint mouseDelta);
    void endTranslation();
    void abortTranslation();
    void setSelection(int id);
    void update(Viewport *viewport); //TODO fix

    // Tool interface
public:
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
};

#endif // GIZMOTRANSFORM_H

