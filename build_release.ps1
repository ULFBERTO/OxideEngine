# Script para construir y empaquetar MarioEngine para GitHub Release
# Genera un ZIP portable que no requiere instalación

param(
    [string]$Version = "1.0.0"
)

$ErrorActionPreference = "Stop"
$ProjectName = "MarioEngine"
$ReleaseDir = "release-package"
$ZipName = "${ProjectName}-v${Version}-Windows-x64.zip"

Write-Host "=== Construyendo $ProjectName v$Version para Release ===" -ForegroundColor Green

# Verificar CMake
if (!(Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Error "CMake no esta instalado o no esta en el PATH"
    exit 1
}

# Limpiar directorio de release anterior
if (Test-Path $ReleaseDir) {
    Write-Host "Limpiando release anterior..." -ForegroundColor Yellow
    Remove-Item $ReleaseDir -Recurse -Force
}

# Crear directorio de build si no existe
$BuildDir = "build-msvc-x64"
if (!(Test-Path $BuildDir)) {
    Write-Host "Configurando proyecto..." -ForegroundColor Cyan
    cmake -S . -B $BuildDir -G "Visual Studio 17 2022" -A x64
}

# Compilar en Release
Write-Host "Compilando en modo Release..." -ForegroundColor Cyan
cmake --build $BuildDir --config Release --parallel

if ($LASTEXITCODE -ne 0) {
    Write-Error "Error en la compilacion"
    exit 1
}

# Crear estructura de release
Write-Host "Empaquetando release..." -ForegroundColor Cyan
New-Item -ItemType Directory -Path $ReleaseDir -Force | Out-Null
New-Item -ItemType Directory -Path "$ReleaseDir\$ProjectName" -Force | Out-Null

$DestDir = "$ReleaseDir\$ProjectName"

# Copiar ejecutable
Copy-Item "$BuildDir\Release\$ProjectName.exe" -Destination $DestDir

# Copiar archivos de configuracion
if (Test-Path "config.txt") { Copy-Item "config.txt" -Destination $DestDir }
if (Test-Path "imgui.ini") { Copy-Item "imgui.ini" -Destination $DestDir }

# Copiar Content si existe
if (Test-Path "Content") {
    Copy-Item "Content" -Destination $DestDir -Recurse
}

# Crear README para el release
$ReadmeContent = @"
# $ProjectName v$Version

## Como ejecutar
1. Extrae todos los archivos a una carpeta
2. Ejecuta $ProjectName.exe

## Controles
- Click derecho + arrastrar: Rotar camara
- WASD: Mover camara
- Scroll: Zoom
- Click izquierdo en Scene: Seleccionar objeto
- W/E/R: Cambiar modo (Mover/Rotar/Escalar)

## Requisitos
- Windows 10/11 64-bit
- Tarjeta grafica compatible con OpenGL 3.3

## Soporte
Reporta problemas en: https://github.com/tu-usuario/MarioEngine/issues
"@

$ReadmeContent | Out-File -FilePath "$DestDir\README.txt" -Encoding UTF8

# Crear ZIP
Write-Host "Creando archivo ZIP..." -ForegroundColor Cyan
$ZipPath = "$ReleaseDir\$ZipName"

if (Test-Path $ZipPath) { Remove-Item $ZipPath }

Compress-Archive -Path "$DestDir\*" -DestinationPath $ZipPath -CompressionLevel Optimal

# Mostrar resultado
$ZipInfo = Get-Item $ZipPath
Write-Host ""
Write-Host "=== Release generado exitosamente ===" -ForegroundColor Green
Write-Host "Archivo: $ZipPath" -ForegroundColor White
Write-Host "Tamano: $([math]::Round($ZipInfo.Length / 1MB, 2)) MB" -ForegroundColor Gray
Write-Host ""
Write-Host "Sube este archivo a GitHub Releases:" -ForegroundColor Yellow
Write-Host "  $((Get-Item $ZipPath).FullName)" -ForegroundColor Cyan
