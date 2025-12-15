#pragma once

#include "../project/project_manager.h"
#include "../utils/math_utils.h"
#include "scene_defs.h"
#include <glad/glad.h>
#include <string>
#include <vector>

class Scene {
public:
  Scene();
  ~Scene();

  void Init();
  void Render(const Mat4 &view, const Mat4 &proj, GLuint shaderProgram);
  // transformMode: 0=Translate, 1=Rotate, 2=Scale
  // hoveredAxis: -1=none, 0=X, 1=Y, 2=Z (for highlight)
  // localSpace: if true, gizmo axes follow object rotation
  void RenderGizmos(const Mat4 &view, const Mat4 &proj, GLuint shaderProgram,
                    int selectedIndex, int transformMode, int hoveredAxis = -1,
                    bool localSpace = false);

  std::vector<CubeInst> &GetCubes() { return m_Cubes; }
  const std::vector<CubeInst> &GetCubes() const { return m_Cubes; }

  void AddCube(const CubeInst &cube) { m_Cubes.push_back(cube); }
  void Clear() { m_Cubes.clear(); }

  void SetWireframe(bool enabled) { m_Wireframe = enabled; }

  // Serialization
  void LoadFromProject(const ProjectData &project);
  void GetProjectData(ProjectData &project) const;

  // Interaction
  // Retorna el índice del cubo seleccionado o -1
  int Raycast(const float ray_origin[3], const float ray_dir[3]);

private:
  std::vector<CubeInst> m_Cubes;
  bool m_Wireframe = false;

  // OpenGL Resources
  GLuint m_VAO_Grid = 0, m_VBO_Grid = 0;
  int m_GridVertexCount = 0;
  GLuint m_GizmoShader = 0;

  GLuint m_VAO_Cube = 0, m_VBO_Cube = 0;
  int m_CubeVertexCount = 0;

  // Gizmo Resources
  GLuint m_VAO_Arrow = 0, m_VBO_Arrow = 0;
  int m_ArrowVertexCount = 0;
  GLuint m_VAO_Ring = 0, m_VBO_Ring = 0;
  int m_RingVertexCount = 0;
  GLuint m_VAO_Scale = 0, m_VBO_Scale = 0;
  int m_ScaleVertexCount = 0;

  void InitGrid();
  void InitCubeResources();
  void InitGizmoResources();
  void ApplyMaterialToShader(const Material &material, GLuint shaderProgram);
  Material GetMaterialFromPath(const std::string &path);
};
