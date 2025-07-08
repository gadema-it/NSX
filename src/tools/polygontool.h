#ifndef POLYGONTOOL_H
#define POLYGONTOOL_H

#include "tool.h"

#include <QVector3D>

class Face;
class Mesh;

class PolygonTool: Tool
{
    Q_OBJECT

public:
    PolygonTool();


    enum State {
      NONE,
      NEW_POLYGON
    };

    State state = NONE;
    std::vector<QVector3D> points;
    QVector3D new_point;
    Face *face;
    Mesh *mesh;
    int edge_id = -1;

    bool dragging;

    void draw(QOpenGLFunctions *f, Camera &camera);
    void initialize();
    void activate();
    void deactivate();
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void registerAction();
};

#endif // POLYGONTOOL_H
