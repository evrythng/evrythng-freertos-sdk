cmake_minimum_required(VERSION 2.8)
project(evrythng-freertos-sdk)

set(COMMON_PREFIX CMakeTimestamps)

set(FreeRTOS_dir ${PROJECT_SOURCE_DIR}/FreeRTOSV8.2.1/FreeRTOS/Source)
set(FreeRTOS_Posix_dir ${PROJECT_SOURCE_DIR}/FreeRTOSV8.2.1/PosixPort)
set(evrythng-lib ${PROJECT_SOURCE_DIR}/evrythng-c-library)

include_directories(${FreeRTOS_dir})
include_directories(${FreeRTOS_dir}/include)
include_directories(${FreeRTOS_Posix_dir})
include_directories(${FreeRTOS_Posix_dir}/GCC/Posix)
include_directories(${evrythng-lib}/evrythng/include)
include_directories(${evrythng-lib}/mqtt/src)
include_directories(${evrythng-lib}/tests)

add_definitions(-D__GCC_POSIX__=1)
add_definitions(-DUSE_STDIO=1)
add_definitions(-DCONFIG_OS_FREERTOS)
add_definitions(-DOPENSSL)
add_definitions(-DNOSTACKTRACE)
add_definitions(-DNO_HEAP_TRACKING)
add_definitions(-DNO_PERSISTENCE)
add_definitions(-DCONFIG_IPV6)

include(cmake/FreeRTOSV8.2.1.cmake)
include(cmake/toolchainfile.cmake)
include(cmake/mqtt.cmake)
include(cmake/evrythng.cmake)
include(cmake/tests.cmake)

add_library(FreeRTOS ${FREERTOS_SRC})
target_link_libraries(FreeRTOS pthread)

add_library(mqtt ${MQTT_SRC})
target_link_libraries(mqtt ssl)
target_link_libraries(mqtt crypto)

add_library(evrythng ${EVT_SRC})
target_link_libraries(evrythng mqtt)
target_link_libraries(evrythng FreeRTOS)

set(FREERTOS_CLI_SRC evrythng-freertos-cli.c)
add_executable(evrythng-freertos-cli ${FREERTOS_CLI_SRC})
target_link_libraries(evrythng-freertos-cli evrythng)

add_executable(evrythng-freertos-tests ${TEST_SRC})
target_link_libraries(evrythng-freertos-tests evrythng)
