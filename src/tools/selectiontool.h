#ifndef SELECTIONTOOL_H
#define SELECTIONTOOL_H

#include "tool.h"
#include "selection.h"

class Viewport;

class SelectionTool : public Tool, protected QOpenGLFunctions
{
public:
    SelectionTool();

    enum selectionMode {
        PICK,
        DRAG
    };

    bool dragging = false;

    // Tool interface

    Viewport* viewport;

    QPoint start_point;
    QPoint end_point;

    SelectionType selection_type = OBJECT;
    selectionMode selection_mode = PICK;

    void keyPressEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

    void draw(QOpenGLFunctions *f, Camera &camera);
};


#endif // SELECTIONTOOL_H
