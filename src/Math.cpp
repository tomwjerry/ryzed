#include "Math.h"

float Math::Radians(float degrees)
{
    return degrees * 3.14159265358979323846f / 180.0f;
}

Mat4 Math::LookAt(Vector3 eye, Vector3 center, Vector3 up)
{
    // Implementation for LookAt matrix
    return Mat4();
}

Mat4 Math::Perspective(float fov, float aspect, float near, float far)
{
    // Implementation for perspective matrix
    return Mat4();
}

Vector3 Math::Rotate(Vector3 v, float radians, Vector3 axis)
{
    // Implementation for rotation
    return Vector3(0.0f, 0.0f, 0.0f);
}

Vector3 Math::Cross(Vector3 a, Vector3 b)
{
    // Implementation for cross product
    return Vector3(0.0f, 0.0f, 0.0f);
}

Vector3 Math::Normalize(Vector3 v)
{
    // Implementation for normalization
    return Vector3(0.0f, 0.0f, 0.0f);
}

float Math::Normalize(Vector3 a, Vector3 b)
{
    // Implementation for normalization
    return 0.0f;
}
