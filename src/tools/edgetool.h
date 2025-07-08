#ifndef SPLITEDGETOOL_H
#define SPLITEDGETOOL_H

#include "tool.h"
#include <QVector3D>

class Mesh;
class Camera;



class EdgeTool: Tool
{
    Q_OBJECT

public:
    EdgeTool();

    enum State {
        NONE,
        START_CUT,
        START_KNIFE
    };

    Mesh* mesh = nullptr;

    int start_vertex_id;
    int vertex_id = -1;
    int edge_id = -1;

    State state = NONE;

    void activate() override;
    void deactivate() override;


    QVector3D start_point;
    QVector3D end_point; //TODO array

    QVector3D edgePoint;
    QVector3D edgePoint2;
    bool overEdge;
    float ratio;


    void draw(QOpenGLFunctions *f, Camera &camera) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

    void registerAction();
};

#endif // SPLITEDGETOOL_H
