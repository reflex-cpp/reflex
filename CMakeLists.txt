# Part of the Reflex project.
# Copyright © Reflex Project Contributors.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.25)

project(reflex VERSION 1.0.1 LANGUAGES CXX)

option(REFLEX_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(REFLEX_INSTALL_CMAKE_CONFIG "Install CMake Config files" ${PROJECT_IS_TOP_LEVEL})
option(REFLEX_USE_FETCH_CONTENT "Use FetchContent for internal dependencies" OFF)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
endif()
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "reflex: C++ standard: ${CMAKE_CXX_STANDARD}")

if(REFLEX_USE_FETCH_CONTENT)
    include("${CMAKE_CURRENT_LIST_DIR}/tools/cmake/fetch_dependencies.cmake")
endif()
if(NOT TARGET reflex::tool)
    find_package(reflex-tool REQUIRED)
endif()
if(NOT TARGET Boost::headers OR NOT TARGET Boost::json)
    find_package(Boost REQUIRED COMPONENTS headers json CONFIG)
endif()
if(NOT TARGET fmt::fmt)
    find_package(fmt REQUIRED)
endif()

include(GNUInstallDirs)
include("${CMAKE_CURRENT_LIST_DIR}/tools/cmake/diagnostics.cmake")

add_subdirectory(cpp_gen)

add_library(reflex INTERFACE)
add_library(reflex::reflex ALIAS reflex)
target_compile_features(reflex INTERFACE cxx_std_23)
target_link_libraries(reflex INTERFACE Boost::headers)
target_sources(
    reflex
    INTERFACE
        FILE_SET HEADERS
            BASE_DIRS include
            FILES
                include/reflex/enum_indexed_array.hpp
                include/reflex/enum_indexed_fwd.hpp
                include/reflex/enum_indexed_tuple.hpp
                include/reflex/enum_indexed_variant.hpp
                include/reflex/enum_indexing.hpp
                include/reflex/enum_utils.hpp
                include/reflex/function_traits.hpp
                include/reflex/meta.hpp
                include/reflex/tuple.hpp
                include/reflex/type_name.hpp
                include/reflex/ranges/contains.hpp
                include/reflex/ranges/enumerate_view.hpp
                include/reflex/ranges/range_utils.hpp
                include/reflex/ranges/to.hpp
                include/reflex/ranges/zip_view.hpp
                include/reflex/detail/config.hpp
)
if(WIN32)
    set(NATVIS_FILE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/reflex/reflex.natvis>$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/reflex/reflex.natvis>"
    )
    target_sources(reflex INTERFACE ${NATVIS_FILE})
    set_target_properties(
        reflex
        PROPERTIES INTERFACE_LINK_OPTIONS "-natvis:${NATVIS_FILE}" INTERFACE_LINK_DEPENDS "${NATVIS_FILE}"
    )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(reflex INTERFACE -wd5030) # attribute '...' is not recognized
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(reflex INTERFACE -Wno-unknown-attributes)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(reflex INTERFACE -Wno-attributes)
endif()

if(REFLEX_BUILD_TESTS)
    enable_testing()
    find_package(Catch2 REQUIRED)

    add_executable(reflex_tests)
    reflex_set_diagnostic_options(reflex_tests)
    target_link_libraries(reflex_tests PRIVATE Catch2::Catch2WithMain reflex::reflex)
    target_sources(
        reflex_tests
        PRIVATE
            tests/enum_indexed_array.test.cpp
            tests/enum_indexed_tuple.test.cpp
            tests/enum_indexed_variant.test.cpp
            tests/enum_indexing.test.cpp
            tests/enum_utils.test.cpp
            tests/function_traits.test.cpp
            tests/structure_traits.test.cpp
            tests/test_types.hpp
            tests/type_name.test.cpp
            tests/ranges/contains.test.cpp
            tests/ranges/enumerate_view.test.cpp
            tests/ranges/to.test.cpp
            tests/ranges/zip_view.test.cpp
    )

    reflex_generate_cpp_metadata(TARGET reflex_tests OUTPUT_VARIABLE TEST_METADATA_FILES HEADERS tests/test_types.hpp)
    add_custom_target(reflex_tests_metadata DEPENDS ${TEST_METADATA_FILES})
    add_dependencies(reflex_tests reflex_tests_metadata)
    target_include_directories(reflex_tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/tests)

    include(Catch)
    catch_discover_tests(reflex_tests)
endif()

install(TARGETS reflex EXPORT reflexTargets FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES include/reflex/reflex.natvis DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/reflex")
if(REFLEX_INSTALL_CMAKE_CONFIG)
    install(
        EXPORT reflexTargets
        FILE reflexTargets.cmake
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/reflex"
        NAMESPACE reflex::
    )
    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        tools/cmake/reflexConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/reflexConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/reflex"
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/reflexConfig.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/reflex")
endif()
