#include "editor_layer.h"
#include "../project/project_manager.h"
#include "../scene/scene.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"
#include <cmath>
#include <iostream>

EditorLayer::EditorLayer() {}

EditorLayer::~EditorLayer() { Shutdown(); }

void EditorLayer::Init(GLFWwindow *window) { 
  m_Window = window;
  InitImGui(window);
  InitFramebuffer();
}

void EditorLayer::InitImGui(GLFWwindow *window) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigDockingAlwaysTabBar = true;

  // Style Polish
  ImGui::StyleColorsDark();
  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowRounding = 5.0f;
  style.FrameRounding = 4.0f;
  style.PopupRounding = 4.0f;
  style.ScrollbarRounding = 12.0f;
  style.GrabRounding = 4.0f;
  style.TabRounding = 4.0f;

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");
}

void EditorLayer::Shutdown() {
  if (m_Framebuffer) glDeleteFramebuffers(1, &m_Framebuffer);
  if (m_SceneTexture) glDeleteTextures(1, &m_SceneTexture);
  if (m_DepthRenderbuffer) glDeleteRenderbuffers(1, &m_DepthRenderbuffer);
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void EditorLayer::InitFramebuffer() {
  glGenFramebuffers(1, &m_Framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

  // Color texture
  glGenTextures(1, &m_SceneTexture);
  glBindTexture(GL_TEXTURE_2D, m_SceneTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_FBWidth, m_FBHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_SceneTexture, 0);

  // Depth renderbuffer
  glGenRenderbuffers(1, &m_DepthRenderbuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, m_DepthRenderbuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_FBWidth, m_FBHeight);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthRenderbuffer);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void EditorLayer::ResizeFramebuffer(int width, int height) {
  if (width <= 0 || height <= 0) return;
  if (width == m_FBWidth && height == m_FBHeight) return;
  
  m_FBWidth = width;
  m_FBHeight = height;

  glBindTexture(GL_TEXTURE_2D, m_SceneTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

  glBindRenderbuffer(GL_RENDERBUFFER, m_DepthRenderbuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
}

void EditorLayer::BeginSceneRender() {
  glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
  glViewport(0, 0, m_FBWidth, m_FBHeight);
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void EditorLayer::EndSceneRender() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void EditorLayer::Render(Scene &scene) {
  // Start Frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // Apply wireframe mode to scene
  scene.SetWireframe(m_WireframeMode);

  DrawDockSpace(scene);

  if (m_ShowSceneViewport)
    DrawSceneViewport();
  if (m_ShowHierarchy)
    DrawHierarchy(scene);
  if (m_ShowProperties)
    DrawProperties(scene);
  if (m_ShowFileExplorer)
    DrawFileExplorer();
  // Demo window disabled - requires imgui_demo.cpp
  // if (m_ShowDemoWindow)
  //   ImGui::ShowDemoWindow(&m_ShowDemoWindow);
  if (m_ShowAbout)
    DrawAboutDialog();

  // Render ImGui
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorLayer::DrawDockSpace(Scene &scene) {
  static ImGuiDockNodeFlags dockspace_flags =
      ImGuiDockNodeFlags_PassthruCentralNode;
  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);
  window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus |
                  ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

  ImGui::Begin("DockSpace Demo", nullptr, window_flags);
  ImGui::PopStyleVar(3);

  ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
  ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New Project", "Ctrl+N")) {
        scene.Clear();
        m_SelectedCubeIndex = -1;
      }
      if (ImGui::MenuItem("Open Project", "Ctrl+O")) {
        ProjectData data;
        if (ProjectManager::LoadProject("myproject.MarioEngine", data)) {
          scene.LoadFromProject(data);
        }
      }
      if (ImGui::MenuItem("Save Project", "Ctrl+S")) {
        ProjectData data;
        data.projectName = "MyProject";
        scene.GetProjectData(data);
        ProjectManager::SaveProject("myproject.MarioEngine", data);
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Exit", "Alt+F4")) {
        // Close - handled by GLFW
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {}
      if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {}
      ImGui::Separator();
      if (ImGui::MenuItem("Delete Selected", "Del")) {
        if (m_SelectedCubeIndex >= 0 && m_SelectedCubeIndex < (int)scene.GetCubes().size()) {
          scene.GetCubes().erase(scene.GetCubes().begin() + m_SelectedCubeIndex);
          m_SelectedCubeIndex = -1;
        }
      }
      if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
        if (m_SelectedCubeIndex >= 0 && m_SelectedCubeIndex < (int)scene.GetCubes().size()) {
          CubeInst copy = scene.GetCubes()[m_SelectedCubeIndex];
          copy.pos[0] += 1.0f;
          copy.selected = false;
          scene.AddCube(copy);
        }
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Hierarchy", nullptr, &m_ShowHierarchy);
      ImGui::MenuItem("Properties", nullptr, &m_ShowProperties);
      ImGui::MenuItem("File Explorer", nullptr, &m_ShowFileExplorer);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Windows")) {
      if (ImGui::MenuItem("Reset Layout")) {
        m_ShowHierarchy = true;
        m_ShowProperties = true;
        m_ShowFileExplorer = true;
        m_ShowSceneViewport = true;
      }
      ImGui::Separator();
      ImGui::MenuItem("Scene Viewport", nullptr, &m_ShowSceneViewport);
      ImGui::MenuItem("Hierarchy", nullptr, &m_ShowHierarchy);
      ImGui::MenuItem("Properties", nullptr, &m_ShowProperties);
      ImGui::MenuItem("File Explorer", nullptr, &m_ShowFileExplorer);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Settings")) {
      if (ImGui::BeginMenu("Theme")) {
        if (ImGui::MenuItem("Dark")) { ImGui::StyleColorsDark(); }
        if (ImGui::MenuItem("Light")) { ImGui::StyleColorsLight(); }
        if (ImGui::MenuItem("Classic")) { ImGui::StyleColorsClassic(); }
        ImGui::EndMenu();
      }
      ImGui::MenuItem("Wireframe Mode", nullptr, &m_WireframeMode);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("About MarioEngine")) {
        m_ShowAbout = true;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  // Toolbar
  ImGui::SetCursorPos(ImVec2(10, 30)); // Manual positioning or use a window
  // But DockSpace is fullscreen. We can create a child window or just draw on
  // top? Drawing on top of DockSpace window might be tricky if it's dockspace.
  // Better: Create a "Toolbar" window that is statically positioned max top.
  // Or just use the MenuBar area if permitted?
  // Let's create a small window "Toolbar"

  // Actually, let's just use ImGui::Begin("Toolbar") in Render and let user
  // dock it. But user wants it visible. Let's put it in the dockspace function
  // call for now as a simple overlay if possible, or just a new window.

  ImGui::End();
}

void EditorLayer::DrawSceneViewport() {
  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  
  if (ImGui::Begin("Scene", &m_ShowSceneViewport)) {
    m_SceneWindowFocused = ImGui::IsWindowFocused();
    m_SceneWindowHovered = ImGui::IsWindowHovered();
    
    // Get content region position (where the image starts)
    ImVec2 contentPos = ImGui::GetCursorScreenPos();
    m_SceneViewportPosX = contentPos.x;
    m_SceneViewportPosY = contentPos.y;
    
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    if (viewportSize.x > 0 && viewportSize.y > 0) {
      // Resize framebuffer if needed
      if ((int)viewportSize.x != m_FBWidth || (int)viewportSize.y != m_FBHeight) {
        ResizeFramebuffer((int)viewportSize.x, (int)viewportSize.y);
      }
      m_SceneViewportWidth = viewportSize.x;
      m_SceneViewportHeight = viewportSize.y;
      
      // Display the scene texture
      ImGui::Image((ImTextureID)(intptr_t)m_SceneTexture, viewportSize, ImVec2(0, 1), ImVec2(1, 0));
    }
    
    // Toolbar overlay inside scene viewport (top-right corner)
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    float toolbarWidth = 300.0f;
    float toolbarHeight = 30.0f;
    
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + windowSize.x - toolbarWidth - 10, windowPos.y + 30));
    ImGui::SetNextWindowBgAlpha(0.7f);
    ImGui::BeginChild("##SceneToolbar", ImVec2(toolbarWidth, toolbarHeight), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    // Transform mode buttons
    bool isTranslate = (m_TransformMode == 0);
    bool isRotate = (m_TransformMode == 1);
    bool isScale = (m_TransformMode == 2);
    
    if (isTranslate) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    if (ImGui::SmallButton("W")) m_TransformMode = 0;
    if (isTranslate) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Move (W)");
    
    ImGui::SameLine();
    if (isRotate) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    if (ImGui::SmallButton("E")) m_TransformMode = 1;
    if (isRotate) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rotate (E)");
    
    ImGui::SameLine();
    if (isScale) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    if (ImGui::SmallButton("R")) m_TransformMode = 2;
    if (isScale) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale (R)");
    
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    
    if (m_LocalSpace) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    if (ImGui::SmallButton("L")) m_LocalSpace = true;
    if (m_LocalSpace) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Local Space");
    
    ImGui::SameLine();
    if (!m_LocalSpace) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    if (ImGui::SmallButton("G")) m_LocalSpace = false;
    if (!m_LocalSpace) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Global Space");
    
    ImGui::EndChild();
  }
  ImGui::End();
  ImGui::PopStyleVar();
}

void EditorLayer::DrawHierarchy(Scene &scene) {
  ImGui::SetNextWindowSize(ImVec2(250, 400), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Hierarchy", &m_ShowHierarchy)) {
    // Add object buttons
    if (ImGui::Button("Add Cube")) {
      CubeInst c;
      c.pos[0] = 0;
      c.pos[1] = 0;
      c.pos[2] = 0;
      c.scale[0] = c.scale[1] = c.scale[2] = 1.0f;
      c.rotation[0] = c.rotation[1] = c.rotation[2] = 0.0f;
      scene.AddCube(c);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear All")) {
      scene.Clear();
      m_SelectedCubeIndex = -1;
    }
    
    ImGui::Separator();
    ImGui::Text("Scene Objects (%d)", (int)scene.GetCubes().size());
    ImGui::Separator();

    auto &cubes = scene.GetCubes();
    for (int i = 0; i < (int)cubes.size(); ++i) {
      std::string label = "Cube " + std::to_string(i);
      bool isSelected = (m_SelectedCubeIndex == i);
      
      if (ImGui::Selectable(label.c_str(), isSelected)) {
        // Deselect all
        for (auto &c : cubes)
          c.selected = false;
        cubes[i].selected = true;
        m_SelectedCubeIndex = i;
      }
      
      // Right-click context menu
      if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Delete")) {
          cubes.erase(cubes.begin() + i);
          if (m_SelectedCubeIndex == i)
            m_SelectedCubeIndex = -1;
          else if (m_SelectedCubeIndex > i)
            m_SelectedCubeIndex--;
          ImGui::EndPopup();
          break;
        }
        if (ImGui::MenuItem("Duplicate")) {
          CubeInst copy = cubes[i];
          copy.pos[0] += 1.0f;
          copy.selected = false;
          scene.AddCube(copy);
        }
        ImGui::EndPopup();
      }
    }
    
    if (cubes.empty()) {
      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No objects in scene");
      ImGui::TextWrapped("Click 'Add Cube' to create an object.");
    }
  }
  ImGui::End();
}

void EditorLayer::DrawProperties(Scene &scene) {
  ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Properties", &m_ShowProperties)) {
    if (m_SelectedCubeIndex >= 0 &&
        m_SelectedCubeIndex < (int)scene.GetCubes().size()) {
      CubeInst &cube = scene.GetCubes()[m_SelectedCubeIndex];
      
      // Object Info Header
      ImGui::Text("Cube %d", m_SelectedCubeIndex);
      ImGui::Separator();
      
      // Transform Section
      if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Position");
        ImGui::PushItemWidth(-1);
        ImGui::DragFloat3("##Pos", cube.pos, 0.1f, -100.0f, 100.0f, "%.2f");
        ImGui::PopItemWidth();
        
        ImGui::Spacing();
        ImGui::Text("Rotation");
        ImGui::PushItemWidth(-1);
        ImGui::DragFloat3("##Rot", cube.rotation, 1.0f, -360.0f, 360.0f, "%.1f deg");
        ImGui::PopItemWidth();
        
        ImGui::Spacing();
        ImGui::Text("Scale");
        ImGui::PushItemWidth(-1);
        ImGui::DragFloat3("##Scale", cube.scale, 0.1f, 0.01f, 100.0f, "%.2f");
        ImGui::PopItemWidth();
      }
      
      // Material Section
      if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Path: %s", cube.materialPath.empty() ? "(default)" : cube.materialPath.c_str());
        // TODO: Material editor
      }
      
      ImGui::Separator();
      
      // Actions
      if (ImGui::Button("Reset Transform")) {
        cube.pos[0] = cube.pos[1] = cube.pos[2] = 0.0f;
        cube.rotation[0] = cube.rotation[1] = cube.rotation[2] = 0.0f;
        cube.scale[0] = cube.scale[1] = cube.scale[2] = 1.0f;
      }
      ImGui::SameLine();
      if (ImGui::Button("Delete")) {
        scene.GetCubes().erase(scene.GetCubes().begin() + m_SelectedCubeIndex);
        m_SelectedCubeIndex = -1;
      }
    } else {
      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No object selected");
      ImGui::Spacing();
      ImGui::TextWrapped("Select an object in the scene or hierarchy to view its properties.");
    }
  }
  ImGui::End();
}

