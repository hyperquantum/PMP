                 ======================
                 = Party Music Player =
                 ======================

Copyright (C) 2011-2014  Kevin André

  !! This project is in a very early stage of development !!

Party Music Player, abbreviated as PMP, is a client-server music system.
The server is responsible for playing music, and a separate program,
a 'remote', is used to connect to the server and instruct it what to do.

The software is licenced under GPLv3.

Rudimentary project website:  http://hyperquantum.be/pmp/

GitHub repository:  https://github.com/hyperquantum/PMP

You can contact the developer here:   hyperquantum@gmail.com


Contents of this file:
    1. Dependencies for running PMP
    2. Running PMP
    3. Dependencies for building
    4. Building on Windows
    5. Planned features


1. DEPENDENCIES FOR RUNNING PMP
-------------------------------

You need to run a MySQL server instance.
PMP will use MySQL to store its data.

Linux users can install MySQL server using their distribution's package manager.

Windows users can get an installer here:
  http://dev.mysql.com/downloads/mysql/

One can pick the 32-bit or 64-bit version of MySQL server, PMP should work with
both.

MySQL should be configured as a developer installation (it uses less memory that
way) and should be set to use Unicode (UTF-8) by default. Default storage engine
should be InnoDB.


2. RUNNING PMP
--------------

The PMP server needs a database connection. When you run it for the first time,
it will generate an empty configuration file. Then you can set the database
connection parameters in the configuration file. On Windows the file can be
found here:

  C:\Users\username\AppData\Roaming\Party Music Player\Party Music Player - Server.ini

An example configuration:

  [database]
  hostname=localhost
  username=root
  password=mysecretpassword


3. DEPENDENCIES FOR BUILDING
----------------------------

* CMake
* pkg-config
* TagLib
* Qt 5      (developed with Qt 5.2)
* MySQL client library
* MinGW-32  if building on Windows

The MySQL client library should match the word-size of PMP.  So if PMP is built
for x86 architecture (32-bit), then the 32-bit version of MySQL client library
should be used.


4. BUILDING ON WINDOWS
----------------------

Building on Linux is trivial, but on Windows it takes some effort.

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
    open cmd.exe in the 'bin' directory of PMP and run "mingw32-make"
* Get the MySQL client library
    --> http://dev.mysql.com/downloads/connector/c/
    the mysql version number does not matter, just pick the latest version
    download 'Windows (x86, 32-bit), ZIP Archive'
    extract 'lib/libmysql.dll' to the PMP 'bin' directory
    OR alternatively:
      if your MySQL server is 32-bit, then you can copy the DLL from that installation


5. PLANNED FEATURES
-------------------

No guarantees here!  This is more like a TO-DO list.

 * Button in remote to manually add more tracks from dynamic mode
 * 'Trim queue' button in remote
 * Differentiate between different users by having them authenticate themselves
 * Public mode / personal mode
 * Separate play history for each user and for public mode
 * Last.fm scrobbling (in personal mode)
 * Syncronization of music databases across different machines
 * Bridging of PMP server instances, to access music from another machine
 * Naming a PMP Server instance; e.g. "living room"
 * Support for other formats than just MP3
 * Support for other database providers than MySQL

