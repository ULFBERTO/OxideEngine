#include "scene.h"
#include <algorithm>
#include <cfloat> // FLT_MAX
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

// Helper temporario para cargar material desde disco
static Material LoadMaterialHelper(const std::string &filepath) {
  Material material;
  std::ifstream file(filepath);
  std::string line;
  if (file.is_open()) {
    while (std::getline(file, line)) {
      if (line.empty() || line[0] == '#')
        continue;
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
        } else if (key == "metallic")
          material.metallic = std::stof(value);
        else if (key == "roughness")
          material.roughness = std::stof(value);
        else if (key == "emission")
          material.emission = std::stof(value);
      }
    }
    file.close();
  }
  return material;
}

static bool RayAABB(const float ro[3], const float rd[3], const float bmin[3],
                    const float bmax[3], float &tHit) {
  float tmin = -FLT_MAX;
  float tmax = FLT_MAX;
  for (int i = 0; i < 3; ++i) {
    const float ro_i = ro[i];
    const float rd_i = rd[i];
    if (std::fabs(rd_i) < 1e-8f) {
      if (ro_i < bmin[i] || ro_i > bmax[i])
        return false;
    } else {
      float invD = 1.0f / rd_i;
      float t0 = (bmin[i] - ro_i) * invD;
      float t1 = (bmax[i] - ro_i) * invD;
      if (t0 > t1)
        std::swap(t0, t1);
      if (t0 > tmin)
        tmin = t0;
      if (t1 < tmax)
        tmax = t1;
      if (tmax < tmin)
        return false;
    }
  }
  tHit = tmin;
  return tmin >= 0.0f; // Simplified, check main definition if needed tmax logic
}

Scene::Scene() {}

Scene::~Scene() {
  if (m_VAO_Grid)
    glDeleteVertexArrays(1, &m_VAO_Grid);
  if (m_VBO_Grid)
    glDeleteBuffers(1, &m_VBO_Grid);
  if (m_VAO_Cube)
    glDeleteVertexArrays(1, &m_VAO_Cube);
  if (m_VBO_Cube)
    glDeleteBuffers(1, &m_VBO_Cube);
}

void Scene::Init() {
  InitGrid();
  InitCubeResources();
  InitGizmoResources();
}

void Scene::LoadFromProject(const ProjectData &project) {
  m_Cubes.clear();
  m_Cubes.reserve(project.cubes.size());
  for (const auto &data : project.cubes) {
    CubeInst inst;
    for (int k = 0; k < 3; k++) {
      inst.pos[k] = data.pos[k];
      inst.rotation[k] = data.rotation[k];
      inst.scale[k] = data.scale[k];
    }
    inst.materialPath = data.materialPath;
    inst.selected = false;
    m_Cubes.push_back(inst);
  }
}

