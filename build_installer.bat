@echo off
setlocal enabledelayedexpansion

echo === Construyendo instalador de MarioEngine ===

:: Verificar CMake
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: CMake no esta instalado o no esta en el PATH
    pause
    exit /b 1
)

:: Crear directorio de construccion
if exist build-installer (
    echo Limpiando directorio anterior...
    rmdir /s /q build-installer
)

mkdir build-installer
cd build-installer

:: Configurar proyecto
echo Configurando proyecto...
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo ERROR: Fallo la configuracion de CMake
    pause
    exit /b 1
)

:: Construir proyecto
echo Construyendo proyecto...
cmake --build . --config Release --parallel
if %errorlevel% neq 0 (
    echo ERROR: Fallo la construccion del proyecto
    pause
    exit /b 1
)

:: Generar instalador
echo Generando instalador...
cpack -C Release
if %errorlevel% neq 0 (
    echo ERROR: Fallo la generacion del instalador
    pause
    exit /b 1
)

echo.
echo === Instalador generado exitosamente ===
echo Busca el archivo .exe en la carpeta build-installer
echo.

:: Mostrar archivos generados
dir *.exe

cd ..
pause
