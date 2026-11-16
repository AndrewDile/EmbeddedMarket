# Use an installed GoogleTest if there is one; otherwise download a pinned release into the
# build tree and build it with the same compiler as the engine.
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(STATUS "GoogleTest not found; fetching v1.15.2 into the build directory")
    include(FetchContent)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        GIT_SHALLOW TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)  # Match the MSVC runtime on Windows.
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

add_executable(em_engine_tests
    SpecValueTest.cpp
    ProjectTest.cpp
    ReportTest.cpp
    EngineTest.cpp
    DeterminismTest.cpp
    SpecSanityRuleTest.cpp
    SupplyVoltageRuleTest.cpp
    InterfaceRuleTest.cpp
    LogicLevelRuleTest.cpp
    PinBudgetRuleTest.cpp
    UncertaintyTest.cpp
)
target_link_libraries(em_engine_tests PRIVATE em_engine GTest::gtest_main)

if(MSVC)
    target_compile_options(em_engine_tests PRIVATE /W4)
else()
    target_compile_options(em_engine_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# One CTest entry per test case, with JUnit-style XML results in <build>/test-results.
gtest_discover_tests(em_engine_tests
    DISCOVERY_MODE PRE_TEST
    XML_OUTPUT_DIR ${CMAKE_BINARY_DIR}/test-results
)
