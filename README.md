﻿# Party Music Player

Copyright (C) 2011-2018  Kevin André

  *!! This project is still in an early stage of development !!*

Party Music Player, abbreviated as PMP, is a multi-user client-server music system. The server is responsible for playing music, and a separate program, a 'remote', is used to connect to the server and instruct it what to do. PMP has an advanced file tracking mechanism; it can deal with moved/renamed and duplicate files without any problems.

PMP is designed to be portable software and should be compatible with most popular operating systems in use today. Only Windows and Linux have been tested, but others like Mac OS X, BSD... should be supported as well.

The software is licenced under [GPLv3](./LICENSE.GPLv3.txt).

*This is (pre-)alpha quality software*. Important functionality is still missing. There are no official releases yet. Anyone interested in trying PMP will need to build it from source.

## Table of Contents
Contents of this file:
  1. [Features](#1-features)
  2. [Dependencies for running PMP](#2-dependencies-for-running-pmp)
  3. [Running PMP](#3-running-pmp)
  4. [Dependencies for building](#4-dependencies-for-building)
  5. [Building on Linux](#5-building-on-linux)
  6. [Building on Windows](#6-building-on-windows)
  7. [Caveats / limitations](#7-caveats--limitations)
  8. [Planned features](#8-planned-features--to-dos)
  9. [More information](#9-more-information)


## 1. Features

 * **Client-server**
   * Server executable without graphical user-interface
   * Client executable to connect to and control the server
   * Multiple clients can be connected to the same server at the same time
 * **Multi-user**
   * Creation of user accounts on the server
   * Authentication with username and password when connecting to the server
   * Each user has their own playback history, track scores, and preferences
 * **Advanced file tracking**
   * Tracks are identified by file contents, ignoring irrelevant bits like metadata
   * Known tracks are still recognized after moving or renaming files
   * Duplicate files (created by a file copy operation) are recognized as being the same track
 * **Queue-based**
   * Tracks to be played are added to a queue
   * The top entry in the queue is the next track to be played
   * Breakpoints can be added to the queue, to stop playback when they are reached
 * **Dynamic mode** with track repetition avoidance
   * Dynamic mode adds random tracks to the queue
   * Tracks with a higher score are more likely to be selected
   * Avoids adding tracks that were played recently
 * **Auto scoring** based on playback history of each track
 * Distinction between *personal* and *public* mode
   * *Public* mode allows playing music for someone else without affecting your personal track scores


## 2. Dependencies For Running PMP

  1. MySQL

     You need to run a MySQL server instance. PMP currently uses MySQL to store its data. It may be possible to use MariaDB instead of MySQL, but I have not verified if that really works.

     Linux users can install MySQL server using their distribution's package manager.

     Windows users can get an installer here:  http://dev.mysql.com/downloads/mysql/

     One can pick the 32-bit or 64-bit version of MySQL server, PMP should work with both.

     MySQL should be configured as a developer installation (it uses less memory that way) and should be set to use Unicode (UTF-8) by default. Default storage engine should be InnoDB.

  2. Codecs / Plugins

     Windows users will need to install the [Xiph codecs](https://xiph.org/dshow/downloads/) or some other more generic codec pack if they need support for FLAC audio.
     
     Linux users may need to install a GStreamer plugin to get support for MP3 files.


## 3. Running PMP

The PMP server needs a database connection. When you run it for the first time, it will generate an empty configuration file. Then you can set the database connection parameters in the configuration file. On Windows the file can be found here:

```
  C:\Users\username\AppData\Roaming\Party Music Player\Party Music Player - Server.ini
```

You also need to tell PMP where to look for music files. This is done in the configuration file as well.

An example configuration:

```
    [database]
    hostname=localhost
    username=root
    password=mysecretpassword
  
    [media]
    scan_directories=C:/Users/myname/Music, C:/Users/Public/Music
```

First make sure MySQL is running. Then start the PMP-Server executable, and finally start the PMP-GUI-Remote executable.


## 4. Dependencies For Building

 * C++ compiler with support for C++ 2011
 * CMake 3.1 or higher
 * pkg-config
 * TagLib 1.11 or higher
 * Qt 5.7 or higher
 * MySQL client library  (libmysql)

The MySQL client library should be 32-bit if PMP was built for the x86 architecture (32-bit), and should be 64-bit if PMP was built for x64 (64-bit).

When building on Windows, there is no need to install MinGW by itself. The Qt installer provides an option to install a MinGW toolchain that is compatible with the version of Qt you wish to install.


## 5. Building On Linux

Building PMP on Linux and other Unix-like operating systems should be relatively simple:

1. Make sure you have installed a C++ compiler, pkg-config and CMake
2. Install development packages for MySQL, Qt 5 and TagLib using your package manager
3. Download and unpack the PMP sourcecode
4. Open a terminal, change current directory into the 'bin' directory of the PMP sourcecode
5. Run the following commands:  
  cmake ..  
  make  

These instructions might need some tweaks, as they haven't been tested (at least not recently).


## 6. Building On Windows

Building PMP on Windows takes some effort.

These steps describe how to do an x86 (32-bit) build of PMP on Windows, with
Qt 5.7 and TagLib 1.11. For a 64-bit build, or for different versions of Qt and/or TagLib,
the steps need to be modified accordingly.

### 1. Download and install CMake

  → http://www.cmake.org/download/  → Win32 Installer  
  install with default options  

### 2. Download and install Qt 5

  → http://qt-project.org/downloads  → Qt Online Installer for Windows  
  run installer, select the following components to install:  
    Qt 5.7 MinGW 5.3.0 32 bit  
    Tools/MinGW 5.3.0  
  edit Windows environment variables, add the following to 'Path':  
    C:\Qt\5.7\mingw53_32\bin  
    C:\Qt\Tools\mingw530_32  
    C:\Qt\Tools\mingw530_32\bin  

### 3. Download and build taglib

  → http://taglib.github.io/  → download sourcecode (at least version 1.11)  
  unpack sourcecode  
  create 'bin' directory in taglib directory  
  run CMake (cmake-gui)  
  'where is the sourcecode': where you unpacked the sourcecode  
  'where to build the binaries': the "bin" directory you created  
  before configuring, add the following CMake variables (not including the quotes):  
    name: "CMAKE_BUILD_TYPE"; type: "STRING"; value: "Release"  
    name: "CMAKE_INSTALL_PREFIX"; type: "FILEPATH"; value: "C:/Qt/Tools/mingw530_32"  
    name: "BUILD_SHARED_LIBS"; type: BOOL; value: enabled (or "1")  
  press 'Configure', select a generator with "MinGW Makefiles"  
  press 'Generate'  
  build and install taglib:  
    open cmd.exe  
    cd into the "bin" directory  
    run command: mingw32-make  
    run command: mingw32-make install  

### 4. Get pkg-config

  → http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/  
  download file pkg-config_0.26-1_win32.zip  
  extract pkg-config.exe to C:\Qt\Tools\mingw530_32\bin  
  download file gettext-runtime_0.18.1.1-2_win32.zip  
  extract intl.dll to C:\Qt\Tools\mingw530_32\bin  
  → http://ftp.gnome.org/pub/gnome/binaries/win32/glib/2.28  
  download file glib_2.28.8-1_win32.zip  
  extract libglib-2.0-0.dll to C:\Qt\Tools\mingw530_32\bin  

### 5. Get the MySQL client library

  → http://dev.mysql.com/downloads/connector/c/  
  the mysql version number does not matter, just pick the latest version  
  download 'Windows (x86, 32-bit), ZIP Archive'  
  extract 'lib/libmysql.dll' to the PMP 'bin' directory  
  OR alternatively:  
    if your MySQL server is 32-bit, then you can copy the DLL from that installation

### 6. Build PMP

  Run CMake (cmake-gui)  
  'where is the sourcecode': select PMP sourcecode folder  
  'where to build the binaries': "bin" subdirectory of sourcecode folder  
  add variable CMAKE_PREFIX_PATH and set it to "C:\Qt\5.7\mingw53_32"  
  optional: add variable CMAKE_BUILD_TYPE and set it to "Debug" or "Release"  
  press 'Configure', select a generator with "MinGW Makefiles"  
  press 'Generate'  
  open cmd.exe in the 'bin' directory of PMP and run "mingw32-make"  

### 7. Running PMP

  First start the server:  
    open cmd.exe and cd into the PMP 'bin' directory  
    run command: src\PMP-Server.exe  
    this cmd window must remain open because closing it will kill the server  
  Then start the client:  
    run PMP-GUI-Remote.exe directly from Windows Explorer  
    OR alternatively, start it in a cmd window like the server  


## 7. Caveats / Limitations

Since this project is in a very early stage of development, you can expect a few things to be missing or not working correctly ;)

 * Only MP3 and FLAC files supported for now  
   → because file tracking is currently only implemented for those formats
 * Database requires MySQL (maybe MariaDB), SQLite is not an option
 * No automatic detection yet of new/modified/deleted files while the server is running, a full indexation must be started manually
 * Queue manipulation in the clients is still limited: only move/delete/insert of one entry at a time.
 * No custom filters for dynamic mode yet
 * Etc.


## 8. Planned Features / To-Do's

Only a list of ideas.  **No promises!**

Effort is a rough estimate. Priorities can still change.

| Feature                                                   | Priority | Effort       |
| --------------------------------------------------------- | -------- | ------------ |
| Save/restore dynamic mode settings for each user          | High     | Medium       |
| Artist-based track repetition avoidance                   | High     | Medium       |
| Scrobbling to Last.fm                                     | High     | Large        |
| Server to server database synchronization                 | High     | Large        |
| Group song duplicates together and treat them as one      | High     | Large        |
| Identify song duplicates which have different hash values | High     | Large        |
| Keyboard shortcuts for adjusting volume                   | Medium   | Small        |
| Queue contextmenu option to duplicate the entry           | Medium   | Small        |
| Fuzzy matching when searching the music collection        | Medium   | Medium       |
| Server to server remote music access                      | Medium   | Medium       |
| Identify artist by something better than text             | Medium   | Large        |
| Android app for controlling the server (only a remote)    | Medium   | Large        |
| Naming a server instance (e.g. "living room")             | Low      | Small        |
| Time-based auto-stop function                             | Low      | Medium       |
| Support for other database providers than MySQL           | Low      | Medium       |
| Support for other music formats than MP3 and FLAC         | Low      | Medium/Large |
| Manual playback to a second audio device (for headphones) | Low      | Medium/Large |
| Silence detection at the start and end of each track      | Low      | Large        |
| Crossfading                                               | Low      | Large        |
| ReplayGain support                                        | Low      | Large(?)     |


## 9. More information

Rudimentary project website:  http://hyperquantum.be/pmp/

GitHub repository:  https://github.com/hyperquantum/PMP

You can contact the developer here:   hyperquantum@gmail.com
