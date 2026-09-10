cmake_minimum_required(VERSION 3.25.1)

enable_testing()

add_executable(
	engine_test
	test_main.cpp
	logger/LoggingTest.cpp
	config/RuntimeConfigTest.cpp
)

# Because apple immediately kills unsigned executables
if(APPLE)
    add_custom_command(TARGET engine_test POST_BUILD
        COMMAND codesign --force --deep --sign - $<TARGET_FILE:engine_test>
        COMMENT "Ad-hoc signing engine_test to prevent SIGKILL"
    )
endif()

target_link_libraries(
	engine_test
	GTest::gtest_main
	engine::logger
	engine::config
)

include(GoogleTest)
gtest_discover_tests(engine_test)
