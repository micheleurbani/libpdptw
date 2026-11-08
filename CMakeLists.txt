cmake_minimum_required(VERSION 3.16)

project(libpdptw VERSION 0.2.0 LANGUAGES C)

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
)
target_link_libraries(vrp_core PUBLIC m)
target_include_directories(vrp_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
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

    add_test(NAME TestRoute COMMAND test_route)
    add_test(NAME TestProblem COMMAND test_problem)
    add_test(NAME TestSolution COMMAND test_solution)
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
