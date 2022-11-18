# Copyright (C) 2016-2022, Kevin Andre <hyperquantum@gmail.com>
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

# define version information
set(PMP_VERSION_MAJOR 0)
set(PMP_VERSION_MINOR 2)
set(PMP_VERSION_PATCH 0)

# define branding information
set(PMP_PRODUCT_NAME "Party Music Player")
set(PMP_ORGANIZATION_NAME "Party Music Player")
set(PMP_ORGANIZATION_DOMAIN "hyperquantum.be")
set(PMP_WEBSITE "http://hyperquantum.be/pmp")
set(PMP_BUGREPORT_LOCATION "https://github.com/hyperquantum/PMP/issues")
set(PMP_COPYRIGHT_YEARS "2014-2022")

# create derived variables

set(
    PMP_VERSION_MAJORMINORPATCH
    ${PMP_VERSION_MAJOR}.${PMP_VERSION_MINOR}.${PMP_VERSION_PATCH}
)

if(${PMP_VERSION_PATCH} EQUAL "0")
    set(PMP_VERSION_DISPLAY "${PMP_VERSION_MAJOR}.${PMP_VERSION_MINOR}")
else()
    set(PMP_VERSION_DISPLAY "${PMP_VERSION_MAJORMINORPATCH}")
endif()

set(VCS_BRANCH "")
set(VCS_REVISION_LONG "")

set(DETECT_VCS_INFO OFF)

if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
    set(DETECT_VCS_INFO ON)

    find_program(GIT_EXECUTABLE git)
    if(NOT GIT_EXECUTABLE OR GIT_EXECUTABLE-NOTFOUND)
        set(DETECT_VCS_INFO OFF)
    endif()
endif()

if(DETECT_VCS_INFO)
    message(STATUS "Getting git branch and revision")

    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE VCS_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --long --tags --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE VCS_REVISION_LONG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
else()
    message(STATUS "Source code not under version control")
endif()

message(STATUS "PMP version (display): ${PMP_VERSION_DISPLAY}")
message(STATUS "VCS branch:   ${VCS_BRANCH}")
message(STATUS "VCS revision: ${VCS_REVISION_LONG}")
