cmake_minimum_required(VERSION 2.8)
project(evrythng-freertos-sdk)

set(COMMON_PREFIX CMakeTimestamps)

set(FreeRTOS_dir ${PROJECT_SOURCE_DIR}/libs/FreeRTOSV8.2.1/FreeRTOS/Source)
set(FreeRTOS_Posix_dir ${PROJECT_SOURCE_DIR}/libs/FreeRTOSV8.2.1/PosixPort)
set(evrythng-lib ${PROJECT_SOURCE_DIR}/libs/evrythng)

set(EXTRA_FLAGS "-Wall -Wno-unused-variable")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_FLAGS}")

add_subdirectory(${PROJECT_SOURCE_DIR}/apps/demo)
add_subdirectory(${PROJECT_SOURCE_DIR}/apps/tests)
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/evrythng)
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/FreeRTOSV8.2.1)
