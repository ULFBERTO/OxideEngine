#include "camera.h"
#include <GLFW/glfw3.h>

float camYaw = -90.0f;
float camPitch = 0.0f;
float targetYaw = -90.0f;
float targetPitch = 0.0f;
float lastX;
float lastY;
bool firstMouse = true;
bool rightMousePressed = false;
float camPos[3] = {0.0f, 1.2f, 4.0f};
float camFront[3] = {0.0f, 0.0f, -1.0f};
float camUp[3] = {0.0f, 1.0f, 0.0f};

void init_camera(unsigned int width, unsigned int height) {
  lastX = static_cast<float>(width) * 0.5f;
  lastY = static_cast<float>(height) * 0.5f;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
  if (!rightMousePressed)
    return; // Solo rotar si el botón derecho está presionado

  float x = static_cast<float>(xpos);
  float y = static_cast<float>(ypos);
  if (firstMouse) {
    lastX = x;
    lastY = y;
    firstMouse = false;
  }
  float xoffset = x - lastX;
  float yoffset = lastY - y; // inverted: positive up
  lastX = x;
  lastY = y;

  const float sensitivity = 0.08f;
  xoffset *= sensitivity;
  yoffset *= sensitivity;

  targetYaw += xoffset;
  targetPitch += yoffset;
  if (targetPitch > 80.0f)
    targetPitch = 80.0f;
  if (targetPitch < -80.0f)
    targetPitch = -80.0f;
  // do not recalculate camFront here; done in loop with smoothing
}

void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods) {
  if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    if (action == GLFW_PRESS) {
      rightMousePressed = true;
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      firstMouse = true; // Reset para evitar saltos
    } else if (action == GLFW_RELEASE) {
      rightMousePressed = false;
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
  }
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  float speed = 0.5f;
  camPos[0] += camFront[0] * (float)yoffset * speed;
  camPos[1] += camFront[1] * (float)yoffset * speed;
  camPos[2] += camFront[2] * (float)yoffset * speed;
}

#include "../utils/math_utils.h" // For vec3_cross and vec3_normalize
#include <cmath>                 // For cosf, sinf, sqrtf


void update_camera_direction() {
  camYaw = targetYaw;
  camPitch = targetPitch;
  if (camYaw > 720.0f)
    camYaw -= 360.0f;
  if (camYaw < -720.0f)
    camYaw += 360.0f;
  float yawR = camYaw * 3.1415926f / 180.0f;
  float pitchR = camPitch * 3.1415926f / 180.0f;
  float dir[3] = {cosf(yawR) * cosf(pitchR), sinf(pitchR),
                  sinf(yawR) * cosf(pitchR)};
  float len = sqrtf(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
  if (len > 0.0f) {
    camFront[0] = dir[0] / len;
    camFront[1] = dir[1] / len;
    camFront[2] = dir[2] / len;
  } else {
    camFront[0] = 0.0f;
    camFront[1] = 0.0f;
    camFront[2] = -1.0f; // default
  }
}

void update_camera_position(float dt, GLFWwindow *window) {
  float speed =
      (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 6.0f : 3.0f;
  float velocity = speed * dt;
  float forward[3] = {camFront[0], camFront[1], camFront[2]};
  float worldUp[3] = {0.0f, 1.0f, 0.0f};
  float right[3];
  vec3_cross(forward, worldUp, right);
  vec3_normalize(right);
  float up[3];
  vec3_cross(right, forward, up);
  vec3_normalize(up);
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    camPos[0] += forward[0] * velocity;
    camPos[1] += forward[1] * velocity;
    camPos[2] += forward[2] * velocity;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    camPos[0] -= forward[0] * velocity;
    camPos[1] -= forward[1] * velocity;
    camPos[2] -= forward[2] * velocity;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    camPos[0] -= right[0] * velocity;
    camPos[1] -= right[1] * velocity;
    camPos[2] -= right[2] * velocity;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    camPos[0] += right[0] * velocity;
    camPos[1] += right[1] * velocity;
    camPos[2] += right[2] * velocity;
  }
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
    camPos[1] -= velocity; // Bajar
  }
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
    camPos[1] += velocity; // Subir
  }
}

const float *get_camera_position() { return camPos; }

const float *get_camera_front() { return camFront; }

const float *get_camera_up() { return camUp; }
