﻿# Party Music Player [![license][license-badge]][LICENSE]

Copyright (C) 2011-2021  Kevin André

Party Music Player, abbreviated as PMP, is a multi-user client-server music system. The server is responsible for playing music, and a separate program, a 'remote', is used to connect to the server and instruct it what to do. PMP has an advanced file tracking mechanism; it can deal with moved/renamed and duplicate files without any problems.

PMP is designed to be portable software and should be compatible with most popular operating systems in use today. Only Windows has been tested recently. Linux has been tested in the past. Others like Mac OS X, BSD... have never been tested but should work as well.

*This is alpha quality software*. Existing features can still be incomplete. Certain 'essential' features that can be found in most music player software might (still) be missing. PMP might contain lots of bugs. Use at your own risk.

## Table of Contents
Contents of this file:
  1. [Features](#1-features)
  2. [Dependencies](#2-dependencies)
  3. [Running PMP](#3-running-pmp)
  4. [Building on Linux](#4-building-on-linux)
  5. [Building on Windows](#5-building-on-windows)
  6. [Caveats / limitations](#6-caveats--limitations)
  7. [Planned features](#7-planned-features--to-dos)
  8. [More information](#8-more-information)


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


## 2. Dependencies

### Common Dependencies

 * Qt 5 (at least 5.8)
   * modules: Core, Gui, Multimedia, Network, Sql, Test, Widgets, Xml
   * database driver for MySQL/MariaDB
 * TagLib 1.11 or higher

### Build-time Dependencies

 * C++ compiler with support for C++ 2014
 * CMake 3.1 or higher
 * pkg-config

When building on Windows, there is no need to install MinGW by itself. The Qt installer provides an option to install a MinGW toolchain that is compatible with the version of Qt you wish to install.

### Runtime Dependencies

 * MySQL/MariaDB client library (libmysql/libmariadb)
 * Codecs

PMP currently uses MySQL for storing its data. MariaDB is probably a valid replacement, but this has not been tested yet. The MySQL server should be configured as a developer installation (it uses less memory that way) and should be set to use Unicode (UTF-8) by default. Default storage engine should be InnoDB.

The MySQL client library that is required by the Qt database driver should be 32-bit if Qt was built for a 32-bit architecture or 64-bit if Qt was built for a 64-bit architecture.

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


## 4. Building On Linux

Building PMP on Linux and other Unix-like operating systems should be relatively simple:

1. Make sure you have installed a C++ compiler, pkg-config and CMake
2. Install development packages for MySQL, Qt 5 and TagLib using your package manager
3. Download and unpack the PMP sourcecode
4. Open a terminal, change current directory into the 'bin' directory of the PMP sourcecode
5. Run the following commands:  
  cmake ..  
  make  

These instructions might need some tweaks, as they haven't been tested (at least not recently).


## 5. Building On Windows

Building PMP on Windows takes some effort.

Please make sure to use a version of Qt that still has the sqldriver plugin for MySQL. The Qt company decided to drop the plugin from its binary distributions for Qt versions 5.12.4 and later.

These steps describe how to do an x86 (32-bit) build of PMP on Windows, with
Qt 5.12.3 and TagLib 1.11. For a 64-bit build, or for different versions of Qt and/or TagLib,
the steps need to be modified accordingly.

### 1. Download and install CMake

  → http://www.cmake.org/download/  → Win32 Installer  
  install with default options  

### 2. Download and install Qt 5

  → http://qt-project.org/downloads  → Qt Online Installer for Windows  
  run installer, select the following components to install:  
    Qt 5.12.3/MinGW 7.3.0 32-bit  
    Developer and Designer Tools/MinGW 7.3.0  
    Developer and Designer Tools/OpenSSL 1.1.1d Toolkit/OpenSSL 32-bit binaries  
  edit Windows environment variables, add the following to 'Path':  
    C:\Qt\5.12.3\mingw73_32\bin  
    C:\Qt\Tools\mingw730_32  
    C:\Qt\Tools\mingw730_32\bin  
  close and reopen all Command Prompt windows for the Path change to take effect  

### 3. Download and build taglib

  → http://taglib.github.io/  → download sourcecode (at least version 1.11)  
  unpack sourcecode  
  create 'bin' directory in taglib directory  
  run CMake (cmake-gui)  
  'where is the sourcecode': where you unpacked the sourcecode  
  'where to build the binaries': the "bin" directory you created  
  before configuring, add the following CMake variables (not including the quotes):  
    name: "CMAKE_BUILD_TYPE"; type: "STRING"; value: "Release"  
    name: "CMAKE_INSTALL_PREFIX"; type: "FILEPATH"; value: "C:/Qt/Tools/mingw730_32"  
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
  extract pkg-config.exe to C:\Qt\Tools\mingw730_32\bin  
  download file gettext-runtime_0.18.1.1-2_win32.zip  
  extract intl.dll to C:\Qt\Tools\mingw730_32\bin  
  → http://ftp.gnome.org/pub/gnome/binaries/win32/glib/2.28  
  download file glib_2.28.8-1_win32.zip  
  extract libglib-2.0-0.dll to C:\Qt\Tools\mingw730_32\bin  

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
  add variable CMAKE_PREFIX_PATH and set it to "C:\Qt\5.12.3\mingw73_32"  
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


## 6. Caveats / Limitations

Since this project is in a very early stage of development, you can expect a few things to be missing or not working correctly ;)

 * Only MP3 and FLAC files supported for now  
   → because file tracking is currently only implemented for those formats
 * Database requires MySQL (maybe MariaDB), SQLite is not an option
 * No automatic detection yet of new/modified/deleted files while the server is running, a full indexation must be started manually
 * Queue manipulation in the clients is still limited: only move/delete/insert of one entry at a time.
 * No custom filters for dynamic mode yet
 * Etc.


## 7. Planned Features / To-Do's

Only a list of ideas.  **No promises!**

Effort is a rough estimate. Priorities can still change.

| Feature                                                   | Priority | Effort       |
| --------------------------------------------------------- | -------- | ------------ |
| Artist-based track repetition avoidance                   | High     | Medium       |
| Scrobbling to Last.fm                                     | High     | Large        |
| Server to server database synchronization                 | High     | Large        |
| Group song duplicates together and treat them as one      | High     | Large        |
| Identify song duplicates which have different hash values | High     | Large        |
| Keyboard shortcuts for adjusting volume                   | Medium   | Small        |
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


## 8. More information

Rudimentary project website:  https://hyperquantum.be/pmp/

GitHub repository:  https://github.com/hyperquantum/PMP

You can contact the developer here:   hyperquantum@gmail.com

[LICENSE]: ./LICENSE
[license-badge]: https://img.shields.io/badge/License-GPLv3-blue.svg
