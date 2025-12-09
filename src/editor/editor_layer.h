#pragma once

#include <vector>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../scene/scene.h"
#include "editor_defs.h"

// Forward declaration
class Scene;

class EditorLayer {
public:
    EditorLayer();
    ~EditorLayer();

    void Init(GLFWwindow* window);
    void Render(Scene& scene);
    
    // Input handling if needed, though ImGui handles most via callbacks
    void Shutdown();

private:
    void InitImGui(GLFWwindow* window);
    void DrawMainMenuBar();
    void DrawDockSpace(Scene& scene);
    void DrawHierarchy(Scene& scene);
    void DrawProperties(Scene& scene);
    void DrawFileExplorer();
    
    // State
    bool m_ShowHierarchy = true;
    bool m_ShowProperties = true;
    bool m_ShowFileExplorer = true;
    bool m_ShowDemoWindow = false;
    
    // Selection state (mirroring what was in main.cpp)
    int m_SelectedCubeIndex = -1;
    // ... other editor states like "TransformMode" ...
};
