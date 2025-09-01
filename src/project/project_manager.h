#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>

// Estructura para representar un cubo en la escena
struct CubeData {
    float pos[3];
    float rotation[3];
    float scale[3];
    std::string materialPath;
    
    CubeData() {
        pos[0] = pos[1] = pos[2] = 0.0f;
        rotation[0] = rotation[1] = rotation[2] = 0.0f;
        scale[0] = scale[1] = scale[2] = 1.0f;
        materialPath = "";
    }
    
    CubeData(const float p[3], const float r[3], const float s[3], const std::string& matPath = "") {
        for (int i = 0; i < 3; i++) {
            pos[i] = p[i];
            rotation[i] = r[i];
            scale[i] = s[i];
        }
        materialPath = matPath;
    }
};

// Estructura para representar un proyecto completo
struct ProjectData {
    std::string projectName;
    std::string version;
    std::vector<CubeData> cubes;
    std::vector<std::string> importedFiles;
    
    ProjectData() : version("1.0") {}
};

class ProjectManager {
public:
    // Guardar proyecto en formato .MarioEngine
    static bool SaveProject(const std::string& filePath, const ProjectData& project);
    
    // Cargar proyecto desde archivo .MarioEngine
    static bool LoadProject(const std::string& filePath, ProjectData& project);
    
    // Obtener extensión del proyecto basada en config.txt
    static std::string GetProjectExtension();
    
    // Validar si un archivo es un proyecto válido
    static bool IsValidProjectFile(const std::string& filePath);
    
    // Crear nombre de archivo de proyecto con extensión correcta
    static std::string CreateProjectFileName(const std::string& baseName);
    
private:
    // Funciones auxiliares para serialización
    static void WriteFloat3(std::ofstream& file, const float values[3]);
    static void ReadFloat3(std::ifstream& file, float values[3]);
    static void WriteString(std::ofstream& file, const std::string& str);
    static std::string ReadString(std::ifstream& file);
};
