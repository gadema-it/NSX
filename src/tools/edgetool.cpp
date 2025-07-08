#include "edgetool.h"
#include "viewport.h"
#include "application.h"
#include "qaction.h"
#include "mainwindow.h"
#include "qmenubar.h"
#include "mesh.h"
//#include "surface_mesh/Surface_mesh.h"


void registerPlugin() {


}


EdgeTool::EdgeTool()
{
    Application &application = Application::instance();
    QAction *action = new QAction(application.tr("&EdgeTool"));
    application.mainWindow->menuBar()->findChild<QMenu*>("modeling")->addAction(action);
    connect(action, &QAction::triggered,
            this, [this](){Application::instance().activateTool(this);
    });
}


void EdgeTool::activate()
{
    Application &app = Application::instance();
    state = NONE;
    start_point = QVector3D();
    end_point = QVector3D();

    if (!app.selection.empty()) {
        Mesh* m = qvariant_cast<Mesh*>(app.selection.at(0));
        if (m) {
            mesh = m;
        }
    }
}

void EdgeTool::deactivate()
{
    mesh = nullptr;
}

void EdgeTool::draw(QOpenGLFunctions *f, Camera &camera)
{
    if(edge_id != -1 || vertex_id != -1 || state == State::START_CUT) {
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glMultMatrixf(camera.viewProjection.data());
        glPointSize(6.0);

        //if (state == NONE) {
            glBegin(GL_POINTS);
                glColor3f(0.0, 1.0, 0.0);
                glVertex3f(end_point.x(), end_point.y(), end_point.z());
            glEnd();
        //}

        if (state == State::START_CUT) {
            glBegin(GL_LINES);
                glColor3f(0.0, 1.0, 0.0);
                glVertex3f(start_point.x(), start_point.y(), start_point.z());
                glVertex3f(end_point.x(), end_point.y(), end_point.z());
            glEnd();
        }

        glPopMatrix();
        glEnable(GL_DEPTH_TEST);
    }
}

void EdgeTool::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::RightButton) {
        if (state == NONE) {
            Viewport::getActiveViewport()->setDefaultTool();
            this->deactivate();
        } else if (state == START_CUT){
            state = NONE;
            vertex_id = -1;
            edge_id = -1;
        }
    }
}

void EdgeTool::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton) {
        if (state == START_CUT) {

            if (vertex_id != -1) {
                qDebug() << "CUT" << start_vertex_id << vertex_id;
                Mesh *mesh = Viewport::getActiveViewport()->edit_mode_object;
                int result = mesh->geometry->splitFace(start_vertex_id, vertex_id);
                mesh->geometry_dirty = true;
                qDebug() << "CUT res" << result;
                state = NONE;
                return;
            } else if (edge_id != -1) {
                Mesh *mesh = Viewport::getActiveViewport()->edit_mode_object;
                vertex_id = mesh->geometry->splitEdge(edge_id*2, ratio);
                int result = mesh->geometry->splitFace(start_vertex_id, vertex_id);
                mesh->geometry_dirty = true;
                qDebug() << "CUT res" << result;
                state = NONE;
            }
        }

        if (vertex_id != -1) {
            state = START_CUT;
            start_point = end_point;
            start_vertex_id = vertex_id;
        } else if (edge_id != -1) {
             Mesh *mesh = Viewport::getActiveViewport()->edit_mode_object;
             start_vertex_id = mesh->geometry->splitEdge(edge_id*2, ratio);
             mesh->geometry_dirty = true;
            // qDebug() << "split at" << ratio << "id" << edgeId*2;
             state = START_CUT;
             start_point = end_point;
         } else {
           Viewport *viewport = Viewport::getActiveViewport();
           QVector3D ray_origin = viewport->camera.view.column(3).toVector3D();
           QVector3D ray_direction = viewport->unprojectClick(event->pos());
           start_point = ray_origin - ray_direction * 10;
           state = START_CUT;
        }
    }
}

static QVector3D intersectTriangle(QVector3D ray_direction, QVector3D ray_origin, Mesh *mesh,  int triangle_id) {
     Geometry *geometry = mesh->geometry;

     QVector3D a = mesh->global_transform.toMatrix() * geometry->vertices[geometry->triangles_indices[triangle_id*3]]->position;
     QVector3D b = mesh->global_transform.toMatrix() * geometry->vertices[geometry->triangles_indices[triangle_id*3 + 1]]->position;
     QVector3D c = mesh->global_transform.toMatrix() * geometry->vertices[geometry->triangles_indices[triangle_id*3 + 2]]->position;

     QVector3D v0v1 = b - a;
     QVector3D v0v2 = c - a;
     QVector3D pvec = QVector3D::crossProduct(ray_direction, v0v2);
     double invDet = 1 / QVector3D::dotProduct(v0v1, pvec);

     QVector3D tvec = ray_origin - a;
     double u = QVector3D::dotProduct(tvec, pvec) * invDet;
     if (u < 0 || u > 1)  u = 0; //qDebug() << "U " << u; // FIXME tra un triangolo e l'altro si hanno valori negativi come - 0.00194416

     QVector3D qvec = QVector3D::crossProduct(tvec, v0v1);
     double v = QVector3D::dotProduct(ray_direction, qvec) * invDet;
     if (v < 0 || u + v > 1) v = 0; //qDebug() << "V " << v;

     return b * u + c * v + a * (1 - u - v);
}


