#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <sstream>
#include <filesystem>
#include <map>
#include <chrono>
#include <iomanip>
#include "utils/math_utils.h"
#include "camera/camera.h" // Include camera header
#include "shaders/shader.h"
#include "project/project_manager.h"

// Dear ImGui
#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// Definiciones de iconos simples (compatibles con todas las fuentes)
#define ICON_FA_FOLDER "[DIR]"
#define ICON_FA_IMAGE "[IMG]"
#define ICON_FA_VOLUME_UP "[SND]"
#define ICON_FA_CUBE "[3D]"
#define ICON_FA_GLOBE "[SCN]"
#define ICON_FA_PALETTE "[MAT]"
#define ICON_FA_FILE "[FILE]"
#define ICON_FA_ARROW_LEFT "<-"
#define ICON_FA_HOME "[HOME]"
#define ICON_FA_EDIT "[EDIT]"
#define ICON_FA_PLUS "[+]"

// Estructura para manejar iconos cargados
struct IconTexture {
    GLuint textureID = 0;
    int width = 0;
    int height = 0;
    bool loaded = false;
};

// Mapa de iconos cargados
static std::map<std::string, IconTexture> g_loaded_icons;

// Asegurar que los docks no oculten la barra de pestañas (para ver la pestaña y la "X")
static void EnsureDockTabsVisible(ImGuiDockNode* node) {
    if (!node) return;
    // Asegurar pestañas visibles y botón de cierre habilitado
    node->LocalFlags &= ~ImGuiDockNodeFlags_AutoHideTabBar;
    node->LocalFlags &= ~ImGuiDockNodeFlags_NoTabBar;
    node->LocalFlags &= ~ImGuiDockNodeFlags_HiddenTabBar;
    node->LocalFlags &= ~ImGuiDockNodeFlags_NoCloseButton;
    if (node->ChildNodes[0]) EnsureDockTabsVisible(node->ChildNodes[0]);
    if (node->ChildNodes[1]) EnsureDockTabsVisible(node->ChildNodes[1]);
}

// Recordar el ID del dock izquierdo para re-dockear la ventana de Presets
static ImGuiID g_dock_left_id = 0;

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
static std::vector<CubeInst> g_scene_cubes;
static int g_selected_cube_index = -1;
static std::vector<int> g_selected_cube_indices; // Para selección múltiple
static GLuint vaoGrid = 0, vboGrid = 0; static int gridVertexCount = 0;
static GLuint vaoCubeSolid = 0, vboCubeSolid = 0; static int cubeSolidVertexCount = 0;
// Gizmos VAOs y VBOs
static GLuint vaoGizmoArrow = 0, vboGizmoArrow = 0; static int gizmoArrowVertexCount = 0;
static GLuint vaoGizmoRing = 0, vboGizmoRing = 0; static int gizmoRingVertexCount = 0;
static GLuint vaoGizmoScale = 0, vboGizmoScale = 0; static int gizmoScaleVertexCount = 0;
static bool g_wireframe = false; // modo de visualización
static bool g_drag_active_last_frame = false; // para asegurar ventana de drop en el frame de entrega
static bool g_drag_preview_active = false; // modo preview: cubo sigue al puntero durante el drag
static int  g_drag_preview_index = -1;     // índice del cubo de preview en g_scene_cubes
static bool g_show_gizmos = true;          // mostrar gizmos de transformación
static bool g_show_properties = true;      // mostrar panel de propiedades
static bool g_show_tree_objects = false;   // mostrar panel de objetos
static bool g_show_file_explorer = false;  // mostrar explorador de archivos

// FileExplorer variables
static std::string g_content_path = "Content/";
static std::string g_current_directory = "Content/";

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

static std::vector<FileItem> g_current_files;

// Variables para el sistema de proyectos
static std::string g_current_project_path = "";
static bool g_project_modified = false;
// Variables eliminadas - ahora usamos diálogos nativos de Windows

// Funciones auxiliares para conversión entre CubeInst y CubeData
CubeData CubeInstToCubeData(const CubeInst& cube) {
    return CubeData(cube.pos, cube.rotation, cube.scale, cube.materialPath);
}

CubeInst CubeDataToCubeInst(const CubeData& data) {
    CubeInst cube;
    for (int i = 0; i < 3; i++) {
        cube.pos[i] = data.pos[i];
        cube.rotation[i] = data.rotation[i];
        cube.scale[i] = data.scale[i];
    }
    cube.materialPath = data.materialPath;
    cube.selected = false;
    return cube;
}

// Funciones para guardar y cargar proyecto
void SaveCurrentProject() {
    if (g_current_project_path.empty()) {
        // Crear nombre de archivo por defecto
        g_current_project_path = ProjectManager::CreateProjectFileName("proyecto1");
    }
    
    ProjectData project;
    project.projectName = "MarioEngine Project";
    
    // Convertir cubos de la escena
    for (const auto& cube : g_scene_cubes) {
        project.cubes.push_back(CubeInstToCubeData(cube));
    }
    
    // TODO: Agregar archivos importados cuando se implemente
    
    if (ProjectManager::SaveProject(g_current_project_path, project)) {
        g_project_modified = false;
        std::cout << "Proyecto guardado en: " << g_current_project_path << std::endl;
    }
}

void LoadProject(const std::string& filePath) {
    ProjectData project;
    if (ProjectManager::LoadProject(filePath, project)) {
        // Limpiar escena actual
        g_scene_cubes.clear();
        g_selected_cube_index = -1;
        
        // Cargar cubos del proyecto
        for (const auto& cubeData : project.cubes) {
            g_scene_cubes.push_back(CubeDataToCubeInst(cubeData));
        }
        
        g_current_project_path = filePath;
        g_project_modified = false;
        
        std::cout << "Proyecto cargado: " << project.cubes.size() << " objetos" << std::endl;
    }
}

// Función eliminada - ya no se usa con diálogos nativos

std::string OpenNativeFileDialog(bool saveDialog = false, const std::string& defaultName = "") {
#ifdef _WIN32
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    
    // Preparar nombre por defecto si se proporciona
    if (!defaultName.empty()) {
        std::string defaultWithExt = defaultName + ProjectManager::GetProjectExtension();
        strncpy_s(szFile, sizeof(szFile), defaultWithExt.c_str(), _TRUNCATE);
    }
    
    // Preparar filtro de archivos - usar buffer estático
    std::string extension = ProjectManager::GetProjectExtension();
    static char filter[256];
    std::string filterStr = "Archivos MarioEngine (*" + extension + ")";
    std::string filterPattern = "*" + extension;
    
    // Construir filtro manualmente
    strcpy_s(filter, sizeof(filter), filterStr.c_str());
    size_t pos = strlen(filter) + 1;
    strcpy_s(filter + pos, sizeof(filter) - pos, filterPattern.c_str());
    pos += strlen(filterPattern.c_str()) + 1;
    strcpy_s(filter + pos, sizeof(filter) - pos, "Todos los archivos (*.*)");
    pos += strlen("Todos los archivos (*.*)") + 1;
    strcpy_s(filter + pos, sizeof(filter) - pos, "*.*");
    pos += strlen("*.*") + 1;
    filter[pos] = '\0';
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (saveDialog) {
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        if (GetSaveFileNameA(&ofn)) {
            return std::string(szFile);
        }
    } else {
        if (GetOpenFileNameA(&ofn)) {
            return std::string(szFile);
        }
    }
#endif
    return ""; // Diálogo cancelado o no disponible
}

void SaveProjectAs() {
    // Generar nombre por defecto con timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    
#ifdef _WIN32
    std::tm timeinfo;
    localtime_s(&timeinfo, &time_t);
    ss << "proyecto_" << std::put_time(&timeinfo, "%Y%m%d_%H%M%S");
#else
    ss << "proyecto_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
#endif
    
    // Abrir diálogo nativo de Windows
    std::string selectedPath = OpenNativeFileDialog(true, ss.str());
    if (!selectedPath.empty()) {
        g_current_project_path = selectedPath;
        
        // Crear carpeta Content automáticamente en el directorio del proyecto
        std::filesystem::path projectDir = std::filesystem::path(selectedPath).parent_path();
        std::filesystem::path contentDir = projectDir / "Content";
        
        try {
            if (!std::filesystem::exists(contentDir)) {
                std::filesystem::create_directories(contentDir);
                
                // Crear subcarpetas estándar
                std::filesystem::create_directories(contentDir / "Materials");
                std::filesystem::create_directories(contentDir / "Models");
                std::filesystem::create_directories(contentDir / "Scenes");
                std::filesystem::create_directories(contentDir / "Audio");
                
                std::cout << "Estructura de carpetas Content creada en: " << contentDir << std::endl;
            }
            
            // Actualizar la ruta del explorador de archivos
            g_content_path = contentDir.string() + "/";
            g_current_directory = g_content_path;
            
        } catch (const std::filesystem::filesystem_error& ex) {
            std::cout << "Error creando carpeta Content: " << ex.what() << std::endl;
        }
        
        SaveCurrentProject();
    }
}

// Función para determinar el tipo de archivo por extensión
FileType GetFileType(const std::string& filename) {
    std::string ext = filename.substr(filename.find_last_of(".") + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "tga") {
        return FILETYPE_TEXTURE;
    } else if (ext == "wav" || ext == "ogg" || ext == "mp3") {
        return FILETYPE_AUDIO;
    } else if (ext == "obj" || ext == "fbx" || ext == "gltf") {
        return FILETYPE_MODEL;
    } else if (ext == "scene") {
        return FILETYPE_SCENE;
    } else if (ext == "mat") {
        return FILETYPE_MATERIAL;
    }
    return FILETYPE_UNKNOWN;
}

// Función para obtener el icono según el tipo de archivo
const char* GetFileIcon(FileType type) {
    switch (type) {
        case FILETYPE_FOLDER: return ICON_FA_FOLDER;
        case FILETYPE_TEXTURE: return ICON_FA_IMAGE;
        case FILETYPE_AUDIO: return ICON_FA_VOLUME_UP;
        case FILETYPE_MODEL: return ICON_FA_CUBE;
        case FILETYPE_SCENE: return ICON_FA_GLOBE;
        case FILETYPE_MATERIAL: return ICON_FA_PALETTE;
        default: return ICON_FA_FILE;
    }
}

// Variables para funcionalidades del File Explorer
static bool g_renaming_item = false;
static int g_renaming_index = -1;
static char g_rename_buffer[256] = "";
static bool g_show_create_folder_dialog = false;
static char g_new_folder_name[256] = "Nueva Carpeta";

// Variables para sistema de materiales
static bool g_show_create_dialog = false;
static bool g_show_import_dialog = false;
static bool g_show_material_editor = false;
static std::string g_editing_material_path = "";

// Estructura para materiales
struct Material {
    std::string name;
    std::string diffuseTexture = "";
    float color[3] = {1.0f, 1.0f, 1.0f}; // RGB
    float metallic = 0.0f;
    float roughness = 0.5f;
    float emission = 0.0f;
};

// Variables del editor de materiales
static Material g_current_material;
static char g_material_name_buffer[256] = "Nuevo Material";
static std::vector<std::string> g_available_textures;
static int g_selected_texture_index = -1;

// Variables para selector de materiales en propiedades
static std::vector<std::string> g_available_materials;
static char g_material_search_buffer[256] = "";

// Variables para copiar/pegar objetos
static std::vector<CubeInst> g_copied_cubes;
static bool g_has_copied_cubes = false;

// Sistema de deshacer (Undo)
struct UndoAction {
    std::vector<CubeInst> scene_state;
    int selected_index;
    std::vector<int> selected_indices;
};
static std::vector<UndoAction> g_undo_history;
static const int MAX_UNDO_HISTORY = 50;

// Función para guardar estado actual en historial
void SaveUndoState() {
    UndoAction action;
    action.scene_state = g_scene_cubes;
    action.selected_index = g_selected_cube_index;
    action.selected_indices = g_selected_cube_indices;
    
    g_undo_history.push_back(action);
    
    // Limitar tamaño del historial
    if (g_undo_history.size() > MAX_UNDO_HISTORY) {
        g_undo_history.erase(g_undo_history.begin());
    }
}

// Forward declarations
Material LoadMaterial(const std::string& filepath);
void ScanDirectory(const std::string& path);

// Función para escanear materiales disponibles
void ScanAvailableMaterials() {
    g_available_materials.clear();
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(g_content_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".mat") {
                g_available_materials.push_back(entry.path().string());
            }
        }
    } catch (const std::filesystem::filesystem_error& ex) {
        std::cout << "Error escaneando materiales: " << ex.what() << std::endl;
    }
}

// Función para aplicar material al shader
void ApplyMaterialToShader(const Material& material, GLuint shaderProgram) {
    // Aplicar color base
    GLint colorLoc = glGetUniformLocation(shaderProgram, "u_material_color");
    if (colorLoc != -1) {
        glUniform3f(colorLoc, material.color[0], material.color[1], material.color[2]);
    }
    
    // Aplicar propiedades del material
    GLint metallicLoc = glGetUniformLocation(shaderProgram, "u_material_metallic");
    if (metallicLoc != -1) {
        glUniform1f(metallicLoc, material.metallic);
    }
    
    GLint roughnessLoc = glGetUniformLocation(shaderProgram, "u_material_roughness");
    if (roughnessLoc != -1) {
        glUniform1f(roughnessLoc, material.roughness);
    }
    
    GLint emissionLoc = glGetUniformLocation(shaderProgram, "u_material_emission");
    if (emissionLoc != -1) {
        glUniform1f(emissionLoc, material.emission);
    }
}

