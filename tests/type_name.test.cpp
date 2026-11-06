// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <catch2/catch_test_macros.hpp>

#include <reflex/type_name.hpp>

namespace reflex::tests {

struct CustomStruct {};

class CustomClass {};

#if defined _MSC_VER && !defined __clang__
    #define IF_MSVC(...) __VA_ARGS__
#else
    #define IF_MSVC(...)
#endif

TEST_CASE("reflex::type_name equals the expected type names") {
    STATIC_REQUIRE(type_name<int> == "int");
    REQUIRE(type_name<int> == "int");

    STATIC_REQUIRE(type_name<CustomStruct> == IF_MSVC("struct ") "reflex::tests::CustomStruct");
    REQUIRE(type_name<CustomStruct> == IF_MSVC("struct ") "reflex::tests::CustomStruct");

    STATIC_REQUIRE(type_name<CustomClass> == IF_MSVC("class ") "reflex::tests::CustomClass");
    REQUIRE(type_name<CustomClass> == IF_MSVC("class ") "reflex::tests::CustomClass");
}

} // namespace reflex::tests
