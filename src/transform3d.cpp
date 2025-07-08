#include "transform3d.h"

void Transform3D::translate(const QVector3D &dt)
{
    _modified = true;
    _translation += dt;
}

void Transform3D::scale(const QVector3D &ds)
{
  _modified = true;
  _scale *= ds;
}

void Transform3D::rotate(const QQuaternion &dr)
{
  _modified = true;
  _rotation = dr * _rotation;
}

void Transform3D::setTranslation(const QVector3D &t)
{
  _modified = true;
  _translation = t;
}

void Transform3D::setScale(const QVector3D &s)
{
  _modified = true;
  _scale = s;
}

void Transform3D::setRotation(const QQuaternion &r)
{
  _modified = true;
  _rotation = r;
}

//void Transform3D::mulVector(QVector3D &v)
//{
//    if (_modified) {
//    _matrix * v;
//}

const QMatrix4x4 &Transform3D::toMatrix()
{
  if (_modified) {
    _modified = false;
    _matrix.setToIdentity();
    _matrix.translate(_translation);
    _matrix.rotate(_rotation);
    _matrix.scale(_scale);
  }
  return _matrix;
}


// TODO check about skewing
Transform3D& Transform3D::operator*=(const Transform3D &t)
{
    _translation += t._translation;
    _rotation *= t._rotation;
    _scale *= t._scale;
    return *this;
}

const Transform3D operator*(const Transform3D& t1, const Transform3D& t2)
{
    Transform3D t;
    t._translation = t1._translation + t1.rotation() * (t2._translation * t1._scale);
    t._rotation = t1._rotation * t2._rotation;
    t._scale = t1._scale * t2._scale;
    return t;
}

