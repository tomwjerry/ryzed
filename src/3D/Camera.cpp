#include "Camera.h"
#include <cmath>

void Camera::Dolly(float offset)
{
    Vector3 dir = _center - _eye;
    _eye += dir * offset;
    _center = _eye + dir;
    _isViewDirty = true;
}

void Camera::Truck(float offset)
{
    Vector3 dir = _center - _eye;
    _eye += Math::Normalize(Math::Cross(dir, Vector3(0.0f, 1.0f, 0.0f))) * offset;
    _center = _eye + dir;
    _isViewDirty = true;
}

void Camera::Pedestal(float offset)
{
    _eye += Vector3(0.0f, offset, 0.0f);
    _center += Vector3(0.0f, offset, 0.0f);
    _isViewDirty = true;
}

void Camera::Pan(float radians)
{
    Vector3 dir = _center - _eye;
    dir = Math::Rotate(dir, radians, Vector3(0.0f, 1.0f, 0.0f));
    _center = _eye + dir;
    _isViewDirty = true;
}

void Camera::Tilt(float radians)
{
    Vector3 dir = _center - _eye;
    Vector3 right = Math::Normalize(Math::Cross(dir, Vector3(0.0f, 1.0f, 0.0f)));
    dir = Math::Rotate(dir, radians, right);
    if (std::abs(Math::Normalize(Math::Normalize(dir), Vector3(0.0f, 1.0f, 0.0f))) < 1.0f) {
        _center = _eye + dir;
        _isViewDirty = true;
    }
}

void Camera::Roll(float radians)
{
    Vector3 dir = _center - _eye;
    _up = Math::Rotate(_up, radians, dir);
    _isViewDirty = true;
}

void Camera::Orbit(float radians)
{
    Vector3 dir = _eye - _center;
    dir = Math::Rotate(dir, radians, Vector3(0.0f, 1.0f, 0.0f));
    _eye = _center + dir;
    _isViewDirty = true;
}

void Camera::UpdateAspectRatio(float aspect)
{
    _aspect = aspect;
    _isProjDirty = true;
}
