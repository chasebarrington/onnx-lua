@echo off
setlocal

for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set VSINSTALL=%%i
)

if not defined VSINSTALL (
    echo visual studio not found
    exit /b 1
)

call "%VSINSTALL%\Common7\Tools\VsDevCmd.bat" -arch=x64
cd /d "%~dp0"
if exist build rmdir /s /q build
mkdir build
cd build
cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo cmake failed
    exit /b 1
)
nmake
if errorlevel 1 (
    echo build failed
    exit /b 1
)
echo build successful

endlocal
