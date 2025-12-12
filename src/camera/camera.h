#ifndef CAMERA_H
#define CAMERA_H

#include <GLFW/glfw3.h>

extern float camYaw;
extern float camPitch;
extern float targetYaw;
extern float targetPitch;
extern float lastX;
extern float lastY;
extern bool firstMouse;
extern float camPos[3];
extern float camFront[3];
extern float camUp[3];

void init_camera(unsigned int width, unsigned int height);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void update_camera_direction();
void update_camera_position(float dt, GLFWwindow *window);

const float *get_camera_position();
const float *get_camera_front();
const float *get_camera_up();

#endif // CAMERA_H
