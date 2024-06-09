# Copyright (C) 2016-2024, Kevin Andre <hyperquantum@gmail.com>
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
)
target_link_libraries(PMP_hash_test $<TARGET_OBJECTS:PmpCommon>)
target_link_libraries(PMP_hash_test Qt5::Core)
target_link_libraries(PMP_hash_test ${TAGLIB_LIBRARIES})

set(HASHTEST_PATH ${CMAKE_SOURCE_DIR}/tests)

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


# TestCommandParser
qt5_wrap_cpp(PMP_TestCommandParser_MOCS
    test_commandparser.h
)
add_executable(test_commandparser test_commandparser.cpp ${PMP_TestCommandParser_MOCS})
target_link_libraries(test_commandparser $<TARGET_OBJECTS:PmpCmdRemote>)
target_link_libraries(test_commandparser $<TARGET_OBJECTS:PmpClient>)
target_link_libraries(test_commandparser $<TARGET_OBJECTS:PmpCommon>)
target_link_libraries(test_commandparser Qt5::Core Qt5::Network Qt5::Test)
target_link_libraries(test_commandparser ${TAGLIB_LIBRARIES})
add_test(test_commandparser test_commandparser)


# TestNullable
qt5_wrap_cpp(PMP_TestNullable_MOCS test_nullable.h)
add_executable(test_nullable test_nullable.cpp ${PMP_TestNullable_MOCS})
target_link_libraries(test_nullable Qt5::Core Qt5::Test)
add_test(test_nullable test_nullable)


# TestFileHash
qt5_wrap_cpp(PMP_TestFileHash_MOCS test_filehash.h)
add_executable(test_filehash test_filehash.cpp ${PMP_TestFileHash_MOCS})
target_link_libraries(test_filehash $<TARGET_OBJECTS:PmpCommon>)
target_link_libraries(test_filehash Qt5::Core Qt5::Test)
target_link_libraries(test_filehash ${TAGLIB_LIBRARIES})
add_test(test_filehash test_filehash)


# TestTriBool
qt5_wrap_cpp(PMP_TestTriBool_MOCS test_tribool.h)
add_executable(test_tribool test_tribool.cpp
    ${PMP_TestTriBool_MOCS}
    ${CMAKE_SOURCE_DIR}/src/common/tribool.cpp
)
target_link_libraries(test_tribool Qt5::Core Qt5::Test)
add_test(test_tribool test_tribool)


# TestUtil
qt5_wrap_cpp(PMP_TestUtil_MOCS test_util.h)
add_executable(test_util test_util.cpp
    ${PMP_TestUtil_MOCS}
    ${CMAKE_SOURCE_DIR}/src/common/util.cpp
)
target_link_libraries(test_util Qt5::Core Qt5::Test)
add_test(test_util test_util)


# TestScrobbler
qt5_wrap_cpp(PMP_TestScrobbler_MOCS test_scrobbler.h
    ${CMAKE_SOURCE_DIR}/src/server/scrobbler.h
    ${CMAKE_SOURCE_DIR}/src/server/scrobblingbackend.h
    ${CMAKE_SOURCE_DIR}/src/server/serverhealthmonitor.h
)
add_executable(test_scrobbler test_scrobbler.cpp
    ${PMP_TestScrobbler_MOCS}
    ${CMAKE_SOURCE_DIR}/src/server/scrobbler.cpp
    ${CMAKE_SOURCE_DIR}/src/server/scrobblingbackend.cpp
    ${CMAKE_SOURCE_DIR}/src/server/selftest.cpp
    ${CMAKE_SOURCE_DIR}/src/server/serverhealthmonitor.cpp
)
target_link_libraries(test_scrobbler Qt5::Core Qt5::Network Qt5::Test)
add_test(test_scrobbler test_scrobbler)


# TestTokenEncoder
qt5_wrap_cpp(PMP_TestTokenEncoder_MOCS test_tokenencoder.h)
add_executable(test_tokenencoder test_tokenencoder.cpp
    ${PMP_TestTokenEncoder_MOCS}
    ${CMAKE_SOURCE_DIR}/src/server/tokenencoder.cpp
    ${CMAKE_SOURCE_DIR}/src/common/obfuscator.cpp
)
target_link_libraries(test_tokenencoder Qt5::Core Qt5::Test)
add_test(test_tokenencoder test_tokenencoder)


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
)
target_link_libraries(test_networkprotocol $<TARGET_OBJECTS:PmpCommon>)
target_link_libraries(test_networkprotocol Qt5::Core Qt5::Test)
target_link_libraries(test_networkprotocol ${TAGLIB_LIBRARIES})
add_test(test_networkprotocol test_networkprotocol)


# TestPasswordStrengthEvaluator
qt5_wrap_cpp(PMP_TestPasswordStrengthEvaluator_MOCS test_passwordstrengthevaluator.h)
add_executable(test_passwordstrengthevaluator test_passwordstrengthevaluator.cpp
    ${PMP_TestPasswordStrengthEvaluator_MOCS}
    ${CMAKE_SOURCE_DIR}/src/common/passwordstrengthevaluator.cpp
)
target_link_libraries(test_passwordstrengthevaluator Qt5::Core Qt5::Test)
add_test(test_passwordstrengthevaluator test_passwordstrengthevaluator)


# TestObfuscator
qt5_wrap_cpp(PMP_TestObfuscator_MOCS test_obfuscator.h)
add_executable(test_obfuscator test_obfuscator.cpp
    ${PMP_TestObfuscator_MOCS}
    ${CMAKE_SOURCE_DIR}/src/common/obfuscator.cpp
)
target_link_libraries(test_obfuscator Qt5::Core Qt5::Test)
add_test(test_obfuscator test_obfuscator)


# TestHashRelations
qt5_wrap_cpp(PMP_TestHashRelations_MOCS test_hashrelations.h)
add_executable(test_hashrelations test_hashrelations.cpp
    ${PMP_TestHashRelations_MOCS}
    ${CMAKE_SOURCE_DIR}/src/server/hashrelations.cpp
)
target_link_libraries(test_hashrelations Qt5::Core Qt5::Test)
add_test(test_hashrelations test_hashrelations)


# TestSortedCollectionTableModel
qt5_wrap_cpp(PMP_TestSortedCollectionTableModel_MOCS test_sortedcollectiontablemodel.h)
add_executable(test_sortedcollectiontablemodel test_sortedcollectiontablemodel.cpp
    ${PMP_TestSortedCollectionTableModel_MOCS}
)
target_link_libraries(test_sortedcollectiontablemodel $<TARGET_OBJECTS:PmpDesktopRemote>)
target_link_libraries(test_sortedcollectiontablemodel $<TARGET_OBJECTS:PmpClient>)
target_link_libraries(test_sortedcollectiontablemodel $<TARGET_OBJECTS:PmpCommon>)
target_link_libraries(test_sortedcollectiontablemodel Qt5::Gui Qt5::Widgets)
target_link_libraries(test_sortedcollectiontablemodel Qt5::Core Qt5::Network Qt5::Test)
target_link_libraries(test_sortedcollectiontablemodel ${TAGLIB_LIBRARIES})
add_test(NAME test_sortedcollectiontablemodel
         COMMAND test_sortedcollectiontablemodel -platform offscreen)
