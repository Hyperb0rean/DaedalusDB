add_executable(functional_tests test.cpp ${SOURCES})
target_include_directories(functional_tests PRIVATE "../src/mem")
target_include_directories(functional_tests PRIVATE "../src/db_struct")
target_include_directories(functional_tests PRIVATE "../src/util")

add_test(
  NAME
    simple_test
  COMMAND
    $<TARGET_FILE:functional_tests>
  )