void Scene::GetProjectData(ProjectData &project) const {
  project.cubes.clear();
  project.cubes.reserve(m_Cubes.size());
  for (const auto &inst : m_Cubes) {
    CubeData data;
    for (int k = 0; k < 3; k++) {
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
    const auto &c = m_Cubes[i];

    // AABB approximation (Unit cube scaled and rotated is hard for AABB)
    // For simplicity, we use a sphere or a conservative AABB?
    // Let's use AABB based on pos +/- 0.5 * max_scale
    float maxScale = std::max({c.scale[0], c.scale[1], c.scale[2]});
    float halfSize = 0.5f * maxScale; // aprox collision box

    float bmin[3] = {c.pos[0] - halfSize, c.pos[1] - halfSize,
                     c.pos[2] - halfSize};
    float bmax[3] = {c.pos[0] + halfSize, c.pos[1] + halfSize,
                     c.pos[2] + halfSize};

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
  const float max = halfExtent * step;
  for (int i = -halfExtent; i <= halfExtent; ++i) {
    float v = i * step;
    gridVerts.push_back(min);
    gridVerts.push_back(0.0f);
    gridVerts.push_back(v);
    gridVerts.push_back(max);
    gridVerts.push_back(0.0f);
    gridVerts.push_back(v);
    gridVerts.push_back(v);
    gridVerts.push_back(0.0f);
    gridVerts.push_back(min);
    gridVerts.push_back(v);
    gridVerts.push_back(0.0f);
    gridVerts.push_back(max);
  }
  m_GridVertexCount = (int)(gridVerts.size() / 3);
  glGenVertexArrays(1, &m_VAO_Grid);
  glGenBuffers(1, &m_VBO_Grid);
  glBindVertexArray(m_VAO_Grid);
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Grid);
  glBufferData(GL_ARRAY_BUFFER, gridVerts.size() * sizeof(float),
               gridVerts.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glBindVertexArray(0);
}

void Scene::InitCubeResources() {
  const float c[] = {// Cara +X
                     0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f,
                     0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f,
                     // Cara -X
                     -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
                     -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f,
                     // Cara +Y
                     -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
                     -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f,
                     // Cara -Y
                     -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f,
                     -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f,
                     // Cara +Z
                     -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
                     -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
                     // Cara -Z
                     -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,
                     -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f,
                     -0.5f};
  m_CubeVertexCount = 36;
  glGenVertexArrays(1, &m_VAO_Cube);
  glGenBuffers(1, &m_VBO_Cube);
  glBindVertexArray(m_VAO_Cube);
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Cube);
  glBufferData(GL_ARRAY_BUFFER, sizeof(c), c, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glBindVertexArray(0);
}

void Scene::InitGizmoResources() {
  // Arrow (Cylinder + Cone simplified to single mesh for now, or just lines)
  // Let's use simple thick lines for arrows (cubes scaled) for visualization
  const float arrow[] = {
      0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // X
      0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, // Y
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f  // Z
  };
  // Actually, let's use the code from main.cpp we saw earlier?
  // It constructed arrows. For simplicity here, I'll build visual indicators.
  // 1. Arrow (Translate): 3 lines + tips.
  // We will use drawing logic in RenderGizmos to position generic shapes.
  // So here we just need a Unit Cylinder or Unit Cube.
  // We already have VBO_Cube (Unit cube centered at 0).

  // Rings (Rotate): Circle lineloop.
  std::vector<float> ring;
  const int segments = 32;
  for (int i = 0; i <= segments; ++i) {
    float theta = 2.0f * 3.1415926f * float(i) / float(segments);
    ring.push_back(cosf(theta)); // X
    ring.push_back(sinf(theta)); // Y
    ring.push_back(0.0f);        // Z
  }
  m_RingVertexCount = segments + 1;
  glGenVertexArrays(1, &m_VAO_Ring);
  glGenBuffers(1, &m_VBO_Ring);
  glBindVertexArray(m_VAO_Ring);
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Ring);
  glBufferData(GL_ARRAY_BUFFER, ring.size() * sizeof(float), ring.data(),
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glBindVertexArray(0);

  // Compile Gizmo Shader (Simple Unlit)
  const char *vs = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 uMVP;
    void main() {
        gl_Position = uMVP * vec4(aPos, 1.0);
    }
  )";
  const char *fs = R"(
    #version 330 core
    out vec4 FragColor;
    uniform vec4 uColor;
    void main() {
        FragColor = uColor;
    }
  )";
  // We need to include shader.h or just use raw GL calls? Scene includes glad.
  // Ideally use Shader namespace helper if available. But Scene includes
  // scene.h which doesn't know Shader helper. We need to include
  // "../shaders/shader.h" in scene.cpp or just replicate code. Scene.cpp top
  // includes "scene.h". Let's add include code in separate edit if needed or
  // assume we can inline raw GL calls helper locally? No, let's just do it raw
  // here to avoid dependency hell or add header. Actually, we can just include
  // "../shaders/shader.h" in scene.cpp. But wait, I'm replacing lines in a
  // middle of file.

  // Implementation of shader compile inline
  auto compile = [](GLenum type, const char *src) -> GLuint {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
  };
  GLuint v = compile(GL_VERTEX_SHADER, vs);
  GLuint f = compile(GL_FRAGMENT_SHADER, fs);
  m_GizmoShader = glCreateProgram();
  glAttachShader(m_GizmoShader, v);
  glAttachShader(m_GizmoShader, f);
  glLinkProgram(m_GizmoShader);
  glDeleteShader(v);
  glDeleteShader(f);
}

void Scene::ApplyMaterialToShader(const Material &material,
                                  GLuint shaderProgram) {
  GLint colorLoc = glGetUniformLocation(shaderProgram, "u_material_color");
  if (colorLoc != -1)
    glUniform3f(colorLoc, material.color[0], material.color[1],
                material.color[2]);
  GLint metallicLoc =
      glGetUniformLocation(shaderProgram, "u_material_metallic");
  if (metallicLoc != -1)
    glUniform1f(metallicLoc, material.metallic);
  GLint roughnessLoc =
      glGetUniformLocation(shaderProgram, "u_material_roughness");
  if (roughnessLoc != -1)
    glUniform1f(roughnessLoc, material.roughness);
  GLint emissionLoc =
      glGetUniformLocation(shaderProgram, "u_material_emission");
  if (emissionLoc != -1)
    glUniform1f(emissionLoc, material.emission);
}

