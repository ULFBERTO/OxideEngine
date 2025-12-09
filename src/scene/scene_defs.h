#pragma once

#include <string>
#include <vector>

// Estructura para materiales
struct Material {
    std::string name;
    std::string diffuseTexture = "";
    float color[3] = {1.0f, 1.0f, 1.0f}; // RGB
    float metallic = 0.0f;
    float roughness = 0.5f;
    float emission = 0.0f;
};

// Objetos de escena y recursos de malla
struct CubeInst { 
    float pos[3]; 
    float rotation[3];  // rotación en grados (X, Y, Z)
    float scale[3];     // escala (X, Y, Z)
    bool selected;
    std::string materialPath; // ruta al archivo de material
    
    CubeInst() : selected(false), materialPath("") { 
        pos[0] = pos[1] = pos[2] = 0.0f; 
        rotation[0] = rotation[1] = rotation[2] = 0.0f;
        scale[0] = scale[1] = scale[2] = 1.0f;
    }
};
