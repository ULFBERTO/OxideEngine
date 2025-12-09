#include "editor_layer.h"
#include <iostream>
#include "imgui.h"
#include "../scene/scene.h"
#include "../project/project_manager.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

EditorLayer::EditorLayer() {}

EditorLayer::~EditorLayer() {
    Shutdown();
}

void EditorLayer::Init(GLFWwindow* window) {
    InitImGui(window);
}

void EditorLayer::InitImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigDockingAlwaysTabBar = true;

    ImGui::StyleColorsDark();
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void EditorLayer::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void EditorLayer::Render(Scene& scene) {
    // Start Frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    DrawDockSpace(scene);
    
    if (m_ShowHierarchy) DrawHierarchy(scene);
    if (m_ShowProperties) DrawProperties(scene);
    DrawFileExplorer();
    
    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorLayer::DrawDockSpace(Scene& scene) {
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    ImGui::PopStyleVar(3);
    
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Project")) {
                ProjectData data;
                if (ProjectManager::LoadProject("myproject.MarioEngine", data)) {
                    scene.LoadFromProject(data);
                }
            }
            if (ImGui::MenuItem("Save Project")) {
                ProjectData data;
                data.projectName = "MyProject";
                scene.GetProjectData(data);
                ProjectManager::SaveProject("myproject.MarioEngine", data);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) { /* TODO: Close App */ }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Hierarchy", nullptr, &m_ShowHierarchy);
            ImGui::MenuItem("Properties", nullptr, &m_ShowProperties);
            ImGui::MenuItem("File Explorer", nullptr, &m_ShowFileExplorer);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    
    ImGui::End();
}

void EditorLayer::DrawHierarchy(Scene& scene) {
    if (ImGui::Begin("Hierarchy", &m_ShowHierarchy)) {
        if (ImGui::Button("Add Cube")) {
            CubeInst c;
            c.pos[0] = 0; c.pos[1] = 0; c.pos[2] = 0;
            scene.AddCube(c);
        }
        ImGui::Separator();
        
        auto& cubes = scene.GetCubes();
        for (int i = 0; i < (int)cubes.size(); ++i) {
            std::string label = "Cube " + std::to_string(i);
            if (ImGui::Selectable(label.c_str(), cubes[i].selected)) {
                // Deselect all
                for (auto& c : cubes) c.selected = false;
                cubes[i].selected = true;
                m_SelectedCubeIndex = i;
            }
        }
        if (cubes.empty()) ImGui::Text("No objects");
    }
    ImGui::End();
}

void EditorLayer::DrawProperties(Scene& scene) {
    if (ImGui::Begin("Properties", &m_ShowProperties)) {
        if (m_SelectedCubeIndex >= 0 && m_SelectedCubeIndex < (int)scene.GetCubes().size()) {
            CubeInst& cube = scene.GetCubes()[m_SelectedCubeIndex];
            if (cube.selected) {
                ImGui::Text("Transform");
                ImGui::DragFloat3("Pos", cube.pos, 0.1f);
                ImGui::DragFloat3("Rot", cube.rotation, 1.0f);
                ImGui::DragFloat3("Scale", cube.scale, 0.1f);
                
                ImGui::Separator();
                ImGui::Text("Material: %s", cube.materialPath.c_str());
            } else {
                // Sync issue: m_SelectedCubeIndex might be stale if selection changed elsewhere
                m_SelectedCubeIndex = -1;
            }
        } else {
            ImGui::Text("No object selected");
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
