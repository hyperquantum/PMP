Party Music Player
==================

Copyright (C) 2011-2014  Kevin Andre


This is the start of a project that will implement a client-server music
system.

License is GPLv3.



DEPENDENCIES FOR BUILDING
-------------------------

* CMake
* pkg-config
* TagLib
* Qt 5



COMPILING ON WINDOWS
--------------------

* Download and install CMake
   --> http://www.cmake.org/cmake/resources/software.html  --> Win32 Installer
* Download and install MinGW
   --> http://www.mingw.org/  --> Download Installer
   install these parts: mingw-developer-toolkit, mingw32-base, mingw32-gcc-g++, msys-base
   add MinGW to Windows User PATH  (see MinGW FAQ)
* Download and install Qt 5
   --> http://qt-project.org/downloads  --> Qt Online Installer for Windows
* Download and build taglib
   --> http://taglib.github.io/  --> download sourcecode
    unpack sourcecode
    edit CmakeLists.txt and change "if(NOT WIN32 AND NOT BUILD_FRAMEWORK)" into "if(NOT BUILD_FRAMEWORK)"
    create 'bin' directory in taglib directory
    run CMake (cmake-gui)
    'where is the sourcecode': where you unpacked the sourcecode
    'where to build the binaries': the "bin" directory you created
    set variable CMAKE_BUILD_TYPE to Release
    change variable CMAKE_INSTALL_PREFIX to C:/MinGW
    press 'Configure', select a generator with "MinGW Makefiles"
    press 'Generate'
    run the makefile with 'make':
      open cmd.exe
      cd into the "bin" directory
      run the command: mingw32-make.exe
    install:
      open cmd.exe with Administrative privileges
      cd into the "bin" directory
      run the command: mingw32-make.exe install
      copy the content of C:\taglib\lib into C:\MinGW\lib
* Get pkg-config
    --> http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/
    download file pkg-config_0.26-1_win32.zip
    extract pkg-config.exe to C:\MinGW\bin
    download file gettext-runtime_0.18.1.1-2_win32.zip
    extract intl.dll to C:\MinGW\bin
    --> http://ftp.gnome.org/pub/gnome/binaries/win32/glib/2.28
    download file glib_2.28.8-1_win32.zip
    extract libglib-2.0-0.dll to C:\MinGW\bin
* Build PMP
    Run CMake (cmake-gui)
    'where is the sourcecode': select PMP sourcecode folder
    'where to build the binaries': "bin" subdirectory of sourcecode folder
    add variable CMAKE_PREFIX_PATH and set it to the Qt install dir (e.g. C:/Qt/5.2.1/mingw48_32)
    copy the following files from C:\Qt\5.2.1\mingw48_32\bin  to the PMP bin directory
       QtCore.dll, icudt51.dll, icuin51.dll, icuuc51.dll, libstdc++-6.dll, libwinpthread-1.dll
    press 'Configure', select a generator with "MinGW Makefiles"
    press 'Generate'



