# Copyright (C) 2016-2020, Kevin Andre <hyperquantum@gmail.com>
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

find_package(Qt5Test REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/src)

configure_file(${CMAKE_SOURCE_DIR}/src/common/version.h.in common/version.h)

add_executable(PMP_hash_test
    hash_test.cpp
    ${CMAKE_SOURCE_DIR}/src/common/audiodata.cpp
    ${CMAKE_SOURCE_DIR}/src/common/fileanalyzer.cpp
    ${CMAKE_SOURCE_DIR}/src/common/filehash.cpp
    ${CMAKE_SOURCE_DIR}/src/common/tagdata.cpp
)
target_link_libraries(PMP_hash_test Qt5::Core)
target_link_libraries(PMP_hash_test ${TAGLIB_LIBRARIES})

set(HASHTEST_PATH ${CMAKE_SOURCE_DIR}/testing)

macro(add_hashtest SHORTFILENAME EXPECTEDRESULTARG)
    add_test(
        hash_${SHORTFILENAME}
        ${EXECUTABLE_OUTPUT_PATH}/PMP_hash_test
          ${HASHTEST_PATH}/${SHORTFILENAME} ${EXPECTEDRESULTARG}
    )
endmacro(add_hashtest)

add_hashtest(
    noise.mp3
    33451-e853396efba4e04a142d1810e4b8ff2a-abcf758c505063fd616439c36b94c871676a58fd
)

add_hashtest(
    tone.mp3
    12512-be0ab48d7f019063bd32647b241b32b9-212874cdd1c5719afa3a4a66b75f250f7ca10dbc
)

add_hashtest(
    noise.flac
    230536-0026c468c4fc1c84ac5cb17cbe504fe0-d0d0224a2259f4ef0cb6bf50903b7dd1e60c7a90
)

add_hashtest(
    tone.flac
    50070-fc8a0825f871a9fd235c2960bd92679e-0f6da8f31d5af63893d087dc1595716400fb0d02
)


# TestUtil
qt5_wrap_cpp(PMP_TestUtil_MOCS test_util.h)
add_executable(test_util test_util.cpp
    ${PMP_TestUtil_MOCS}
    ${CMAKE_SOURCE_DIR}/src/common/util.cpp
)
target_link_libraries(test_util Qt5::Core Qt5::Test)
add_test(test_util test_util)


# TestNetworkUtil
qt5_wrap_cpp(PMP_TestNetworkUtil_MOCS test_networkutil.h)
add_executable(test_networkutil test_networkutil.cpp
    ${PMP_TestNetworkUtil_MOCS}
    ${CMAKE_SOURCE_DIR}/src/common/networkutil.cpp
)
target_link_libraries(test_networkutil Qt5::Core Qt5::Test)
add_test(test_networkutil test_networkutil)


# TestNetworkProtocol
qt5_wrap_cpp(PMP_TestNetworkProtocol_MOCS test_networkprotocol.h)
add_executable(test_networkprotocol test_networkprotocol.cpp
    ${PMP_TestNetworkProtocol_MOCS}
    ${CMAKE_SOURCE_DIR}/src/common/networkprotocol.cpp
    ${CMAKE_SOURCE_DIR}/src/common/networkutil.cpp
    ${CMAKE_SOURCE_DIR}/src/common/filehash.cpp
    ${CMAKE_SOURCE_DIR}/src/common/util.cpp
)
target_link_libraries(test_networkprotocol Qt5::Core Qt5::Test)
add_test(test_networkprotocol test_networkprotocol)
