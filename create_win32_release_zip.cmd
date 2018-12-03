:: ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: This batch script packages the binaries for Windows-x86.
::
:: PREREQUISITES:
::  - Qt5 MinGW edition installed with same version Qt MinGW toolkit
::  - Qt5 "bin" directory added to the Windows PATH variable
::  - CMake installed in Windows
::  - 7-Zip installed in Windows
::  - taglib and pkg-config installed as described in the README
::  - libmysql.dll copied into the build directory as described in the README
::
:: The installation paths of MinGW, CMake and 7-Zip are configured below.
::
:: ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

@ECHO OFF
SETLOCAL ENABLEEXTENSIONS

:: Installation paths -- ADJUST AS NEEDED
SET MINGW_BASE_DIR=C:\Qt\Tools\mingw530_32
SET MINGW_BIN_DIR="%MINGW_BASE_DIR%"\bin
SET CMAKE_BIN_DIR=%programfiles%\CMake\bin
SET TOOL_7Z_BIN_DIR=%programfiles%\7-Zip

ECHO(
ECHO #                              #
ECHO #   Win32 PMP release script   #
ECHO #                              #
ECHO(

SET scriptdir=%~dp0
SET bin_dir=win32_bin_release
SET zipsrcfull_dir=win32_archive_full
SET ziproot_dir=PMP_win32
SET full_zip=PMP_win32.zip

ECHO Scriptdir: %scriptdir%
ECHO CMake location: %CMAKE_BIN_DIR%
ECHO ZIP preparation dir: %zipsrcfull_dir%
ECHO(

:: check if everything we need is present
IF NOT EXIST "%MINGW_BIN_DIR%" (
    ECHO Error: MINGW_BIN_DIR not found: %MINGW_BIN_DIR%
    GOTO :EOF
)
IF NOT EXIST "%MINGW_BIN_DIR%\mingw32-make.exe" (
    ECHO Error: mingw32-make.exe not found in %MINGW_BIN_DIR%
    GOTO :EOF
)
IF NOT EXIST "%CMAKE_BIN_DIR%" (
    ECHO Error: CMAKE_BIN_DIR not found: %CMAKE_BIN_DIR%
    GOTO :EOF
)
IF NOT EXIST "%CMAKE_BIN_DIR%\cmake.exe" (
    ECHO Error: cmake.exe not found in %CMAKE_BIN_DIR%
    GOTO :EOF
)
where /q windeployqt || ECHO Error: windeployqt tool not in PATH && GOTO :EOF

:: cleanup from previous runs (if necessary)
IF EXIST "%full_zip%" (
    DEL /q "%full_zip%" || GOTO :EOF
)
IF EXIST "%zipsrcfull_dir%" (
    RD /q /s "%zipsrcfull_dir%" || GOTO :EOF
)

:: prepare archive content structure
MKDIR "%zipsrcfull_dir%"
MKDIR "%zipsrcfull_dir%\%ziproot_dir%"

:: create build directory if first time
IF NOT EXIST "%bin_dir%" (
    ECHO Creating build directory...
    MKDIR "%bin_dir%"
)

:: run CMake for the first time
IF NOT EXIST "%bin_dir%\Makefile" (
    IF NOT EXIST "%CMAKE_BIN_DIR%\cmake.exe" (
        ECHO Error: CMake executable not found at: %CMAKE_BIN_DIR%\cmake.exe
        GOTO :EOF
    )
    
    ECHO Running CMake...
    CD "%bin_dir%"
    "%CMAKE_BIN_DIR%"\cmake.exe ^
        -G "MinGW Makefiles" ^
        -D "CMAKE_BUILD_TYPE:STRING=Release" .. || GOTO :EOF
    
    ECHO(
    CD "%scriptdir%"
)

:: run build
ECHO Running make...
CD "%bin_dir%"
"%MINGW_BIN_DIR%"\mingw32-make.exe || GOTO :EOF
ECHO(
CD "%scriptdir%"

:: copy files to release directory

ECHO Copying files...

IF NOT EXIST "%bin_dir%\libmysql.dll" (
    ECHO Error: libmysql.dll not found in directory %BIN_DIR%
    GOTO :EOF
)

SET dist_dir=%scriptdir%\%zipsrcfull_dir%\%ziproot_dir%

copy README* "%dist_dir%" >NUL
copy TODOs* "%dist_dir%" >NUL
copy *LICENSE* "%dist_dir%" >NUL

copy "%bin_dir%\src\PMP-Server.exe" "%dist_dir%" >NUL
copy "%bin_dir%\src\PMP-GUI-Remote.exe" "%dist_dir%" >NUL
copy "%bin_dir%\src\PMP-Cmd-Remote.exe" "%dist_dir%" >NUL
copy "%bin_dir%\src\PMP-HashTool.exe" "%dist_dir%" >NUL

copy "%bin_dir%"\libmysql.dll "%dist_dir%" >NUL

copy "%MINGW_BIN_DIR%"\libtag.dll "%dist_dir%" >NUL

windeployqt --release ^
    "%dist_dir%\PMP-HashTool.exe" ^
    "%dist_dir%\PMP-Cmd-Remote.exe" ^
    "%dist_dir%\PMP-GUI-Remote.exe" ^
    "%dist_dir%\PMP-Server.exe" ^
    || ECHO Error while copying Qt dependencies && GOTO :EOF

ECHO(

:: create ZIP file

ECHO Creating ZIP file...

SET tool_7z=%TOOL_7Z_BIN_DIR%\7z.exe

IF NOT EXIST "%tool_7z%" (
    ECHO Error: 7z.exe not found in directory %TOOL_7Z_BIN_DIR%
    GOTO :EOF
)

CD "%zipsrcfull_dir%"
"%TOOL_7Z_BIN_DIR%"\7z.exe a -tzip "%full_zip%" %ziproot_dir%"
CD "%scriptdir%"
MOVE "%ZIPSRCFULL_DIR%\%FULL_ZIP%" . >NUL
RD /q /s "%zipsrcfull_dir%"

ECHO(
ECHO Created %full_zip%
