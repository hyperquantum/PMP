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
SET TOOL_VCPKG_DIR=C:\src\vcpkg

:: CMake Generator
SET CMAKE_GENERATOR=Visual Studio 17 2022

ECHO(
ECHO #                                    #
ECHO #   x64-windows PMP release script   #
ECHO #                                    #
ECHO(

:: determine absolute paths of this script and the PMP sources
SET scriptdir=%~dp0
CD %scriptdir%
CD ..
SET pmpsrcdir=%cd%

:: relative paths and filenames
SET bin_dir_from_src=x64-windows-release-bin
SET exe_dir_from_bin_dir=src\Release
SET libraries_bin_dir_from_vcpkg_dir=installed\x64-windows\bin
SET windeployqt_exe_from_vcpkg_dir=installed\x64-windows\tools\Qt6\bin\windeployqt6.exe
SET zip_staging_dir=x64-windows-archive-staging
SET zip_root_dir=PMP-win64
SET zip_file_name=PMP-win64.zip

ECHO ---------------------- Settings ----------------------
ECHO PMP root directory: %pmpsrcdir%
ECHO CMake directory:    %CMAKE_BIN_DIR%
ECHO vcpkg directory:    %TOOL_VCPKG_DIR%
ECHO 7-ZIP directory:    %TOOL_7Z_BIN_DIR%
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
IF NOT EXIST "%TOOL_VCPKG_DIR%" (
    ECHO Error: TOOL_VCPKG_DIR not found: %TOOL_VCPKG_DIR%
    GOTO :EOF
)
IF NOT EXIST "%TOOL_VCPKG_DIR%\vcpkg.exe" (
    ECHO Error: vcpkg.exe not found in %TOOL_VCPKG_DIR%
    GOTO :EOF
)

CD "%pmpsrcdir%"

:: cleanup from previous runs (if necessary)
IF EXIST "%zip_file_name%" (
    DEL /q "%zip_file_name%" || GOTO :EOF
)
IF EXIST "%zip_staging_dir%" (
    RD /q /s "%zip_staging_dir%" || GOTO :EOF
)

:: create build directory if first time
IF NOT EXIST "%bin_dir_from_src%" (
    ECHO Creating build directory...
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
        -D "CMAKE_BUILD_TYPE:STRING=Release" .. || GOTO :EOF

    ECHO(

    ECHO Hi there. Delete this file if you want to re-run CMake. >ran_cmake_already
)

:: run build
ECHO Building...
CD "%pmpsrcdir%\%bin_dir_from_src%"
"%CMAKE_BIN_DIR%\cmake.exe" ^
    --build . ^
    --config Release ^
    -j 4 || GOTO :EOF
ECHO(

:: create staging area
MKDIR "%pmpsrcdir%\%zip_staging_dir%"
MKDIR "%pmpsrcdir%\%zip_staging_dir%\%zip_root_dir%"
SET staging_root_dir=%pmpsrcdir%\%zip_staging_dir%\%zip_root_dir%

:: copy files to directory structure for creating the ZIP archive
ECHO Copying files from build directory...
CD "%pmpsrcdir%"
robocopy "%pmpsrcdir%\%bin_dir_from_src%\%exe_dir_from_bin_dir%" "%staging_root_dir%" /s >NUL
DEL /q "%staging_root_dir%\quicktest.exe" || GOTO :EOF
ECHO(

:: copy extra files not found in the build directory
ECHO Copying extra files...
CD "%TOOL_VCPKG_DIR%\%libraries_bin_dir_from_vcpkg_dir%"
COPY libmysql* "%staging_root_dir%" >NUL
COPY libssl* "%staging_root_dir%" >NUL
CD "%pmpsrcdir%"
:: copy README* "%staging_root_dir%" >NUL
COPY *LICENSE* "%staging_root_dir%" >NUL
ECHO(

:: run windeployqt so we get all runtime dependencies
ECHO Running windeployqt...
CD "%staging_root_dir%"
"%TOOL_VCPKG_DIR%\%windeployqt_exe_from_vcpkg_dir%" --release --no-translations ^
    PMP-HashTool.exe ^
    PMP-Cmd-Remote.exe ^
    PMP-Desktop-Remote.exe ^
    PMP-Server.exe
ECHO(

:: create ZIP archive
ECHO Creating ZIP file...
CD "%pmpsrcdir%\%zip_staging_dir%"
"%TOOL_7Z_BIN_DIR%\7z.exe" a -tzip "%zip_file_name%" %zip_root_dir%"
CD "%pmpsrcdir%"
MOVE "%zip_staging_dir%\%zip_file_name%" . >NUL
RD /q /s "%zip_staging_dir%"
ECHO(

IF EXIST "%zip_file_name%" (
    ECHO Created %zip_file_name%
    ECHO Finished successfully.
    ECHO(
)
