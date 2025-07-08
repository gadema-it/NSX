#include "navigationtool.h"
#include "../viewport.h"

#include <QMouseEvent>
#include <QWheelEvent>


NavigationTool::NavigationTool(): Tool()
{

}


void NavigationTool::mouseMoveEvent(QMouseEvent *event)
{
    Viewport* viewport = Viewport::getActiveViewport();
    QPoint mouseDelta =  viewport->prevPos - event->pos();
    Camera* camera = &viewport->camera; //TODO application->getActiveViewport()->getActiveCameera();
    switch(event->buttons()) {
        case Qt::MouseButton::LeftButton:
            camera->pan(mouseDelta);
        break;
        case Qt::RightButton:
            camera->orbit(mouseDelta);
        break;
        case Qt::MiddleButton:

            //TODO per zoomToCursor serve salvare target in pressButton
            //TODO zoom quando cursore va verso il centro, da tenere conto del quadrante in cui si trova.
            QVector3D screen_point = QVector3D(event->pos().rx(), viewport->height - event->pos().ry() , 0.0f);
            QVector3D camera_ray_start = screen_point.unproject(camera->view, camera->projection, QRect(0, 0, viewport->width, viewport->height)); //camera near plane projection

            QVector3D camera_ray = (camera->transform.column(3).toVector3D() - camera_ray_start).normalized();
            QVector3D cameraPlaneNormal = (camera->transform.column(3).toVector3D() - camera->translation).normalized();
            qDebug() << "camera->transform" << camera->transform;
            qDebug() << "camera_ray" << camera_ray;
             qDebug() << "cameraPlaneNormal" << camera_ray;

            float distance = QVector3D::dotProduct(cameraPlaneNormal, (camera->translation - camera_ray_start)) / QVector3D::dotProduct(camera_ray, cameraPlaneNormal);

            QVector3D rayPlanePoint = camera_ray_start + distance * camera_ray;
            QVector3D target = camera->translation - rayPlanePoint;

            camera->zoomToCursor(mouseDelta, target);
        break;
    }
}


//per ora controllato da viewport
void NavigationTool::wheelEvent(QWheelEvent *event)
{
//    Camera* camera = &Viewport::getActiveViewport()->camera;
//    if (event->delta() > 0) {
//       camera->zoom(-1);
//    } else {
//       camera->zoom(1);
//    }
}


void NavigationTool::draw(QOpenGLFunctions *f, QVector3D cameraPosition, QMatrix4x4 &viewProjection)
{
    //Draw prev tool if in hold mode
    //if hold mode
    //Viewport::getActiveViewport()->previousTool->draw(*f, cameraPosition, viewProjection)
}


void NavigationTool::mousePressEvent(QMouseEvent *event)
{

    Viewport* viewport = Viewport::getActiveViewport();
    Camera* camera = &viewport->camera; //TODO application->getActiveViewport()->getActiveCameera();

    if (event->buttons() == Qt::RightButton) {
        QVector3D vNewZAxis = camera->rotation.rotatedVector(camera->LocalUp);
        camera->upsideDown = (vNewZAxis[1] < 0);
    } else if (event->buttons() == Qt::MiddleButton) {
        //TODO zoom target
    }

}

void NavigationTool::mouseReleaseEvent(QMouseEvent *event)
{

}


void NavigationTool::activate()
{
}

void NavigationTool::deactivate()
{
}
