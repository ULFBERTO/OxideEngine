cmake_minimum_required(VERSION 3.11)

# Read project name from config.txt
file(STRINGS "${CMAKE_SOURCE_DIR}/config.txt" CONFIG_LINES)
foreach(line ${CONFIG_LINES})
    if(line MATCHES "^project_name=(.+)$")
        set(PROJECT_NAME ${CMAKE_MATCH_1})
        break()
    endif()
endforeach()

# Set default project name if not found in config
if(NOT PROJECT_NAME)
    set(PROJECT_NAME "MarioEngine")
endif()

project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ensure compatibility with GLAD/vcpkg policies on modern CMake
cmake_policy(VERSION 3.5...4.0)
set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING "Allow subprojects with older minimums" FORCE)

# --- Graphics Libraries ---
find_package(OpenGL REQUIRED)

include(FetchContent)

# Allow legacy FetchContent_Populate calls for projects without native CMake (e.g., imgui)
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

# --- Download and configure GLFW ---
set(GLFW_URL "https://github.com/glfw/glfw/archive/refs/tags/3.4.tar.gz")
if(DEFINED GLFW_LOCAL_TARBALL)
    set(GLFW_URL "${GLFW_LOCAL_TARBALL}")
endif()
if(DEFINED GLFW_LOCAL_SOURCE_DIR)
    FetchContent_Declare(glfw SOURCE_DIR ${GLFW_LOCAL_SOURCE_DIR})
else()
    FetchContent_Declare(glfw URL ${GLFW_URL})
endif()
FetchContent_MakeAvailable(glfw)

# --- Download and configure GLAD ---
set(GLAD_URL "https://github.com/Dav1dde/glad/archive/refs/tags/v0.1.36.tar.gz")
if(DEFINED GLAD_LOCAL_TARBALL)
    set(GLAD_URL "${GLAD_LOCAL_TARBALL}")
endif()
if(DEFINED GLAD_LOCAL_SOURCE_DIR)
    FetchContent_Declare(glad SOURCE_DIR ${GLAD_LOCAL_SOURCE_DIR})
else()
    FetchContent_Declare(glad URL ${GLAD_URL})
endif()
FetchContent_MakeAvailable(glad)
# --------------------------------

# --- Dear ImGui (docking branch) ---
set(IMGUI_URL "https://github.com/ocornut/imgui/archive/refs/tags/v1.91.5-docking.tar.gz")
if(DEFINED IMGUI_LOCAL_TARBALL)
    set(IMGUI_URL "${IMGUI_LOCAL_TARBALL}")
endif()
if(DEFINED IMGUI_LOCAL_SOURCE_DIR)
    FetchContent_Declare(imgui SOURCE_DIR ${IMGUI_LOCAL_SOURCE_DIR})
else()
    FetchContent_Declare(imgui URL ${IMGUI_URL})
endif()
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
    add_library(imgui
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
    target_link_libraries(imgui PUBLIC glfw OpenGL::GL glad)
endif()
# --------------------------------

add_executable(${PROJECT_NAME} WIN32 src/main.cpp src/core/application.cpp src/scene/scene.cpp src/editor/editor_layer.cpp src/camera/camera.cpp src/utils/math_utils.cpp src/shaders/shader.cpp src/project/project_manager.cpp)

# --- Link libraries and include paths ---
# Linking to the 'glad' and 'glfw' targets automatically handles their sources and include directories.
target_link_libraries(${PROJECT_NAME} PRIVATE glad OpenGL::GL glfw imgui)
# Ensure the main target also sees ImGui headers (helps IntelliSense and compilers)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
# ---------------------------------------

# --- Configuración del instalador ---
# Instalar el ejecutable
install(TARGETS ${PROJECT_NAME} 
    RUNTIME DESTINATION bin
    COMPONENT Runtime
)

# Instalar archivos de configuración
install(FILES config.txt imgui.ini
    DESTINATION bin
    COMPONENT Runtime
    OPTIONAL
)

# Instalar carpeta Content si existe
if(EXISTS "${CMAKE_SOURCE_DIR}/Content")
    install(DIRECTORY Content/
        DESTINATION bin/Content
        COMPONENT Runtime
    )
endif()

# Configuración de CPack
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "MarioEngine Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Motor gráfico avanzado con OpenGL")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")

# Configuración específica para Windows
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "${PROJECT_NAME}")
    set(CPACK_NSIS_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_NSIS_CONTACT "marioengine@example.com")
    set(CPACK_NSIS_HELP_LINK "https://github.com/marioengine")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/marioengine")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    
    # Crear acceso directo en el escritorio
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$DESKTOP\\\\${PROJECT_NAME}.lnk' '$INSTDIR\\\\bin\\\\${PROJECT_NAME}.exe'"
    )
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$DESKTOP\\\\${PROJECT_NAME}.lnk'"
    )
    
    # Crear entrada en el menú inicio
    set(CPACK_NSIS_MENU_LINKS
        "bin/${PROJECT_NAME}.exe" "${PROJECT_NAME}"
    )
endif()

# Incluir CPack
include(CPack)