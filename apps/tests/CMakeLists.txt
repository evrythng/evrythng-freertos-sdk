

set(TEST_SRC
    ${evrythng-lib}/core/tests/CuTest.c 
    ${evrythng-lib}/core/tests/tests.c
    main.c
)

include_directories(${FreeRTOS_Posix_dir})
include_directories(${FreeRTOS_dir}/include)
include_directories(${FreeRTOS_Posix_dir}/GCC/Posix)
include_directories(${evrythng-lib}/core/evrythng/include)
include_directories(${evrythng-lib}/core/tests)
include_directories(${evrythng-lib}/platform)

add_definitions(-DFREERTOS_POSIX_PLATFORM)

add_executable(evrythng-tests ${TEST_SRC})
target_link_libraries(evrythng-tests evrythng)
