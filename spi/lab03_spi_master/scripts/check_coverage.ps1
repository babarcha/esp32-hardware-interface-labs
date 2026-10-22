
# ============================================================
# SPI Lab 03 — Pass 10.10
# Native C Testing and Production Coverage Quality Gates
#
# Platform: Windows 11 / PowerShell
# Toolchain: MSYS2 UCRT64 GCC
# Framework: ESP-IDF v5.5.5
#
# Usage:
# powershell -NoProfile -ExecutionPolicy Bypass -File `
#   .\spi\lab03_spi_master\scripts\check_coverage.ps1
# ============================================================

param(
    [int]$MinLine = 90,
    [int]$MinBranch = 80
)

$ErrorActionPreference = "Stop"

# ------------------------------------------------------------
# 1. Project directories
# ------------------------------------------------------------

$LabRoot = (
    Resolve-Path (Join-Path $PSScriptRoot "..")
).Path

$SourceDir = Join-Path $LabRoot "tests/native"
$BuildDir = Join-Path $LabRoot "build-coverage"
$ReportDir = Join-Path $LabRoot "reports/coverage"
$ProductionDir = Join-Path $LabRoot "main"

# ------------------------------------------------------------
# 2. Development tools
# ------------------------------------------------------------

$CMakeBin = "C:\Espressif\tools\cmake\3.30.2\bin"
$MsysBin = "C:\msys64\ucrt64\bin"

$Python = "C:\Espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe"

$CMake = Join-Path $CMakeBin "cmake.exe"
$CTest = Join-Path $CMakeBin "ctest.exe"
$Gcc = Join-Path $MsysBin "gcc.exe"
$Make = Join-Path $MsysBin "mingw32-make.exe"
$Gcov = Join-Path $MsysBin "gcov.exe"

# Include required DLL and executable directories.
$env:PATH = "$CMakeBin;$MsysBin;$env:PATH"

# ------------------------------------------------------------
# 3. Helper functions
# ------------------------------------------------------------

function Write-Step {
    param([string]$Message)

    Write-Host ""
    Write-Host "==================================================" `
        -ForegroundColor Cyan
    Write-Host $Message -ForegroundColor Cyan
    Write-Host "==================================================" `
        -ForegroundColor Cyan
}

function Assert-FileExists {
    param(
        [string]$Path,
        [string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description not found: $Path"
    }
}

function Invoke-Checked {
    param(
        [string]$Description,
        [scriptblock]$Command
    )

    Write-Step $Description

    # Clear any previous native-command exit status.
    $global:LASTEXITCODE = 0

    & $Command

    if ($LASTEXITCODE -ne 0) {
        throw "$Description failed (exit code $LASTEXITCODE)"
    }
}

# ------------------------------------------------------------
# 4. Validate environment
# ------------------------------------------------------------

Write-Step "Validate development environment"

Assert-FileExists $Python "Python"
Assert-FileExists $CMake "CMake"
Assert-FileExists $CTest "CTest"
Assert-FileExists $Gcc "GCC"
Assert-FileExists $Make "MinGW Make"
Assert-FileExists $Gcov "GCOV"

Write-Host "Project: $LabRoot"
Write-Host "Python:  $Python"
Write-Host "CMake:   $CMake"
Write-Host "GCC:     $Gcc"
Write-Host "GCOV:    $Gcov"

Invoke-Checked "Check CMake version" {
    & $CMake --version
}

Invoke-Checked "Check GCC version" {
    & $Gcc --version
}

Invoke-Checked "Check GCOV version" {
    & $Gcov --version
}

Invoke-Checked "Check GCOVR installation" {
    & $Python -m gcovr --version
}

# Verify that Python subprocess can execute GCOV.
# This detects the WinError 2 failure seen previously.



Invoke-Checked "Check Python-to-GCOV execution" {

    # Use Python's standard input instead of python -c.
    # This avoids Windows PowerShell 5.1 argument quoting issues.

    $PythonCode = @'
import subprocess
import sys

gcov = sys.argv[1]

subprocess.run(
    [gcov, "--version"],
    check=True
)

print("Python can launch GCOV successfully")
'@

    $PythonCode | & $Python - $Gcov
}


# Configure a clean or existing coverage build directory.
Invoke-Checked "Configure native coverage build" {
    & $CMake `
        -S $SourceDir `
        -B $BuildDir `
        -G "MinGW Makefiles" `
        "-DCMAKE_C_COMPILER=$Gcc" `
        "-DCMAKE_MAKE_PROGRAM=$Make" `
        -DENABLE_COVERAGE=ON `
        -DCMAKE_BUILD_TYPE=Debug
}

# Ensure report output directory exists.
New-Item `
    -ItemType Directory `
    -Path $ReportDir `
    -Force | Out-Null

# ------------------------------------------------------------
# 7. Build native C tests
# ------------------------------------------------------------

Invoke-Checked "Build native tests" {
    & $CMake --build $BuildDir
}

# ------------------------------------------------------------
# 8. Remove previous execution coverage data
# ------------------------------------------------------------

Write-Step "Clear previous coverage counters"

Get-ChildItem `
    -Path $BuildDir `
    -Recurse `
    -Filter "*.gcda" `
    -File `
    -ErrorAction SilentlyContinue |
Remove-Item -Force

# ------------------------------------------------------------
# 9. Run native unit tests
# ------------------------------------------------------------

Invoke-Checked "Run native tests" {
    & $CTest `
        --test-dir $BuildDir `
        --output-on-failure
}

# ------------------------------------------------------------
# 10. Generate production-only HTML coverage report
# ------------------------------------------------------------

$HtmlReport = Join-Path $ReportDir "index.html"

# GCOVR is invoked with the same GCC toolchain
# that compiled the native tests.

Invoke-Checked "Generate HTML coverage report" {
    & $Python -m gcovr `
        --root $ProductionDir `
        --filter ".*spi_bus\.c$" `
        --filter ".*bmp280_spi\.c$" `
        --exclude ".*tests/.*" `
        --object-directory $BuildDir `
        --gcov-executable gcov `
        --print-summary `
        --html-details $HtmlReport
}

# ------------------------------------------------------------
# 11. Enforce minimum coverage thresholds
# ------------------------------------------------------------

Invoke-Checked "Enforce production coverage gates" {
    & $Python -m gcovr `
        --root $ProductionDir `
        --filter ".*spi_bus\.c$" `
        --filter ".*bmp280_spi\.c$" `
        --exclude ".*tests/.*" `
        --object-directory $BuildDir `
        --gcov-executable gcov `
        --fail-under-line $MinLine `
        --fail-under-branch $MinBranch `
        --print-summary
}

# ------------------------------------------------------------
# 12. Successful completion
# ------------------------------------------------------------

Write-Host ""
Write-Host "==================================================" `
    -ForegroundColor Green
Write-Host "PASS: SPI Lab 03 coverage qualification completed" `
    -ForegroundColor Green
Write-Host "==================================================" `
    -ForegroundColor Green

Write-Host ""
Write-Host "Native tests: PASSED"
Write-Host "Minimum line coverage: $MinLine%"
Write-Host "Minimum branch coverage: $MinBranch%"
Write-Host "HTML report: $HtmlReport"
