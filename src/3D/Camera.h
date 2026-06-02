#pragma once
#include "../Math.h"
#include <array>

/*
 * MIT License, Based on code with Copyright (c) 2025 Loïc Chen
 */
class Camera
{
public:
    Camera(
        Vector3 eye = { 0.0f, 0.0f, 0.0f },
        Vector3 center = { 0.0f, -1.0f, 0.0f },
        Vector3 up = { 0.0f, 1.0f, 0.0f },
        float fov = Math::Radians(45.0f),
        float aspect = 1.0f,
        float near = 0.1f,
        float far = 500.0f
    ) : _eye(eye), _center(center), _up(up), _fov(fov), _aspect(aspect), _near(near), _far(far) {}

    Vector3 GetEye() const { return _eye; }
    void SetEye(Vector3 position) { _eye = position; }

    Mat4 GetViewMatrix()
    {
        if (_isViewDirty)
        {
            _viewMatrix = Math::LookAt(_eye, _center, _up);
            _isViewDirty = false;
        }
        return _viewMatrix;
    };
    Mat4 GetProjMatrix()
    {
        if (_isProjDirty) {
            _projMatrix = Math::Perspective(_fov, _aspect, _near, _far);
            _isProjDirty = false;
        }
        return _projMatrix;
    }

    void Dolly(float offset);
    void Truck(float offset);
    void Pedestal(float offset);

    void Pan(float angle);
    void Tilt(float angle);
    void Roll(float angle);

    void Orbit(float angle);

    void UpdateAspectRatio(float aspect);

private:
    Vector3 _eye;
    Vector3 _center;
    Vector3 _up;
    float _fov;
    float _aspect;
    float _near;
    float _far;
    bool _isViewDirty = true;
    bool _isProjDirty = true;
    Mat4 _viewMatrix;
    Mat4 _projMatrix;
};
