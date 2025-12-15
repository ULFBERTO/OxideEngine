# Script para compilar y correr el motor en modo desarrollo

Write-Host "Construyendo y Corriendo MarioEngine..." -ForegroundColor Cyan

# 1. Configurar y Compilar
$buildDir = "build-msvc-x64"

# Si no existe la carpeta, configuramos primero
if (!(Test-Path $buildDir)) {
    Write-Host "Configurando proyecto por primera vez..." -ForegroundColor Cyan
    cmake -S . -B $buildDir -G "Visual Studio 17 2022" -A x64
    if ($LASTEXITCODE -ne 0) {
        Write-Error "La configuracion fallo."
        exit 1
    }
}

Write-Host "Compilando..." -ForegroundColor Cyan
cmake --build $buildDir --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Error "La compilacion fallo."
    exit 1
}

# 2. Copiar assets necesarios
$dest = "build-msvc-x64\Release"
Write-Host "Copiando archivos a $dest..." -ForegroundColor Yellow
Copy-Item "config.txt" -Destination $dest -Force -ErrorAction SilentlyContinue
Copy-Item "imgui.ini" -Destination $dest -Force -ErrorAction SilentlyContinue
Copy-Item "deps" -Destination $dest -Recurse -Force -ErrorAction SilentlyContinue
if (Test-Path "Content") { 
    Copy-Item "Content" -Destination $dest -Recurse -Force 
}

# 3. Ejecutar
Write-Host "Iniciando Motor..." -ForegroundColor Green
Start-Process "$dest\MarioEngine.exe"
