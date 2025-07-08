#include "camera.h"

#include <QVector2D>

const QVector3D Camera::LocalForward(0.0f, 0.0f, -1.0f);
const QVector3D Camera::LocalUp(0.0f, 1.0f, 0.0f);
const QVector3D Camera::LocalRight(1.0f, 0.0f, 0.0f);

Camera::Camera(bool orthogonal, bool orbit_disabled): orthogonal(orthogonal), orbit_disabled(orbit_disabled)
{
    reset();
    //orthoDistance = 10;

}

void Camera::reset()
{
    translation = QVector3D(0, 0, 0);
    if (orthogonal) {
        orthoDistance = 2;
        orbitDistance = 30;
    } else {
        orbitDistance = 15;
        rotation = QQuaternion::fromAxisAndAngle(LocalRight, -10);
    }

    updateView();
}

void Camera::setTranslation(QVector3D translation)
{
    this->translation = translation;
    updateView();
}


//void ortho(QMatrix4x4 &m, float left, float right, float bottom, float top) {
//    float near = 0.0f;
//    float far = 1000.0f;
//    m(0, 0) = 2 / (right - left);
//    m(1, 1) = 2 / (top - bottom);
//    m(2, 2) = -2 / (far - near);
//    m(3, 3) = 1;
//    m(0, 3) = -((right + left) / (right - left));
//    m(1, 3) = -((top + bottom) / (top - bottom));
//    m(2, 3) = -((far + near) / (far - near));
//}


void Camera::updateProjection(int width, int height)
{
    screenRation = width / float(height);
    projection.setToIdentity();
    if (orthogonal) {
        projection.ortho(-orthoDistance*screenRation, orthoDistance*screenRation, -orthoDistance, orthoDistance, 0.01f, 30000.0f);
        //ortho(projection, -orthoSize*screenRation, orthoSize*screenRation, -orthoSize, orthoSize);
    } else {
        projection.perspective(45.0f, screenRation, 0.01f, 30000.0f);
    }
    updateView();
}



void Camera::pan(QPoint mouseDelta)
{
    if (orthogonal) {
        translation += rotation.rotatedVector(QVector3D(mouseDelta.x(), mouseDelta.y()*-1, 0.0f)) * ( orthoDistance * 0.002f + 0.002f); //TODO value on orbitDistance
    } else {
        translation += rotation.rotatedVector(QVector3D(mouseDelta.x(), mouseDelta.y()*-1, 0.0f)) * ( orbitDistance * 0.002f + 0.002f); //TODO value on orbitDistance
    }

    updateView();
}


void Camera::orbit(QPoint mouseDelta)
{
    if (orbit_disabled) return;

    if (upsideDown) {
        mouseDelta.setX(mouseDelta.rx() * - 1);
    }

    rotation = QQuaternion::fromAxisAndAngle(LocalUp, 0.5f * mouseDelta.x()) * rotation;
    rotation = QQuaternion::fromAxisAndAngle(rotation.rotatedVector(LocalRight), 0.5f * mouseDelta.y()) * rotation;
    updateView();
}


void Camera::zoomToCursor(QPoint mouseDelta, QVector3D target)
{  
    float magnitude = ((QVector2D)mouseDelta).length();
    if (magnitude < 0.05f) return; //prevent axis fight

    if (orthogonal) {
        orthoDistance += orthoDistance * ( mouseDelta.y()*-1 + mouseDelta.x()) * 0.01f;
        projection.setToIdentity();
        projection.ortho(-orthoDistance*screenRation, orthoDistance*screenRation, -orthoDistance, orthoDistance, 0.01f, 30000.0f);
    } else {
        orbitDistance += orbitDistance * ( mouseDelta.y()*-1 + mouseDelta.x()) * 0.01f;
        if (orbitDistance < 0.1f) {
            orbitDistance = 0.1f;
            return;
        }
        //TODO zoomToCursor
      //  translation += target * (mouseDelta.x() + mouseDelta.y()) * 0.25f;
    }
    updateView();
}

void Camera::zoomToCursor(short forward, QVector3D target)
{
    if (orthogonal) {
        orthoDistance += forward * (orthoDistance * 0.25f);
        projection.setToIdentity();
        projection.ortho(-orthoDistance*screenRation, orthoDistance*screenRation, -orthoDistance, orthoDistance, 0.01f, 30000.0f);
    } else {
        orbitDistance += forward * (orbitDistance * 0.25f);
        if (orbitDistance < 0.1f) {
            orbitDistance = 0.1f;
            return;
        }
        translation += forward * target * 0.25f;
    }
    updateView();
}

//void Camera::zoom(short direction)
//{
//    //translation += rotation.rotatedVector(LocalForward*direction);
//    orbitDistance += direction * (0.25f + orbitDistance*0.25f);  //TODO value on orbitDistance
//    if (orbitDistance < 0.1f) orbitDistance = 0.1f;
//    updateView();
//}


void Camera::updateView()
{
    transform.setToIdentity();
    transform.translate(translation);
    transform.rotate(rotation);
    transform.translate(0, 0, orbitDistance);

    view = transform.inverted();
    viewProjection = projection * view;
}
