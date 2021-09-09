:: ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: This batch script packages the PMP binaries for Windows-x64.
::
:: It will use vcpkg to install dependencies and then build and package PMP.
:: The final output is a ZIP archive containing the binaries.
::
:: PREREQUISITES:
::  - CMake installed
::  - 7-Zip installed
::  - vcpkg installed
::
:: The installation paths of CMake, 7-Zip, and vcpkg are configured below.
::
:: ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

@ECHO OFF
SETLOCAL ENABLEEXTENSIONS

:: Installation paths -- ADJUST AS NEEDED
SET CMAKE_BIN_DIR=%programfiles%\CMake\bin
SET TOOL_7Z_BIN_DIR=%programfiles%\7-Zip
SET TOOL_VCPKG_BIN_DIR=C:\src\vcpkg

:: CMake Generator
SET CMAKE_GENERATOR=Visual Studio 16 2019

ECHO(
ECHO #                                    #
ECHO #   x64-windows PMP release script   #
ECHO #                                    #
ECHO(

SET scriptdir=%~dp0
SET bin_dir=x64-windows-release-bin
SET bin_to_release_dir=src\Release
SET zipsrcfull_dir=x64-windows-archive-full
SET ziproot_dir=PMP_win64
SET full_zip=PMP_win64.zip

ECHO ---------------------- Settings ----------------------
ECHO Script directory: %scriptdir%
ECHO CMake directory:  %CMAKE_BIN_DIR%
ECHO 7-ZIP directory:  %TOOL_7Z_BIN_DIR%
ECHO vcpkg directory:  %TOOL_VCPKG_BIN_DIR%
:: ECHO ZIP preparation dir: %zipsrcfull_dir%
ECHO(
ECHO CMake generator:  %CMAKE_GENERATOR%
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
IF NOT EXIST "%TOOL_7Z_BIN_DIR%" (
    ECHO Error: TOOL_7Z_BIN_DIR not found: %TOOL_7Z_BIN_DIR%
    GOTO :EOF
)
SET tool_7z=%TOOL_7Z_BIN_DIR%\7z.exe
IF NOT EXIST "%tool_7z%" (
    ECHO Error: 7z.exe not found in directory %TOOL_7Z_BIN_DIR%
    GOTO :EOF
)
IF NOT EXIST "%TOOL_VCPKG_BIN_DIR%" (
    ECHO Error: TOOL_VCPKG_BIN_DIR not found: %TOOL_VCPKG_BIN_DIR%
    GOTO :EOF
)
IF NOT EXIST "%TOOL_VCPKG_BIN_DIR%\vcpkg.exe" (
    ECHO Error: vcpkg.exe not found in %TOOL_VCPKG_BIN_DIR%
    GOTO :EOF
)

CD "%scriptdir%"

:: cleanup from previous runs (if necessary)
IF EXIST "%full_zip%" (
    DEL /q "%full_zip%" || GOTO :EOF
)
IF EXIST "%zipsrcfull_dir%" (
    RD /q /s "%zipsrcfull_dir%" || GOTO :EOF
)

:: create build directory if first time
IF NOT EXIST "%bin_dir%" (
    ECHO Creating build directory...
    MKDIR "%bin_dir%"
)

:: install PMP dependencies using vcpkg
IF NOT EXIST "%bin_dir%\ran_vcpkg_already" (
    ECHO Running vcpkg to install PMP dependencies...
    
    CD "%TOOL_VCPKG_BIN_DIR%"
    vcpkg install taglib --triplet x64-windows || GOTO :EOF
    vcpkg install qt5-base[mysqlplugin] --triplet x64-windows || GOTO :EOF
    vcpkg install qt5[essentials] --triplet x64-windows || GOTO :EOF
    
    ECHO(
    
    CD "%scriptdir%"
    CD "%bin_dir%"
    ECHO Hi there. Delete this file if you want to re-run vcpkg. >ran_vcpkg_already
)

CD "%scriptdir%"

:: run CMake if it's the first time
IF NOT EXIST "%bin_dir%\ran_cmake_already" (
    ECHO Running CMake...
    CD "%bin_dir%"
    
    "%CMAKE_BIN_DIR%"\cmake.exe ^
        -G "%CMAKE_GENERATOR%" ^
        -D "VCPKG_TARGET_TRIPLET:STRING=x64-windows" ^
        -D "CMAKE_TOOLCHAIN_FILE:FILEPATH=%TOOL_VCPKG_BIN_DIR%\scripts\buildsystems\vcpkg.cmake" ^
        -D "CMAKE_BUILD_TYPE:STRING=Release" .. || GOTO :EOF
    
    ECHO(
    
    ECHO Hi there. Delete this file if you want to re-run CMake. >ran_cmake_already
)

:: run build
ECHO Building...
CD "%scriptdir%"
CD "%bin_dir%"
"%CMAKE_BIN_DIR%\cmake.exe" ^
    --build . ^
    --config Release || GOTO :EOF
ECHO(

:: copy files to directory structure for creating the ZIP archive
ECHO Copying files...
CD "%scriptdir%"
SET dist_dir=%scriptdir%\%zipsrcfull_dir%\%ziproot_dir%
robocopy "%scriptdir%\%bin_dir%\%bin_to_release_dir%" "%dist_dir%" /s >NUL
DEL /q "%dist_dir%\quicktest.exe" || GOTO :EOF
ECHO(

:: create ZIP archive
ECHO Creating ZIP file...
CD "%scriptdir%\%zipsrcfull_dir%"
"%TOOL_7Z_BIN_DIR%"\7z.exe a -tzip "%full_zip%" %ziproot_dir%"
CD "%scriptdir%"
MOVE "%zipsrcfull_dir%\%full_zip%" . >NUL
RD /q /s "%zipsrcfull_dir%"
ECHO(

IF EXIST "%full_zip%" (
    ECHO Created %full_zip%
    ECHO Finished successfully.
    ECHO(
)
