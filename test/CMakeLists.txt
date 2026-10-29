cmake_minimum_required(VERSION 3.21)

# This is a subproject, not a standalone root project
# Do NOT call project() again unless you want separate languages/toolchains



# ---- Test sources ----------------------------------------------------------

set(uqm_test_sources_all "main.cpp")

include("tests/tests.cmake")
list(APPEND uqm_test_sources_all ${uqm_tests_master_files})

set(UQM_TEST_SOURCES
    ${uqm_test_sources_all}
)
message(STATUS "Test sources: ${UQM_TEST_SOURCES}")
message(STATUS "CMake source dir: ${CMAKE_SOURCE_DIR}")

# ---- Test executable -------------------------------------------------------

add_executable(UQM_Tests
    ${UQM_TEST_SOURCES}
)
target_link_libraries(UQM_Tests PRIVATE uqm_project_features)

target_compile_options(UQM_Tests
    PRIVATE
        $<$<CONFIG:RelWithDebInfo>:/Zi /Od>
)

target_link_options(UQM_Tests
    PRIVATE
        $<$<CONFIG:RelWithDebInfo>:/DEBUG>
)

# ---- Include paths ---------------------------------------------------------

# Include the main project's src/ directory
target_include_directories(UQM_Tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# ---- Link libraries --------------------------------------------------------

find_package(fmt REQUIRED)
find_package(gsl-lite REQUIRED)
find_package(GTest REQUIRED)
find_package(lua REQUIRED)
find_package(magic_enum REQUIRED)
find_package(spdlog REQUIRED)
find_package(utf8proc REQUIRED)
find_package(yyjson REQUIRED)
find_package(CLI11 REQUIRED)

# GTest::gtest and GTest::gtest_main are the modern imported targets
target_link_libraries(UQM_Tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
        lua::lua
        magic_enum::magic_enum
        fmt::fmt
        gsl-lite::gsl-lite
        spdlog::spdlog
        utf8proc::utf8proc
        yyjson::yyjson
        CLI11::CLI11

)

# ---- Compiler options (inherit from main project if needed) ---------------

# If you want tests to match main compile settings:
# target_link_libraries(UQM_Tests PRIVATE UrQuanMasters)

# ---- Runtime DLLs ----------------------------------------------------------

add_custom_command(TARGET UQM_Tests POST_BUILD
    COMMAND "$<$<BOOL:$<TARGET_RUNTIME_DLLS:UQM_Tests>>:${CMAKE_COMMAND};-E;copy_if_different;$<TARGET_RUNTIME_DLLS:UQM_Tests>;$<TARGET_FILE_DIR:UQM_Tests>>"
    COMMAND_EXPAND_LISTS
)

# ---- Enable GTest integration ---------------------------------------------

include(GoogleTest)
gtest_discover_tests(UQM_Tests)