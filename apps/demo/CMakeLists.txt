
include_directories(${FreeRTOS_Posix_dir})
include_directories(${FreeRTOS_dir}/include)
include_directories(${FreeRTOS_Posix_dir}/GCC/Posix)
include_directories(${evrythng-lib}/core/evrythng/include)
include_directories(${evrythng-lib}/platform/FreeRTOS_POSIX)

set(EVRYTHNG_DEMO_SRC demo.c)

add_executable(evrythng-demo ${EVRYTHNG_DEMO_SRC})
target_link_libraries(evrythng-demo evrythng)
