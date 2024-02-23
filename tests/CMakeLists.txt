enable_testing()

file(GLOB_RECURSE TEST  CONFIGURE_DEPENDS  ${SOURCES} ./*.hpp ./*.cpp)
add_executable(${PROJECT_ARTIFACT_TEST}  ${TEST})

target_include_directories(${PROJECT_ARTIFACT_TEST} PRIVATE "../src/mem")
target_include_directories(${PROJECT_ARTIFACT_TEST} PRIVATE "../src/db_struct")
target_include_directories(${PROJECT_ARTIFACT_TEST} PRIVATE "../src/util")
target_include_directories(${PROJECT_ARTIFACT_TEST} PRIVATE "../src/type_system")


target_link_libraries(${PROJECT_ARTIFACT_TEST} gtest_main gmock_main)
target_include_directories(${PROJECT_ARTIFACT_TEST} PRIVATE "../googletest/googletest/googlemock/include/gmock")
target_include_directories(${PROJECT_ARTIFACT_TEST} PRIVATE "../googletest/googletest/googlemock/include/gtest")


# add_test(NAME TypeTests COMMAND ${PROJECT_ARTIFACT_TEST} )