// Función para obtener material por ruta de archivo
Material GetMaterialFromPath(const std::string& materialPath) {
    Material material;
    
    if (materialPath.empty()) {
        // Material por defecto
        material.color[0] = 0.8f;
        material.color[1] = 0.8f;
        material.color[2] = 0.8f;
        material.metallic = 0.0f;
        material.roughness = 0.5f;
        material.emission = 0.0f;
        return material;
    }
    
    // Cargar material desde archivo
    material = LoadMaterial(materialPath);
    return material;
}

// Función para cargar textura desde archivo de imagen
GLuint LoadTextureFromFile(const char* filename, int* width, int* height) {
    // Esta función requiere una librería como stb_image o SOIL
    // Por ahora retornamos 0 (sin textura)
    *width = 16;
    *height = 16;
    return 0;
}

// Función para cargar icono desde archivo
bool LoadIcon(const std::string& iconName, const std::string& filePath) {
    if (g_loaded_icons.find(iconName) != g_loaded_icons.end()) {
        return g_loaded_icons[iconName].loaded;
    }
    
    IconTexture icon;
    
    // Verificar si el archivo existe
    if (std::filesystem::exists(filePath)) {
        icon.textureID = LoadTextureFromFile(filePath.c_str(), &icon.width, &icon.height);
        icon.loaded = (icon.textureID != 0);
    } else {
        icon.loaded = false;
    }
    
    g_loaded_icons[iconName] = icon;
    return icon.loaded;
}

// Función para obtener icono cargado o fallback a Unicode
const char* GetFileIconWithTexture(FileType type, ImTextureID* textureOut = nullptr, ImVec2* sizeOut = nullptr) {
    std::string iconName;
    const char* fallbackIcon;
    
    switch (type) {
        case FILETYPE_FOLDER:
            iconName = "folder";
            fallbackIcon = ICON_FA_FOLDER;
            break;
        case FILETYPE_TEXTURE:
            iconName = "image";
            fallbackIcon = ICON_FA_IMAGE;
            break;
        case FILETYPE_AUDIO:
            iconName = "audio";
            fallbackIcon = ICON_FA_VOLUME_UP;
            break;
        case FILETYPE_MODEL:
            iconName = "model";
            fallbackIcon = ICON_FA_CUBE;
            break;
        case FILETYPE_SCENE:
            iconName = "scene";
            fallbackIcon = ICON_FA_GLOBE;
            break;
        case FILETYPE_MATERIAL:
            iconName = "material";
            fallbackIcon = ICON_FA_PALETTE;
            break;
        default:
            iconName = "file";
            fallbackIcon = ICON_FA_FILE;
            break;
    }
    
    // Intentar cargar el icono si no está cargado
    std::string iconPath = "Content/Icons/" + iconName + ".ico";
    if (g_loaded_icons.find(iconName) == g_loaded_icons.end()) {
        LoadIcon(iconName, iconPath);
    }
    
    // Si el icono está cargado, devolver la textura
    if (g_loaded_icons[iconName].loaded && textureOut && sizeOut) {
        *textureOut = (ImTextureID)(intptr_t)g_loaded_icons[iconName].textureID;
        *sizeOut = ImVec2((float)g_loaded_icons[iconName].width, (float)g_loaded_icons[iconName].height);
        return nullptr; // Usar textura en lugar de texto
    }
    
    return fallbackIcon; // Fallback a Unicode
}

// Función para escanear texturas disponibles
void ScanAvailableTextures() {
    g_available_textures.clear();
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(g_content_path)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
                    g_available_textures.push_back(entry.path().string());
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& ex) {
        std::cout << "Error escaneando texturas: " << ex.what() << std::endl;
    }
}

// Función para guardar material
void SaveMaterial(const Material& material, const std::string& filepath) {
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << "# Material MarioEngine\n";
        file << "name=" << material.name << "\n";
        file << "diffuseTexture=" << material.diffuseTexture << "\n";
        file << "color=" << material.color[0] << "," << material.color[1] << "," << material.color[2] << "\n";
        file << "metallic=" << material.metallic << "\n";
        file << "roughness=" << material.roughness << "\n";
        file << "emission=" << material.emission << "\n";
        file.close();
        std::cout << "Material guardado: " << filepath << std::endl;
    }
}

// Función para cargar material
Material LoadMaterial(const std::string& filepath) {
    Material material;
    std::ifstream file(filepath);
    std::string line;
    
    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                if (key == "name") {
                    material.name = value;
                } else if (key == "diffuseTexture") {
                    material.diffuseTexture = value;
                } else if (key == "color") {
                    std::stringstream ss(value);
                    std::string item;
                    int i = 0;
                    while (std::getline(ss, item, ',') && i < 3) {
                        material.color[i] = std::stof(item);
                        i++;
                    }
                } else if (key == "metallic") {
                    material.metallic = std::stof(value);
                } else if (key == "roughness") {
                    material.roughness = std::stof(value);
                } else if (key == "emission") {
                    material.emission = std::stof(value);
                }
            }
        }
        file.close();
    }
    
    return material;
}

// Función para crear nuevo material
void CreateNewMaterial() {
    std::string materialPath = g_current_directory + g_material_name_buffer + ".mat";
    
    // Verificar que no exista ya
    if (std::filesystem::exists(materialPath)) {
        std::cout << "Ya existe un material con ese nombre" << std::endl;
        return;
    }
    
    Material newMaterial;
    newMaterial.name = g_material_name_buffer;
    
    SaveMaterial(newMaterial, materialPath);
    ScanDirectory(g_current_directory);
    
    g_show_create_dialog = false;
    strcpy_s(g_material_name_buffer, sizeof(g_material_name_buffer), "Nuevo Material");
}

// Función para escanear directorio
void ScanDirectory(const std::string& path) {
    g_current_files.clear();
    
    // Agregar botón "Atrás" si no estamos en la raíz
    if (path != g_content_path) {
        FileItem backItem;
        backItem.name = "..";
        backItem.path = std::filesystem::path(path).parent_path().string() + "/";
        backItem.type = FILETYPE_FOLDER;
        backItem.isDirectory = true;
        g_current_files.push_back(backItem);
    }
    
    try {
        if (std::filesystem::exists(path)) {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                FileItem item;
                item.name = entry.path().filename().string();
                item.path = entry.path().string();
                item.isDirectory = entry.is_directory();
                
                if (item.isDirectory) {
                    item.type = FILETYPE_FOLDER;
                } else {
                    item.type = GetFileType(item.name);
                }
                
                g_current_files.push_back(item);
            }
        }
    } catch (const std::filesystem::filesystem_error& ex) {
        std::cout << "Error escaneando directorio: " << ex.what() << std::endl;
    }
    
    // Ordenar: carpetas primero, luego archivos
    std::sort(g_current_files.begin(), g_current_files.end(), [](const FileItem& a, const FileItem& b) {
        if (a.name == "..") return true;
        if (b.name == "..") return false;
        if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
        return a.name < b.name;
    });
}

enum TransformMode { TRANSLATE, ROTATE, SCALE };
static TransformMode g_transform_mode = TRANSLATE;
static bool g_gizmo_dragging = false;      // si se está arrastrando un gizmo
static int g_dragging_axis = -1;           // eje que se está arrastrando (-1=ninguno, 0=X, 1=Y, 2=Z)
static float g_drag_start_pos[2];          // posición inicial del mouse al empezar arrastre
static int g_hovered_gizmo_axis = -1;      // Para resaltado de hover
static float g_drag_start_object_pos[3];   // posición inicial del objeto
static float g_drag_start_object_rot[3];   // rotación inicial del objeto
static float g_drag_start_object_scale[3]; // escala inicial del objeto al empezar arrastre
static float g_drag_total_delta[2];        // delta acumulado desde el inicio del arrastre

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

static void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height) {
    glViewport(0, 0, width, height);
}

struct Config {
    float cube_size;
    int particle_count;
    std::string project_name;
    std::string window_title;
};

static Config load_config(const char* path) {
    Config cfg;
    std::ifstream in(path);
    if (!in) {
        // Intentar rutas alternativas comunes al ejecutar desde build dirs
        const char* candidates[] = { path, "../config.txt", "../../config.txt" };
        for (const char* p : candidates) {
            in.close();
            in.clear();
            in.open(p);
            if (in) {
                std::cerr << "Config cargado desde: " << p << "\n";
                break;
            }
        }
        if (!in) {
            std::cerr << "No se pudo abrir config en rutas conocidas, usando valores por defecto.\n";
            return cfg;
        }
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);
        if (key == "cube_size") cfg.cube_size = std::strtof(val.c_str(), nullptr);
        else if (key == "particle_count") cfg.particle_count = std::atoi(val.c_str());
        else if (key == "project_name") cfg.project_name = val;
        else if (key == "window_title") cfg.window_title = val;
    }
    return cfg;
}

// Matrix functions are now defined in math_utils.h/cpp

// Intersección rayo-AABB (método de slabs). Retorna true si hay impacto con t>=0 y escribe tHit
static bool RayAABB(const float ro[3], const float rd[3], const float bmin[3], const float bmax[3], float& tHit) {
    float tmin = -FLT_MAX;
    float tmax =  FLT_MAX;
    for (int i = 0; i < 3; ++i) {
        const float ro_i = ro[i];
        const float rd_i = rd[i];
        if (std::fabs(rd_i) < 1e-8f) {
            // Rayo paralelo a los planos de este eje: comprobar si origen está dentro del slab
            if (ro_i < bmin[i] || ro_i > bmax[i]) return false;
        } else {
            float invD = 1.0f / rd_i;
            float t0 = (bmin[i] - ro_i) * invD;
            float t1 = (bmax[i] - ro_i) * invD;
            if (t0 > t1) std::swap(t0, t1);
            if (t0 > tmin) tmin = t0;
            if (t1 < tmax) tmax = t1;
            if (tmax < tmin) return false;
        }
    }
    float t = (tmin >= 0.0f) ? tmin : ((tmax >= 0.0f) ? tmax : -1.0f);
    if (t < 0.0f) return false;
    tHit = t;
    return true;
}

// Intersección rayo-línea en 3D (para gizmos). Retorna distancia mínima y parámetro t
static bool RayLineIntersect(const float ro[3], const float rd[3], const float lineStart[3], const float lineEnd[3], float& t, float& distance) {
    float lineDir[3] = { lineEnd[0] - lineStart[0], lineEnd[1] - lineStart[1], lineEnd[2] - lineStart[2] };
    float lineLength = std::sqrt(lineDir[0]*lineDir[0] + lineDir[1]*lineDir[1] + lineDir[2]*lineDir[2]);
    if (lineLength < 1e-6f) return false;
    
    lineDir[0] /= lineLength; lineDir[1] /= lineLength; lineDir[2] /= lineLength;
    
    float toStart[3] = { lineStart[0] - ro[0], lineStart[1] - ro[1], lineStart[2] - ro[2] };
    
    // Producto cruz rd x lineDir
    float cross[3] = {
        rd[1] * lineDir[2] - rd[2] * lineDir[1],
        rd[2] * lineDir[0] - rd[0] * lineDir[2],
        rd[0] * lineDir[1] - rd[1] * lineDir[0]
    };
    
    float crossMag = std::sqrt(cross[0]*cross[0] + cross[1]*cross[1] + cross[2]*cross[2]);
    if (crossMag < 1e-6f) return false; // Líneas paralelas
    
    // Distancia entre las líneas
    float dot = toStart[0]*cross[0] + toStart[1]*cross[1] + toStart[2]*cross[2];
    distance = std::abs(dot) / crossMag;
    
    // Calcular parámetro t en la línea del gizmo
    float rdDotLine = rd[0]*lineDir[0] + rd[1]*lineDir[1] + rd[2]*lineDir[2];
    float toStartDotLine = toStart[0]*lineDir[0] + toStart[1]*lineDir[1] + toStart[2]*lineDir[2];
    float toStartDotRd = toStart[0]*rd[0] + toStart[1]*rd[1] + toStart[2]*rd[2];
    
    float denom = 1.0f - rdDotLine * rdDotLine;
    if (std::abs(denom) < 1e-6f) return false;
    
    t = (toStartDotLine - rdDotLine * toStartDotRd) / denom;
    
    return (t >= 0.0f && t <= lineLength && distance < 0.2f); // 0.2f es el radio de selección mejorado
}

int main() {
    // glfw: initialize and configure
    // ------------------------------
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    // glfw window creation
    // --------------------
    Config cfg = load_config("config.txt");
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, cfg.window_title.c_str(), NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    // No capturar el mouse por defecto - solo cuando se presione click derecho
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    init_camera(SCR_WIDTH, SCR_HEIGHT); // Initialize camera module after callbacks are set

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Dear ImGui: setup context and backends
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // keyboard navigation
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // enable docking
    io.ConfigDockingAlwaysTabBar = true;                  // always show tab bar even with a single window
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Set the viewport
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glLineWidth(1.0f);

    // Shaders mínimos para dibujar un triángulo
    // Config already loaded above
    if (cfg.particle_count < 1) cfg.particle_count = 1;

    // Shaders 3D (cubo y partículas comparten programa, con color uniforme)
    const char* vs3D = R"(#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 uMVP;
void main(){
    gl_Position = uMVP * vec4(aPos, 1.0);
    gl_PointSize = 5.0;
}
)";
    const char* fs3D = R"(#version 330 core
