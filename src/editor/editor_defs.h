#pragma once

#include <string>
#include <glad/glad.h>

// Estructura para manejar iconos cargados
struct IconTexture {
    GLuint textureID = 0;
    int width = 0;
    int height = 0;
    bool loaded = false;
};

// Enum para tipos de archivos
enum FileType {
    FILETYPE_UNKNOWN,
    FILETYPE_TEXTURE,   // .png, .jpg, .bmp
    FILETYPE_AUDIO,     // .wav, .ogg
    FILETYPE_MODEL,     // .obj
    FILETYPE_SCENE,     // .scene
    FILETYPE_MATERIAL,  // .mat
    FILETYPE_FOLDER
};

// Estructura para representar un archivo/carpeta
struct FileItem {
    std::string name;
    std::string path;
    FileType type;
    bool isDirectory;
};
