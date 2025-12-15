#include "math_utils.h"
#include <cmath>
#include <algorithm>

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

float vec3_length(const float v[3]) {
    return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

void vec3_sub(const float a[3], const float b[3], float o[3]) {
    o[0] = a[0] - b[0];
    o[1] = a[1] - b[1];
    o[2] = a[2] - b[2];
}

void vec3_add(const float a[3], const float b[3], float o[3]) {
    o[0] = a[0] + b[0];
    o[1] = a[1] + b[1];
    o[2] = a[2] + b[2];
}

void vec3_scale(const float v[3], float s, float o[3]) {
    o[0] = v[0] * s;
    o[1] = v[1] * s;
    o[2] = v[2] * s;
}

float point_to_line_distance(const Vec3& point, const Vec3& lineStart, const Vec3& lineEnd) {
    Vec3 line = lineEnd - lineStart;
    Vec3 toPoint = point - lineStart;
    float lineLen = line.length();
    if (lineLen < 0.0001f) return toPoint.length();
    
    float t = std::max(0.0f, std::min(1.0f, toPoint.dot(line) / (lineLen * lineLen)));
    Vec3 closest = lineStart + line * t;
    return (point - closest).length();
}

float ray_line_distance(const Vec3& rayOrigin, const Vec3& rayDir, 
                        const Vec3& lineStart, const Vec3& lineEnd, float& outDist) {
    Vec3 u = rayDir;
    Vec3 v = lineEnd - lineStart;
    Vec3 w = rayOrigin - lineStart;
    
    float a = u.dot(u);
    float b = u.dot(v);
    float c = v.dot(v);
    float d = u.dot(w);
    float e = v.dot(w);
    float D = a * c - b * b;
    
    float sc, tc;
    
    if (D < 0.0001f) {
        sc = 0.0f;
        tc = (b > c ? d / b : e / c);
    } else {
        sc = (b * e - c * d) / D;
        tc = (a * e - b * d) / D;
    }
    
    sc = std::max(0.0f, sc);
    tc = std::max(0.0f, std::min(1.0f, tc));
    
    Vec3 pointOnRay = rayOrigin + u * sc;
    Vec3 pointOnLine = lineStart + v * tc;
    outDist = (pointOnRay - pointOnLine).length();
    
    return sc;
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

bool world_to_screen(const float worldPos[3], const Mat4& view, const Mat4& proj, 
                     float viewportW, float viewportH, float& screenX, float& screenY) {
    // Transform to clip space
    float pos[4] = { worldPos[0], worldPos[1], worldPos[2], 1.0f };
    
    // Apply view matrix
    float viewPos[4];
    for (int i = 0; i < 4; ++i) {
        viewPos[i] = view.m[i] * pos[0] + view.m[i + 4] * pos[1] + 
                     view.m[i + 8] * pos[2] + view.m[i + 12] * pos[3];
    }
    
    // Apply projection matrix
    float clipPos[4];
    for (int i = 0; i < 4; ++i) {
        clipPos[i] = proj.m[i] * viewPos[0] + proj.m[i + 4] * viewPos[1] + 
                     proj.m[i + 8] * viewPos[2] + proj.m[i + 12] * viewPos[3];
    }
    
    // Behind camera check
    if (clipPos[3] <= 0.0f) return false;
    
    // Perspective divide
    float ndcX = clipPos[0] / clipPos[3];
    float ndcY = clipPos[1] / clipPos[3];
    
    // To screen coordinates
    screenX = (ndcX + 1.0f) * 0.5f * viewportW;
    screenY = (1.0f - ndcY) * 0.5f * viewportH;
    
    return true;
}

void screen_to_world_ray(float screenX, float screenY, float viewportW, float viewportH,
                         const Mat4& view, const Mat4& proj, float rayOrigin[3], float rayDir[3]) {
    // Convert to NDC
    float ndcX = (2.0f * screenX) / viewportW - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY) / viewportH;
    
    // Get camera parameters from projection matrix
    float tanHalfFovy = 1.0f / proj.m[5];
    float aspect = proj.m[5] / proj.m[0];
    
    // Ray in camera space
    float rx = ndcX * aspect * tanHalfFovy;
    float ry = ndcY * tanHalfFovy;
    float rz = -1.0f;
    
    // Extract camera basis from view matrix (transpose of rotation part)
    float right[3] = { view.m[0], view.m[4], view.m[8] };
    float up[3] = { view.m[1], view.m[5], view.m[9] };
    float back[3] = { view.m[2], view.m[6], view.m[10] };
    
    // Transform ray direction to world space
    rayDir[0] = rx * right[0] + ry * up[0] + rz * back[0];
    rayDir[1] = rx * right[1] + ry * up[1] + rz * back[1];
    rayDir[2] = rx * right[2] + ry * up[2] + rz * back[2];
    vec3_normalize(rayDir);
    
    // Camera position (extract from view matrix)
    // view = R * T, so pos = -R^T * translation
    float tx = view.m[12];
    float ty = view.m[13];
    float tz = view.m[14];
    rayOrigin[0] = -(right[0] * tx + up[0] * ty + back[0] * tz);
    rayOrigin[1] = -(right[1] * tx + up[1] * ty + back[1] * tz);
    rayOrigin[2] = -(right[2] * tx + up[2] * ty + back[2] * tz);
}

float ray_circle_distance(const Vec3& rayOrigin, const Vec3& rayDir,
                          const Vec3& center, const Vec3& normal, float radius, float& outT) {
    // Intersect ray with the plane of the circle
    float denom = rayDir.dot(normal);
    outT = 0;
    
    Vec3 toCenter = center - rayOrigin;
    
    // If ray is nearly parallel to plane, check distance to center
    if (std::fabs(denom) < 0.001f) {
        float t = toCenter.dot(rayDir);
        outT = t > 0 ? t : 0;
        return 1e10f; // Can't hit circle edge when parallel
    }
    
    float t = toCenter.dot(normal) / denom;
    outT = t;
    
    if (t < 0) {
        return 1e10f; // Behind camera
    }
    
    // Point where ray hits the plane
    Vec3 hitPoint = rayOrigin + rayDir * t;
    Vec3 fromCenter = hitPoint - center;
    float distFromCenter = fromCenter.length();
    
    // Return distance from the circle's edge
    return std::fabs(distFromCenter - radius);
}
