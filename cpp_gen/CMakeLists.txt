# Part of the Reflex project.
# Copyright © Reflex Project Contributors.
# SPDX-License-Identifier: Apache-2.0

add_executable(cpp_gen)
add_executable(reflex::cpp_gen ALIAS cpp_gen)
set_target_properties(cpp_gen PROPERTIES OUTPUT_NAME reflex_cpp_gen)
target_compile_features(cpp_gen PRIVATE cxx_std_23)
reflex_set_diagnostic_options(cpp_gen)
target_link_libraries(cpp_gen PRIVATE Boost::headers Boost::json fmt::fmt)
configure_file(src/reflex_cpp_gen.manifest.in src/reflex_cpp_gen.manifest @ONLY)
configure_file(src/reflex_cpp_gen.rc.in src/reflex_cpp_gen.rc @ONLY)
target_sources(
    cpp_gen
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/src/reflex_cpp_gen.manifest"
        "${CMAKE_CURRENT_BINARY_DIR}/src/reflex_cpp_gen.rc"
        src/generate.cpp
        src/generate.hpp
        src/main.cpp
        src/metadata.hpp
)

install(TARGETS cpp_gen EXPORT reflexTargets RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(WIN32)
    install(FILES "$<TARGET_PDB_FILE:cpp_gen>" DESTINATION "${CMAKE_INSTALL_BINDIR}" OPTIONAL)
endif()
install(FILES ReflexCppMetadata.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/reflex)

include(ReflexCppMetadata.cmake)
