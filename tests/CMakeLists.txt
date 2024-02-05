add_executable(functional_tests test.cpp ${SOURCES})
target_include_directories(functional_tests PRIVATE "../src/mem")
target_include_directories(functional_tests PRIVATE "../src/db_struct")
target_include_directories(functional_tests PRIVATE "../src/util")
target_include_directories(functional_tests PRIVATE "../src/type_system")


target_link_libraries(functional_tests gtest_main gmock_main)
target_include_directories(functional_tests PRIVATE "../googletest/googletest/googlemock/include/gmock")
target_include_directories(functional_tests PRIVATE "../googletest/googletest/googlemock/include/gtest")

