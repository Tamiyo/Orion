# Create an executable to test this test suite.
add_executable(
        util_tests
        strings_test.cc
        views_test.cc
)

target_link_libraries(
        util_tests
        PRIVATE GTest::gtest_main
        PRIVATE utils
)


# Discover Tests
gtest_discover_tests(util_tests)
