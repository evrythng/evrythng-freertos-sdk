cmake_minimum_required(VERSION 2.8)
project(evrythng-freertos-sdk)

set(COMMON_PREFIX CMakeTimestamps)

set(FreeRTOS_dir ${PROJECT_SOURCE_DIR}/FreeRTOS_Posix)
set(FreeRTOS_Kernel_dir ${FreeRTOS_dir}/FreeRTOS_Kernel)

add_subdirectory(${PROJECT_SOURCE_DIR}/evrythng/tests)

include_directories(${PROJECT_SOURCE_DIR}/evrythng)
include_directories(${PROJECT_SOURCE_DIR}/FreeRTOS_Posix)
include_directories(${PROJECT_SOURCE_DIR}/FreeRTOS_Posix/FreeRTOS_Kernel/include)
include_directories(${PROJECT_SOURCE_DIR}/FreeRTOS_Posix/FreeRTOS_Kernel/portable/GCC/Posix)
#include_directories(${PROJECT_SOURCE_DIR}/FreeRTOS_Posix/Common_Demo/include)
include_directories(${PROJECT_SOURCE_DIR}/mqtt/src)

add_definitions(-DOPENSSL)
add_definitions(-D__GCC_POSIX__=1)
add_definitions(-DUSE_STDIO=1)
add_definitions(-DCONFIG_OS_FREERTOS)
add_definitions(-DNOSTACKTRACE)
add_definitions(-DNO_HEAP_TRACKING)
add_definitions(-DNO_PERSISTENCE)
add_definitions(-DCONFIG_IPV6)

set(SRC evrythng-demo.c)
include(cmake/freertos.cmake)
include(cmake/mqtt.cmake)
include(cmake/evrythng.cmake)
include(cmake/toolchainfile.cmake)

add_executable(evrythng-demo ${SRC})
target_link_libraries(evrythng-demo pthread)
target_link_libraries(evrythng-demo ssl)
target_link_libraries(evrythng-demo crypto)
target_link_libraries(evrythng-demo rt)
