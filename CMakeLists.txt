cmake_minimum_required(VERSION 3.16)

project(libpdptw VERSION 0.3.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

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
    src/rng.c
    src/objectives.c
    src/construction.c
    src/insertion.c
    src/placement.c
    src/rearrange.c
    src/crossover.c
    lib/pcg/pcg_basic.c
)
target_link_libraries(vrp_core PUBLIC m)
target_include_directories(vrp_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/pcg>
    $<INSTALL_INTERFACE:include>
)

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
    add_executable(test_rng tests/test_rng.c)
    target_link_libraries(test_rng vrp_core unity m)
    add_executable(test_objectives tests/test_objectives.c)
    target_link_libraries(test_objectives vrp_core unity m)
    add_executable(test_construction tests/test_construction.c)
    target_link_libraries(test_construction vrp_core unity m)
    add_executable(test_insertion tests/test_insertion.c)
    target_link_libraries(test_insertion vrp_core unity m)
    add_executable(test_rearrange tests/test_rearrange.c)
    target_link_libraries(test_rearrange vrp_core unity m)
    add_executable(test_crossover tests/test_crossover.c)
    target_link_libraries(test_crossover vrp_core unity m)

    add_test(NAME TestRoute COMMAND test_route)
    add_test(NAME TestProblem COMMAND test_problem)
    add_test(NAME TestSolution COMMAND test_solution)
    add_test(NAME TestRng COMMAND test_rng)
    add_test(NAME TestObjectives COMMAND test_objectives)
    add_test(NAME TestConstruction COMMAND test_construction)
    add_test(NAME TestInsertion COMMAND test_insertion)
    add_test(NAME TestRearrange COMMAND test_rearrange)
    add_test(NAME TestCrossover COMMAND test_crossover)
endif()

# === Install / find_package support ===
include(GNUInstallDirs)

install(TARGETS vrp_core
    EXPORT libpdptwTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION include)
install(FILES lib/pcg/pcg_basic.h DESTINATION include)
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
