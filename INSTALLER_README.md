# Generador de Instalador - MarioEngine

Este proyecto incluye herramientas para generar un instalador profesional de MarioEngine para Windows.

## Requisitos Previos

### Obligatorios
- **CMake** (versión 3.11 o superior)
- **Visual Studio 2022** o **Visual Studio Build Tools 2022**
- **NSIS** (Nullsoft Scriptable Install System)

### Instalación de NSIS
1. Descarga NSIS desde: https://nsis.sourceforge.io/Download
2. O instala con Chocolatey: `choco install nsis`

## Métodos de Generación

### Método 1: Script PowerShell (Recomendado)
```powershell
.\build_installer.ps1
```

**Opciones avanzadas:**
```powershell
# Construcción Debug
.\build_installer.ps1 -BuildType Debug

# Usar Visual Studio 2019
.\build_installer.ps1 -Generator "Visual Studio 16 2019"

# Plataforma x86
.\build_installer.ps1 -Platform Win32
```


PS D:\AI\Pysics> cmake --build build-msvc-x64 --config Release --parallel
>> .\build-msvc-x64\Release\MarioEngine.exe



### Método 2: Script Batch (Simple)
```batch
build_installer.bat
```

### Método 3: Manual
```batch
# 1. Crear directorio de construcción
mkdir build-installer
cd build-installer

# 2. Configurar con CMake
cmake .. -G "Visual Studio 17 2022" -A x64

# 3. Construir proyecto
cmake --build . --config Release

# 4. Generar instalador
cpack -C Release
```

## Características del Instalador

El instalador generado incluye:

- ✅ **Ejecutable principal** (`MarioEngine.exe`)
- ✅ **Archivos de configuración** (`config.txt`, `imgui.ini`)
- ✅ **Recursos del proyecto** (carpeta `Content/`)
- ✅ **Acceso directo en escritorio**
- ✅ **Entrada en menú inicio**
- ✅ **Desinstalador automático**
- ✅ **Detección de versiones previas**

## Estructura de Instalación

```
C:\Program Files\MarioEngine\
├── bin\
│   ├── MarioEngine.exe
│   ├── config.txt
│   ├── imgui.ini
│   └── Content\
│       ├── Audio\
│       ├── Icons\
│       ├── Materials\
│       └── Models\
└── Uninstall.exe
```

## Solución de Problemas

### Error: "CMake no encontrado"
- Instala CMake desde: https://cmake.org/download/
- Asegúrate de que esté en el PATH del sistema

### Error: "Visual Studio no encontrado"
- Instala Visual Studio 2022 Community (gratuito)
- O instala solo las Build Tools desde: https://visualstudio.microsoft.com/downloads/

### Error: "NSIS no encontrado"
- Descarga e instala NSIS desde el enlace oficial
- Reinicia la terminal después de la instalación

### Error: "No se puede escribir en Program Files"
- Ejecuta el instalador como administrador
- Click derecho → "Ejecutar como administrador"

## Personalización

### Cambiar información del instalador
Edita las siguientes líneas en `CMakeLists.txt`:

```cmake
set(CPACK_PACKAGE_VENDOR "Tu Empresa")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Tu descripción")
set(CPACK_NSIS_CONTACT "tu-email@ejemplo.com")
```

### Cambiar versión
Modifica en `CMakeLists.txt`:

```cmake
set(CPACK_PACKAGE_VERSION "2.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "2")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
```

## Distribución

El archivo `.exe` generado es completamente autónomo y puede distribuirse sin dependencias adicionales.

**Tamaño aproximado:** 15-25 MB (dependiendo de los recursos incluidos)

## Soporte

Para problemas específicos del instalador, revisa:
1. Los logs de CMake en `build-installer/`
2. Los logs de CPack en el mismo directorio
3. Que todas las dependencias estén correctamente instaladas
