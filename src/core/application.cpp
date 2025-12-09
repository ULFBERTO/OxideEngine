#include "application.h"
#include <iostream>
#include <cmath>
#include "imgui.h"
#include "../camera/camera.h"
#include "../scene/scene.h"
#include "../editor/editor_layer.h"
#include "../shaders/shader.h"

// Forward declaration if not included
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

Application::Application() : m_Window(nullptr), m_Width(1280), m_Height(720), m_Title("MarioEngine Editor") {
}

Application::~Application() {
    // Cleanup se puede mover aquí
    // glfwTerminate(); // Cuidado si se llama multiple veces
}

bool Application::Init() {
    // 1. Inicializar GLFW
    if (!glfwInit()) {
        std::cerr << "Fallo al inicializar GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    // 2. Crear Ventana
    m_Window = glfwCreateWindow(m_Width, m_Height, m_Title.c_str(), nullptr, nullptr);
    if (m_Window == nullptr) {
        std::cerr << "Fallo al crear ventana GLFW" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(m_Window);

    // 3. Callbacks
    glfwSetFramebufferSizeCallback(m_Window, framebuffer_size_callback);
    glfwSetCursorPosCallback(m_Window, mouse_callback);
    glfwSetMouseButtonCallback(m_Window, mouse_button_callback);

    // Init Camera Global State
    init_camera(m_Width, m_Height);

    // 4. Cargar GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Fallo al inicializar GLAD" << std::endl;
        return false;
    }

    // Config global OpenGL
    glEnable(GL_DEPTH_TEST);
    
    // Icono (placeholder logic)

    return true;
}

void Application::Run() {
    // Systems
    Scene scene;
    scene.Init();
    
    EditorLayer editor;
    editor.Init(m_Window);
    
    // Shader (Temporary location)
    const char* vs3D = R"(#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 uMVP;
void main(){ gl_Position = uMVP * vec4(aPos, 1.0); }
)";
    const char* fs3D = R"(#version 330 core
uniform vec4 uColor;
out vec4 FragColor;
void main(){ FragColor = uColor; }
)";
    GLuint shaderProgram = Shader::createProgram(vs3D, fs3D);
    
    // Loop
    while (!glfwWindowShouldClose(m_Window)) {
        glfwPollEvents();
        
        // Input logic (Camera)
        // Note: Camera handling is still effectively global/static in camera.cpp
        // We need to call update_camera_direction/position in loop
        // For now, assuming camera.cpp's functions are available
        update_camera_direction();
        update_camera_direction();
        update_camera_position(0.016f, m_Window); // fixed dt for now
        
        // --- INPUT: Selection ---
        static bool mousePressedLastFrame = false;
        bool mousePressed = glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (mousePressed && !mousePressedLastFrame && !ImGui::GetIO().WantCaptureMouse) {
             // Raycast logic
             double xpos, ypos;
             glfwGetCursorPos(m_Window, &xpos, &ypos);
             int width, height;
             glfwGetWindowSize(m_Window, &width, &height);
             
             float x = (2.0f * (float)xpos) / width - 1.0f;
             float y = 1.0f - (2.0f * (float)ypos) / height;
             float z = 1.0f;
             float ray_nds[3] = { x, y, z };
             float ray_clip[4] = { ray_nds[0], ray_nds[1], -1.0, 1.0 };
             
             // Inverse Projection
             float aspect = (float)width/(float)height;
             Mat4 proj = mat4_perspective(45.0f * 3.1415926f/180.0f, aspect, 0.1f, 100.0f);
             // Invert Proj (Simplified for perspective)
             // ... actually better to use a math library invert function, but we don't have one in math_utils?
             // Let's assume math_utils is limited. 
             // Alternative: Use existing logic if any, basically unproject.
             // For now, implementing a basic unproject approximation or adding Invert to math_utils would be best.
             // Let's implement a simple ray calculation assuming standard perspective:
             
             float tanHalfFovy = tanf(45.0f * 3.1415926f / 360.0f); // 45 degree
             float rx = x * aspect * tanHalfFovy;
             float ry = y * tanHalfFovy;
             float rz = -1.0f; // camera facing -Z
             
             float ray_eye[3] = { rx, ry, rz };
             vec3_normalize(ray_eye);
             
             // View Matrix to World
             Mat4 view = create_view_matrix(get_camera_position(), get_camera_front(), get_camera_up());
             // Transpose of rotation part of View is effectively Inverse (if orthogonal)
             // View = R * T. Inverse = T^-1 * R^-1. 
             // Actually, simply transforming the ray direction by the camera's orientation is enough.
             // Camera Basis: Front, Up, Right.
             const float* F = get_camera_front();
             const float* U = get_camera_up();
             float R[3];
             vec3_cross(F, U, R); // Right
             vec3_normalize(R);
             // Real Up might differ if we use Gram-Schmidt, but U is world up usually? 
             // In camera.cpp: zaxis = -front. xaxis = cross(Up, zaxis). yaxis = cross(zaxis, xaxis).
             // matrix rows were xaxis, yaxis, zaxis. 
             // So Inverse Rotation columns are xaxis, yaxis, zaxis.
             
             // ray_world = ray_eye.x * Right + ray_eye.y * Up + ray_eye.z * (-Front) ??
             // Let's check coord systems. Camera looks down -Z. 
             
             float dx = ray_eye[0]; 
             float dy = ray_eye[1];
             float dz = ray_eye[2];
             
             // View Matrix m[0,4,8] is Right (X local). m[1,5,9] is Up (Y local). m[2,6,10] is -Front (Z local).
             // We want to transform Direction (dx, dy, dz) from Camera Local to World.
             // Local X is aligned with Right. Local Y with Real Up. Local Z with Back (-Front).
             
             float world_dir[3];
             // Right = {view.m[0], view.m[4], view.m[8]}
             // Up    = {view.m[1], view.m[5], view.m[9]}
             // Back  = {view.m[2], view.m[6], view.m[10]}
             
             for(int k=0; k<3; ++k) world_dir[k] = dx * view.m[k*4+0] + dy * view.m[k*4+1] + dz * view.m[k*4+2];
             vec3_normalize(world_dir);
             
             int hit = scene.Raycast(get_camera_position(), world_dir);
             
             // Handle selection
             if (hit != -1) {
                 // Deselect all
                 for(auto& c : scene.GetCubes()) c.selected = false;
                 scene.GetCubes()[hit].selected = true;
             } else {
                 for(auto& c : scene.GetCubes()) c.selected = false;
             }
        }
        mousePressedLastFrame = mousePressed;
        
        // Render
        int display_w, display_h;
        glfwGetFramebufferSize(m_Window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Scene Render
        float aspect = (float)display_w / (float)display_h;
        Mat4 proj = mat4_perspective(45.0f * 3.1415926f/180.0f, aspect, 0.1f, 100.0f);
        Mat4 view = create_view_matrix(get_camera_position(), get_camera_front(), get_camera_up());
        
        scene.Render(view, proj, shaderProgram);
        
        // Editor Render
        editor.Render(scene);
        
        glfwSwapBuffers(m_Window);
    }
}
