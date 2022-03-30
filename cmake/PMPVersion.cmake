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
set(PMP_VERSION_MINOR 0)
set(PMP_VERSION_PATCH 7)

# define branding information
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

message(STATUS "PMP version (display): ${PMP_VERSION_DISPLAY}")