Material Scene::GetMaterialFromPath(const std::string &path) {
  if (path.empty()) {
    Material m; // defaul
    m.color[0] = 0.8f;
    m.color[1] = 0.8f;
    m.color[2] = 0.8f;
    return m;
  }
  return LoadMaterialHelper(path);
}

void Scene::Render(const Mat4 &view, const Mat4 &proj, GLuint shaderProgram) {
  glUseProgram(shaderProgram);

  // Uniform locations common
  GLint locMVP = glGetUniformLocation(shaderProgram, "uMVP");
  GLint locColor = glGetUniformLocation(shaderProgram, "uColor");

  Mat4 vp = mat4_mul(proj, view); // Precompute VP

  // 1. Draw Grid
  if (locColor != -1)
    glUniform4f(locColor, 0.35f, 0.35f, 0.35f, 1.0f);
  Mat4 identity = mat4_identity();
  Mat4 mvpGrid = mat4_mul(vp, identity);
  if (locMVP != -1)
    glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvpGrid.m);

  glBindVertexArray(m_VAO_Grid);
  glDrawArrays(GL_LINES, 0, m_GridVertexCount);
  glBindVertexArray(0);

  // 2. Draw Cubes
  glPolygonMode(GL_FRONT_AND_BACK, m_Wireframe ? GL_LINE : GL_FILL);
  glBindVertexArray(m_VAO_Cube);

  for (const auto &c : m_Cubes) {
    // Transform
    Mat4 model = mat4_identity();

    Mat4 scaleM = mat4_identity();
    scaleM.m[0] = c.scale[0];
    scaleM.m[5] = c.scale[1];
    scaleM.m[10] = c.scale[2];

    float radX = c.rotation[0] * 3.1415926f / 180.0f;
    float radY = c.rotation[1] * 3.1415926f / 180.0f;
    float radZ = c.rotation[2] * 3.1415926f / 180.0f;

    Mat4 rotX = mat4_identity();
    rotX.m[5] = cos(radX);
    rotX.m[6] = -sin(radX);
    rotX.m[9] = sin(radX);
    rotX.m[10] = cos(radX);
    Mat4 rotY = mat4_identity();
    rotY.m[0] = cos(radY);
    rotY.m[2] = sin(radY);
    rotY.m[8] = -sin(radY);
    rotY.m[10] = cos(radY);
    Mat4 rotZ = mat4_identity();
    rotZ.m[0] = cos(radZ);
    rotZ.m[1] = -sin(radZ);
    rotZ.m[4] = sin(radZ);
    rotZ.m[5] = cos(radZ);

    Mat4 rotation = mat4_mul(mat4_mul(rotZ, rotY), rotX);
    model = mat4_mul(rotation, scaleM);

    model.m[12] = c.pos[0];
    model.m[13] = c.pos[1];
    model.m[14] = c.pos[2];

    Mat4 mvp = mat4_mul(vp, model);
    if (locMVP != -1)
      glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvp.m);

    // Material
    Material mat = GetMaterialFromPath(c.materialPath);

    if (c.selected) {
      glUniform4f(locColor, mat.color[0] * 1.2f, mat.color[1] * 0.8f,
                  mat.color[2] * 0.4f, 1.0f);
    } else {
      glUniform4f(locColor, mat.color[0], mat.color[1], mat.color[2], 1.0f);
    }

    ApplyMaterialToShader(mat, shaderProgram);
    glDrawArrays(GL_TRIANGLES, 0, m_CubeVertexCount);
  }

  glBindVertexArray(0);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Scene::RenderGizmos(const Mat4 &view, const Mat4 &proj,
                         GLuint shaderProgram, int selectedIndex,
                         int transformMode, int hoveredAxis,
                         bool localSpace) {
  if (selectedIndex < 0 || selectedIndex >= (int)m_Cubes.size())
    return;

  const auto &c = m_Cubes[selectedIndex];

  // Use our specific Gizmo shader
  glUseProgram(m_GizmoShader);
  glDisable(GL_DEPTH_TEST); // Gizmos visible thru walls

  GLint locMVP = glGetUniformLocation(m_GizmoShader, "uMVP");
  GLint locColor = glGetUniformLocation(m_GizmoShader, "uColor");

  Mat4 vp = mat4_mul(proj, view);

  // Position of gizmo
  float px = c.pos[0];
  float py = c.pos[1];
  float pz = c.pos[2];
  
  // Colors: normal and highlighted (brighter)
  float colorsNormal[3][4] = {
    {1.0f, 0.25f, 0.25f, 1.0f},  // X - Red
    {0.25f, 1.0f, 0.25f, 1.0f},  // Y - Green
    {0.4f, 0.4f, 1.0f, 1.0f}     // Z - Blue
  };
  float colorHighlight[4] = {1.0f, 1.0f, 0.2f, 1.0f}; // Yellow highlight

  // Gizmo size
  const float gizmoLength = 1.5f;
  const float arrowTipSize = 0.1f;
  
  // Build gizmo transform matrix (position + optional rotation for local space)
  Mat4 t_pos = mat4_identity();
  t_pos.m[12] = px;
  t_pos.m[13] = py;
  t_pos.m[14] = pz;
  
  // Build rotation matrix for local space
  Mat4 objRot = mat4_identity();
  if (localSpace) {
    float radX = c.rotation[0] * 3.1415926f / 180.0f;
    float radY = c.rotation[1] * 3.1415926f / 180.0f;
    float radZ = c.rotation[2] * 3.1415926f / 180.0f;
    
    Mat4 rotX = mat4_identity();
    rotX.m[5] = cos(radX); rotX.m[6] = -sin(radX);
    rotX.m[9] = sin(radX); rotX.m[10] = cos(radX);
    
    Mat4 rotY = mat4_identity();
    rotY.m[0] = cos(radY); rotY.m[2] = sin(radY);
    rotY.m[8] = -sin(radY); rotY.m[10] = cos(radY);
    
    Mat4 rotZ = mat4_identity();
    rotZ.m[0] = cos(radZ); rotZ.m[1] = -sin(radZ);
    rotZ.m[4] = sin(radZ); rotZ.m[5] = cos(radZ);
    
    objRot = mat4_mul(mat4_mul(rotZ, rotY), rotX);
  }
  
  // Combined gizmo base transform: position * rotation
  Mat4 gizmoBase = mat4_mul(t_pos, objRot);

  auto setAxisColor = [&](int axis) {
    if (axis == hoveredAxis) {
      glUniform4fv(locColor, 1, colorHighlight);
    } else {
      glUniform4fv(locColor, 1, colorsNormal[axis]);
    }
  };

  // Create temporary VAO/VBO for lines
  GLuint tempVAO, tempVBO;
  glGenVertexArrays(1, &tempVAO);
  glGenBuffers(1, &tempVBO);

  if (transformMode == 0) { // TRANSLATE - Lines with arrow tips
    glLineWidth(3.0f);
    
    // Draw axis lines
    float lineVerts[] = {
      0.0f, 0.0f, 0.0f, gizmoLength, 0.0f, 0.0f,  // X
      0.0f, 0.0f, 0.0f, 0.0f, gizmoLength, 0.0f,  // Y
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, gizmoLength   // Z
    };
    
    glBindVertexArray(tempVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tempVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVerts), lineVerts, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    Mat4 mvp = mat4_mul(vp, gizmoBase);
    glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvp.m);
    
    setAxisColor(0);
    glDrawArrays(GL_LINES, 0, 2);
    setAxisColor(1);
    glDrawArrays(GL_LINES, 2, 2);
    setAxisColor(2);
    glDrawArrays(GL_LINES, 4, 2);
    
    // Draw arrow tips as small cubes
    glBindVertexArray(m_VAO_Cube);
    
    Mat4 s_tip = mat4_identity();
    s_tip.m[0] = arrowTipSize * 1.8f;
    s_tip.m[5] = arrowTipSize;
    s_tip.m[10] = arrowTipSize;
    Mat4 t_tip = mat4_identity();
    t_tip.m[12] = gizmoLength;
    mvp = mat4_mul(vp, mat4_mul(gizmoBase, mat4_mul(t_tip, s_tip)));
    glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvp.m);
    setAxisColor(0);
    glDrawArrays(GL_TRIANGLES, 0, m_CubeVertexCount);

    s_tip = mat4_identity();
    s_tip.m[0] = arrowTipSize;
    s_tip.m[5] = arrowTipSize * 1.8f;
    s_tip.m[10] = arrowTipSize;
    t_tip = mat4_identity();
    t_tip.m[13] = gizmoLength;
    mvp = mat4_mul(vp, mat4_mul(gizmoBase, mat4_mul(t_tip, s_tip)));
    glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvp.m);
    setAxisColor(1);
    glDrawArrays(GL_TRIANGLES, 0, m_CubeVertexCount);

    s_tip = mat4_identity();
    s_tip.m[0] = arrowTipSize;
    s_tip.m[5] = arrowTipSize;
    s_tip.m[10] = arrowTipSize * 1.8f;
    t_tip = mat4_identity();
    t_tip.m[14] = gizmoLength;
    mvp = mat4_mul(vp, mat4_mul(gizmoBase, mat4_mul(t_tip, s_tip)));
    glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvp.m);
    setAxisColor(2);
    glDrawArrays(GL_TRIANGLES, 0, m_CubeVertexCount);
    
  } else if (transformMode == 1) { // ROTATE - Rings
    glLineWidth(2.5f);
    glBindVertexArray(m_VAO_Ring);
    
    Mat4 s = mat4_identity();
    s.m[0] = gizmoLength;
    s.m[5] = gizmoLength;
    s.m[10] = gizmoLength;

    // X Axis Rotation (YZ plane)
    Mat4 rot = mat4_identity();
    rot.m[5] = 0; rot.m[6] = 1;
    rot.m[9] = -1; rot.m[10] = 0;
    Mat4 mvp = mat4_mul(vp, mat4_mul(gizmoBase, mat4_mul(rot, s)));
    glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvp.m);
    setAxisColor(0);
    glDrawArrays(GL_LINE_LOOP, 0, m_RingVertexCount);

    // Y Axis Rotation (XZ plane)
    rot = mat4_identity();
    rot.m[0] = 0; rot.m[2] = -1;
    rot.m[8] = 1; rot.m[10] = 0;
    mvp = mat4_mul(vp, mat4_mul(gizmoBase, mat4_mul(rot, s)));
    glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvp.m);
    setAxisColor(1);
    glDrawArrays(GL_LINE_LOOP, 0, m_RingVertexCount);

    // Z Axis Rotation (XY plane)
    mvp = mat4_mul(vp, mat4_mul(gizmoBase, s));
    glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvp.m);
    setAxisColor(2);
    glDrawArrays(GL_LINE_LOOP, 0, m_RingVertexCount);
    
  } else if (transformMode == 2) { // SCALE - Lines with boxes (always local)
    glLineWidth(3.0f);
    
    // Scale gizmo always uses local space (object rotation)
    Mat4 scaleBase = mat4_mul(t_pos, objRot);
    
    float lineVerts[] = {
      0.0f, 0.0f, 0.0f, gizmoLength - 0.1f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 0.0f, gizmoLength - 0.1f, 0.0f,
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, gizmoLength - 0.1f
    };
    
    glBindVertexArray(tempVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tempVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVerts), lineVerts, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    Mat4 mvp = mat4_mul(vp, scaleBase);
    glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvp.m);
    
    setAxisColor(0);
    glDrawArrays(GL_LINES, 0, 2);
    setAxisColor(1);
    glDrawArrays(GL_LINES, 2, 2);
    setAxisColor(2);
    glDrawArrays(GL_LINES, 4, 2);
    
    // Draw end boxes
    glBindVertexArray(m_VAO_Cube);
    const float boxSize = 0.1f;
    
    Mat4 s_box = mat4_identity();
    s_box.m[0] = boxSize;
    s_box.m[5] = boxSize;
    s_box.m[10] = boxSize;
    
    Mat4 t_end = mat4_identity();
    t_end.m[12] = gizmoLength;
    mvp = mat4_mul(vp, mat4_mul(scaleBase, mat4_mul(t_end, s_box)));
    glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvp.m);
    setAxisColor(0);
    glDrawArrays(GL_TRIANGLES, 0, m_CubeVertexCount);

    t_end = mat4_identity();
    t_end.m[13] = gizmoLength;
    mvp = mat4_mul(vp, mat4_mul(scaleBase, mat4_mul(t_end, s_box)));
    glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvp.m);
    setAxisColor(1);
    glDrawArrays(GL_TRIANGLES, 0, m_CubeVertexCount);

    t_end = mat4_identity();
    t_end.m[14] = gizmoLength;
    mvp = mat4_mul(vp, mat4_mul(scaleBase, mat4_mul(t_end, s_box)));
    glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvp.m);
    setAxisColor(2);
    glDrawArrays(GL_TRIANGLES, 0, m_CubeVertexCount);
  }

  // Cleanup temp buffers
  glDeleteBuffers(1, &tempVBO);
  glDeleteVertexArrays(1, &tempVAO);

  glBindVertexArray(0);
  glLineWidth(1.0f);
  glEnable(GL_DEPTH_TEST);
}
