

set(TEST_SRC
    ${evrythng-lib}/core/tests/CuTest.c 
    ${evrythng-lib}/core/tests/tests.c
    ${evrythng-lib}/core/tests/main.c
)

add_executable(evrythng-tests ${TEST_SRC})
target_link_libraries(evrythng-tests evrythng)
