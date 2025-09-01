#include "math_utils.h"
#include <cmath>

Mat4 mat4_identity() {
    Mat4 r{};
    for (int i = 0; i < 16; ++i) r.m[i] = 0.0f;
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

float vec3_dot(const float a[3], const float b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void vec3_cross(const float a[3], const float b[3], float o[3]) {
    o[0] = a[1] * b[2] - a[2] * b[1];
    o[1] = a[2] * b[0] - a[0] * b[2];
    o[2] = a[0] * b[1] - a[1] * b[0];
}

void vec3_normalize(float v[3]) {
    float l = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (l > 0.0f) {
        v[0] /= l;
        v[1] /= l;
        v[2] /= l;
    }
}

Mat4 mat4_perspective(float fovy_rad, float aspect, float znear, float zfar) {
    float f = 1.0f / std::tan(fovy_rad / 2.0f);
    Mat4 r{};
    for (int i = 0; i < 16; ++i) r.m[i] = 0.0f;
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (zfar + znear) / (znear - zfar);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * zfar * znear) / (znear - zfar);
    return r;
}

Mat4 mat4_mul(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int c = 0; c < 4; ++c) {
        for (int rI = 0; rI < 4; ++rI) {
            r.m[c * 4 + rI] = a.m[0 * 4 + rI] * b.m[c * 4 + 0] + a.m[1 * 4 + rI] * b.m[c * 4 + 1] + a.m[2 * 4 + rI] * b.m[c * 4 + 2] + a.m[3 * 4 + rI] * b.m[c * 4 + 3];
        }
    }
    return r;
}

Mat4 create_view_matrix(const float pos[3], const float front[3], const float world_up[3]) {
    float zaxis[3] = { -front[0], -front[1], -front[2] }; // -front
    vec3_normalize(zaxis);

    float xaxis[3];
    vec3_cross(world_up, zaxis, xaxis);
    vec3_normalize(xaxis);

    float yaxis[3];
    vec3_cross(zaxis, xaxis, yaxis);

    Mat4 rotation = mat4_identity();
    rotation.m[0] = xaxis[0]; rotation.m[4] = xaxis[1]; rotation.m[8] = xaxis[2];
    rotation.m[1] = yaxis[0]; rotation.m[5] = yaxis[1]; rotation.m[9] = yaxis[2];
    rotation.m[2] = zaxis[0]; rotation.m[6] = zaxis[1]; rotation.m[10] = zaxis[2];

    Mat4 translation = mat4_identity();
    translation.m[12] = -pos[0];
    translation.m[13] = -pos[1];
    translation.m[14] = -pos[2];

    return mat4_mul(rotation, translation);
}