void EdgeTool::mouseMoveEvent(QMouseEvent *event)
{
    //qDebug() << start_point;
    Viewport* viewport = Viewport::getActiveViewport();
    Transform3D *global_transform;

     if (0) {

         Mesh *m =  mesh;

         global_transform = &m->global_transform;

         std::vector<int> ids = viewport->getPickBufferIDs(POLYGON, true, event->pos(), event->pos() + QPoint(1,1), mesh); // TODO TEST

         if (ids.empty()) {
             return;
         }

         edge_id = ids[0];

         edgePoint = intersectTriangle(
                     viewport->unprojectClick(event->pos()),
                     viewport->camera.transform.column(3).toVector3D(),
                     mesh,
                     ids[0]
                     );
     }

     if (viewport->edit_mode_object != nullptr) {
        Geometry *geometry = viewport->edit_mode_object->geometry;
        global_transform = &viewport->edit_mode_object->global_transform;

        std::vector<int> ids = viewport->getPickBufferIDs(VERTEX, true, event->pos(), event->pos() + QPoint(1,1));
        if (!ids.empty()) {
            vertex_id = ids[0];
            end_point = global_transform->toMatrix() * geometry->vertices[vertex_id]->position;
            return;
        } else {
            vertex_id = -1;
        }

        ids = viewport->getPickBufferIDs(EDGE, true, event->pos(), event->pos() + QPoint(1,1));
        if (!ids.empty()) {
             edge_id = ids[0];

             HalfEdge *edge = geometry->half_edges[edge_id*2];
             edgePoint = global_transform->toMatrix() * edge->vertex->position;
             edgePoint2 = global_transform->toMatrix() * edge->twin->vertex->position;

             //calcola distanza da v1, pessimo unproject sul near plane ?
//             GLint view[4];
//             glGetIntegerv(GL_VIEWPORT, &view[0]);
//             QRect r = QRect(view[0], view[1], view[2], view[3]);
//             QMatrix4x4 modelView = viewport->camera.view.inverted() * viewport->edit_mode_object->global_transform.toMatrix();
//             QVector3D prj_v1 = edgePoint.project(modelView, viewport->camera.projection, r);
//             QVector3D prj_v2 = edgePoint2.project(modelView, viewport->camera.projection, r);
//             //prj_v1.setZ(0);
//            // prj_v2.setZ(0);
//             QVector3D line = prj_v2 - prj_v1;
//             float length = line.length();
//             line.normalize();
//             QVector3D point = QVector3D(event->x(), viewport->height - event->y(), 1);
//             float ratio = line.dotProduct(line, point - prj_v1 )  /  length;
//             line = edgePoint2 - edgePoint;
//             edgePoint = edgePoint + line * ratio;

             float t0, t1;
             QVector3D click_line = viewport->unprojectClick(event->pos());
             QVector3D line_direction = (edgePoint - edgePoint2);
             float line_lenght = line_direction.length();
             line_direction.normalize();
          //   qDebug() << line_lenght;
             QVector3D q = viewport->camera.transform.column(3).toVector3D() - edgePoint;

             double bDotB = QVector3D::dotProduct(line_direction, line_direction);
             double bDotD = QVector3D::dotProduct(line_direction, click_line);
             double dDotD = QVector3D::dotProduct(click_line, click_line);
             double qDotB = QVector3D::dotProduct( q, line_direction );

             double sqrLengthProduct = bDotB * dDotD;

             double determinant = sqrLengthProduct  -  bDotD * bDotD;

             if ( fabs( determinant )  <  ( sqrLengthProduct * 1.0e-10 ) )
             {
                 //lines are parallel, use any points
                 t0 = 0.0;
                 //t1 = -qDotB / bDotB;
                 //return false;
             }
             else
             {
                 double invDet = 1.0 / determinant;

                 double qDotD = QVector3D::dotProduct( q, click_line );

                 t0 = ( ( dDotD * qDotB )  -  ( bDotD * qDotD ) )  *  invDet;
                // t1 = ( ( bDotD * qDotB )  -  ( bDotB * qDotD ) )  *  invDet;
              //   qDebug() << t0;
             }

             ratio = fabs(t0) / line_lenght;
         //    qDebug() << "ratio" << ratio;
             //edgePoint = edgePoint + t0 * (edgePoint2 - edgePoint);
             end_point = edgePoint + (edgePoint - edgePoint2).normalized() * t0; // FIXME va da 0 a -2 ?
            // qDebug() << edgePoint;
             //edgePoint = t0 * edgePoint + edgePoint2 * (1 - t0);
             return;
            } else {
                edge_id = -1;
            }

            if (state == START_CUT) {
                Viewport *viewport = Viewport::getActiveViewport();
                QVector3D ray_origin = viewport->camera.transform.column(3).toVector3D();
                QVector3D ray_direction = viewport->unprojectClick(event->pos());
                end_point =  ray_origin - ray_direction * 10;
            }
     }
}

void EdgeTool::wheelEvent(QWheelEvent *event)
{
    if (state == START_CUT) {
        Viewport *viewport = Viewport::getActiveViewport();
        QVector3D ray_origin = viewport->camera.transform.column(3).toVector3D();
        QVector3D ray_direction = viewport->unprojectClick(event->pos());
        end_point =  ray_origin - ray_direction * 10;
    }
}

void EdgeTool::registerAction()
{
//    QMenu *test = menuBar()->findChild<QMenu*>("modeling");
//    test->addAction(newWin);
//    qDebug() << test;




}
