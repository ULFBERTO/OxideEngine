#include "scene.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cfloat> // FLT_MAX

// Helper temporario para cargar material desde disco
static Material LoadMaterialHelper(const std::string& filepath) {
    Material material;
    std::ifstream file(filepath);
    std::string line;
    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                if (key == "color") {
                    std::stringstream ss(value);
                    std::string item;
                    int i = 0;
                    while (std::getline(ss, item, ',') && i < 3) {
                        material.color[i] = std::stof(item);
                        i++;
                    }
                } else if (key == "metallic") material.metallic = std::stof(value);
                else if (key == "roughness") material.roughness = std::stof(value);
                else if (key == "emission") material.emission = std::stof(value);
            }
        }
        file.close();
    }
    return material;
}

static bool RayAABB(const float ro[3], const float rd[3], const float bmin[3], const float bmax[3], float& tHit) {
    float tmin = -FLT_MAX;
    float tmax =  FLT_MAX;
    for (int i = 0; i < 3; ++i) {
        const float ro_i = ro[i];
        const float rd_i = rd[i];
        if (std::fabs(rd_i) < 1e-8f) {
            if (ro_i < bmin[i] || ro_i > bmax[i]) return false;
        } else {
            float invD = 1.0f / rd_i;
            float t0 = (bmin[i] - ro_i) * invD;
            float t1 = (bmax[i] - ro_i) * invD;
            if (t0 > t1) std::swap(t0, t1);
            if (t0 > tmin) tmin = t0;
            if (t1 < tmax) tmax = t1;
            if (tmax < tmin) return false;
        }
    }
    tHit = tmin;
    return tmin >= 0.0f; // Simplified, check main definition if needed tmax logic
}

Scene::Scene() {}

Scene::~Scene() {
    if (m_VAO_Grid) glDeleteVertexArrays(1, &m_VAO_Grid);
    if (m_VBO_Grid) glDeleteBuffers(1, &m_VBO_Grid);
    if (m_VAO_Cube) glDeleteVertexArrays(1, &m_VAO_Cube);
    if (m_VBO_Cube) glDeleteBuffers(1, &m_VBO_Cube);
}

void Scene::Init() {
    InitGrid();
    InitCubeResources();
}

void Scene::LoadFromProject(const ProjectData& project) {
    m_Cubes.clear();
    m_Cubes.reserve(project.cubes.size());
    for (const auto& data : project.cubes) {
        CubeInst inst;
        for(int k=0;k<3;k++) {
            inst.pos[k] = data.pos[k];
            inst.rotation[k] = data.rotation[k];
            inst.scale[k] = data.scale[k];
        }
        inst.materialPath = data.materialPath;
        inst.selected = false;
        m_Cubes.push_back(inst);
    }
}

void Scene::GetProjectData(ProjectData& project) const {
    project.cubes.clear();
    project.cubes.reserve(m_Cubes.size());
    for (const auto& inst : m_Cubes) {
        CubeData data;
        for(int k=0;k<3;k++) {
            data.pos[k] = inst.pos[k];
            data.rotation[k] = inst.rotation[k];
            data.scale[k] = inst.scale[k];
        }
        data.materialPath = inst.materialPath;
        project.cubes.push_back(data);
    }
}

int Scene::Raycast(const float ro[3], const float rd[3]) {
    int hitIndex = -1;
    float minT = FLT_MAX;
    
    for (int i = 0; i < (int)m_Cubes.size(); ++i) {
        const auto& c = m_Cubes[i];
        
        // AABB approximation (Unit cube scaled and rotated is hard for AABB)
        // For simplicity, we use a sphere or a conservative AABB?
        // Let's use AABB based on pos +/- 0.5 * max_scale
        float maxScale = std::max({c.scale[0], c.scale[1], c.scale[2]});
        float halfSize = 0.5f * maxScale; // aprox collision box
        
        float bmin[3] = { c.pos[0] - halfSize, c.pos[1] - halfSize, c.pos[2] - halfSize };
        float bmax[3] = { c.pos[0] + halfSize, c.pos[1] + halfSize, c.pos[2] + halfSize };
        
        float t;
        if (RayAABB(ro, rd, bmin, bmax, t)) {
            if (t < minT) {
                minT = t;
                hitIndex = i;
            }
        }
    }
    return hitIndex;
}