void EditorLayer::DrawFileExplorer() {
  if (ImGui::Begin("File Explorer", &m_ShowFileExplorer)) {
    ImGui::Text("Content Browser (Placeholder)");
    // TODO: Port full file explorer logic
  }
  ImGui::End();
}

void EditorLayer::DrawAboutDialog() {
  ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("About MarioEngine", &m_ShowAbout, ImGuiWindowFlags_NoResize)) {
    ImGui::Text("MarioEngine Editor");
    ImGui::Separator();
    ImGui::Text("Version: 1.0.0");
    ImGui::Text("A simple 3D game engine editor");
    ImGui::Spacing();
    ImGui::Text("Built with:");
    ImGui::BulletText("OpenGL 3.3");
    ImGui::BulletText("GLFW 3.4");
    ImGui::BulletText("Dear ImGui (Docking)");
    ImGui::Spacing();
    if (ImGui::Button("Close")) {
      m_ShowAbout = false;
    }
  }
  ImGui::End();
}


void EditorLayer::HandleGizmoInput(Scene &scene, GLFWwindow *window, const float *camPos, const float *camFront) {
  if (m_SelectedCubeIndex < 0 || m_SelectedCubeIndex >= (int)scene.GetCubes().size())
    return;
  
  if (ImGui::GetIO().WantCaptureMouse)
    return;
    
  CubeInst &cube = scene.GetCubes()[m_SelectedCubeIndex];
  
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);
  
  bool leftPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
  
  // Check for keyboard shortcuts to change transform mode
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) m_TransformMode = 0;
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) m_TransformMode = 1;
  if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) m_TransformMode = 2;
  
  if (leftPressed && !m_IsDraggingGizmo) {
    // Start dragging - detect which axis based on screen position relative to object
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    // Simple axis detection based on mouse position
    // In a real implementation, you'd do proper ray-gizmo intersection
    float objScreenX = (float)width / 2.0f;  // Simplified
    float objScreenY = (float)height / 2.0f;
    
    float dx = (float)xpos - objScreenX;
    float dy = (float)ypos - objScreenY;
    
    // Determine axis based on drag direction
    if (std::abs(dx) > std::abs(dy)) {
      m_DragAxis = 0; // X axis (horizontal drag)
    } else {
      m_DragAxis = 1; // Y axis (vertical drag)
    }
    
    // Check if Z key is held for Z axis
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
      m_DragAxis = 2;
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
      m_DragAxis = 0;
    }
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
      m_DragAxis = 1;
    }
    
    m_IsDraggingGizmo = true;
    m_DragStartPos[0] = (float)xpos;
    m_DragStartPos[1] = (float)ypos;
    
    if (m_TransformMode == 0) {
      m_DragStartValue[0] = cube.pos[0];
      m_DragStartValue[1] = cube.pos[1];
      m_DragStartValue[2] = cube.pos[2];
    } else if (m_TransformMode == 1) {
      m_DragStartValue[0] = cube.rotation[0];
      m_DragStartValue[1] = cube.rotation[1];
      m_DragStartValue[2] = cube.rotation[2];
    } else {
      m_DragStartValue[0] = cube.scale[0];
      m_DragStartValue[1] = cube.scale[1];
      m_DragStartValue[2] = cube.scale[2];
    }
  }
  
  if (m_IsDraggingGizmo && leftPressed) {
    float deltaX = ((float)xpos - m_DragStartPos[0]) * 0.01f;
    float deltaY = -((float)ypos - m_DragStartPos[1]) * 0.01f; // Invert Y
    
    float delta = (m_DragAxis == 1) ? deltaY : deltaX;
    
    if (m_TransformMode == 0) { // Translate
      cube.pos[m_DragAxis] = m_DragStartValue[m_DragAxis] + delta * 5.0f;
    } else if (m_TransformMode == 1) { // Rotate
      cube.rotation[m_DragAxis] = m_DragStartValue[m_DragAxis] + delta * 100.0f;
    } else { // Scale
      float newScale = m_DragStartValue[m_DragAxis] + delta * 2.0f;
      if (newScale > 0.01f) {
        cube.scale[m_DragAxis] = newScale;
      }
    }
  }
  
  if (!leftPressed && m_IsDraggingGizmo) {
    m_IsDraggingGizmo = false;
    m_DragAxis = -1;
  }
}

bool EditorLayer::GetMousePosInViewport(float &x, float &y) const {
  if (!m_SceneWindowHovered) return false;
  
  double mouseX, mouseY;
  glfwGetCursorPos(m_Window, &mouseX, &mouseY);
  
  // Convert to viewport-relative coordinates
  x = (float)mouseX - m_SceneViewportPosX;
  y = (float)mouseY - m_SceneViewportPosY;
  
  // Check if within bounds
  if (x < 0 || y < 0 || x > m_SceneViewportWidth || y > m_SceneViewportHeight) {
    return false;
  }
  
  return true;
}
