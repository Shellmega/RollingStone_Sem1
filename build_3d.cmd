@echo off
setlocal EnableExtensions
chcp 65001 >nul
set "VSLANG=1033"
pushd "%~dp0"
set "RS_CONFIG=Release"
if /I "%~1"=="Debug" set "RS_CONFIG=Debug"
set "RS_GUI=ON"
set "RS_BUILD_DIR=build\3d_final"
if /I "%~2"=="--core" set "RS_GUI=OFF"
if /I "%~2"=="--core" set "RS_BUILD_DIR=build\3d_final_core"
call scripts3d\vs_cmake.cmd
if not defined RS_CMAKE goto missing
if not exist .deps3d\JoltPhysics\Build\CMakeLists.txt (
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts3d\bootstrap.ps1
  if errorlevel 1 goto fail
)
if /I "%RS_GUI%"=="ON" if not exist .deps3d\raylib\CMakeLists.txt (
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts3d\bootstrap.ps1
  if errorlevel 1 goto fail
)
set "RS_NINJA=%RS_VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
if not exist "%RS_NINJA%" goto visual_studio
call "%RS_VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 goto fail
"%RS_CMAKE%" -S src3d -B "%RS_BUILD_DIR%" -G Ninja -DCMAKE_MAKE_PROGRAM="%RS_NINJA%" -DCMAKE_BUILD_TYPE=%RS_CONFIG% -DRS3D_GUI=%RS_GUI%
if errorlevel 1 goto fail
"%RS_CMAKE%" --build "%RS_BUILD_DIR%" --parallel 4
if errorlevel 1 goto fail
goto success
:visual_studio
set "RS_GEN_ARG="
if not exist "%RS_BUILD_DIR%\CMakeCache.txt" if defined RS_GENERATOR set RS_GEN_ARG=-G "%RS_GENERATOR%"
"%RS_CMAKE%" -S src3d -B "%RS_BUILD_DIR%" %RS_GEN_ARG% -A x64 -DRS3D_GUI=%RS_GUI%
if errorlevel 1 goto fail
"%RS_CMAKE%" --build "%RS_BUILD_DIR%" --config %RS_CONFIG% --parallel 4 -- /nr:false
if errorlevel 1 goto fail
:success
echo Build complete. Release GUI: build\3d_final\bin\RollingStone3D.exe
popd
exit /b 0
:missing
echo ERROR: Install Visual Studio C++ desktop tools with CMake, or set CMAKE_EXE.
:fail
popd
exit /b 1
