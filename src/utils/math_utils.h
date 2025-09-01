#ifndef MATH_UTILS_H
#define MATH_UTILS_H

struct Mat4 {
    float m[16];
};

Mat4 mat4_identity();
float vec3_dot(const float a[3], const float b[3]);
void vec3_cross(const float a[3], const float b[3], float o[3]);
void vec3_normalize(float v[3]);
Mat4 mat4_perspective(float fovy_rad, float aspect, float znear, float zfar);
Mat4 mat4_mul(const Mat4& a, const Mat4& b);
Mat4 create_view_matrix(const float pos[3], const float front[3], const float world_up[3]);

#endif // MATH_UTILS_H
