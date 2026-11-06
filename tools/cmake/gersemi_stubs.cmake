# Part of the Reflex project.
# Copyright © Reflex Project Contributors.
# SPDX-License-Identifier: Apache-2.0

function(reflex_generate_metadata)
    set(options USE_INTERFACE_PROPERTIES)
    set(oneValueArgs TARGET HEADER OUTPUT DEPFILE)
    set(multiValueArgs EXTRA_COMPILE_DEFINITIONS)
    cmake_parse_arguments(_ "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
endfunction()
