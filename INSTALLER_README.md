# Guía de Inicio - OxideEngine

Esta guía explica cómo compilar y ejecutar el proyecto de manera sencilla para desarrollo, así como generar el instalador final.

## 🚀 Inicio Rápido (Quick Start)

Para ejecutar el motor rápidamente en tu PC.

### 1. Requisitos Previos
*   **Visual Studio 2022** con la carga de trabajo "Desarrollo de escritorio con C++".
*   **CMake** (Instalado y en el PATH).
*   **Python 3** (Necesario para generar archivos GLAD, asegúrate de marcar "Add to PATH" al instalar).

### 2. Compilar y Correr (Manual)
Abre una terminal en la carpeta del proyecto y ejecuta:

```powershell
# 1. Limpiar construcciones previas (opcional, si hay errores)
Remove-Item -Recurse -Force build-msvc-x64

# 2. Configurar el proyecto
cmake -S . -B build-msvc-x64 -G "Visual Studio 17 2022" -A x64

# 3. Compilar en modo Release
cmake --build build-msvc-x64 --config Release

# 4. Preparar y ejecutar
# Copia los archivos necesarios y corre el motor
Copy-Item "config.txt", "imgui.ini" -Destination "build-msvc-x64\Release"
Copy-Item "deps" -Destination "build-msvc-x64\Release" -Recurse
if (Test-Path "Content") { Copy-Item "Content" -Destination "build-msvc-x64\Release" -Recurse }

Start-Process "build-msvc-x64\Release\MarioEngine.exe"
```

---

## 📦 Generar Instalador (Para distribuir)

Si deseas crear un archivo `.exe` instalable para enviar a otras personas.

### Opción A: Script Automático (Recomendado)
Ejecuta el script de PowerShell que hace todo el trabajo sucio:

```powershell
.\build_installer.ps1
```
*Esto generará una carpeta `build-installer` y pondrá el instalador ahí.*

### Opción B: Manual
```powershell
mkdir build-installer
cd build-installer
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
cpack -C Release
```

## 🧹 Limpieza
Si tienes demasiadas carpetas `build`, puedes borrarlas todas de forma segura para empezar de cero:
*   `build-msvc-x64` (Tu entorno de desarrollo local)
*   `build-installer` (Archivos temporales del instalador)

