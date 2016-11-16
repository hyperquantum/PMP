# Copyright (C) 2011-2016, Kevin Andre <hyperquantum@gmail.com>
#
# This file is part of PMP (Party Music Player).
#
# PMP is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# PMP is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along
# with PMP.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(PMP)

enable_testing()

include(FindPkgConfig)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Multimedia REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Sql REQUIRED)
find_package(Qt5Widgets REQUIRED)

pkg_check_modules(TAGLIB REQUIRED taglib>=1.11)

include_directories(${TAGLIB_INCLUDE_DIRS})
link_directories(${TAGLIB_LIBRARY_DIRS})

# enable compiler warnings for GCC
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(WIN32)
  option(ENABLE_WIN32_CONSOLE "Show the windows console even outside Debug mode" OFF)
endif(WIN32)

add_subdirectory(src)
add_subdirectory(testing)
