#include "application.h"
#include "../camera/camera.h"
#include "../editor/editor_layer.h"
#include "../scene/scene.h"
#include "../shaders/shader.h"
#include "imgui.h"
#include <cmath>
#include <iostream>

// Forward declaration if not included
void framebuffer_size_callback(GLFWwindow *window, int width, int height);

Application::Application()
    : m_Window(nullptr), m_Width(1280), m_Height(720),
      m_Title("MarioEngine Editor") {}

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
  m_Window =
      glfwCreateWindow(m_Width, m_Height, m_Title.c_str(), nullptr, nullptr);
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
  glfwSetScrollCallback(m_Window, scroll_callback);

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
  const char *vs3D = R"(#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 uMVP;
void main(){ gl_Position = uMVP * vec4(aPos, 1.0); }
)";
  const char *fs3D = R"(#version 330 core
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

    // Get viewport size from editor for matrices
    float vpW_gizmo, vpH_gizmo;
    editor.GetSceneViewportSize(vpW_gizmo, vpH_gizmo);
    if (vpW_gizmo <= 0) vpW_gizmo = 800;
    if (vpH_gizmo <= 0) vpH_gizmo = 600;
    
    float aspect_gizmo = vpW_gizmo / vpH_gizmo;
    Mat4 proj_gizmo = mat4_perspective(45.0f * 3.1415926f / 180.0f, aspect_gizmo, 0.1f, 100.0f);
    Mat4 view_gizmo = create_view_matrix(get_camera_position(), get_camera_front(), get_camera_up());
    
    // --- INPUT: Gizmo Interaction ---
    editor.HandleGizmoInput(scene, m_Window, view_gizmo, proj_gizmo);

    // --- INPUT: Selection (only if not dragging gizmo and mouse is in viewport) ---
    static bool mousePressedLastFrame = false;
    bool mousePressed =
        glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    
    float viewportMouseX, viewportMouseY;
    bool mouseInViewport = editor.GetMousePosInViewport(viewportMouseX, viewportMouseY);
    
    if (mousePressed && !mousePressedLastFrame && mouseInViewport && !editor.IsDraggingGizmo() && !editor.IsHoveringGizmo()) {
      // Get viewport size for raycast
      float vpW, vpH;
      editor.GetSceneViewportSize(vpW, vpH);
      if (vpW <= 0) vpW = 800;
      if (vpH <= 0) vpH = 600;

      // Convert viewport mouse position to NDC
      float x = (2.0f * viewportMouseX) / vpW - 1.0f;
      float y = 1.0f - (2.0f * viewportMouseY) / vpH;
      float z = 1.0f;
      float ray_nds[3] = {x, y, z};
      float ray_clip[4] = {ray_nds[0], ray_nds[1], -1.0, 1.0};

      // Inverse Projection using viewport aspect ratio
      float aspect = vpW / vpH;
      Mat4 proj =
          mat4_perspective(45.0f * 3.1415926f / 180.0f, aspect, 0.1f, 100.0f);
      // Invert Proj (Simplified for perspective)
      // ... actually better to use a math library invert function, but we don't
      // have one in math_utils? Let's assume math_utils is limited.
      // Alternative: Use existing logic if any, basically unproject.
      // For now, implementing a basic unproject approximation or adding Invert
      // to math_utils would be best. Let's implement a simple ray calculation
      // assuming standard perspective:

      float tanHalfFovy = tanf(45.0f * 3.1415926f / 360.0f); // 45 degree
      float rx = x * aspect * tanHalfFovy;
      float ry = y * tanHalfFovy;
      float rz = -1.0f; // camera facing -Z

      float ray_eye[3] = {rx, ry, rz};
      vec3_normalize(ray_eye);

      // View Matrix to World
      Mat4 view = create_view_matrix(get_camera_position(), get_camera_front(),
                                     get_camera_up());
      // Transpose of rotation part of View is effectively Inverse (if
      // orthogonal) View = R * T. Inverse = T^-1 * R^-1. Actually, simply
      // transforming the ray direction by the camera's orientation is enough.
      // Camera Basis: Front, Up, Right.
      const float *F = get_camera_front();
      const float *U = get_camera_up();
      float R[3];
      vec3_cross(F, U, R); // Right
      vec3_normalize(R);
      // Real Up might differ if we use Gram-Schmidt, but U is world up usually?
      // In camera.cpp: zaxis = -front. xaxis = cross(Up, zaxis). yaxis =
      // cross(zaxis, xaxis). matrix rows were xaxis, yaxis, zaxis. So Inverse
      // Rotation columns are xaxis, yaxis, zaxis.

      // ray_world = ray_eye.x * Right + ray_eye.y * Up + ray_eye.z * (-Front)
      // ?? Let's check coord systems. Camera looks down -Z.

      float dx = ray_eye[0];
      float dy = ray_eye[1];
      float dz = ray_eye[2];

      // View Matrix m[0,4,8] is Right (X local). m[1,5,9] is Up (Y local).
      // m[2,6,10] is -Front (Z local). We want to transform Direction (dx, dy,
      // dz) from Camera Local to World. Local X is aligned with Right. Local Y
      // with Real Up. Local Z with Back (-Front).

      float world_dir[3];
      // Right = {view.m[0], view.m[4], view.m[8]}
      // Up    = {view.m[1], view.m[5], view.m[9]}
      // Back  = {view.m[2], view.m[6], view.m[10]}

      for (int k = 0; k < 3; ++k)
        world_dir[k] = dx * view.m[k * 4 + 0] + dy * view.m[k * 4 + 1] +
                       dz * view.m[k * 4 + 2];
      vec3_normalize(world_dir);

      int hit = scene.Raycast(get_camera_position(), world_dir);

      // Handle selection
      if (hit != -1) {
        // Deselect all
        for (auto &c : scene.GetCubes())
          c.selected = false;
        scene.GetCubes()[hit].selected = true;
        editor.SetSelectedCubeIndex(hit);
      } else {
        for (auto &c : scene.GetCubes())
          c.selected = false;
        editor.SetSelectedCubeIndex(-1);
      }
    }
    mousePressedLastFrame = mousePressed;

    // Get viewport size from editor
    float vpWidth, vpHeight;
    editor.GetSceneViewportSize(vpWidth, vpHeight);
    if (vpWidth <= 0) vpWidth = 800;
    if (vpHeight <= 0) vpHeight = 600;

    // Render scene to framebuffer
    editor.BeginSceneRender();
    
    float aspect = vpWidth / vpHeight;
    Mat4 proj =
        mat4_perspective(45.0f * 3.1415926f / 180.0f, aspect, 0.1f, 100.0f);
    Mat4 view = create_view_matrix(get_camera_position(), get_camera_front(),
                                   get_camera_up());

    scene.Render(view, proj, shaderProgram);
    scene.RenderGizmos(view, proj, shaderProgram, editor.GetSelectedCubeIndex(),
                       editor.GetTransformMode(), editor.GetHoveredAxis(),
                       editor.IsLocalSpace());
    
    editor.EndSceneRender();

    // Render main window
    int display_w, display_h;
    glfwGetFramebufferSize(m_Window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Editor Render (ImGui)
    editor.Render(scene);

    glfwSwapBuffers(m_Window);
  }
}
