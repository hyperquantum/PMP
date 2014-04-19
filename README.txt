                 ======================
                 = Party Music Player =
                 ======================

Copyright (C) 2011-2014  Kevin Andre


Party Music Player, abbreviated as PMP, is a client-server music system.
The server is responsible for playing music, and a separate program,
a 'remote', is used to connect to the server and instruct it what to do.

The software is licenced under GPLv3. Sourcecode is available upon
request, but not published on a website yet.

You can contact the developer at:   hyperquantum@gmail.com


DEPENDENCIES FOR BUILDING
-------------------------

* CMake
* pkg-config
* TagLib
* Qt 5     (developed with Qt 5.2)



COMPILING ON WINDOWS
--------------------

Compilation on Linux is trivial, but on Windows it takes some effort.

How to compile from source on Windows, using MinGW 32-bit:

* Download and install CMake
   --> http://www.cmake.org/cmake/resources/software.html  --> Win32 Installer
* Download and install MinGW
   --> http://www.mingw.org/  --> Download Installer
   install these parts: mingw-developer-toolkit, mingw32-base, mingw32-gcc-g++, msys-base
   add MinGW to Windows User PATH  (see MinGW FAQ)
* Download and install Qt 5
   --> http://qt-project.org/downloads  --> Qt Online Installer for Windows
   add the Qt MinGW bin directory to the Windows User PATH  (e.g. C:\Qt\5.2.1\mingw48_32\bin)
      make sure to put this directory TO THE LEFT of the MinGW path that is already there
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
    press 'Configure', select a generator with "MinGW Makefiles"
    press 'Generate'


PLANNED FEATURES
----------------

No guarantees here!  This is more like a TO-DO list.

 * Remote with GUI interface
 * Differentiate between different users by having them authenticate themselves
 * Last.fm scrobbling
 * Syncronization of music databases across different machines
 * Bridging of PMP server instances, to access music from another machine
 * Naming a PMP Server instance; e.g. "living room"