uniform vec4 uColor;
out vec4 FragColor;
void main(){ FragColor = uColor; }
)";
    GLuint prog3D = Shader::createProgram(vs3D, fs3D);
    if (!prog3D) {
        std::cerr << "Error: programa OpenGL no creado.\n";
        return -1;
    }
    GLint locMVP = glGetUniformLocation(prog3D, "uMVP");
    GLint locColor = glGetUniformLocation(prog3D, "uColor");
    if (locMVP == -1 || locColor == -1) {
        std::cerr << "Error: no se encontraron uniformes uMVP/uColor (locMVP=" << locMVP << ", locColor=" << locColor << ").\n";
    }

    // Cuadrícula en el plano y=0 (piso)
    {
        const int halfExtent = 50;
        const float step = 1.0f;
        std::vector<float> gridVerts;
        gridVerts.reserve((size_t)(halfExtent * 2 + 1) * 4 * 3);
        const float min = -halfExtent * step;
        const float max =  halfExtent * step;
        for (int i = -halfExtent; i <= halfExtent; ++i) {
            float v = i * step;
            // Línea paralela a X (variando Z)
            gridVerts.push_back(min); gridVerts.push_back(0.0f); gridVerts.push_back(v);
            gridVerts.push_back(max); gridVerts.push_back(0.0f); gridVerts.push_back(v);
            // Línea paralela a Z (variando X)
            gridVerts.push_back(v);   gridVerts.push_back(0.0f); gridVerts.push_back(min);
            gridVerts.push_back(v);   gridVerts.push_back(0.0f); gridVerts.push_back(max);
        }
        gridVertexCount = (int)(gridVerts.size() / 3);
        glGenVertexArrays(1, &vaoGrid);
        glGenBuffers(1, &vboGrid);
        glBindVertexArray(vaoGrid);
        glBindBuffer(GL_ARRAY_BUFFER, vboGrid);
        glBufferData(GL_ARRAY_BUFFER, gridVerts.size() * sizeof(float), gridVerts.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    // Cubo sólido unitario centrado en el origen (12 triángulos = 36 vértices)
    {
        const float c[] = {
            // Cara +X
            0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
            0.5f,-0.5f,-0.5f,  0.5f, 0.5f, 0.5f,  0.5f,-0.5f, 0.5f,
            // Cara -X
           -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
           -0.5f,-0.5f,-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f,
            // Cara +Y
           -0.5f, 0.5f,-0.5f, -0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
           -0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,
            // Cara -Y
           -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
           -0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f,
            // Cara +Z
           -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
           -0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
            // Cara -Z
           -0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
           -0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f
        };
        cubeSolidVertexCount = 36;
        glGenVertexArrays(1, &vaoCubeSolid);
        glGenBuffers(1, &vboCubeSolid);
        glBindVertexArray(vaoCubeSolid);
        glBindBuffer(GL_ARRAY_BUFFER, vboCubeSolid);
        glBufferData(GL_ARRAY_BUFFER, sizeof(c), c, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    // Gizmo de flecha para traslación
    {
        std::vector<float> arrowVerts;
        // Línea del eje
        arrowVerts.insert(arrowVerts.end(), {0.0f, 0.0f, 0.0f, 1.2f, 0.0f, 0.0f});
        // Punta de flecha (triángulos)
        arrowVerts.insert(arrowVerts.end(), {
            1.2f, 0.0f, 0.0f,  1.0f, 0.1f, 0.0f,  1.0f, -0.1f, 0.0f,
            1.2f, 0.0f, 0.0f,  1.0f, 0.0f, 0.1f,  1.0f, 0.0f, -0.1f
        });
        // Control deslizable (cubo pequeño)
        const float s = 0.05f;
        float handle[] = {
            1.3f-s, -s, -s,  1.3f+s, -s, -s,  1.3f+s, s, s,
            1.3f-s, -s, -s,  1.3f+s, s, s,   1.3f-s, s, s
        };
        arrowVerts.insert(arrowVerts.end(), handle, handle + 18);
        
        gizmoArrowVertexCount = (int)(arrowVerts.size() / 3);
        glGenVertexArrays(1, &vaoGizmoArrow);
        glGenBuffers(1, &vboGizmoArrow);
        glBindVertexArray(vaoGizmoArrow);
        glBindBuffer(GL_ARRAY_BUFFER, vboGizmoArrow);
        glBufferData(GL_ARRAY_BUFFER, arrowVerts.size() * sizeof(float), arrowVerts.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    // Gizmo de anillo para rotación
    {
        std::vector<float> ringVerts;
        const int segments = 32;
        const float radius = 1.0f;
        for (int i = 0; i < segments; ++i) {
            float angle1 = (float)i / segments * 2.0f * 3.1415926f;
            float angle2 = (float)(i + 1) / segments * 2.0f * 3.1415926f;
            ringVerts.insert(ringVerts.end(), {
                radius * cos(angle1), radius * sin(angle1), 0.0f,
                radius * cos(angle2), radius * sin(angle2), 0.0f
            });
        }
        
        gizmoRingVertexCount = (int)(ringVerts.size() / 3);
        glGenVertexArrays(1, &vaoGizmoRing);
        glGenBuffers(1, &vboGizmoRing);
        glBindVertexArray(vaoGizmoRing);
        glBindBuffer(GL_ARRAY_BUFFER, vboGizmoRing);
        glBufferData(GL_ARRAY_BUFFER, ringVerts.size() * sizeof(float), ringVerts.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    // Gizmo de cubo para escala
    {
        std::vector<float> scaleVerts;
        // Línea del eje
        scaleVerts.insert(scaleVerts.end(), {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f});
        // Cubo de control
        const float s = 0.08f;
        float cube[] = {
            1.2f-s, -s, -s,  1.2f+s, -s, -s,  1.2f+s, s, s,
            1.2f-s, -s, -s,  1.2f+s, s, s,   1.2f-s, s, s
        };
        scaleVerts.insert(scaleVerts.end(), cube, cube + 18);
        
        gizmoScaleVertexCount = (int)(scaleVerts.size() / 3);
        glGenVertexArrays(1, &vaoGizmoScale);
        glGenBuffers(1, &vboGizmoScale);
        glBindVertexArray(vaoGizmoScale);
        glBindBuffer(GL_ARRAY_BUFFER, vboGizmoScale);
        glBufferData(GL_ARRAY_BUFFER, scaleVerts.size() * sizeof(float), scaleVerts.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    // Sin partículas: el escenario usa solo cuadrícula y cubos

    // Cámara simple
    float aspect = (float)SCR_WIDTH / (float)SCR_HEIGHT;
    Mat4 proj = mat4_perspective(45.0f * 3.1415926f/180.0f, aspect, 0.1f, 100.0f);
    Mat4 mvp; // Se inicializa en el bucle de renderizado

    // Temporizador para actualizar el título de ventana (debug)
    float titleAccum = 0.0f;

    // Tiempo y física
    float lastTime = (float)glfwGetTime();
    // Física desactivada

    // render loop
    // -----------
    bool prevG = false;
    while (!glfwWindowShouldClose(window)) {
        // input
        // -----
        // Deshabilitar cierre con ESC
        // if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) { /* ignorado */ }

        // delta time
        float currentTime = (float)glfwGetTime();
        float dt = currentTime - lastTime;
        if (dt > 0.033f) dt = 0.033f; // clamp para estabilidad
        lastTime = currentTime;

        // Poll events and start a new ImGui frame
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Avoid moving camera when UI wants to capture input
        {
            ImGuiIO& io_frame = ImGui::GetIO();
            if (!(io_frame.WantCaptureMouse || io_frame.WantCaptureKeyboard)) {
                update_camera_direction();
                update_camera_position(dt, window);
            }
        }

        // Preview DnD: actualizar posición del cubo fantasma y finalizar/cancelar en suelta
        {
            const ImGuiViewport* vp0 = ImGui::GetMainViewport();
            if (g_drag_preview_active && g_drag_preview_index >= 0 && g_drag_preview_index < (int)g_scene_cubes.size()) {
                ImGuiIO& io_fb = ImGui::GetIO();
                ImVec2 mpos = ImGui::GetMousePos();
                int fbw = 0, fbh = 0; glfwGetFramebufferSize(window, &fbw, &fbh);
                if (fbw <= 0) fbw = SCR_WIDTH; if (fbh <= 0) fbh = SCR_HEIGHT;
                float mx_fb = (mpos.x - vp0->Pos.x) * io_fb.DisplayFramebufferScale.x;
                float my_fb = (mpos.y - vp0->Pos.y) * io_fb.DisplayFramebufferScale.y;
                float x_ndc = (2.0f * mx_fb / (float)fbw - 1.0f);
                float y_ndc = (1.0f - 2.0f * my_fb / (float)fbh);

                const float* cpos = get_camera_position();
                const float* cfront = get_camera_front();
                const float* cup = get_camera_up();
                float right[3]; vec3_cross(cfront, cup, right); vec3_normalize(right);
                float upv[3] = { cup[0], cup[1], cup[2] }; vec3_normalize(upv);
                float tanHalfFovy = std::tan(45.0f * 3.1415926f/180.0f / 2.0f);
                float aspect_rt = (float)fbw / (float)fbh;
                float dir[3] = {
                    cfront[0] + x_ndc * aspect_rt * tanHalfFovy * right[0] + y_ndc * tanHalfFovy * upv[0],
                    cfront[1] + x_ndc * aspect_rt * tanHalfFovy * right[1] + y_ndc * tanHalfFovy * upv[1],
                    cfront[2] + x_ndc * aspect_rt * tanHalfFovy * right[2] + y_ndc * tanHalfFovy * upv[2]
                };
                vec3_normalize(dir);

                // Calcular posición destino del preview
                CubeInst& preview = g_scene_cubes[g_drag_preview_index];
                int hitIndex = -1; float bestT = FLT_MAX;
                for (int i = 0; i < (int)g_scene_cubes.size(); ++i) {
                    if (i == g_drag_preview_index) continue;
                    const CubeInst& c = g_scene_cubes[i];
                    float bmin[3] = { c.pos[0] - 0.5f, c.pos[1] - 0.5f, c.pos[2] - 0.5f };
                    float bmax[3] = { c.pos[0] + 0.5f, c.pos[1] + 0.5f, c.pos[2] + 0.5f };
                    float tHit = 0.0f;
                    if (RayAABB(cpos, dir, bmin, bmax, tHit)) { if (tHit < bestT) { bestT = tHit; hitIndex = i; } }
                }
                if (hitIndex >= 0) {
                    // Calcular punto de impacto en la superficie del cubo
                    const CubeInst& base = g_scene_cubes[hitIndex];
                    float hitPoint[3] = { cpos[0] + dir[0] * bestT, cpos[1] + dir[1] * bestT, cpos[2] + dir[2] * bestT };
                    
                    // Determinar en qué cara del cubo impactó y posicionar en consecuencia
                    float relX = hitPoint[0] - base.pos[0];
                    float relY = hitPoint[1] - base.pos[1];
                    float relZ = hitPoint[2] - base.pos[2];
                    
                    // Encontrar la cara más cercana al punto de impacto
                    float absX = std::abs(relX), absY = std::abs(relY), absZ = std::abs(relZ);
                    if (absY >= absX && absY >= absZ) {
                        // Cara superior o inferior
                        preview.pos[0] = hitPoint[0];
                        preview.pos[2] = hitPoint[2];
                        preview.pos[1] = base.pos[1] + (relY > 0 ? 1.0f : -1.0f);
                    } else if (absX >= absZ) {
                        // Cara lateral X
                        preview.pos[0] = base.pos[0] + (relX > 0 ? 1.0f : -1.0f);
                        preview.pos[1] = hitPoint[1];
                        preview.pos[2] = hitPoint[2];
                    } else {
                        // Cara lateral Z
                        preview.pos[0] = hitPoint[0];
                        preview.pos[1] = hitPoint[1];
                        preview.pos[2] = base.pos[2] + (relZ > 0 ? 1.0f : -1.0f);
                    }
                } else {
                    // Intersección con plano del suelo y=0
                    if (std::fabs(dir[1]) > 1e-6f) {
                        float t = (0.0f - cpos[1]) / dir[1];
                        if (t < 0.0f) t = 0.0f;
                        float hitx = cpos[0] + dir[0] * t;
                        float hitz = cpos[2] + dir[2] * t;
                        preview.pos[0] = hitx;
                        preview.pos[1] = 0.5f; // apoyado en el suelo
                        preview.pos[2] = hitz;
                    } else {
                        // Fallback: frente a la cámara
                        float frontN[3] = { cfront[0], cfront[1], cfront[2] }; vec3_normalize(frontN);
                        const float k = 5.0f;
                        preview.pos[0] = cpos[0] + frontN[0] * k;
                        preview.pos[1] = cpos[1] + frontN[1] * k;
                        preview.pos[2] = cpos[2] + frontN[2] * k;
                    }
                }

                // Finalizar o cancelar en suelta
                if (io_fb.MouseReleased[0]) {
                    bool inside_vp = (mpos.x >= vp0->Pos.x && mpos.y >= vp0->Pos.y &&
                                      mpos.x < (vp0->Pos.x + vp0->Size.x) &&
                                      mpos.y < (vp0->Pos.y + vp0->Size.y));
                    if (inside_vp) {
                        std::cout << "[DnD][Preview] Finalize at (" << preview.pos[0] << ", " << preview.pos[1] << ", " << preview.pos[2] << ")\n";
                    } else {
                        std::cout << "[DnD][Preview] Cancel (outside viewport)\n";
                        g_scene_cubes.erase(g_scene_cubes.begin() + g_drag_preview_index);
                    }
                    g_drag_preview_active = false;
                    g_drag_preview_index = -1;
                }
            }
        }

        // Detección de hover para gizmos (antes del click)
        g_hovered_gizmo_axis = -1;
        {
            ImGuiIO& io_sel = ImGui::GetIO();
            if (!io_sel.WantCaptureMouse && !g_gizmo_dragging && g_selected_cube_index >= 0 && g_selected_cube_index < (int)g_scene_cubes.size() && g_show_gizmos) {
                const ImGuiViewport* vp_sel = ImGui::GetMainViewport();
                ImVec2 mpos = ImGui::GetMousePos();
                int fbw = 0, fbh = 0; glfwGetFramebufferSize(window, &fbw, &fbh);
                if (fbw <= 0) fbw = SCR_WIDTH; if (fbh <= 0) fbh = SCR_HEIGHT;
                float mx_fb = (mpos.x - vp_sel->Pos.x) * io_sel.DisplayFramebufferScale.x;
                float my_fb = (mpos.y - vp_sel->Pos.y) * io_sel.DisplayFramebufferScale.y;
                float x_ndc = (2.0f * mx_fb / (float)fbw - 1.0f);
                float y_ndc = (1.0f - 2.0f * my_fb / (float)fbh);

                const float* cpos = get_camera_position();
                const float* cfront = get_camera_front();
                const float* cup = get_camera_up();
                float right[3]; vec3_cross(cfront, cup, right); vec3_normalize(right);
                float upv[3] = { cup[0], cup[1], cup[2] }; vec3_normalize(upv);
                float tanHalfFovy = std::tan(45.0f * 3.1415926f/180.0f / 2.0f);
                float aspect_sel = (float)fbw / (float)fbh;
                float dir[3] = {
                    cfront[0] + x_ndc * aspect_sel * tanHalfFovy * right[0] + y_ndc * tanHalfFovy * upv[0],
                    cfront[1] + x_ndc * aspect_sel * tanHalfFovy * right[1] + y_ndc * tanHalfFovy * upv[1],
                    cfront[2] + x_ndc * aspect_sel * tanHalfFovy * right[2] + y_ndc * tanHalfFovy * upv[2]
                };
                vec3_normalize(dir);
                
                const CubeInst& selected = g_scene_cubes[g_selected_cube_index];
                // Generar líneas de gizmos según el modo
                float gizmoAxes[3][6];
                if (g_transform_mode == ROTATE) {
                    // Para rotación, usar anillos circulares con múltiples puntos de detección
                    float radius = 1.2f;
                    gizmoAxes[0][0] = selected.pos[0]; gizmoAxes[0][1] = selected.pos[1] + radius; gizmoAxes[0][2] = selected.pos[2];
                    gizmoAxes[0][3] = selected.pos[0]; gizmoAxes[0][4] = selected.pos[1] - radius; gizmoAxes[0][5] = selected.pos[2];
                    
                    gizmoAxes[1][0] = selected.pos[0] + radius; gizmoAxes[1][1] = selected.pos[1]; gizmoAxes[1][2] = selected.pos[2];
                    gizmoAxes[1][3] = selected.pos[0] - radius; gizmoAxes[1][4] = selected.pos[1]; gizmoAxes[1][5] = selected.pos[2];
                    
                    gizmoAxes[2][0] = selected.pos[0]; gizmoAxes[2][1] = selected.pos[1]; gizmoAxes[2][2] = selected.pos[2] + radius;
                    gizmoAxes[2][3] = selected.pos[0]; gizmoAxes[2][4] = selected.pos[1]; gizmoAxes[2][5] = selected.pos[2] - radius;
                } else {
                    // Para traslación y escala, usar líneas
                    gizmoAxes[0][0] = selected.pos[0]; gizmoAxes[0][1] = selected.pos[1]; gizmoAxes[0][2] = selected.pos[2];
                    gizmoAxes[0][3] = selected.pos[0] - 1.5f; gizmoAxes[0][4] = selected.pos[1]; gizmoAxes[0][5] = selected.pos[2];
                    
                    gizmoAxes[1][0] = selected.pos[0]; gizmoAxes[1][1] = selected.pos[1]; gizmoAxes[1][2] = selected.pos[2];
                    gizmoAxes[1][3] = selected.pos[0]; gizmoAxes[1][4] = selected.pos[1] + 1.5f; gizmoAxes[1][5] = selected.pos[2];
                    
                    gizmoAxes[2][0] = selected.pos[0]; gizmoAxes[2][1] = selected.pos[1]; gizmoAxes[2][2] = selected.pos[2];
                    gizmoAxes[2][3] = selected.pos[0]; gizmoAxes[2][4] = selected.pos[1]; gizmoAxes[2][5] = selected.pos[2] - 1.5f;
                }
                
                // Verificar hover en cada eje con tolerancia mejorada
                float bestDistance = FLT_MAX;
                for (int axis = 0; axis < 3; ++axis) {
                    float t, distance;
                    if (RayLineIntersect(cpos, dir, &gizmoAxes[axis][0], &gizmoAxes[axis][3], t, distance)) {
                        float tolerance = (g_transform_mode == ROTATE) ? 0.3f : 0.15f; // Mayor tolerancia para rotación
                        if (distance < tolerance && distance < bestDistance) {
                            bestDistance = distance;
                            g_hovered_gizmo_axis = axis;
                        }
                    }
                }
            }
        }

        // Sistema de selección de objetos y gizmos con click izquierdo
        {
            ImGuiIO& io_sel = ImGui::GetIO();
            if (io_sel.MouseClicked[0] && !io_sel.WantCaptureMouse && !g_gizmo_dragging) {
                // Raycast para selección de objetos y gizmos
                const ImGuiViewport* vp_sel = ImGui::GetMainViewport();
                ImVec2 mpos = ImGui::GetMousePos();
                int fbw = 0, fbh = 0; glfwGetFramebufferSize(window, &fbw, &fbh);
                if (fbw <= 0) fbw = SCR_WIDTH; if (fbh <= 0) fbh = SCR_HEIGHT;
                float mx_fb = (mpos.x - vp_sel->Pos.x) * io_sel.DisplayFramebufferScale.x;
                float my_fb = (mpos.y - vp_sel->Pos.y) * io_sel.DisplayFramebufferScale.y;
                float x_ndc = (2.0f * mx_fb / (float)fbw - 1.0f);
                float y_ndc = (1.0f - 2.0f * my_fb / (float)fbh);

                const float* cpos = get_camera_position();
                const float* cfront = get_camera_front();
                const float* cup = get_camera_up();
                float right[3]; vec3_cross(cfront, cup, right); vec3_normalize(right);
                float upv[3] = { cup[0], cup[1], cup[2] }; vec3_normalize(upv);
                float tanHalfFovy = std::tan(45.0f * 3.1415926f/180.0f / 2.0f);
                float aspect_sel = (float)fbw / (float)fbh;
                float dir[3] = {
                    cfront[0] + x_ndc * aspect_sel * tanHalfFovy * right[0] + y_ndc * tanHalfFovy * upv[0],
                    cfront[1] + x_ndc * aspect_sel * tanHalfFovy * right[1] + y_ndc * tanHalfFovy * upv[1],
                    cfront[2] + x_ndc * aspect_sel * tanHalfFovy * right[2] + y_ndc * tanHalfFovy * upv[2]
                };
                vec3_normalize(dir);

                // Primero verificar si se clickeó un gizmo (si hay objeto seleccionado)
                bool gizmoClicked = false;
                if (g_show_gizmos && g_selected_cube_index >= 0 && g_selected_cube_index < (int)g_scene_cubes.size()) {
                    const CubeInst& selected = g_scene_cubes[g_selected_cube_index];
                    
                    // Verificar intersección con cada eje del gizmo
                    float gizmoAxes[3][6];
                    if (g_transform_mode == ROTATE) {
                        // Para rotación, usar anillos circulares con múltiples puntos de detección
                        float radius = 1.2f;
                        gizmoAxes[0][0] = selected.pos[0]; gizmoAxes[0][1] = selected.pos[1] + radius; gizmoAxes[0][2] = selected.pos[2];
                        gizmoAxes[0][3] = selected.pos[0]; gizmoAxes[0][4] = selected.pos[1] - radius; gizmoAxes[0][5] = selected.pos[2];
                        
                        gizmoAxes[1][0] = selected.pos[0] + radius; gizmoAxes[1][1] = selected.pos[1]; gizmoAxes[1][2] = selected.pos[2];
                        gizmoAxes[1][3] = selected.pos[0] - radius; gizmoAxes[1][4] = selected.pos[1]; gizmoAxes[1][5] = selected.pos[2];
                        
                        gizmoAxes[2][0] = selected.pos[0]; gizmoAxes[2][1] = selected.pos[1]; gizmoAxes[2][2] = selected.pos[2] + radius;
                        gizmoAxes[2][3] = selected.pos[0]; gizmoAxes[2][4] = selected.pos[1]; gizmoAxes[2][5] = selected.pos[2] - radius;
                    } else {
                        // Para traslación y escala, usar líneas
                        gizmoAxes[0][0] = selected.pos[0]; gizmoAxes[0][1] = selected.pos[1]; gizmoAxes[0][2] = selected.pos[2];
                        gizmoAxes[0][3] = selected.pos[0] - 1.5f; gizmoAxes[0][4] = selected.pos[1]; gizmoAxes[0][5] = selected.pos[2];
                        
                        gizmoAxes[1][0] = selected.pos[0]; gizmoAxes[1][1] = selected.pos[1]; gizmoAxes[1][2] = selected.pos[2];
                        gizmoAxes[1][3] = selected.pos[0]; gizmoAxes[1][4] = selected.pos[1] + 1.5f; gizmoAxes[1][5] = selected.pos[2];
                        
                        gizmoAxes[2][0] = selected.pos[0]; gizmoAxes[2][1] = selected.pos[1]; gizmoAxes[2][2] = selected.pos[2];
                        gizmoAxes[2][3] = selected.pos[0]; gizmoAxes[2][4] = selected.pos[1]; gizmoAxes[2][5] = selected.pos[2] - 1.5f;
                    }
                    
                    // Verificar intersección con cada eje con mejor tolerancia
                    float bestGizmoDistance = FLT_MAX;
                    int bestGizmoAxis = -1;
                    for (int axis = 0; axis < 3; ++axis) {
                        float t, distance;
                        if (RayLineIntersect(cpos, dir, &gizmoAxes[axis][0], &gizmoAxes[axis][3], t, distance)) {
                            float tolerance = (g_transform_mode == ROTATE) ? 0.3f : 0.15f; // Mayor tolerancia para rotación
                            if (distance < tolerance && distance < bestGizmoDistance) {
                                bestGizmoDistance = distance;
                                bestGizmoAxis = axis;
                            }
                        }
                    }
                    
                    if (bestGizmoAxis >= 0) {
                        // Guardar estado antes de comenzar transformación
                        SaveUndoState();
                        
                        g_gizmo_dragging = true;
                        g_dragging_axis = bestGizmoAxis;
                        g_drag_start_pos[0] = mpos.x;
                        g_drag_start_pos[1] = mpos.y;
                        g_drag_total_delta[0] = 0.0f;
                        g_drag_total_delta[1] = 0.0f;
                        
                        // Guardar valores iniciales de todos los objetos seleccionados
                        for (int selectedIndex : g_selected_cube_indices) {
                            if (selectedIndex >= 0 && selectedIndex < (int)g_scene_cubes.size()) {
                                CubeInst& selected = g_scene_cubes[selectedIndex];
                                // Usar el primer objeto como referencia para los valores iniciales
                                if (selectedIndex == g_selected_cube_indices[0]) {
                                    for (int i = 0; i < 3; i++) {
                                        g_drag_start_object_pos[i] = selected.pos[i];
                                        g_drag_start_object_rot[i] = selected.rotation[i];
                                        g_drag_start_object_scale[i] = selected.scale[i];
                                    }
                                }
                            }
                        }
                        gizmoClicked = true;
                        std::cout << "Gizmo clickeado - Eje: " << (bestGizmoAxis == 0 ? "X" : (bestGizmoAxis == 1 ? "Y" : "Z")) << "\n";
                    }
                }
                
                // Si no se clickeó un gizmo, buscar cubos
                if (!gizmoClicked) {
                    int closestIndex = -1;
                    float closestT = FLT_MAX;
                    for (int i = 0; i < (int)g_scene_cubes.size(); ++i) {
                        const CubeInst& c = g_scene_cubes[i];
                        // Usar la escala real del objeto para el AABB
                        float halfScaleX = c.scale[0] * 0.5f;
                        float halfScaleY = c.scale[1] * 0.5f;
                        float halfScaleZ = c.scale[2] * 0.5f;
                        float bmin[3] = { c.pos[0] - halfScaleX, c.pos[1] - halfScaleY, c.pos[2] - halfScaleZ };
                        float bmax[3] = { c.pos[0] + halfScaleX, c.pos[1] + halfScaleY, c.pos[2] + halfScaleZ };
                        float tHit = 0.0f;
                        if (RayAABB(cpos, dir, bmin, bmax, tHit)) {
                            if (tHit < closestT) {
                                closestT = tHit;
                                closestIndex = i;
                            }
                        }
                    }

                    // Actualizar selección
                    if (closestIndex >= 0) {
                        ImGuiIO& io_sel = ImGui::GetIO();
                        
                        if (io_sel.KeyCtrl) {
                            // Selección múltiple con Ctrl
                            bool alreadySelected = g_scene_cubes[closestIndex].selected;
                            
                            if (alreadySelected) {
                                // Deseleccionar si ya estaba seleccionado
                                g_scene_cubes[closestIndex].selected = false;
                                auto it = std::find(g_selected_cube_indices.begin(), g_selected_cube_indices.end(), closestIndex);
                                if (it != g_selected_cube_indices.end()) {
                                    g_selected_cube_indices.erase(it);
                                }
                                std::cout << "Cubo deseleccionado: " << closestIndex << std::endl;
                            } else {
                                // Agregar a la selección
                                g_scene_cubes[closestIndex].selected = true;
                                g_selected_cube_indices.push_back(closestIndex);
                                std::cout << "Cubo agregado a selección: " << closestIndex << std::endl;
                            }
                            
                            // Actualizar índice principal (último seleccionado)
                            if (!g_selected_cube_indices.empty()) {
                                g_selected_cube_index = g_selected_cube_indices.back();
                            } else {
                                g_selected_cube_index = -1;
                            }
                        } else {
                            // Selección simple (comportamiento original)
                            for (auto& cube : g_scene_cubes) {
                                cube.selected = false;
                            }
                            g_selected_cube_indices.clear();
                            
                            g_scene_cubes[closestIndex].selected = true;
                            g_selected_cube_index = closestIndex;
                            g_selected_cube_indices.push_back(closestIndex);
                            std::cout << "Cubo seleccionado: " << closestIndex << " en (" << g_scene_cubes[closestIndex].pos[0] << ", " << g_scene_cubes[closestIndex].pos[1] << ", " << g_scene_cubes[closestIndex].pos[2] << ")\n";
                        }
                    } else {
                        // Deseleccionar todo si no se clickeó ningún cubo (solo si no se mantiene Ctrl)
                        ImGuiIO& io_sel = ImGui::GetIO();
                        if (!io_sel.KeyCtrl) {
                            for (auto& cube : g_scene_cubes) {
                                cube.selected = false;
                            }
                            g_selected_cube_index = -1;
                            g_selected_cube_indices.clear();
                        }
                    }
                }
            }
            
            // Manejar arrastre de gizmos
            if (g_gizmo_dragging) {
                if (io_sel.MouseDown[0]) {
                    // Continuar arrastrando
                    ImVec2 currentPos = ImGui::GetMousePos();
                    
                    // Calcular delta total desde el inicio del arrastre
                    g_drag_total_delta[0] = currentPos.x - g_drag_start_pos[0];
                    g_drag_total_delta[1] = currentPos.y - g_drag_start_pos[1];
                    
                    // Aplicar transformación a todos los objetos seleccionados
                    for (int selectedIndex : g_selected_cube_indices) {
                        if (selectedIndex >= 0 && selectedIndex < (int)g_scene_cubes.size()) {
                            CubeInst& selected = g_scene_cubes[selectedIndex];
                            
                            // Calcular offset relativo para objetos múltiples
                            float offsetX = 0.0f, offsetY = 0.0f, offsetZ = 0.0f;
                            if (selectedIndex != g_selected_cube_indices[0]) {
                                offsetX = selected.pos[0] - g_drag_start_object_pos[0];
                                offsetY = selected.pos[1] - g_drag_start_object_pos[1];
                                offsetZ = selected.pos[2] - g_drag_start_object_pos[2];
                            }
                            
                            // Aplicar transformación según el eje y modo
                            if (g_transform_mode == TRANSLATE) {
                                float sensitivity = 0.015f; // Reducir sensibilidad para movimiento más suave
                                if (g_dragging_axis == 0) { // X
                                    selected.pos[0] = g_drag_start_object_pos[0] + offsetX + g_drag_total_delta[0] * sensitivity;
                                } else if (g_dragging_axis == 1) { // Y
                                    selected.pos[1] = g_drag_start_object_pos[1] + offsetY - g_drag_total_delta[1] * sensitivity; // Invertir Y
                                } else if (g_dragging_axis == 2) { // Z
                                    selected.pos[2] = g_drag_start_object_pos[2] + offsetZ + g_drag_total_delta[1] * sensitivity; // Corregir dirección Z
                                }
                            } else if (g_transform_mode == ROTATE) {
                                float sensitivity = 0.5f; // Restaurar sensibilidad original
                                if (g_dragging_axis == 0) { // Rotación X
                                    selected.rotation[0] = fmod(g_drag_start_object_rot[0] - g_drag_total_delta[1] * sensitivity, 360.0f);
                                    if (selected.rotation[0] < 0) selected.rotation[0] += 360.0f;
                                } else if (g_dragging_axis == 1) { // Rotación Y
                                    selected.rotation[1] = fmod(g_drag_start_object_rot[1] + g_drag_total_delta[0] * sensitivity, 360.0f);
                                    if (selected.rotation[1] < 0) selected.rotation[1] += 360.0f;
                                } else if (g_dragging_axis == 2) { // Rotación Z
                                    selected.rotation[2] = fmod(g_drag_start_object_rot[2] + g_drag_total_delta[0] * sensitivity, 360.0f);
                                    if (selected.rotation[2] < 0) selected.rotation[2] += 360.0f;
                                }
                            } else if (g_transform_mode == SCALE) {
                                float sensitivity = 0.01f; // Restaurar sensibilidad original
                                float scaleFactor = 1.0f + g_drag_total_delta[0] * sensitivity;
                                if (scaleFactor < 0.1f) scaleFactor = 0.1f; // Mínimo
                                if (scaleFactor > 5.0f) scaleFactor = 5.0f;   // Máximo
                                
                                if (g_dragging_axis == 0) { // Escala X
                                    selected.scale[0] = g_drag_start_object_scale[0] * scaleFactor;
                                } else if (g_dragging_axis == 1) { // Escala Y
                                    selected.scale[1] = g_drag_start_object_scale[1] * scaleFactor;
                                } else if (g_dragging_axis == 2) { // Escala Z
                                    selected.scale[2] = g_drag_start_object_scale[2] * scaleFactor;
                                }
                            }
                        }
                    }
                } else {
                    // Soltar arrastre
                    g_gizmo_dragging = false;
                    g_dragging_axis = -1;
                    std::cout << "Arrastre de gizmo terminado\n";
                }
            }
        }

        // Hotkeys para proyecto y objetos
        {
            ImGuiIO& io_keys = ImGui::GetIO();
            
            // Ctrl+N - Nuevo proyecto
            if (io_keys.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N)) {
                g_scene_cubes.clear();
                g_selected_cube_index = -1;
                g_current_project_path = "";
                g_project_modified = false;
                std::cout << "Nuevo proyecto creado (Ctrl+N)" << std::endl;
            }
            
            // Ctrl+O - Abrir proyecto
            if (io_keys.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O)) {
                std::string selectedPath = OpenNativeFileDialog(false);
                if (!selectedPath.empty()) {
                    LoadProject(selectedPath);
                }
            }
            
            // Ctrl+S - Guardar proyecto
            if (io_keys.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
                if (io_keys.KeyShift) {
                    // Ctrl+Shift+S - Guardar como
                    SaveProjectAs();
                } else {
                    // Ctrl+S - Guardar
                    SaveCurrentProject();
                }
            }
            
            // Ctrl+C - Copiar objetos seleccionados
            if (io_keys.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C)) {
                if (!g_selected_cube_indices.empty()) {
                    g_copied_cubes.clear();
                    
                    // Copiar todos los objetos seleccionados
                    for (int index : g_selected_cube_indices) {
                        if (index >= 0 && index < (int)g_scene_cubes.size()) {
                            CubeInst copiedCube = g_scene_cubes[index];
                            copiedCube.selected = false; // El objeto copiado no debe estar seleccionado
                            g_copied_cubes.push_back(copiedCube);
                        }
                    }
                    
                    g_has_copied_cubes = true;
                    std::cout << "Objetos copiados: " << g_copied_cubes.size() << " cubos" << std::endl;
                }
            }
            
            // Ctrl+V - Pegar objetos copiados
            if (io_keys.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V)) {
                if (g_has_copied_cubes && !g_copied_cubes.empty()) {
                    // Guardar estado antes de pegar
                    SaveUndoState();
                    
                    // Deseleccionar todo primero
                    for (auto& cube : g_scene_cubes) {
                        cube.selected = false;
                    }
                    g_selected_cube_indices.clear();
                    
                    // Pegar todos los objetos copiados
                    for (size_t i = 0; i < g_copied_cubes.size(); ++i) {
                        CubeInst pastedCube = g_copied_cubes[i];
                        
                        // Posicionar cada objeto pegado desplazado
                        pastedCube.pos[0] += 1.0f + (i * 0.5f); // Desplazar en X con separación
                        pastedCube.selected = true; // Seleccionar los objetos pegados
                        
                        // Agregar el cubo pegado a la escena
                        g_scene_cubes.push_back(pastedCube);
                        g_selected_cube_indices.push_back((int)g_scene_cubes.size() - 1);
                    }
                    
                    // Actualizar índice principal al último pegado
                    g_selected_cube_index = g_selected_cube_indices.back();
                    g_project_modified = true;
                    
                    std::cout << "Objetos pegados: " << g_copied_cubes.size() << " cubos" << std::endl;
                }
            }
            
            // Ctrl+Z - Deshacer última acción
            if (io_keys.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) {
                if (!g_undo_history.empty()) {
                    // Restaurar último estado guardado
                    UndoAction lastAction = g_undo_history.back();
                    g_undo_history.pop_back();
                    
                    g_scene_cubes = lastAction.scene_state;
                    g_selected_cube_index = lastAction.selected_index;
                    g_selected_cube_indices = lastAction.selected_indices;
                    
                    // Actualizar estado de selección visual
                    for (size_t i = 0; i < g_scene_cubes.size(); ++i) {
                        g_scene_cubes[i].selected = false;
                    }
                    for (int index : g_selected_cube_indices) {
                        if (index >= 0 && index < (int)g_scene_cubes.size()) {
                            g_scene_cubes[index].selected = true;
                        }
                    }
                    
                    g_project_modified = true;
                    std::cout << "Acción deshecha - Estado restaurado" << std::endl;
                }
            }
            
            // Hotkey 'C' desactivado - ahora se usa para Ctrl+C (copiar)
        }

        // render
        // ------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(prog3D);
        // Actualizar proyección con el tamaño real del framebuffer (aspect correcto)
        int fbw=0, fbh=0; glfwGetFramebufferSize(window, &fbw, &fbh);
        if (fbw <= 0) fbw = SCR_WIDTH; if (fbh <= 0) fbh = SCR_HEIGHT;
        glViewport(0, 0, fbw, fbh); // asegurar viewport correcto cada frame
        aspect = (float)fbw/(float)fbh;
        proj = mat4_perspective(45.0f * 3.1415926f/180.0f, aspect, 0.1f, 100.0f);
        // Cámara FPS: sin órbita, mirando desde camPos
        Mat4 view = create_view_matrix(get_camera_position(), get_camera_front(), get_camera_up());
        mvp = mat4_mul(proj, view);
        glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvp.m);

        // Actualizar título de la ventana con estado del proyecto
        titleAccum += dt;
        if (titleAccum > 0.25f) {
            titleAccum = 0.0f;
            std::string windowTitle = cfg.window_title;
            
            // Agregar nombre del proyecto y estado de modificación
            if (!g_current_project_path.empty()) {
                std::filesystem::path projectPath(g_current_project_path);
                std::string projectName = projectPath.stem().string();
                windowTitle += " - " + projectName;
                if (g_project_modified) {
                    windowTitle += "*";
                }
            } else {
                windowTitle += " - Sin Proyecto";
                if (g_project_modified) {
                    windowTitle += "*";
                }
            }
            
            glfwSetWindowTitle(window, windowTitle.c_str());
        }

        // Dibujar cuadrícula
        glUniform4f(locColor, 0.35f, 0.35f, 0.35f, 1.0f);
        glBindVertexArray(vaoGrid);
        glDrawArrays(GL_LINES, 0, gridVertexCount);
        glBindVertexArray(0);

        // Dibujar cubos de la escena (sólidos 1x1x1)
        glPolygonMode(GL_FRONT_AND_BACK, g_wireframe ? GL_LINE : GL_FILL);
        for (size_t i = 0; i < g_scene_cubes.size(); ++i) {
            const CubeInst& c = g_scene_cubes[i];
            
            // Crear matriz de transformación completa (Escala * Rotación * Traslación)
            Mat4 model = mat4_identity();
            
            // Aplicar escala
            Mat4 scaleMatrix = mat4_identity();
            scaleMatrix.m[0] = c.scale[0];   // X
            scaleMatrix.m[5] = c.scale[1];   // Y
            scaleMatrix.m[10] = c.scale[2];  // Z
            
            // Aplicar rotación (en orden Z, Y, X)
            float radX = c.rotation[0] * 3.1415926f / 180.0f;
            float radY = c.rotation[1] * 3.1415926f / 180.0f;
            float radZ = c.rotation[2] * 3.1415926f / 180.0f;
            
            Mat4 rotX = mat4_identity();
            rotX.m[5] = cos(radX); rotX.m[6] = -sin(radX);
            rotX.m[9] = sin(radX); rotX.m[10] = cos(radX);
            
            Mat4 rotY = mat4_identity();
            rotY.m[0] = cos(radY); rotY.m[2] = sin(radY);
            rotY.m[8] = -sin(radY); rotY.m[10] = cos(radY);
            
            Mat4 rotZ = mat4_identity();
            rotZ.m[0] = cos(radZ); rotZ.m[1] = -sin(radZ);
            rotZ.m[4] = sin(radZ); rotZ.m[5] = cos(radZ);
            
            // Combinar transformaciones: Escala -> Rotación -> Traslación
            Mat4 rotation = mat4_mul(mat4_mul(rotZ, rotY), rotX);
            model = mat4_mul(rotation, scaleMatrix);
            
            // Aplicar traslación
            model.m[12] = c.pos[0];
            model.m[13] = c.pos[1];
            model.m[14] = c.pos[2];
            
            Mat4 mvpCube = mat4_mul(mvp, model);
            glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvpCube.m);
            
            // Aplicar material del objeto
            Material objectMaterial = GetMaterialFromPath(c.materialPath);
            
            // Color diferente para objetos seleccionados (mezclar con material)
            if (c.selected) {
                // Tinte naranja para seleccionados
                float selectedColor[3] = {
                    objectMaterial.color[0] * 1.2f,
                    objectMaterial.color[1] * 0.8f,
                    objectMaterial.color[2] * 0.4f
                };
                glUniform4f(locColor, selectedColor[0], selectedColor[1], selectedColor[2], 1.0f);
            } else {
                // Usar color del material
                glUniform4f(locColor, objectMaterial.color[0], objectMaterial.color[1], objectMaterial.color[2], 1.0f);
            }
            
            // Aplicar propiedades del material al shader (si están disponibles)
            ApplyMaterialToShader(objectMaterial, prog3D);
            
            glBindVertexArray(vaoCubeSolid);
            glDrawArrays(GL_TRIANGLES, 0, cubeSolidVertexCount);
            glBindVertexArray(0);
        }
        
        // Dibujar gizmos de transformación para el objeto seleccionado (SIEMPRE ENCIMA)
        if (g_show_gizmos && g_selected_cube_index >= 0 && g_selected_cube_index < (int)g_scene_cubes.size()) {
            const CubeInst& selected = g_scene_cubes[g_selected_cube_index];
            
            // Desactivar depth test para que los gizmos siempre se vean encima
            glDisable(GL_DEPTH_TEST);
            
            // Crear matriz de transformación para los gizmos
            Mat4 gizmoModel = mat4_identity();
            gizmoModel.m[12] = selected.pos[0];
            gizmoModel.m[13] = selected.pos[1];
            gizmoModel.m[14] = selected.pos[2];
            
            // Dibujar gizmos según el modo de transformación
            if (g_transform_mode == TRANSLATE) {
                // Gizmos de flecha para traslación
                
                // Eje X (rojo)
                Mat4 mvpGizmoX = mat4_mul(mvp, gizmoModel);
                glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvpGizmoX.m);
                float alphaX = (g_gizmo_dragging && g_dragging_axis == 0) ? 1.0f : (g_hovered_gizmo_axis == 0 ? 0.9f : 0.7f);
                glUniform4f(locColor, 1.0f, 0.0f, 0.0f, alphaX);
                glBindVertexArray(vaoGizmoArrow);
                glDrawArrays(GL_LINES, 0, 2); // Línea
                glDrawArrays(GL_TRIANGLES, 2, 6); // Punta de flecha
                glDrawArrays(GL_TRIANGLES, 8, 6); // Control deslizable
                
                // Eje Y (verde) - rotar 90° en Z
                Mat4 rotY = mat4_identity();
                rotY.m[0] = 0; rotY.m[1] = -1; rotY.m[4] = 1; rotY.m[5] = 0;
                Mat4 gizmoY = mat4_mul(gizmoModel, rotY);
                Mat4 mvpGizmoY = mat4_mul(mvp, gizmoY);
                glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvpGizmoY.m);
                float alphaY = (g_gizmo_dragging && g_dragging_axis == 1) ? 1.0f : (g_hovered_gizmo_axis == 1 ? 0.9f : 0.7f);
                glUniform4f(locColor, 0.0f, 1.0f, 0.0f, alphaY);
                glDrawArrays(GL_LINES, 0, 2);
                glDrawArrays(GL_TRIANGLES, 2, 6);
                glDrawArrays(GL_TRIANGLES, 8, 6);
                
                // Eje Z (azul) - rotar -90° en Y
                Mat4 rotZ = mat4_identity();
                rotZ.m[0] = 0; rotZ.m[2] = 1; rotZ.m[8] = -1; rotZ.m[10] = 0;
                Mat4 gizmoZ = mat4_mul(gizmoModel, rotZ);
                Mat4 mvpGizmoZ = mat4_mul(mvp, gizmoZ);
                glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvpGizmoZ.m);
                float alphaZ = (g_gizmo_dragging && g_dragging_axis == 2) ? 1.0f : (g_hovered_gizmo_axis == 2 ? 0.9f : 0.7f);
                glUniform4f(locColor, 0.0f, 0.0f, 1.0f, alphaZ);
                glDrawArrays(GL_LINES, 0, 2);
                glDrawArrays(GL_TRIANGLES, 2, 6);
                glDrawArrays(GL_TRIANGLES, 8, 6);
                
            } else if (g_transform_mode == ROTATE) {
                // Gizmos de anillo para rotación
                glLineWidth(3.0f);
                
                // Anillo X (rojo) - en plano YZ
                Mat4 rotRingX = mat4_identity();
                rotRingX.m[5] = 0; rotRingX.m[6] = -1; rotRingX.m[9] = 1; rotRingX.m[10] = 0;
                Mat4 gizmoRingX = mat4_mul(gizmoModel, rotRingX);
                Mat4 mvpRingX = mat4_mul(mvp, gizmoRingX);
                glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvpRingX.m);
                float alphaRX = (g_gizmo_dragging && g_dragging_axis == 0) ? 1.0f : (g_hovered_gizmo_axis == 0 ? 0.9f : 0.7f);
                glUniform4f(locColor, 1.0f, 0.0f, 0.0f, alphaRX);
                glBindVertexArray(vaoGizmoRing);
                glDrawArrays(GL_LINES, 0, gizmoRingVertexCount);
                
                
                // Anillo Y (verde) - en plano XZ
                Mat4 mvpRingY = mat4_mul(mvp, gizmoModel);
                glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvpRingY.m);
                float alphaRY = (g_gizmo_dragging && g_dragging_axis == 1) ? 1.0f : (g_hovered_gizmo_axis == 1 ? 0.9f : 0.7f);
                glUniform4f(locColor, 0.0f, 1.0f, 0.0f, alphaRY);
                glDrawArrays(GL_LINES, 0, gizmoRingVertexCount);
                
                // Anillo Z (azul) - en plano XY
                Mat4 rotRingZ = mat4_identity();
                rotRingZ.m[0] = 0; rotRingZ.m[1] = -1; rotRingZ.m[4] = 1; rotRingZ.m[5] = 0;
                Mat4 gizmoRingZ = mat4_mul(gizmoModel, rotRingZ);
                Mat4 mvpRingZ = mat4_mul(mvp, gizmoRingZ);
                glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvpRingZ.m);
                float alphaRZ = (g_gizmo_dragging && g_dragging_axis == 2) ? 1.0f : (g_hovered_gizmo_axis == 2 ? 0.9f : 0.7f);
                glUniform4f(locColor, 0.0f, 0.0f, 1.0f, alphaRZ);
                glDrawArrays(GL_LINES, 0, gizmoRingVertexCount);
                
            } else if (g_transform_mode == SCALE) {
                // Gizmos de cubo para escala
                
                // Eje X (rojo)
                Mat4 mvpScaleX = mat4_mul(mvp, gizmoModel);
                glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvpScaleX.m);
                float alphaSX = (g_gizmo_dragging && g_dragging_axis == 0) ? 1.0f : (g_hovered_gizmo_axis == 0 ? 0.9f : 0.7f);
                glUniform4f(locColor, 1.0f, 0.0f, 0.0f, alphaSX);
                glBindVertexArray(vaoGizmoScale);
                glDrawArrays(GL_LINES, 0, 2); // Línea
                glDrawArrays(GL_TRIANGLES, 2, 6); // Cubo de control
                
                // Eje Y (verde) - rotar 90° en Z
                Mat4 rotSY = mat4_identity();
                rotSY.m[0] = 0; rotSY.m[1] = -1; rotSY.m[4] = 1; rotSY.m[5] = 0;
                Mat4 gizmoSY = mat4_mul(gizmoModel, rotSY);
                Mat4 mvpScaleY = mat4_mul(mvp, gizmoSY);
                glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvpScaleY.m);
                float alphaSY = (g_gizmo_dragging && g_dragging_axis == 1) ? 1.0f : (g_hovered_gizmo_axis == 1 ? 0.9f : 0.7f);
                glUniform4f(locColor, 0.0f, 1.0f, 0.0f, alphaSY);
                glDrawArrays(GL_LINES, 0, 2);
                glDrawArrays(GL_TRIANGLES, 2, 6);
                
                // Eje Z (azul) - rotar -90° en Y
                Mat4 rotSZ = mat4_identity();
                rotSZ.m[0] = 0; rotSZ.m[2] = 1; rotSZ.m[8] = -1; rotSZ.m[10] = 0;
                Mat4 gizmoSZ = mat4_mul(gizmoModel, rotSZ);
                Mat4 mvpScaleZ = mat4_mul(mvp, gizmoSZ);
                glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvpScaleZ.m);
                float alphaSZ = (g_gizmo_dragging && g_dragging_axis == 2) ? 1.0f : (g_hovered_gizmo_axis == 2 ? 0.9f : 0.7f);
                glUniform4f(locColor, 0.0f, 0.0f, 1.0f, alphaSZ);
                glDrawArrays(GL_LINES, 0, 2);
                glDrawArrays(GL_TRIANGLES, 2, 6);
            }
            
            // Restaurar depth test
            glEnable(GL_DEPTH_TEST);
            glLineWidth(1.0f);
            glBindVertexArray(0);
        }
        // Asegurar volver a sólido por defecto para cualquier otro draw que use triángulos
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        // Restaurar PV para futuros usos
        glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvp.m);

        // =====================
        // ImGui UI
        // =====================
        // Main menu (embedded inside DockSpaceHost window)
        static bool show_presets = false;
        bool was_open_presets = show_presets;
        // Host window + DockSpace covering the viewport
        ImGuiWindowFlags host_flags = ImGuiWindowFlags_NoDocking;
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);
        ImGui::SetNextWindowViewport(vp->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        host_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        host_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpaceHost", nullptr, host_flags);
        ImGui::PopStyleVar();
        ImGui::PopStyleVar(2);

        // Menu bar inside the DockSpaceHost window
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Nuevo Proyecto", "Ctrl+N")) {
                    // Limpiar escena actual
                    g_scene_cubes.clear();
                    g_selected_cube_index = -1;
                    g_current_project_path = "";
                    g_project_modified = false;
                    std::cout << "Nuevo proyecto creado" << std::endl;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Abrir Proyecto...", "Ctrl+O")) {
                    // Abrir diálogo nativo de Windows
                    std::string selectedPath = OpenNativeFileDialog(false);
                    if (!selectedPath.empty()) {
                        LoadProject(selectedPath);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Guardar Proyecto", "Ctrl+S")) {
                    SaveCurrentProject();
                }
                if (ImGui::MenuItem("Guardar Como...", "Ctrl+Shift+S")) {
                    SaveProjectAs();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                // Empty for now
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                // Alterna entre Wireframe y Solid
                bool wfSelected = g_wireframe;
                if (ImGui::MenuItem("Wireframe", nullptr, wfSelected)) g_wireframe = true;
                bool solidSelected = !g_wireframe;
                if (ImGui::MenuItem("Solid", nullptr, solidSelected)) g_wireframe = false;
                ImGui::Separator();
                ImGui::MenuItem("Mostrar Gizmos", nullptr, &g_show_gizmos);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Windows")) {
                ImGui::MenuItem("Presets", nullptr, &show_presets);
                ImGui::MenuItem("Propiedades", nullptr, &g_show_properties);
                ImGui::MenuItem("TreeObjects", nullptr, &g_show_tree_objects);
                ImGui::MenuItem("FileExplorer", nullptr, &g_show_file_explorer);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        bool presets_just_opened = (show_presets && !was_open_presets);

        ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_PassthruCentralNode;
        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0, 0), dock_flags);
        // Aplicar cada frame: no ocultar las pestañas en ningún nodo del dockspace
        if (ImGuiDockNode* root_node = ImGui::DockBuilderGetNode(dockspace_id)) {
            EnsureDockTabsVisible(root_node);
        }

        // Fallback: aceptar drop directamente en la ventana host por si el overlay no estuviera activo
        if (ImGui::BeginDragDropTarget()) {
            bool hovered_host = ImGui::IsWindowHovered(ImGuiHoveredFlags_None);
            const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ADD_CUBE", ImGuiDragDropFlags_AcceptBeforeDelivery);
            if (payload) {
                std::cout << "[DnD][Host] Accept OK. (preview active=" << g_drag_preview_active << ") hovered=" << hovered_host << "\n";
                // En modo preview, no spawneamos aquí: el preview controla el drop.
            }
            ImGui::EndDragDropTarget();
        }

        // Aceptar drag & drop sobre el escenario (overlay más pequeño y menos intrusivo)
        bool dragActive = ImGui::IsDragDropActive();
        if (dragActive || g_drag_active_last_frame) {
            // Crear overlay más pequeño que no cubra toda la pantalla
            ImGuiWindowFlags dropFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus;
            
            // Posicionar overlay solo en área central, no toda la pantalla
            ImVec2 center = ImVec2(vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + vp->Size.y * 0.5f);
            ImVec2 overlaySize = ImVec2(200.0f, 50.0f);
            ImGui::SetNextWindowPos(ImVec2(center.x - overlaySize.x * 0.5f, center.y - overlaySize.y * 0.5f));
            ImGui::SetNextWindowSize(overlaySize);
            
            ImGui::Begin("SceneDropTarget", nullptr, dropFlags);
            ImGui::TextUnformatted("Suelta para crear cubo");
            if (ImGui::BeginDragDropTarget()) {
                const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ADD_CUBE", ImGuiDragDropFlags_AcceptBeforeDelivery);
                if (payload) {
                    // En modo preview, no spawneamos aquí: el preview controla el drop.
                }
                ImGui::EndDragDropTarget();
            }
            ImGui::End();
        }
        // Guardar estado para habilitar overlay también en el frame de entrega
        g_drag_active_last_frame = dragActive;

        // If Presets was just opened, create a left dock and dock it there
        if (presets_just_opened) {
            ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, vp->Size);
            ImGuiID dock_main_id = dockspace_id;
            ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f, nullptr, &dock_main_id);
            ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.3f, nullptr, &dock_main_id);
            g_dock_left_id = dock_left_id;
            ImGui::DockBuilderDockWindow("Presets", dock_left_id);
            if (g_show_properties) {
                ImGui::DockBuilderDockWindow("Propiedades", dock_right_id);
            }
            // Forzar que la barra de pestañas sea visible y habilitar botón de cierre
            if (ImGuiDockNode* node_root = ImGui::DockBuilderGetNode(dockspace_id)) {
                node_root->LocalFlags &= ~(ImGuiDockNodeFlags_AutoHideTabBar | ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_HiddenTabBar | ImGuiDockNodeFlags_NoCloseButton);
            }
            if (ImGuiDockNode* node_left = ImGui::DockBuilderGetNode(dock_left_id)) {
                node_left->LocalFlags &= ~(ImGuiDockNodeFlags_AutoHideTabBar | ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_HiddenTabBar | ImGuiDockNodeFlags_NoCloseButton);
            }
            if (ImGuiDockNode* node_right = ImGui::DockBuilderGetNode(dock_right_id)) {
                node_right->LocalFlags &= ~(ImGuiDockNodeFlags_AutoHideTabBar | ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_HiddenTabBar | ImGuiDockNodeFlags_NoCloseButton);
            }
            ImGui::DockBuilderFinish(dockspace_id);
            // Aplicar recursivamente por seguridad tras finalizar el layout
            if (ImGuiDockNode* node_after = ImGui::DockBuilderGetNode(dockspace_id)) {
                EnsureDockTabsVisible(node_after);
            }
        }

        // Presets panel (dockable, closable)
        if (show_presets) {
            // Sugerir acople inicial al dock izquierdo solo la primera vez
            if (g_dock_left_id != 0) {
                ImGui::SetNextWindowDockID(g_dock_left_id, ImGuiCond_FirstUseEver);
            }
            if (ImGui::Begin("Presets", &show_presets)) {
                ImGui::TextUnformatted("Arrastra al escenario:");
                if (ImGui::Button("Cubo 1x1x1")) {
                    // (Opcional) Click simple no hace nada; use drag & drop
                }
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    static int payload_dummy = 1;
                    ImGui::SetDragDropPayload("DND_ADD_CUBE", &payload_dummy, sizeof(payload_dummy));
                    ImGui::TextUnformatted("Soltar para crear cubo 1x1x1");
                    // Iniciar preview si aún no está activo
                    if (!g_drag_preview_active) {
                        const float* cpos = get_camera_position();
                        const float* cfront = get_camera_front();
                        float frontN[3] = { cfront[0], cfront[1], cfront[2] }; vec3_normalize(frontN);
                        CubeInst ci;
                        const float k = 5.0f;
                        ci.pos[0] = cpos[0] + frontN[0] * k;
                        ci.pos[1] = cpos[1] + frontN[1] * k;
                        ci.pos[2] = cpos[2] + frontN[2] * k;
                        
                        // Guardar estado antes de agregar nuevo objeto
                        SaveUndoState();
                        
                        g_scene_cubes.push_back(ci);
                        g_drag_preview_active = true;
                        g_drag_preview_index = (int)g_scene_cubes.size() - 1;
                        g_project_modified = true;
                        std::cout << "[DnD][Source] Preview created at front-of-camera, index=" << g_drag_preview_index << "\n";
                    }
                    ImGui::EndDragDropSource();
                }
            }
            ImGui::End();
        }
        
        // Ventana de propiedades para el objeto seleccionado
        if (g_show_properties) {
            if (ImGui::Begin("Propiedades", &g_show_properties)) {
                if (g_selected_cube_indices.size() > 1) {
                    // Selección múltiple - mostrar información general
                    ImGui::Text("Selección múltiple");
                    ImGui::Text("Objetos seleccionados: %d", (int)g_selected_cube_indices.size());
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Las propiedades no están disponibles");
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "para selección múltiple");
                } else if (g_selected_cube_index >= 0 && g_selected_cube_index < (int)g_scene_cubes.size()) {
                    CubeInst& selected = g_scene_cubes[g_selected_cube_index];
                    
                    ImGui::Text("Objeto seleccionado: Cubo %d", g_selected_cube_index);
                    ImGui::Separator();
                    
                    // Controles según el modo de transformación
                    if (g_transform_mode == TRANSLATE) {
                        ImGui::Text("Posición:");
                        bool posChanged = false;
                        posChanged |= ImGui::DragFloat("X##pos", &selected.pos[0], 0.1f);
                        posChanged |= ImGui::DragFloat("Y##pos", &selected.pos[1], 0.1f);
                        posChanged |= ImGui::DragFloat("Z##pos", &selected.pos[2], 0.1f);
                        
                        if (posChanged) {
                            g_project_modified = true;
                            std::cout << "Posición actualizada: (" << selected.pos[0] << ", " << selected.pos[1] << ", " << selected.pos[2] << ")\n";
                        }
                    } else if (g_transform_mode == ROTATE) {
                        ImGui::Text("Rotación (grados):");
                        bool rotChanged = false;
                        rotChanged |= ImGui::DragFloat("X##rot", &selected.rotation[0], 1.0f, -360.0f, 360.0f);
                        rotChanged |= ImGui::DragFloat("Y##rot", &selected.rotation[1], 1.0f, -360.0f, 360.0f);
                        rotChanged |= ImGui::DragFloat("Z##rot", &selected.rotation[2], 1.0f, -360.0f, 360.0f);
                        
                        if (rotChanged) {
                            g_project_modified = true;
                            std::cout << "Rotación actualizada: (" << selected.rotation[0] << ", " << selected.rotation[1] << ", " << selected.rotation[2] << ")\n";
                        }
                    } else if (g_transform_mode == SCALE) {
                        ImGui::Text("Escala:");
                        bool scaleChanged = false;
                        scaleChanged |= ImGui::DragFloat("X##scale", &selected.scale[0], 0.01f, 0.1f, 5.0f);
                        scaleChanged |= ImGui::DragFloat("Y##scale", &selected.scale[1], 0.01f, 0.1f, 5.0f);
                        scaleChanged |= ImGui::DragFloat("Z##scale", &selected.scale[2], 0.01f, 0.1f, 5.0f);
                        
                        if (scaleChanged) {
                            g_project_modified = true;
                            std::cout << "Escala actualizada: (" << selected.scale[0] << ", " << selected.scale[1] << ", " << selected.scale[2] << ")\n";
                        }
                    }
                    
                    ImGui::Separator();
                    
                    // Selector de material
                    ImGui::Text("Material:");
                    
                    // Mostrar material actual
                    std::string currentMaterialName = "Ninguno";
                    if (!selected.materialPath.empty()) {
                        std::filesystem::path matPath(selected.materialPath);
                        currentMaterialName = matPath.stem().string();
                    }
                    
                    // Combo box para seleccionar material
                    if (ImGui::BeginCombo("##MaterialSelector", currentMaterialName.c_str())) {
                        // Opción "Ninguno"
                        if (ImGui::Selectable("Ninguno", selected.materialPath.empty())) {
                            // Guardar estado antes de cambiar material
                            SaveUndoState();
                            selected.materialPath = "";
                            g_project_modified = true;
                            std::cout << "Material removido del objeto\n";
                        }
                        
                        // Escanear materiales disponibles si la lista está vacía
                        if (g_available_materials.empty()) {
                            ScanAvailableMaterials();
                        }
                        
                        // Mostrar materiales disponibles
                        for (const auto& materialPath : g_available_materials) {
                            std::filesystem::path matPath(materialPath);
                            std::string materialName = matPath.stem().string();
                            bool isSelected = (selected.materialPath == materialPath);
                            
                            if (ImGui::Selectable(materialName.c_str(), isSelected)) {
                                // Guardar estado antes de cambiar material
                                SaveUndoState();
                                selected.materialPath = materialPath;
                                g_project_modified = true;
                                std::cout << "Material aplicado: " << materialName << std::endl;
                            }
                        }
                        ImGui::EndCombo();
                    }
                    
                    // Botón para actualizar lista de materiales
                    ImGui::SameLine();
                    if (ImGui::Button("Actualizar")) {
                        ScanAvailableMaterials();
                        std::cout << "Lista de materiales actualizada\n";
                    }
                    ImGui::Text("Modo de Transformación:");
                    if (ImGui::RadioButton("Traslación", g_transform_mode == TRANSLATE)) {
                        g_transform_mode = TRANSLATE;
                        std::cout << "Modo cambiado a: Traslación\n";
                    }
                    ImGui::SameLine();
                    if (ImGui::RadioButton("Rotación", g_transform_mode == ROTATE)) {
                        g_transform_mode = ROTATE;
                        std::cout << "Modo cambiado a: Rotación\n";
                    }
                    ImGui::SameLine();
                    if (ImGui::RadioButton("Escala", g_transform_mode == SCALE)) {
                        g_transform_mode = SCALE;
                        std::cout << "Modo cambiado a: Escala\n";
                    }
                    
                    ImGui::Separator();
                    ImGui::Checkbox("Mostrar Gizmos", &g_show_gizmos);
                } else {
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Ningún objeto seleccionado");
                    ImGui::Text("Haz click en un cubo para seleccionarlo");
                }
            }
            ImGui::End();
        }

        // TreeObjects panel (dockable, closable)
        if (g_show_tree_objects) {
            if (ImGui::Begin("TreeObjects", &g_show_tree_objects)) {
                ImGui::Text("Objetos en la Escena:");
                ImGui::Separator();
                
                // Lista de objetos
                for (int i = 0; i < (int)g_scene_cubes.size(); ++i) {
                    const CubeInst& cube = g_scene_cubes[i];
                    
                    // Crear nombre único para cada objeto
                    char objectName[64];
                    snprintf(objectName, sizeof(objectName), "Cubo %d", i);
                    
                    // Crear selectable item
                    bool is_selected = (g_selected_cube_index == i);
                    if (ImGui::Selectable(objectName, is_selected)) {
                        // Deseleccionar todos los cubos
                        for (auto& c : g_scene_cubes) {
                            c.selected = false;
                        }
                        // Seleccionar el cubo clickeado
                        g_scene_cubes[i].selected = true;
                        g_selected_cube_index = i;
                        std::cout << "Objeto seleccionado desde TreeObjects: " << i << "\n";
                    }
                    
                    // Mostrar información adicional del objeto
                    if (is_selected) {
                        ImGui::Indent();
                        ImGui::Text("Pos: (%.2f, %.2f, %.2f)", cube.pos[0], cube.pos[1], cube.pos[2]);
                        ImGui::Text("Rot: (%.1f°, %.1f°, %.1f°)", cube.rotation[0], cube.rotation[1], cube.rotation[2]);
                        ImGui::Text("Scale: (%.2f, %.2f, %.2f)", cube.scale[0], cube.scale[1], cube.scale[2]);
                        ImGui::Unindent();
                    }
                }
                
                if (g_scene_cubes.empty()) {
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No hay objetos en la escena");
                    ImGui::Text("Arrastra un cubo desde Presets para crear objetos");
                }
            }
            ImGui::End();
        }

        // FileExplorer panel mejorado (dockable, closable)
        if (g_show_file_explorer) {
            if (ImGui::Begin("File Explorer", &g_show_file_explorer)) {
                // Inicializar directorio si es la primera vez
                static bool first_scan = true;
                if (first_scan) {
                    ScanDirectory(g_current_directory);
                    first_scan = false;
                }
                
                // Barra de navegación mejorada
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.3f, 0.8f, 1.0f));
                
                // Botón Home
                if (ImGui::Button("Home")) {
                    g_current_directory = g_content_path;
                    ScanDirectory(g_current_directory);
                }
                ImGui::SameLine();
                
                // Botón Atrás
                if (ImGui::Button("<- Atras")) {
                    // Remover la última carpeta del path actual
                    std::string currentDir = g_current_directory;
                    
                    // Remover el slash final si existe
                    if (currentDir.back() == '/' || currentDir.back() == '\\') {
                        currentDir.pop_back();
                    }
                    
                    // Encontrar la última barra
                    size_t lastSlash = currentDir.find_last_of("/\\");
                    if (lastSlash != std::string::npos) {
                        std::string parentDir = currentDir.substr(0, lastSlash + 1);
                        
                        // Verificar que no vayamos más arriba que Content
                        if (parentDir.find(g_content_path.substr(0, g_content_path.length()-1)) != std::string::npos) {
                            g_current_directory = parentDir;
                            ScanDirectory(g_current_directory);
                            std::cout << "Navegando atrás a: " << g_current_directory << std::endl;
                        }
                    }
                }
                ImGui::SameLine();
                
                // Botón Nueva Carpeta
                if (ImGui::Button("+ Nueva Carpeta")) {
                    g_show_create_folder_dialog = true;
                    strcpy_s(g_new_folder_name, sizeof(g_new_folder_name), "Nueva Carpeta");
                }
                ImGui::SameLine();
                
                // Botón Actualizar
                if (ImGui::Button("Actualizar")) {
                    ScanDirectory(g_current_directory);
                }
                ImGui::SameLine();
                
                // Botón Crear
                if (ImGui::Button("Crear")) {
                    g_show_create_dialog = true;
                }
                ImGui::SameLine();
                
                // Botón Importar
                if (ImGui::Button("Importar")) {
                    g_show_import_dialog = true;
                }
                
                ImGui::PopStyleColor();
                
                // Mostrar ruta actual con breadcrumbs
                ImGui::Separator();
                ImGui::Text("Ubicacion:");
                ImGui::SameLine();
                
                // Crear breadcrumbs clickeables
                std::filesystem::path currentPath(g_current_directory);
                std::filesystem::path contentPath(g_content_path);
                std::vector<std::string> pathParts;
                
                std::filesystem::path relativePath = std::filesystem::relative(currentPath, contentPath.parent_path());
                for (const auto& part : relativePath) {
                    pathParts.push_back(part.string());
                }
                
                for (size_t i = 0; i < pathParts.size(); ++i) {
                    if (i > 0) {
                        ImGui::SameLine();
                        ImGui::Text(" > ");
                        ImGui::SameLine();
                    }
                    
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.1f, 0.1f, 0.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
                    
                    if (ImGui::SmallButton(pathParts[i].c_str())) {
                        // Navegar a esta parte del path
                        std::string newPath = contentPath.parent_path().string();
                        for (size_t j = 0; j <= i; ++j) {
                            newPath += "/" + pathParts[j];
                        }
                        newPath += "/";
                        g_current_directory = newPath;
                        ScanDirectory(g_current_directory);
                    }
                    
                    ImGui::PopStyleColor(2);
                }
                
                ImGui::Separator();
                
                // Lista de archivos y carpetas mejorada
                ImGui::BeginChild("FileList", ImVec2(0, 0), true);
                
                for (size_t i = 0; i < g_current_files.size(); ++i) {
                    const auto& file = g_current_files[i];
                    
                    // Modo renombrado
                    if (g_renaming_item && g_renaming_index == (int)i) {
                        ImGui::SetNextItemWidth(-1);
                        if (ImGui::InputText("##rename", g_rename_buffer, sizeof(g_rename_buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                            // Confirmar renombrado
                            try {
                                std::filesystem::path oldPath(file.path);
                                std::filesystem::path newPath = oldPath.parent_path() / g_rename_buffer;
                                
                                if (file.isDirectory) {
                                    newPath += "/";
                                }
                                
                                std::filesystem::rename(oldPath, newPath);
                                ScanDirectory(g_current_directory);
                                std::cout << "Renombrado: " << file.name << " -> " << g_rename_buffer << std::endl;
                            } catch (const std::filesystem::filesystem_error& ex) {
                                std::cout << "Error renombrando: " << ex.what() << std::endl;
                            }
                            
                            g_renaming_item = false;
                            g_renaming_index = -1;
                        }
                        
                        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                            g_renaming_item = false;
                            g_renaming_index = -1;
                        }
                    } else {
                        // Modo normal
                        std::string displayName = std::string(GetFileIcon(file.type)) + " " + file.name;
                        
                        // Color según tipo
                        ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                        if (file.isDirectory) {
                            color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // Amarillo dorado para carpetas
                        } else {
                            switch (file.type) {
                                case FILETYPE_TEXTURE: color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f); break; // Verde
                                case FILETYPE_AUDIO: color = ImVec4(0.2f, 0.6f, 1.0f, 1.0f); break;   // Azul
                                case FILETYPE_MODEL: color = ImVec4(1.0f, 0.6f, 0.2f, 1.0f); break;   // Naranja
                                case FILETYPE_SCENE: color = ImVec4(0.8f, 0.2f, 0.8f, 1.0f); break;   // Magenta
                                case FILETYPE_MATERIAL: color = ImVec4(0.6f, 1.0f, 0.6f, 1.0f); break; // Verde claro
                            }
                        }
                        
                        ImGui::PushStyleColor(ImGuiCol_Text, color);
                        
                        // Selectable con click derecho para menú contextual
                        if (ImGui::Selectable(displayName.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                            if (ImGui::IsMouseDoubleClicked(0)) {
                                if (file.isDirectory) {
                                    // Navegar a carpeta con doble click
                                    g_current_directory = file.path;
                                    if (g_current_directory.back() != '/') {
                                        g_current_directory += "/";
                                    }
                                    ScanDirectory(g_current_directory);
                                    std::cout << "Navegando a: " << g_current_directory << std::endl;
                                } else if (file.type == FILETYPE_MATERIAL) {
                                    // Abrir editor de materiales
                                    g_editing_material_path = file.path;
                                    g_current_material = LoadMaterial(file.path);
                                    strcpy_s(g_material_name_buffer, sizeof(g_material_name_buffer), g_current_material.name.c_str());
                                    ScanAvailableTextures();
                                    g_show_material_editor = true;
                                    std::cout << "Abriendo editor de material: " << file.name << std::endl;
                                }
                            }
                        }
                        
                        // Menú contextual con click derecho
                        if (ImGui::BeginPopupContextItem()) {
                            if (file.name != "..") {
                                if (ImGui::MenuItem("Renombrar")) {
                                    g_renaming_item = true;
                                    g_renaming_index = (int)i;
                                    strcpy_s(g_rename_buffer, sizeof(g_rename_buffer), file.name.c_str());
                                }
                                
                                if (ImGui::MenuItem("Eliminar")) {
                                    try {
                                        if (file.isDirectory) {
                                            std::filesystem::remove_all(file.path);
                                        } else {
                                            std::filesystem::remove(file.path);
                                        }
                                        ScanDirectory(g_current_directory);
                                        std::cout << "Eliminado: " << file.name << std::endl;
                                    } catch (const std::filesystem::filesystem_error& ex) {
                                        std::cout << "Error eliminando: " << ex.what() << std::endl;
                                    }
                                }
                            }
                            ImGui::EndPopup();
                        }
                        
                        // Drag & Drop para archivos
                        if (!file.isDirectory && file.name != "..") {
                            if (ImGui::BeginDragDropSource()) {
                                ImGui::SetDragDropPayload("DND_FILE", &file, sizeof(FileItem));
                                ImGui::Text("Arrastrando: %s", file.name.c_str());
                                ImGui::EndDragDropSource();
                            }
                        }
                        
                        ImGui::PopStyleColor();
                    }
                }
                
                if (g_current_files.empty()) {
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Carpeta vacia");
                }
                
                ImGui::EndChild();
                
                // Diálogo para crear nueva carpeta
                if (g_show_create_folder_dialog) {
                    ImGui::OpenPopup("Nueva Carpeta");
                }
                
                if (ImGui::BeginPopupModal("Nueva Carpeta", &g_show_create_folder_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Nombre de la nueva carpeta:");
                    ImGui::SetNextItemWidth(300);
                    
                    if (ImGui::InputText("##newfolder", g_new_folder_name, sizeof(g_new_folder_name), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        try {
                            std::string newFolderPath = g_current_directory + g_new_folder_name + "/";
                            std::filesystem::create_directory(newFolderPath);
                            ScanDirectory(g_current_directory);
                            std::cout << "Carpeta creada: " << newFolderPath << std::endl;
                            g_show_create_folder_dialog = false;
                        } catch (const std::filesystem::filesystem_error& ex) {
                            std::cout << "Error creando carpeta: " << ex.what() << std::endl;
                        }
                    }
                    
                    ImGui::Separator();
                    
                    if (ImGui::Button("Crear", ImVec2(120, 0))) {
                        try {
                            std::string newFolderPath = g_current_directory + g_new_folder_name + "/";
                            std::filesystem::create_directory(newFolderPath);
                            ScanDirectory(g_current_directory);
                            std::cout << "Carpeta creada: " << newFolderPath << std::endl;
                            g_show_create_folder_dialog = false;
                        } catch (const std::filesystem::filesystem_error& ex) {
                            std::cout << "Error creando carpeta: " << ex.what() << std::endl;
                        }
                    }
                    
                    ImGui::SameLine();
                    if (ImGui::Button("Cancelar", ImVec2(120, 0))) {
                        g_show_create_folder_dialog = false;
                    }
                    
                    ImGui::EndPopup();
                }
                
                // Diálogo para crear nuevo contenido
                if (g_show_create_dialog) {
                    ImGui::OpenPopup("Crear Nuevo");
                }
                
                if (ImGui::BeginPopupModal("Crear Nuevo", &g_show_create_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Selecciona el tipo de contenido a crear:");
                    ImGui::Separator();
                    
                    if (ImGui::Button("Material", ImVec2(200, 40))) {
                        ImGui::OpenPopup("Crear Material");
                    }
                    
                    ImGui::Separator();
                    if (ImGui::Button("Cancelar", ImVec2(200, 0))) {
                        g_show_create_dialog = false;
                    }
                    
                    // Sub-diálogo para crear material
                    if (ImGui::BeginPopupModal("Crear Material", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                        ImGui::Text("Nombre del material:");
                        ImGui::SetNextItemWidth(300);
                        
                        if (ImGui::InputText("##materialname", g_material_name_buffer, sizeof(g_material_name_buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                            CreateNewMaterial();
                            ImGui::CloseCurrentPopup();
                        }
                        
                        ImGui::Separator();
                        
                        if (ImGui::Button("Crear", ImVec2(120, 0))) {
                            CreateNewMaterial();
                            ImGui::CloseCurrentPopup();
                        }
                        
                        ImGui::SameLine();
                        if (ImGui::Button("Cancelar", ImVec2(120, 0))) {
                            ImGui::CloseCurrentPopup();
                        }
                        
                        ImGui::EndPopup();
                    }
                    
                    ImGui::EndPopup();
                }
                
                // Diálogo de importar (placeholder)
                if (g_show_import_dialog) {
                    ImGui::OpenPopup("Importar Archivos");
                }
                
                if (ImGui::BeginPopupModal("Importar Archivos", &g_show_import_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Funcionalidad de importar en desarrollo...");
                    ImGui::Separator();
                    
                    if (ImGui::Button("Cerrar", ImVec2(200, 0))) {
                        g_show_import_dialog = false;
                    }
                    
                    ImGui::EndPopup();
                }
            }
            ImGui::End();
        }
        
        // Editor de Materiales
        if (g_show_material_editor) {
            if (ImGui::Begin("Editor de Material", &g_show_material_editor, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Editando Material");
                ImGui::Separator();
                
                // Nombre del material
                ImGui::Text("Nombre:");
                ImGui::SetNextItemWidth(300);
                ImGui::InputText("##matname", g_material_name_buffer, sizeof(g_material_name_buffer));
                
                ImGui::Separator();
                
                // Color picker
                ImGui::Text("Color Base:");
                ImGui::ColorEdit3("##color", g_current_material.color);
                
                ImGui::Separator();
                
                // Propiedades del material
                ImGui::Text("Propiedades:");
                ImGui::SliderFloat("Metalico", &g_current_material.metallic, 0.0f, 1.0f);
                ImGui::SliderFloat("Rugosidad", &g_current_material.roughness, 0.0f, 1.0f);
                ImGui::SliderFloat("Emision", &g_current_material.emission, 0.0f, 2.0f);
                
                ImGui::Separator();
                
                // Selector de textura
                ImGui::Text("Textura Difusa:");
                if (ImGui::BeginCombo("##texture", g_current_material.diffuseTexture.empty() ? "Ninguna" : std::filesystem::path(g_current_material.diffuseTexture).filename().string().c_str())) {
                    if (ImGui::Selectable("Ninguna", g_current_material.diffuseTexture.empty())) {
                        g_current_material.diffuseTexture = "";
                    }
                    
                    for (const auto& texture : g_available_textures) {
                        bool isSelected = (g_current_material.diffuseTexture == texture);
                        std::string displayName = std::filesystem::path(texture).filename().string();
                        
                        if (ImGui::Selectable(displayName.c_str(), isSelected)) {
                            g_current_material.diffuseTexture = texture;
                        }
                        
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                
                ImGui::Separator();
                
                // Botones de acción
                if (ImGui::Button("Guardar", ImVec2(120, 0))) {
                    g_current_material.name = g_material_name_buffer;
                    SaveMaterial(g_current_material, g_editing_material_path);
                    ScanDirectory(g_current_directory);
                    g_show_material_editor = false;
                }
                
                ImGui::SameLine();
                if (ImGui::Button("Cancelar", ImVec2(120, 0))) {
                    g_show_material_editor = false;
                }
            }
            ImGui::End();
        }

        // Los diálogos personalizados han sido reemplazados por diálogos nativos de Windows

        ImGui::End(); // DockSpaceHost

        // Render Dear ImGui and present
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    glDeleteVertexArrays(1, &vaoGrid);
    glDeleteBuffers(1, &vboGrid);
    glDeleteVertexArrays(1, &vaoCubeSolid);
    glDeleteBuffers(1, &vboCubeSolid);
    glDeleteVertexArrays(1, &vaoGizmoArrow);
    glDeleteBuffers(1, &vboGizmoArrow);
    glDeleteVertexArrays(1, &vaoGizmoRing);
    glDeleteBuffers(1, &vboGizmoRing);
    glDeleteVertexArrays(1, &vaoGizmoScale);
    glDeleteBuffers(1, &vboGizmoScale);
    glDeleteProgram(prog3D);

    // Dear ImGui cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}