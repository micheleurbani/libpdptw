cmake_minimum_required(VERSION 3.16)

project(libpdptw C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Only export compile commands / position-independent code when this is the
# top-level build; a consumer that pulls this in via add_subdirectory or
# FetchContent keeps its own settings.
set(LIBPDPTW_IS_TOP_LEVEL OFF)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(LIBPDPTW_IS_TOP_LEVEL ON)
endif()

option(LIBPDPTW_BUILD_TESTS "Build libpdptw's own test suite" ${LIBPDPTW_IS_TOP_LEVEL})

# Data structures, problem/route/solution model, and PD operators (C99).
add_library(vrp_core
    src/utils.c
    src/route.c
    src/problem.c
    src/solution.c
)
target_link_libraries(vrp_core PUBLIC m)
target_include_directories(vrp_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Metaheuristics: tabu-embedded simulated annealing (Li & Lim 2003) and ALNS
# (Ropke & Pisinger 2006), both C++17, both reaching into vrp_core through the
# extern "C" blocks in include/*.h.
add_library(vrp_algorithms
    src/algorithms/tabu.cpp
    src/algorithms/alns.cpp
)
target_link_libraries(vrp_algorithms PUBLIC vrp_core)

if(LIBPDPTW_BUILD_TESTS)
    enable_testing()

    add_library(unity STATIC lib/unity/unity.c)
    target_include_directories(unity PUBLIC lib/unity)
    target_compile_definitions(unity PUBLIC UNITY_INCLUDE_DOUBLE)

    add_executable(test_route tests/test_route.c)
    target_link_libraries(test_route vrp_core unity m)
    add_executable(test_problem tests/test_problem.c)
    target_link_libraries(test_problem vrp_core unity m)
    add_executable(test_solution tests/test_solution.c)
    target_link_libraries(test_solution vrp_core unity m)
    add_executable(test_tabu tests/test_tabu.cpp)
    target_link_libraries(test_tabu vrp_algorithms unity m)
    add_executable(test_alns tests/test_alns.cpp)
    target_link_libraries(test_alns vrp_algorithms unity m)

    add_test(NAME TestRoute COMMAND test_route)
    add_test(NAME TestProblem COMMAND test_problem)
    add_test(NAME TestSolution COMMAND test_solution)
    add_test(NAME TestTabu COMMAND test_tabu)
    add_test(NAME TestAlns COMMAND test_alns)
endif()

# === Install / find_package support ===
include(GNUInstallDirs)

install(TARGETS vrp_core vrp_algorithms
    EXPORT libpdptwTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT libpdptwTargets
    FILE libpdptwTargets.cmake
    NAMESPACE libpdptw::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libpdptw
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/libpdptwConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/libpdptwConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libpdptw
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpdptwConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libpdptw
)
