#pragma once

struct Vector3
{
    float x;
    float y;
    float z;

    Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vector3 operator*(float scalar) const
    {
        return { x * scalar, y * scalar, z * scalar };
    }
    Vector3 operator-(const Vector3& other) const
    {
        return { x - other.x, y - other.y, z - other.z };
    }
    Vector3 operator+(const Vector3& other) const
    {
        return { x + other.x, y + other.y, z + other.z };
    }
    void operator-=(const Vector3& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
    }
    void operator+=(const Vector3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
    }
};

struct Mat4
{
    float m0;
    float m1;
    float m2;
    float m3;
    float m4;
    float m5;
    float m6;
    float m7;
    float m8;
    float m9;
    float m10;
    float m11;
    float m12;
    float m13;
    float m14;
    float m15;
};

class Math
{
public:
    static float Radians(float degrees);
    static Mat4 LookAt(Vector3 eye, Vector3 center, Vector3 up);
    static Mat4 Perspective(float fov, float aspect, float near, float far);
    static Vector3 Rotate(Vector3 v, float radians, Vector3 axis);
    static Vector3 Cross(Vector3 a, Vector3 b);
    static Vector3 Normalize(Vector3 v);
    static float Normalize(Vector3 a, Vector3 b);
};
