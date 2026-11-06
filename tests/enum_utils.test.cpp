// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <catch2/catch_test_macros.hpp>

#include <reflex/enum_utils.hpp>

#include "test_types.hpp"

namespace reflex::tests {

TEST_CASE("enumerator_name() returns the correct name") {
    STATIC_REQUIRE(enumerator_name(index_enum_e::first) == "first");
    STATIC_REQUIRE(enumerator_name(index_enum_e::second) == "second");
    STATIC_REQUIRE(enumerator_name(index_enum_e::third) == "third");
    STATIC_REQUIRE(enumerator_name(static_cast<index_enum_e>(-77)) == std::nullopt);
    STATIC_REQUIRE(enumerator_name(static_cast<index_enum_e>(1024)) == std::nullopt);
}

TEST_CASE("enum_to_string() returns either enumerator names or '<unknown>'") {
    STATIC_REQUIRE(enum_to_string(index_enum_e::first) == "first");
    STATIC_REQUIRE(enum_to_string(index_enum_e::second) == "second");
    STATIC_REQUIRE(enum_to_string(index_enum_e::third) == "third");
    STATIC_REQUIRE(enum_to_string(static_cast<index_enum_e>(-1)) == "<unknown>");
    STATIC_REQUIRE(enum_to_string(static_cast<index_enum_e>(123)) == "<unknown>");
}

} // namespace reflex::tests
