set(SRC
    ${PROJECT_SOURCE_DIR}/evrythng/evrythng.c 
    ${PROJECT_SOURCE_DIR}/evrythng/tests/CuTest.c 
    ${PROJECT_SOURCE_DIR}/evrythng/tests/tests.c

    ${PROJECT_SOURCE_DIR}/mqtt/src/Log.c
    ${PROJECT_SOURCE_DIR}/mqtt/src/StackTrace.c
)
include(${PROJECT_SOURCE_DIR}/cmake/mqtt.cmake)

include_directories(${PROJECT_SOURCE_DIR}/evrythng)
include_directories(${PROJECT_SOURCE_DIR}/evrythng/tests)
include_directories(${PROJECT_SOURCE_DIR}/mqtt/src)

add_definitions(-DOPENSSL)
add_definitions(-DNO_PERSISTENCE)
#add_definitions(-D__GCC_POSIX__=1)
#add_definitions(-DUSE_STDIO=1)
#add_definitions(-DNOSTACKTRACE)
#add_definitions(-DNO_HEAP_TRACKING)
#add_definitions(-DCONFIG_IPV6)

add_executable(evrythng-test ${SRC})

target_link_libraries(evrythng-test pthread)
#target_link_libraries(evrythng-test rt)
target_link_libraries(evrythng-test ssl crypto)
