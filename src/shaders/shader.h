#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

namespace Shader {

GLuint compileShader(GLenum type, const char* src);
GLuint createProgram(const char* vs, const char* fs);

} // namespace Shader

#endif // SHADER_H
