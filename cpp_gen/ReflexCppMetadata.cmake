# Part of the Reflex project.
# Copyright © Reflex Project Contributors.
# SPDX-License-Identifier: Apache-2.0

include(ReflexTool)

find_program(CLANG_FORMAT_PROGRAM clang-format REQUIRED)
message(STATUS "Using clang-format: ${CLANG_FORMAT_PROGRAM}")

define_property(TARGET PROPERTY REFLEX_HEADERS)

function(target_reflex_headers TARGET_NAME)
    foreach(HEADER_PATH ${ARGN})
        cmake_path(ABSOLUTE_PATH HEADER_PATH NORMALIZE)
        set_property(TARGET ${TARGET_NAME} APPEND PROPERTY REFLEX_HEADERS "${HEADER_PATH}")
    endforeach()
endfunction()

function(reflex_generate_cpp_metadata)
    if(NOT TARGET reflex::cpp_gen)
        message(
            FATAL_ERROR
            "The reflex::cpp_gen target doesn't exist when calling reflex_generate_cpp_metadata().\n"
            "A call to find_package(reflex) may be missing"
        )
    endif()

    set(options USE_INTERFACE_PROPERTIES)
    set(oneValueArgs TARGET OUTPUT_VARIABLE METADATA_FILE_LIST_VARIABLE)
    set(multiValueArgs HEADERS EXTRA_COMPILE_DEFINITIONS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach(SOURCE_FILENAME ${ARG_HEADERS})
        cmake_path(ABSOLUTE_PATH SOURCE_FILENAME OUTPUT_VARIABLE SOURCE_ABSOLUTE_PATH)
        cmake_path(RELATIVE_PATH SOURCE_ABSOLUTE_PATH OUTPUT_VARIABLE SOURCE_RELATIVE_PATH)

        set(METADATA_FILENAME "${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_RELATIVE_PATH}.metadata.json")
        set(DEPFILE_PATH "${METADATA_FILENAME}.d")

        set(additional_args)
        if(ARG_USE_INTERFACE_PROPERTIES)
            set(additional_args USE_INTERFACE_PROPERTIES)
        endif()

        reflex_generate_metadata(
            ${additional_args}
            TARGET ${ARG_TARGET}
            HEADER ${SOURCE_FILENAME}
            OUTPUT ${METADATA_FILENAME}
            DEPFILE ${DEPFILE_PATH}
            EXTRA_COMPILE_DEFINITIONS ${ARG_EXTRA_COMPILE_DEFINITIONS}
        )

        set(GENERATED_HEADER_FILENAME "${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_RELATIVE_PATH}")
        cmake_path(REPLACE_EXTENSION GENERATED_HEADER_FILENAME LAST_ONLY "meta.inc")

        add_custom_command(
            OUTPUT "${GENERATED_HEADER_FILENAME}"
            COMMAND reflex::cpp_gen ${METADATA_FILENAME} ${GENERATED_HEADER_FILENAME}
            COMMAND ${CLANG_FORMAT_PROGRAM} -i ${GENERATED_HEADER_FILENAME}
            MAIN_DEPENDENCY "${METADATA_FILENAME}"
            DEPENDS reflex::cpp_gen
            VERBATIM
        )

        list(APPEND ${ARG_OUTPUT_VARIABLE} ${GENERATED_HEADER_FILENAME})
        if(ARG_METADATA_FILE_LIST_VARIABLE)
            list(APPEND ${ARG_METADATA_FILE_LIST_VARIABLE} ${METADATA_FILENAME})
        endif()
    endforeach()

    set(${ARG_OUTPUT_VARIABLE} ${${ARG_OUTPUT_VARIABLE}} PARENT_SCOPE)

    if(ARG_METADATA_FILE_LIST_VARIABLE)
        set(${ARG_METADATA_FILE_LIST_VARIABLE} ${${ARG_METADATA_FILE_LIST_VARIABLE}} PARENT_SCOPE)
    endif()
endfunction()
