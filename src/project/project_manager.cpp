#include "project_manager.h"
#include <sstream>
#include <iomanip>

bool ProjectManager::SaveProject(const std::string& filePath, const ProjectData& project) {
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo crear el archivo de proyecto: " << filePath << std::endl;
        return false;
    }
    
    try {
        // Escribir cabecera del archivo
        WriteString(file, "MARIOENGINE_PROJECT");
        WriteString(file, project.version);
        WriteString(file, project.projectName);
        
        // Escribir número de cubos
        uint32_t cubeCount = static_cast<uint32_t>(project.cubes.size());
        file.write(reinterpret_cast<const char*>(&cubeCount), sizeof(cubeCount));
        
        // Escribir datos de cada cubo
        for (const auto& cube : project.cubes) {
            WriteFloat3(file, cube.pos);
            WriteFloat3(file, cube.rotation);
            WriteFloat3(file, cube.scale);
            WriteString(file, cube.materialPath);
        }
        
        // Escribir número de archivos importados
        uint32_t fileCount = static_cast<uint32_t>(project.importedFiles.size());
        file.write(reinterpret_cast<const char*>(&fileCount), sizeof(fileCount));
        
        // Escribir rutas de archivos importados
        for (const auto& importedFile : project.importedFiles) {
            WriteString(file, importedFile);
        }
        
        file.close();
        std::cout << "Proyecto guardado exitosamente: " << filePath << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error al guardar proyecto: " << e.what() << std::endl;
        file.close();
        return false;
    }
}

bool ProjectManager::LoadProject(const std::string& filePath, ProjectData& project) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo de proyecto: " << filePath << std::endl;
        return false;
    }
    
    try {
        // Leer y verificar cabecera
        std::string header = ReadString(file);
        if (header != "MARIOENGINE_PROJECT") {
            std::cerr << "Error: Archivo de proyecto inválido (cabecera incorrecta)" << std::endl;
            file.close();
            return false;
        }
        
        // Leer versión y nombre del proyecto
        project.version = ReadString(file);
        project.projectName = ReadString(file);
        
        // Leer número de cubos
        uint32_t cubeCount;
        file.read(reinterpret_cast<char*>(&cubeCount), sizeof(cubeCount));
        
        // Leer datos de cada cubo
        project.cubes.clear();
        project.cubes.reserve(cubeCount);
        
        for (uint32_t i = 0; i < cubeCount; ++i) {
            CubeData cube;
            ReadFloat3(file, cube.pos);
            ReadFloat3(file, cube.rotation);
            ReadFloat3(file, cube.scale);
            cube.materialPath = ReadString(file);
            project.cubes.push_back(cube);
        }
        
        // Leer número de archivos importados
        uint32_t fileCount;
        file.read(reinterpret_cast<char*>(&fileCount), sizeof(fileCount));
        
        // Leer rutas de archivos importados
        project.importedFiles.clear();
        project.importedFiles.reserve(fileCount);
        
        for (uint32_t i = 0; i < fileCount; ++i) {
            std::string importedFile = ReadString(file);
            project.importedFiles.push_back(importedFile);
        }
        
        file.close();
        std::cout << "Proyecto cargado exitosamente: " << filePath << std::endl;
        std::cout << "- Nombre: " << project.projectName << std::endl;
        std::cout << "- Versión: " << project.version << std::endl;
        std::cout << "- Cubos: " << project.cubes.size() << std::endl;
        std::cout << "- Archivos importados: " << project.importedFiles.size() << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error al cargar proyecto: " << e.what() << std::endl;
        file.close();
        return false;
    }
}

std::string ProjectManager::GetProjectExtension() {
    // Leer el nombre del proyecto desde config.txt
    std::ifstream configFile("config.txt");
    if (!configFile.is_open()) {
        // Intentar rutas alternativas
        const char* candidates[] = { "config.txt", "../config.txt", "../../config.txt" };
        for (const char* path : candidates) {
            configFile.close();
            configFile.clear();
            configFile.open(path);
            if (configFile.is_open()) break;
        }
        
        if (!configFile.is_open()) {
            return ".MarioEngine"; // Valor por defecto
        }
    }
    
    std::string line;
    while (std::getline(configFile, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        if (key == "project_name") {
            configFile.close();
            return "." + value;
        }
    }
    
    configFile.close();
    return ".MarioEngine"; // Valor por defecto
}

bool ProjectManager::IsValidProjectFile(const std::string& filePath) {
    if (!std::filesystem::exists(filePath)) {
        return false;
    }
    
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    try {
        std::string header = ReadString(file);
        file.close();
        return header == "MARIOENGINE_PROJECT";
    } catch (...) {
        file.close();
        return false;
    }
}

std::string ProjectManager::CreateProjectFileName(const std::string& baseName) {
    std::string extension = GetProjectExtension();
    std::string fileName = baseName;
    
    // Remover extensión existente si la hay
    size_t lastDot = fileName.find_last_of('.');
    if (lastDot != std::string::npos) {
        fileName = fileName.substr(0, lastDot);
    }
    
    return fileName + extension;
}

void ProjectManager::WriteFloat3(std::ofstream& file, const float values[3]) {
    file.write(reinterpret_cast<const char*>(values), 3 * sizeof(float));
}

void ProjectManager::ReadFloat3(std::ifstream& file, float values[3]) {
    file.read(reinterpret_cast<char*>(values), 3 * sizeof(float));
}

void ProjectManager::WriteString(std::ofstream& file, const std::string& str) {
    uint32_t length = static_cast<uint32_t>(str.length());
    file.write(reinterpret_cast<const char*>(&length), sizeof(length));
    file.write(str.c_str(), length);
}

std::string ProjectManager::ReadString(std::ifstream& file) {
    uint32_t length;
    file.read(reinterpret_cast<char*>(&length), sizeof(length));
    
    std::string str(length, '\0');
    file.read(&str[0], length);
    return str;
}
