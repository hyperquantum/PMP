:: ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: This script (re)generates the .ico file for the application icon.
::
:: PREREQUISITES:
::  - Inkscape installed
::  - ImageMagick installed and added to PATH
::
:: The installation paths of Inkscape is configured below.
::
:: ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

@ECHO OFF
SETLOCAL ENABLEEXTENSIONS

:: Installation paths -- ADJUST AS NEEDED
SET INKSCAPE_EXE=%programfiles%\Inkscape\bin\inkscape.exe

ECHO(
ECHO - Regenerating application icon from the original SVG -
ECHO(

SET scriptdir=%~dp0
SET app_icon_svg=app-icon.svg
SET app_icon_ico=app-icon.ico
SET png_output_dir=temp-output

:: check if everything we need is present
IF NOT EXIST "%scriptdir%\%app_icon_svg%" (
    ECHO Error: application icon SVG not found in the expected location
    GOTO :EOF
)
IF NOT EXIST "%INKSCAPE_EXE%" (
    ECHO Error: Inkscape executable not found in the expected location
    GOTO :EOF
)
magick --version >NUL || (
    ECHO Error: ImageMagick 'magick' command not available in PATH
)

CD "%scriptdir%"

:: clean up intermediate output from previous runs
IF EXIST "%png_output_dir%" (
    RD /q /s "%png_output_dir%" || GOTO :EOF
)

MKDIR "%png_output_dir%"

:: generate PNGs using Inkscape command-line
ECHO Generating PNGs...
"%INKSCAPE_EXE%" -w 16 -h 16 -o "%png_output_dir%/16.png" "%app_icon_svg%"
"%INKSCAPE_EXE%" -w 20 -h 20 -o "%png_output_dir%/20.png" "%app_icon_svg%"
"%INKSCAPE_EXE%" -w 24 -h 24 -o "%png_output_dir%/24.png" "%app_icon_svg%"
"%INKSCAPE_EXE%" -w 32 -h 32 -o "%png_output_dir%/32.png" "%app_icon_svg%"
"%INKSCAPE_EXE%" -w 40 -h 40 -o "%png_output_dir%/40.png" "%app_icon_svg%"
"%INKSCAPE_EXE%" -w 48 -h 48 -o "%png_output_dir%/48.png" "%app_icon_svg%"
"%INKSCAPE_EXE%" -w 64 -h 64 -o "%png_output_dir%/64.png" "%app_icon_svg%"
"%INKSCAPE_EXE%" -w 256 -h 256 -o "%png_output_dir%/256.png" "%app_icon_svg%"

:: generate icon file from the PNGs
ECHO Generating ICO...
magick ^
    "%png_output_dir%/16.png" ^
    "%png_output_dir%/20.png" ^
    "%png_output_dir%/24.png" ^
    "%png_output_dir%/32.png" ^
    "%png_output_dir%/40.png" ^
    "%png_output_dir%/48.png" ^
    "%png_output_dir%/64.png" ^
    "%png_output_dir%/256.png" ^
    "%app_icon_ico%"

:: delete intermediate files
::RD /q /s "%png_output_dir%"

ECHO(
ECHO Script finished.
