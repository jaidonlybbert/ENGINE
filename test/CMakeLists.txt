cmake_minimum_required(VERSION 3.25.1)

enable_testing()

add_executable(
	engine_test
	test_main.cpp
)

target_link_libraries(
	engine_test
	GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(engine_test)
