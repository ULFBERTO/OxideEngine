#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <cmath>

struct Mat4 {
    float m[16];
};

struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 cross(const Vec3& o) const { return Vec3(y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x); }
    float length() const { return std::sqrt(x * x + y * y + z * z); }
    Vec3 normalized() const { float l = length(); return l > 0 ? Vec3(x / l, y / l, z / l) : Vec3(); }
};

Mat4 mat4_identity();
float vec3_dot(const float a[3], const float b[3]);
void vec3_cross(const float a[3], const float b[3], float o[3]);
void vec3_normalize(float v[3]);
float vec3_length(const float v[3]);
void vec3_sub(const float a[3], const float b[3], float o[3]);
void vec3_add(const float a[3], const float b[3], float o[3]);
void vec3_scale(const float v[3], float s, float o[3]);
Mat4 mat4_perspective(float fovy_rad, float aspect, float znear, float zfar);
Mat4 mat4_mul(const Mat4& a, const Mat4& b);
Mat4 create_view_matrix(const float pos[3], const float front[3], const float world_up[3]);

// Gizmo helper: distance from point to line segment
float point_to_line_distance(const Vec3& point, const Vec3& lineStart, const Vec3& lineEnd);

// Ray-line closest point (returns parameter t on ray, and distance)
float ray_line_distance(const Vec3& rayOrigin, const Vec3& rayDir, 
                        const Vec3& lineStart, const Vec3& lineEnd, float& outDist);

// Project world point to screen coordinates
bool world_to_screen(const float worldPos[3], const Mat4& view, const Mat4& proj, 
                     float viewportW, float viewportH, float& screenX, float& screenY);

// Unproject screen to world ray
void screen_to_world_ray(float screenX, float screenY, float viewportW, float viewportH,
                         const Mat4& view, const Mat4& proj, float rayOrigin[3], float rayDir[3]);

// Ray-circle distance (for rotation gizmo detection)
// Returns distance from ray to closest point on circle
// center: circle center, normal: circle plane normal, radius: circle radius
float ray_circle_distance(const Vec3& rayOrigin, const Vec3& rayDir,
                          const Vec3& center, const Vec3& normal, float radius, float& outT);

#endif // MATH_UTILS_H