void Scene::InitGrid() {
    const int halfExtent = 50;
    const float step = 1.0f;
    std::vector<float> gridVerts;
    gridVerts.reserve((size_t)(halfExtent * 2 + 1) * 4 * 3);
    const float min = -halfExtent * step;
    const float max =  halfExtent * step;
    for (int i = -halfExtent; i <= halfExtent; ++i) {
        float v = i * step;
        gridVerts.push_back(min); gridVerts.push_back(0.0f); gridVerts.push_back(v);
        gridVerts.push_back(max); gridVerts.push_back(0.0f); gridVerts.push_back(v);
        gridVerts.push_back(v);   gridVerts.push_back(0.0f); gridVerts.push_back(min);
        gridVerts.push_back(v);   gridVerts.push_back(0.0f); gridVerts.push_back(max);
    }
    m_GridVertexCount = (int)(gridVerts.size() / 3);
    glGenVertexArrays(1, &m_VAO_Grid);
    glGenBuffers(1, &m_VBO_Grid);
    glBindVertexArray(m_VAO_Grid);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Grid);
    glBufferData(GL_ARRAY_BUFFER, gridVerts.size() * sizeof(float), gridVerts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void Scene::InitCubeResources() {
    const float c[] = {
        // Cara +X
        0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
        0.5f,-0.5f,-0.5f,  0.5f, 0.5f, 0.5f,  0.5f,-0.5f, 0.5f,
        // Cara -X
       -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
       -0.5f,-0.5f,-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f,
        // Cara +Y
       -0.5f, 0.5f,-0.5f, -0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
       -0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,
        // Cara -Y
       -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
       -0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f,
        // Cara +Z
       -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
       -0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        // Cara -Z
       -0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
       -0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f
    };
    m_CubeVertexCount = 36;
    glGenVertexArrays(1, &m_VAO_Cube);
    glGenBuffers(1, &m_VBO_Cube);
    glBindVertexArray(m_VAO_Cube);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Cube);
    glBufferData(GL_ARRAY_BUFFER, sizeof(c), c, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void Scene::ApplyMaterialToShader(const Material& material, GLuint shaderProgram) {
    GLint colorLoc = glGetUniformLocation(shaderProgram, "u_material_color");
    if (colorLoc != -1) glUniform3f(colorLoc, material.color[0], material.color[1], material.color[2]);
    GLint metallicLoc = glGetUniformLocation(shaderProgram, "u_material_metallic");
    if (metallicLoc != -1) glUniform1f(metallicLoc, material.metallic);
    GLint roughnessLoc = glGetUniformLocation(shaderProgram, "u_material_roughness");
    if (roughnessLoc != -1) glUniform1f(roughnessLoc, material.roughness);
    GLint emissionLoc = glGetUniformLocation(shaderProgram, "u_material_emission");
    if (emissionLoc != -1) glUniform1f(emissionLoc, material.emission);
}

Material Scene::GetMaterialFromPath(const std::string& path) {
    if (path.empty()) {
        Material m; // defaul
        m.color[0] = 0.8f; m.color[1] = 0.8f; m.color[2] = 0.8f;
        return m;
    }
    return LoadMaterialHelper(path);
}

void Scene::Render(const Mat4& view, const Mat4& proj, GLuint shaderProgram) {
    glUseProgram(shaderProgram);

    // Uniform locations common
    GLint locMVP = glGetUniformLocation(shaderProgram, "uMVP");
    GLint locColor = glGetUniformLocation(shaderProgram, "uColor");

    Mat4 vp = mat4_mul(proj, view); // Precompute VP

    // 1. Draw Grid
    if (locColor != -1) glUniform4f(locColor, 0.35f, 0.35f, 0.35f, 1.0f);
    Mat4 identity = mat4_identity();
    Mat4 mvpGrid = mat4_mul(vp, identity);
    if (locMVP != -1) glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvpGrid.m);
    
    glBindVertexArray(m_VAO_Grid);
    glDrawArrays(GL_LINES, 0, m_GridVertexCount);
    glBindVertexArray(0);

    // 2. Draw Cubes
    glPolygonMode(GL_FRONT_AND_BACK, m_Wireframe ? GL_LINE : GL_FILL);
    glBindVertexArray(m_VAO_Cube);

    for (const auto& c : m_Cubes) {
        // Transform
        Mat4 model = mat4_identity();
        
        Mat4 scaleM = mat4_identity();
        scaleM.m[0] = c.scale[0]; scaleM.m[5] = c.scale[1]; scaleM.m[10] = c.scale[2];
        
        float radX = c.rotation[0] * 3.1415926f / 180.0f;
        float radY = c.rotation[1] * 3.1415926f / 180.0f;
        float radZ = c.rotation[2] * 3.1415926f / 180.0f;
        
        Mat4 rotX = mat4_identity(); rotX.m[5] = cos(radX); rotX.m[6] = -sin(radX); rotX.m[9] = sin(radX); rotX.m[10] = cos(radX);
        Mat4 rotY = mat4_identity(); rotY.m[0] = cos(radY); rotY.m[2] = sin(radY); rotY.m[8] = -sin(radY); rotY.m[10] = cos(radY);
        Mat4 rotZ = mat4_identity(); rotZ.m[0] = cos(radZ); rotZ.m[1] = -sin(radZ); rotZ.m[4] = sin(radZ); rotZ.m[5] = cos(radZ);
        
        Mat4 rotation = mat4_mul(mat4_mul(rotZ, rotY), rotX);
        model = mat4_mul(rotation, scaleM);
        
        model.m[12] = c.pos[0]; model.m[13] = c.pos[1]; model.m[14] = c.pos[2];
        
        Mat4 mvp = mat4_mul(vp, model);
        if (locMVP != -1) glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvp.m);
        
        // Material
        Material mat = GetMaterialFromPath(c.materialPath);
        
        if (c.selected) {
             glUniform4f(locColor, mat.color[0] * 1.2f, mat.color[1] * 0.8f, mat.color[2] * 0.4f, 1.0f);
        } else {
             glUniform4f(locColor, mat.color[0], mat.color[1], mat.color[2], 1.0f);
        }
        
        ApplyMaterialToShader(mat, shaderProgram);
        glDrawArrays(GL_TRIANGLES, 0, m_CubeVertexCount);
    }
    
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
