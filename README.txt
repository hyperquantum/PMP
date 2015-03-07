                 ======================
                 = Party Music Player =
                 ======================

Copyright (C) 2011-2015  Kevin André

  !! This project is in a very early stage of development !!

Party Music Player, abbreviated as PMP, is a client-server music system.
The server is responsible for playing music, and a separate program,
a 'remote', is used to connect to the server and instruct it what to do.
PMP uses hashes to keep track of music files, so it can deal with moved/renamed
and duplicate files without any problems.

PMP is portable software and should be compatible with most popular operating
systems in use today. I have only tested Windows and Linux, but others like
Mac OS X, BSD... should be supported as well.

The software is licenced under GPLv3.

Rudimentary project website:  http://hyperquantum.be/pmp/

GitHub repository:  https://github.com/hyperquantum/PMP

You can contact the developer here:   hyperquantum@gmail.com


Contents of this file:
    1. Features
    2. Dependencies for running PMP
    3. Running PMP
    4. Dependencies for building
    5. Building on Windows
    6. Caveats / limitations
    7. Planned features


1. FEATURES
-----------

 * Server executable
 * Command-line remote  (very primitive)
 * Remote with graphical user-interface
 * Use of hashing to identify tracks
 * Dynamic mode with track repetition avoidance


2. DEPENDENCIES FOR RUNNING PMP
-------------------------------

You need to run a MySQL server instance. PMP currently uses MySQL to store its
data. It may be possible to use MariaDB instead of MySQL, but I have not
verified if that really works.

Linux users can install MySQL server using their distribution's package manager.

Windows users can get an installer here:
  http://dev.mysql.com/downloads/mysql/

One can pick the 32-bit or 64-bit version of MySQL server, PMP should work with
both.

MySQL should be configured as a developer installation (it uses less memory that
way) and should be set to use Unicode (UTF-8) by default. Default storage engine
should be InnoDB.


3. RUNNING PMP
--------------

The PMP server needs a database connection. When you run it for the first time,
it will generate an empty configuration file. Then you can set the database
connection parameters in the configuration file. On Windows the file can be
found here:

  C:\Users\username\AppData\Roaming\Party Music Player\Party Music Player - Server.ini

You also need to tell PMP where to look for music files. This is done in the
configuration file as well.

An example configuration:

  [database]
  hostname=localhost
  username=root
  password=mysecretpassword
  
  [media]
  scan_directories=C:/Users/myname/Music, C:/Users/Public/Music


4. DEPENDENCIES FOR BUILDING
----------------------------

* CMake
* pkg-config
* TagLib    (I have version 1.9.1)
* Qt 5      (I have Qt 5.2)
* MySQL client library
* MinGW-32  if building on Windows

The MySQL client library should match the word-size of PMP.  So if PMP is built
for the x86 architecture (32-bit), then the 32-bit version of MySQL client
library should be used.


5. BUILDING ON WINDOWS
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

Disclaimer: these steps were written during a long process of trial and error,
so they can possibly contain some mistakes.


6. CAVEATS / LIMITATIONS
------------------------

Since this project is in a very early stage of development, you can expect a few
things to be missing or not working correctly ;)

 * Only MP3 files supported for now
    --> because hashing is only implemented for MP3 files at this time
 * Database requires MySQL (maybe MariaDB), SQLite is not an option
 * No manual selection of tracks yet, only dynamic mode
 * No scanning yet for new/modified files while the server is running, only a
   simple indexation is performed when the server is started
 * Only limited queue manipulation supported: deleting yes, moving no
 * Playback history is not saved in the database yet
 * No custom filters for dynamic mode yet
 * Etc.


7. PLANNED FEATURES / TO-DO's
-----------------------------

Only a list of ideas.  No promises!

 * Button in remote to manually add more tracks from dynamic mode
 * 'Trim queue' button in remote
 * Differentiate between different users by having them authenticate themselves
 * Public mode / personal mode
 * Separate play history for each user and for public mode
 * Last.fm scrobbling (in personal mode)
 * Syncronization of music databases across different machines
 * Handling of two files that are the same track but have a different hash
 * Search functionality with fuzzy matching option
 * Find a way to identify artists
 * Dynamic mode: avoid repeating the same artist within a certain timespan
 * Silence detection at the start and end of each track
 * Crossfading
 * Bridging of PMP server instances, to access music from another machine
 * Naming a PMP Server instance; e.g. "living room"
 * Support for other formats than just MP3
 * Manual song play to a second audio device (for headphones)
 * Support for other database providers than MySQL

