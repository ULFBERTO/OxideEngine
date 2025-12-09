#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>

class Application {
public:
    Application();
    ~Application();

    // Inicializa GLFW, ventana y contextos
    bool Init();

    // Loop principal (Aún no implementado completamente)
    void Run();

    // Obtener la ventana cruda (para transición)
    GLFWwindow* GetWindow() const { return m_Window; }

private:
    GLFWwindow* m_Window;
    int m_Width;
    int m_Height;
    std::string m_Title;
};
