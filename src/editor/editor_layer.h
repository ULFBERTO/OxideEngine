#pragma once

#include "../scene/scene.h"
#include "../utils/math_utils.h"
#include "editor_defs.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <string>
#include <vector>

// Forward declaration
class Scene;

class EditorLayer {
public:
  EditorLayer();
  ~EditorLayer();

  void Init(GLFWwindow *window);
  void Render(Scene &scene);

  // Input handling if needed, though ImGui handles most via callbacks
  void Shutdown();

  int GetSelectedCubeIndex() const { return m_SelectedCubeIndex; }
  void SetSelectedCubeIndex(int index) { m_SelectedCubeIndex = index; }
  int GetTransformMode() const { return m_TransformMode; }
  int GetHoveredAxis() const { return m_HoveredAxis; }
  bool IsLocalSpace() const { return m_LocalSpace; }
  
  // Gizmo interaction
  void HandleGizmoInput(Scene &scene, GLFWwindow *window, const Mat4& view, const Mat4& proj);
  bool IsDraggingGizmo() const { return m_IsDraggingGizmo; }
  bool IsHoveringGizmo() const { return m_HoveredAxis >= 0; }
  
  // Scene viewport
  void BeginSceneRender();
  void EndSceneRender();
  GLuint GetSceneTexture() const { return m_SceneTexture; }
  bool IsSceneWindowFocused() const { return m_SceneWindowFocused; }
  bool IsSceneWindowHovered() const { return m_SceneWindowHovered; }
  void GetSceneViewportSize(float &width, float &height) const { width = m_SceneViewportWidth; height = m_SceneViewportHeight; }
  void GetSceneViewportPos(float &x, float &y) const { x = m_SceneViewportPosX; y = m_SceneViewportPosY; }
  bool GetMousePosInViewport(float &x, float &y) const;

private:
  void InitImGui(GLFWwindow *window);
  void InitFramebuffer();
  void ResizeFramebuffer(int width, int height);
  void DrawMainMenuBar();
  void DrawDockSpace(Scene &scene);
  void DrawSceneViewport();
  void DrawHierarchy(Scene &scene);
  void DrawProperties(Scene &scene);
  void DrawFileExplorer();
  void DrawAboutDialog();
  
  // Gizmo helpers
  int DetectHoveredGizmoAxis(const float objPos[3], const Mat4& view, const Mat4& proj, float mouseX, float mouseY);
  void ApplyGizmoDrag(CubeInst& cube, float deltaX, float deltaY, const Mat4& view);

  GLFWwindow *m_Window = nullptr;

  // Framebuffer for scene rendering
  GLuint m_Framebuffer = 0;
  GLuint m_SceneTexture = 0;
  GLuint m_DepthRenderbuffer = 0;
  int m_FBWidth = 800;
  int m_FBHeight = 600;

  // State
  bool m_ShowHierarchy = true;
  bool m_ShowProperties = true;
  bool m_ShowFileExplorer = true;
  bool m_ShowSceneViewport = true;
  bool m_ShowAbout = false;
  bool m_WireframeMode = false;
  bool m_LocalSpace = false;
  
  // Scene viewport state
  bool m_SceneWindowFocused = false;
  bool m_SceneWindowHovered = false;
  float m_SceneViewportWidth = 800.0f;
  float m_SceneViewportHeight = 600.0f;
  float m_SceneViewportPosX = 0.0f;
  float m_SceneViewportPosY = 0.0f;

  // Selection state
  int m_SelectedCubeIndex = -1;
  int m_TransformMode = 0; // 0=Translate, 1=Rotate, 2=Scale
  
  // Gizmo state
  bool m_IsDraggingGizmo = false;
  int m_DragAxis = -1;      // 0=X, 1=Y, 2=Z, -1=none
  int m_HoveredAxis = -1;   // For visual feedback
  float m_DragStartPos[2] = {0, 0};
  float m_DragStartValue[3] = {0, 0, 0};
  float m_GizmoSize = 1.5f; // Size of gizmo axes
};
