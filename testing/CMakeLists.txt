
include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(PMP_hash_test
    hash_test.cpp
    ${CMAKE_SOURCE_DIR}/src/common/audiodata.cpp
    ${CMAKE_SOURCE_DIR}/src/common/fileanalyzer.cpp
    ${CMAKE_SOURCE_DIR}/src/common/filehash.cpp
    ${CMAKE_SOURCE_DIR}/src/common/tagdata.cpp
)
qt5_use_modules(PMP_hash_test Core)
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
