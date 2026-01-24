#include "selectiontool.h"
#include "application.h"
#include "viewport.h"
#include "mesh.h"
#include "object3d.h"
#include "mainwindow.h"
#include "qstatusbar.h"

#include <QPainter>
#include <QPainterPath>
#include <QOpenGLFunctions_2_1>

//using //namespace NSX;

SelectionTool::SelectionTool()
{

}


void SelectionTool::keyPressEvent(QKeyEvent *event)
{
    viewport = Viewport::getActiveViewport();
    std::vector<QVariant> &selection = Application::instance().selection;

    if (event->key() == Qt::Key_Space) {
        selection_type = OBJECT;
        selection_mode = PICK;
        if (viewport->edit_mode_object != nullptr) { // sposta oggetto in edit mode in selection array
            viewport->edit_mode_object->selection_type = OBJECT;
            selection.push_back(QVariant::fromValue(viewport->edit_mode_object));
            viewport->edit_mode_object = nullptr;
        }
    } else {
        if (event->key() == Qt::Key_V) {
            selection_type = VERTEX;
            selection_mode = PICK;
        } else if (event->key() == Qt::Key_C) {
            selection_type = EDGE;
            selection_mode = PICK;
        } else if (event->key() == Qt::Key_X) {
            selection_type = POLYGON;
            selection_mode = PICK;
        }  else if (event->key() == Qt::Key_Z) {
            selection_type = POLYGON;
            selection_mode = DRAG;
        }

        if (viewport->edit_mode_object != nullptr) {
            //TODO reload selected_component_array from component
            viewport->edit_mode_object->selected_component_array.clear();
            viewport->edit_mode_object->selection_type = selection_type;
        } else if (!selection.empty()) {
            Mesh* first_selected = qvariant_cast<Mesh*>(selection.at(0)); // TODO multiple
            if (first_selected) {
                viewport->edit_mode_object = first_selected;
                first_selected->selection_type = selection_type;
                selection.clear();
                //TODO deseleziona altri e recupera selezione
            }
        }
    }
}


void SelectionTool::mousePressEvent(QMouseEvent *event)
{


    viewport = Viewport::getActiveViewport();
    if (event->button() == Qt::LeftButton) {
        dragging = true;
        if (selection_mode == PICK) {
            start_point = event->pos();
            end_point = event->pos();
        }
    }
}


void SelectionTool::mouseReleaseEvent(QMouseEvent *event)
{
    dragging = false;
    if (selection_mode != PICK) return;

    std::vector<QVariant> &selection = Application::instance().selection;//&Application::instance().selection;

    std::vector<int> ids;
    if ((start_point - end_point).manhattanLength() < 2) {
        start_point.setX(start_point.x() - 5);
        start_point.setY(start_point.y() - 5);
        end_point.setX(end_point.x() + 5);
        end_point.setY(end_point.y() + 5);
        ids = viewport->getPickBufferIDs(selection_type, true, start_point, end_point);
    } else {
        ids = viewport->getPickBufferIDs(selection_type, false, start_point, end_point);
    }


    if (!(event->modifiers() & Qt::ShiftModifier)) { // deseleziona se non shift(add to selection)
        if (viewport->edit_mode_object != nullptr) {
            viewport->edit_mode_object->deselectComponents();
        } else {
            for(QVariant &variant: selection) {
                Object3D* o = qvariant_cast<Object3D*>(variant);
                Q_ASSERT(o != nullptr);
                o->selected = false;
            }
            selection.clear();
        }
    }


    if (!ids.empty()) {

        if (viewport->edit_mode_object != nullptr) {
            // component selection

            for(auto &id: ids) {
                viewport->edit_mode_object->selectComponent(id);   
            }
            //qDebug() << "Selected ids" << viewport->edit_mode_object->selected_component_array.at(0);

            // TODO command setSelected(ids) addSelected

        } else {
            // object selection

            for(auto &id: ids) {
                Object3D* object3D = viewport->scene.objectList.at(id);
                object3D->selected = true;
                selection.push_back(QVariant::fromValue(object3D));
            }

            //qDebug() << "Selected ids: " << ids;
        }

        //QString message = "Selected ids: " << ids.;

       // Application::instance().mainWindow->statusBar->showMessage(message, 300);
    }
}


void SelectionTool::mouseMoveEvent(QMouseEvent *event)
{
    if (dragging && event->buttons() == Qt::LeftButton) {
        if (selection_mode == PICK) {
            end_point = event->pos();
             //qDebug() << rectangle_sel_end;
        } else {
            std::vector<int> ids = viewport->getPickBufferIDs(selection_type, false, start_point, end_point);
        }
    }
}


//TODO painter? buffer?
void SelectionTool::draw(QOpenGLFunctions *f, Camera &camera)
{
    Viewport* viewport = Viewport::getActiveViewport();
    if (!dragging) return;

    glPushMatrix();
   // glLoadIdentity();
    glOrtho(0.0, viewport->width, 0.0, viewport->height, -1.0, 1.0);
        glBegin(GL_LINE_LOOP);
            glColor3f(0.0f, 0.0f, 1.0f);
            glVertex2f(start_point.x(), viewport->height - start_point.y());
            glVertex2f(end_point.x(), viewport->height - start_point.y());
            glVertex2f(end_point.x(), viewport->height - end_point.y());
            glVertex2f(start_point.x(), viewport->height - end_point.y());
        glEnd();
     glPopMatrix();
}
