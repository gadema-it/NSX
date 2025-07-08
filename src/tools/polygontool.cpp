#include "polygontool.h"
#include "viewport.h"
#include "mainwindow.h"
#include "mesh.h"
#include "application.h"

#include <QAction>
#include "qmenubar.h"
#include <application.h>

PolygonTool::PolygonTool()
{
    Application &application = Application::instance();
    QAction *action = new QAction(application.tr("&PolygonTool"));
    application.mainWindow->menuBar()->findChild<QMenu*>("modeling")->addAction(action);
    connect(action, &QAction::triggered,
            this, [this](){Application::instance().activateTool(this);
    });
}


void PolygonTool::draw(QOpenGLFunctions *f, Camera &camera)
{
  //  if (dragging) {
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        //glMultMatrixf((viewProjection * object->local_transform.toMatrix()).data());
        glMultMatrixf(camera.viewProjection.data());
        glPointSize(6.0);
        glBegin(GL_POINTS);
            glColor3f(0.0, 1.0, 0.0);
            glVertex3f(new_point.x(), new_point.y(), new_point.z());
            for(auto &p: points) {
                glVertex3f(p.x(), p.y(), p.z());
            }
        glEnd();
        glPopMatrix();
        glEnable(GL_DEPTH_TEST);
  //  }
}

void PolygonTool::initialize()
{
}

void PolygonTool::activate()
{
    mesh = nullptr;
    state = NONE;
    new_point = QVector3D();

    Application &app = Application::instance();
    if (!app.selection.empty()) {
        Mesh* m = qvariant_cast<Mesh*>(app.selection.at(0));
        if (m) {
            mesh = m;
        }
    }
}

void PolygonTool::deactivate()
{
    mesh = nullptr;
    points.clear();
}

void PolygonTool::mousePressEvent(QMouseEvent *event)
{
//    if (event->button() == Qt::MouseButton::LeftButton) {
//        dragging = true;
//        Viewport *viewport = Viewport::getActiveViewport();
//        QVector3D ray_origin = viewport->camera.view.column(3).toVector3D();
//        QVector3D ray_direction = viewport->unprojectClick(event->pos());
//        new_point =  ray_origin - ray_direction * 10;
//    }

    if (event->button() == Qt::MouseButton::LeftButton) {

        if (state == NONE) {

            //TODO if points == 0 save plane

            points.push_back(new_point);

            if (points.size() == 3) {

                if (mesh == nullptr) {
                    Application &app = Application::instance();

                    Mesh* m = new Mesh();
                    m->setObjectName("Polygon Mesh");
                    m->geometry = new Geometry;
                    m->init();

                    Viewport *viewport = app.mainWindow->viewport;
                    m->setMaterial(viewport->material);
                    viewport->scene.addObject3D(m);
                    viewport->scene.updateObjectList();

                    mesh = m;

                    mesh->selected = true;
                    mesh->selection_type = OBJECT;
                    app.selection.push_back(QVariant::fromValue(mesh));
                    viewport->edit_mode_object = nullptr;
                }

                int face_id = mesh->geometry->addVertexAndFace(points);
                mesh->geometry_dirty = true;
                points.clear();

                state = NEW_POLYGON;
                edge_id = mesh->geometry->half_edges.size()/2 - 1; // last edge
                qDebug() << "edge " << edge_id;
            }
        } else if (state == NEW_POLYGON && edge_id != -1) {
            qDebug() << "new point for " << edge_id;
            int new_vertex = mesh->geometry->splitEdge(edge_id*2, 0.5); // TODO ratio length v1 v2
            Vertex *v = mesh->geometry->vertices.at(new_vertex);
            v->position = new_point;
            qDebug() << "new_vertex " << new_vertex;
            mesh->geometry_dirty = true;

            edge_id = mesh->geometry->half_edges.size()/2 - 1;
        }
    }
}

void PolygonTool::mouseReleaseEvent(QMouseEvent *event)
{

}

void PolygonTool::mouseMoveEvent(QMouseEvent *event)
{

    Viewport *viewport = Viewport::getActiveViewport();
    Camera camera = viewport->camera;
    QVector3D camera_position = camera.transform.column(3).toVector3D();
    QVector3D ray_origin = camera_position - viewport->unprojectClick(event->pos());
    QVector3D ray_direction = (camera_position - ray_origin).normalized();
    QVector3D camera_direction = (camera_position - camera.translation).normalized(); // plane
    float distance = QVector3D::dotProduct(camera_direction, (camera.translation - ray_origin))
            / QVector3D::dotProduct(ray_direction, camera_direction);
    new_point = camera_position + ray_direction * distance;

    //new_point = mesh->global_transform.toMatrix().inverted() * new_point;

    if (state == NEW_POLYGON) {
        std::vector<int> id = viewport->getPickBufferIDs(VERTEX, true, event->pos(), event->pos() + QPoint(1,1), mesh);
        if (!id.empty()) {
            return;
        }

        id = viewport->getPickBufferIDs(EDGE, true, event->pos(), event->pos() + QPoint(1,1), mesh);
        if (!id.empty()) {
            edge_id = id.at(0);
            qDebug() << "edge" << id;
        }


    }

//    float dist1 = QVector3D::dotProduct(s1-v1, normal);
//    float dist2 = QVector3D::dotProduct(s2-v1, normal);
//    if ((dist1*dist2)>= 0.0f)
//        return QVector3D();
//    if (dist1==dist2)        //ray parallel to triangle plane
//        return QVector3D();
//    //intersection between ray and triangle
//    return s1+(s2-s1)*(-dist1/(dist2-dist1));

}

void PolygonTool::registerAction()
{
}
