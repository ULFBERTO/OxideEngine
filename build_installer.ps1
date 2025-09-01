# Script para construir y generar el instalador de MarioEngine
# Requiere: CMake, Visual Studio Build Tools, NSIS

param(
    [string]$BuildType = "Release",
    [string]$Generator = "Visual Studio 17 2022",
    [string]$Platform = "x64"
)

Write-Host "=== Construyendo instalador de MarioEngine ===" -ForegroundColor Green

# Verificar que CMake esté instalado
if (!(Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Error "CMake no está instalado o no está en el PATH"
    exit 1
}

# Verificar que NSIS esté instalado
$nsisPath = "${env:ProgramFiles(x86)}\NSIS\makensis.exe"
if (!(Test-Path $nsisPath)) {
    $nsisPath = "${env:ProgramFiles}\NSIS\makensis.exe"
    if (!(Test-Path $nsisPath)) {
        Write-Warning "NSIS no encontrado. Instalando desde Chocolatey..."
        if (Get-Command choco -ErrorAction SilentlyContinue) {
            choco install nsis -y
        } else {
            Write-Error "NSIS no está instalado. Por favor instala NSIS desde https://nsis.sourceforge.io/"
            exit 1
        }
    }
}

# Crear directorio de construcción
$buildDir = "build-installer"
if (Test-Path $buildDir) {
    Write-Host "Limpiando directorio de construcción anterior..." -ForegroundColor Yellow
    Remove-Item $buildDir -Recurse -Force
}

New-Item -ItemType Directory -Path $buildDir | Out-Null
Set-Location $buildDir

try {
    # Configurar el proyecto
    Write-Host "Configurando proyecto con CMake..." -ForegroundColor Cyan
    cmake .. -G $Generator -A $Platform -DCMAKE_BUILD_TYPE=$BuildType
    
    if ($LASTEXITCODE -ne 0) {
        throw "Error en la configuración de CMake"
    }

    # Construir el proyecto
    Write-Host "Construyendo proyecto..." -ForegroundColor Cyan
    cmake --build . --config $BuildType --parallel
    
    if ($LASTEXITCODE -ne 0) {
        throw "Error en la construcción del proyecto"
    }

    # Generar el instalador
    Write-Host "Generando instalador..." -ForegroundColor Cyan
    cpack -C $BuildType
    
    if ($LASTEXITCODE -ne 0) {
        throw "Error al generar el instalador"
    }

    # Mostrar resultados
    Write-Host "=== Instalador generado exitosamente ===" -ForegroundColor Green
    $installers = Get-ChildItem -Filter "*.exe" | Where-Object { $_.Name -like "*MarioEngine*" }
    
    if ($installers) {
        Write-Host "Instaladores generados:" -ForegroundColor Yellow
        foreach ($installer in $installers) {
            $fullPath = Join-Path (Get-Location) $installer.Name
            Write-Host "  - $fullPath" -ForegroundColor White
            Write-Host "    Tamaño: $([math]::Round($installer.Length / 1MB, 2)) MB" -ForegroundColor Gray
        }
    }

} catch {
    Write-Error "Error durante el proceso: $_"
    exit 1
} finally {
    Set-Location ..
}

Write-Host "`n=== Proceso completado ===" -ForegroundColor Green
Write-Host "Para instalar, ejecuta el archivo .exe generado como administrador." -ForegroundColor Yellow
