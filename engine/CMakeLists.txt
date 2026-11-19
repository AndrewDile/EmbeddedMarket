cmake_minimum_required(VERSION 3.18)
project(EmbeddedMarketEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(EM_BUILD_TESTS "Build the engine unit tests (requires GoogleTest)" ON)
option(EM_BUILD_SERVICE "Build the engine's HTTP service (downloads cpp-httplib and nlohmann/json)" OFF)

add_library(em_engine STATIC
    src/model/Model.cpp
    src/model/Project.cpp
    src/util/Format.cpp
    src/engine/Report.cpp
    src/engine/CompatibilityEngine.cpp
    src/rules/RuleRegistry.cpp
    src/rules/SpecSanityRule.cpp
    src/rules/SupplyVoltageRule.cpp
    src/rules/InterfaceRule.cpp
    src/rules/LogicLevelRule.cpp
    src/rules/PinBudgetRule.cpp
    src/rules/PowerBudgetRule.cpp
    src/rules/ProjectConstraintsRule.cpp
)
target_include_directories(em_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(em_engine PRIVATE /W4)
else()
    target_compile_options(em_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(EM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(EM_BUILD_SERVICE)
    add_subdirectory(service)
endif()
