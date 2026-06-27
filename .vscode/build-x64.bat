@echo off

for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -property installationPath`) do (
    set "VSPATH=%%i"
)

if not defined VSPATH (
    echo ERROR: Could not find Visual Studio installation
    exit /b 1
)

call "%VSPATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64

cmake --build build
