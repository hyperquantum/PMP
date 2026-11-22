# Copyright (C) 2011-2026, Kevin André <hyperquantum@gmail.com>
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

cmake_minimum_required(VERSION 3.21.0 FATAL_ERROR)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(PMP)

include(GNUInstallDirs)

option(PMP_BUILD_DEB_PACKAGE
    "Enable creation of a Debian package"
    OFF
)

set(CMAKE_INSTALL_DOCDIR
    "${CMAKE_INSTALL_DATAROOTDIR}/doc/pmp"
)

# install documentation
install(FILES
    README.md
    CHANGELOG.md
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)

enable_testing()

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(PMPVersion)

# find Qt 6
find_package(Qt6 6.2.0
    COMPONENTS Core Widgets Network Sql Xml Multimedia REQUIRED
)

# disable Qt 5 API that is removed in Qt 6
#add_compile_definitions(QT_DISABLE_DEPRECATED_UP_TO=0x050F00)

# minimum TagLib version is 1.12
find_path(TAGLIB_INCLUDE_DIRS taglib/taglib.h)
find_library(TAGLIB_LIBRARIES tag)
include_directories(${TAGLIB_INCLUDE_DIRS})
#target_link_libraries(myprogram ${TAGLIB_LIBRARIES})

# enable compiler warnings
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-long-long -pedantic")
endif()
if(MSVC)
    # /W4 triggers (too) many warnings in Qt header files
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(WIN32)
  option(ENABLE_WIN32_CONSOLE "Show the windows console even outside Debug mode" OFF)
endif(WIN32)

add_subdirectory(src)
add_subdirectory(tests)

if(PMP_BUILD_DEB_PACKAGE)

    if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
        message(FATAL_ERROR
            "Debian packages can only be generated on Linux"
        )
    endif()

    find_program(DPKG_DEB_EXECUTABLE dpkg-deb)
    find_program(DPKG_SHLIBDEPS_EXECUTABLE dpkg-shlibdeps)

    if(NOT DPKG_DEB_EXECUTABLE OR NOT DPKG_SHLIBDEPS_EXECUTABLE)
        message(FATAL_ERROR
            "Debian packaging requires dpkg-deb and dpkg-shlibdeps"
        )
    endif()

    # CPack configuration
    set(CPACK_GENERATOR "DEB")

    set(CPACK_PACKAGE_NAME "pmp")
    set(CPACK_PACKAGE_VERSION "${PMP_VERSION_MAJORMINORPATCH}")
    set(CPACK_PACKAGE_CONTACT "Kevin André <hyperquantum@gmail.com>")
    set(CPACK_PACKAGE_VENDOR "Kevin André")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
        "Multi-user client-server music player"
    )
    set(CPACK_PACKAGE_HOMEPAGE_URL
        "https://github.com/hyperquantum/PMP"
    )

    # Debian-specific settings
    set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER
        "Kevin André <hyperquantum@gmail.com>"
    )
    set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
    set(CPACK_DEBIAN_PACKAGE_RELEASE "1")

    # Automatically determine dependencies on shared libraries
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

    # Dependencies that cannot be detected automatically
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "libqt6sql6-mysql"
    )

    # Debian copyright file
    install(FILES
        "${PROJECT_SOURCE_DIR}/dist/debian/copyright"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}"
    )

    include(CPack)

endif()
