#include "Math.h"
#include <cmath>

float Math::Radians(float degrees)
{
    return degrees * 3.14159265358979323846f / 180.0f;
}

Mat4 Math::LookAt(Vector3 eye, Vector3 center, Vector3 up)
{
    Vector3 forward = Normalize(center - eye);
    Vector3 right = Normalize(Cross(forward, up));
    Vector3 new_up = Cross(right, forward);
    
    Mat4 result;
    result.m0 = right.x;
    result.m1 = right.y;
    result.m2 = right.z;
    result.m3 = -(right.x * eye.x + right.y * eye.y + right.z * eye.z);
    
    result.m4 = new_up.x;
    result.m5 = new_up.y;
    result.m6 = new_up.z;
    result.m7 = -(new_up.x * eye.x + new_up.y * eye.y + new_up.z * eye.z);
    
    result.m8 = -forward.x;
    result.m9 = -forward.y;
    result.m10 = -forward.z;
    result.m11 = (forward.x * eye.x + forward.y * eye.y + forward.z * eye.z);
    
    result.m12 = 0.0f;
    result.m13 = 0.0f;
    result.m14 = 0.0f;
    result.m15 = 1.0f;
    
    return result;
}

Mat4 Math::Perspective(float fov, float aspect, float near, float far)
{
    float f = 1.0f / std::tan(fov / 2.0f);
    float nf = 1.0f / (near - far);
    
    Mat4 result;
    result.m0 = f / aspect;
    result.m1 = 0.0f;
    result.m2 = 0.0f;
    result.m3 = 0.0f;
    
    result.m4 = 0.0f;
    result.m5 = f;
    result.m6 = 0.0f;
    result.m7 = 0.0f;
    
    result.m8 = 0.0f;
    result.m9 = 0.0f;
    result.m10 = (far + near) * nf;
    result.m11 = (2.0f * far * near) * nf;
    
    result.m12 = 0.0f;
    result.m13 = 0.0f;
    result.m14 = -1.0f;
    result.m15 = 0.0f;
    
    return result;
}

Vector3 Math::Rotate(Vector3 v, float radians, Vector3 axis)
{
    // Normalize the axis
    axis = Normalize(axis);
    
    float cos_theta = std::cos(radians);
    float sin_theta = std::sin(radians);
    
    // Rodrigues' rotation formula: v' = v*cos(θ) + (k × v)*sin(θ) + k*(k·v)*(1-cos(θ))
    float dot = axis.x * v.x + axis.y * v.y + axis.z * v.z;
    Vector3 cross_product = Cross(axis, v);
    
    return v * cos_theta + cross_product * sin_theta + axis * (dot * (1.0f - cos_theta));
}

Vector3 Math::Cross(Vector3 a, Vector3 b)
{
    return Vector3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

Vector3 Math::Normalize(Vector3 v)
{
    float magnitude = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (magnitude == 0.0f) return v;
    return v * (1.0f / magnitude);
}

float Math::Normalize(Vector3 a, Vector3 b)
{
    return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z));
}
