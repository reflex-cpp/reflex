// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __has_builtin
    #define REFLEX_HAS_BUILTIN(arg) __has_builtin(arg)
#else
    #define REFLEX_HAS_BUILTIN(arg) 0
#endif

#ifdef __has_cpp_attribute
    #if __has_cpp_attribute(msvc::no_unique_address)
        #define REFLEX_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
    #elif __has_cpp_attribute(no_unique_address)
        #define REFLEX_NO_UNIQUE_ADDRESS [[no_unique_address]]
    #else
        #define REFLEX_NO_UNIQUE_ADDRESS
    #endif
#else
    #define REFLEX_NO_UNIQUE_ADDRESS
#endif
