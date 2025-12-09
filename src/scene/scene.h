#pragma once

#include <vector>
#include <string>
#include <glad/glad.h>
#include "scene_defs.h"
#include "../utils/math_utils.h"
#include "../project/project_manager.h"

class Scene {
public:
    Scene();
    ~Scene();

    void Init();
    void Render(const Mat4& view, const Mat4& proj, GLuint shaderProgram);
    
    std::vector<CubeInst>& GetCubes() { return m_Cubes; }
    const std::vector<CubeInst>& GetCubes() const { return m_Cubes; }
    
    void AddCube(const CubeInst& cube) { m_Cubes.push_back(cube); }
    void Clear() { m_Cubes.clear(); }

    void SetWireframe(bool enabled) { m_Wireframe = enabled; }

    // Serialization
    void LoadFromProject(const ProjectData& project);
    void GetProjectData(ProjectData& project) const;

    // Interaction
    // Retorna el índice del cubo seleccionado o -1
    int Raycast(const float ray_origin[3], const float ray_dir[3]);

private:
    std::vector<CubeInst> m_Cubes;
    bool m_Wireframe = false;

    // OpenGL Resources
    GLuint m_VAO_Grid = 0, m_VBO_Grid = 0;
    int m_GridVertexCount = 0;
    
    GLuint m_VAO_Cube = 0, m_VBO_Cube = 0;
    int m_CubeVertexCount = 0;

    void InitGrid();
    void InitCubeResources();
    void ApplyMaterialToShader(const Material& material, GLuint shaderProgram);
    Material GetMaterialFromPath(const std::string& path);
};
