#ifndef CAMERA_H
#define CAMERA_H

#include "transform3d.h"

#include <QMatrix4x4>
#include <QObject>

enum Camera_projection {
    PERSPECTIVE,
    ORTHOGRAPHIC
};

class Camera
{
public:
    static const QVector3D LocalUp;
    static const QVector3D LocalForward;
    static const QVector3D LocalRight;

    bool orthogonal;
    bool orbit_disabled;

    Camera(bool orthogonal = false, bool orbit_disabled = false);

    QMatrix4x4 projection;
    QMatrix4x4 view; //->transform
    QMatrix4x4 viewProjection; //TODO getViewProjectionMatrix(); if dirty viewProjection = projection * view.inverted(nullptr);

    QVector3D translation;
    QQuaternion rotation;
    QMatrix4x4 transform;

    //QVector3D pointOfInterest;  //lookat?
    float orbitDistance;
    bool upsideDown;
    float fieldOfView;
    float orthoDistance;
    float screenRation;
    //bool ortho;

    void setTranslation(QVector3D translation);
    void setRotation(QQuaternion rotation);
    void reset();
    void frameObject();

    void updateProjection(int w, int h);
    void updateView();

    void pan(QPoint mouseDelta); //TODO spostare in navigationTool
    void orbit(QPoint mouseDelta);
    void zoom(QPoint mouseDelta);
    void zoom(short forward);
    void zoomToCursor(short forward, QVector3D direction);
    void zoomToCursor(QPoint mouseDelta, QVector3D direction);

};


#endif // CAMERA_H
