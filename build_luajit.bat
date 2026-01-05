@echo off
setlocal

for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set VSINSTALL=%%i
)

if not defined VSINSTALL (
    echo visual studio not found
    exit /b 1
)

call "%VSINSTALL%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64

cd /d "%~dp0LuaJIT-2.1.0-beta3\src"

if exist lua51.lib (
    echo luajit already built
    exit /b 0
)

echo patching msvcbuild.bat...
powershell -Command "(Get-Content msvcbuild.bat) -replace '@minilua', '@.\minilua.exe' -replace '^minilua ', '.\minilua.exe ' -replace '^buildvm ', '.\buildvm.exe ' | Set-Content msvcbuild_patched.bat"

echo building luajit...
call msvcbuild_patched.bat static

if errorlevel 1 (
    echo luajit build failed
    del msvcbuild_patched.bat 2>nul
    exit /b 1
)

del msvcbuild_patched.bat 2>nul
echo luajit build successful

endlocal
