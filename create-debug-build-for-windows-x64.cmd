:: ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: This batch script builds PMP in debug mode for Windows-x64.
::
:: It will use vcpkg to build PMP's dependencies and windeployqt to copy all
:: runtime dependencies to the build directory.
::
:: PREREQUISITES:
::  - Visual Studio installed
::  - CMake installed
::  - vcpkg installed
::
:: The installation paths of CMake and vcpkg are configured below.
:: Adjust the CMAKE_GENERATOR variable to match your version of Visual Studio.
::
:: ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

@ECHO OFF
SETLOCAL ENABLEEXTENSIONS

:: Installation paths -- ADJUST AS NEEDED
SET CMAKE_BIN_DIR=%programfiles%\CMake\bin
SET TOOL_VCPKG_DIR=C:\src\vcpkg

:: CMake Generator
SET CMAKE_GENERATOR=Visual Studio 17 2022

ECHO(
ECHO #                                        #
ECHO #   x64-windows PMP debug build script   #
ECHO #                                        #
ECHO(

:: determine absolute paths of this script and the PMP sources
SET scriptdir=%~dp0
CD %scriptdir%
SET pmpsrcdir=%cd%

:: relative paths and file names
SET bin_dir_from_src=x64-windows-debug-bin
SET exe_dir_from_bin_dir=src\Debug
SET libraries_bin_dir_from_vcpkg_dir=installed\x64-windows\debug\bin
SET windeployqt_exe_from_vcpkg_dir=installed\x64-windows\tools\Qt6\bin\windeployqt.debug.bat

ECHO ---------------------- Settings ----------------------
ECHO  PMP root directory:  %pmpsrcdir%
ECHO  CMake directory:     %CMAKE_BIN_DIR%
ECHO  vcpkg directory:     %TOOL_VCPKG_DIR%
ECHO(
ECHO  CMake generator:     %CMAKE_GENERATOR%
ECHO(
ECHO  PMP build directory: %pmpsrcdir%\%bin_dir_from_src%
ECHO ------------------------------------------------------
ECHO(

:: check if everything we need is present
IF NOT EXIST "%CMAKE_BIN_DIR%" (
    ECHO Error: CMAKE_BIN_DIR not found: %CMAKE_BIN_DIR%
    GOTO :EOF
)
IF NOT EXIST "%CMAKE_BIN_DIR%\cmake.exe" (
    ECHO Error: cmake.exe not found in %CMAKE_BIN_DIR%
    GOTO :EOF
)
IF NOT EXIST "%TOOL_VCPKG_DIR%" (
    ECHO Error: TOOL_VCPKG_DIR not found: %TOOL_VCPKG_DIR%
    GOTO :EOF
)
IF NOT EXIST "%TOOL_VCPKG_DIR%\vcpkg.exe" (
    ECHO Error: vcpkg.exe not found in %TOOL_VCPKG_DIR%
    GOTO :EOF
)

CD "%pmpsrcdir%"

:: create build directory if first time
IF NOT EXIST "%bin_dir_from_src%" (
    ECHO Creating build directory %bin_dir_from_src%
    MKDIR "%bin_dir_from_src%"
)

:: install PMP dependencies using vcpkg
IF NOT EXIST "%bin_dir_from_src%\ran_vcpkg_already" (
    ECHO Running vcpkg to install PMP dependencies...

    CD "%TOOL_VCPKG_DIR%"
    vcpkg install taglib --triplet x64-windows || GOTO :EOF
    vcpkg install qtbase[sql-mysql] --triplet x64-windows || GOTO :EOF
    vcpkg install qtdoc --triplet x64-windows || GOTO :EOF
    vcpkg install qttools --triplet x64-windows || GOTO :EOF
    vcpkg install qtmultimedia --triplet x64-windows || GOTO :EOF
    vcpkg install qtsvg --triplet x64-windows || GOTO :EOF
    vcpkg install qtimageformats --triplet x64-windows || GOTO :EOF

    ECHO(

    CD "%pmpsrcdir%"
    CD "%bin_dir_from_src%"
    ECHO Hi there. Delete this file if you want to re-run vcpkg. >ran_vcpkg_already
)

CD "%pmpsrcdir%"

:: run CMake if it's the first time
IF NOT EXIST "%bin_dir_from_src%\ran_cmake_already" (
    ECHO Running CMake...
    CD "%bin_dir_from_src%"

    "%CMAKE_BIN_DIR%\cmake.exe" ^
        -G "%CMAKE_GENERATOR%" ^
        -D "VCPKG_TARGET_TRIPLET:STRING=x64-windows" ^
        -D "CMAKE_TOOLCHAIN_FILE:FILEPATH=%TOOL_VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake" ^
        -D "CMAKE_BUILD_TYPE:STRING=Debug" .. || GOTO :EOF

    ECHO(

    ECHO Hi there. Delete this file if you want to re-run CMake. >ran_cmake_already
)

:: run build
ECHO Building...
CD "%pmpsrcdir%\%bin_dir_from_src%"
"%CMAKE_BIN_DIR%\cmake.exe" ^
    --build . ^
    --config Debug ^
    -j 4 || GOTO :EOF
ECHO(

:: copy files not covered by windeployqt
ECHO Copying extra files...
CD "%TOOL_VCPKG_DIR%\%libraries_bin_dir_from_vcpkg_dir%"
COPY libmysql* "%pmpsrcdir%\%bin_dir_from_src%\%exe_dir_from_bin_dir%" >NUL
COPY libssl* "%pmpsrcdir%\%bin_dir_from_src%\%exe_dir_from_bin_dir%" >NUL
ECHO(

:: run windeployqt so we get all runtime dependencies
ECHO Running windeployqt...
"%TOOL_VCPKG_DIR%\%windeployqt_exe_from_vcpkg_dir%" --debug --pdb --no-translations ^
    "%pmpsrcdir%\%bin_dir_from_src%\%exe_dir_from_bin_dir%\PMP-HashTool.exe" ^
    "%pmpsrcdir%\%bin_dir_from_src%\%exe_dir_from_bin_dir%\PMP-Cmd-Remote.exe" ^
    "%pmpsrcdir%\%bin_dir_from_src%\%exe_dir_from_bin_dir%\PMP-Desktop-Remote.exe" ^
    "%pmpsrcdir%\%bin_dir_from_src%\%exe_dir_from_bin_dir%\PMP-Server.exe" ^
    "%pmpsrcdir%\%bin_dir_from_src%\%exe_dir_from_bin_dir%\quicktest.exe"
ECHO(

ECHO(
ECHO Script completed.
ECHO(
