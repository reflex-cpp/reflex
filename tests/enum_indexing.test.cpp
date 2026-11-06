// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <array>

#include <catch2/catch_test_macros.hpp>

#include <reflex/enum_indexing.hpp>

#include "test_types.hpp"

namespace reflex::tests {

TEST_CASE("index_enumerators_of<E>() generates a range of enumerators in 'indexing' order") {
    REQUIRE(std::ranges::equal(index_enumerators_of<index_enum_e>(),
                               std::array{index_enum_e::first, index_enum_e::second, index_enum_e::third}));
    REQUIRE(std::ranges::equal(
            index_enumerators_of<unordered_index_enum_e>(),
            std::array{unordered_index_enum_e::first, unordered_index_enum_e::second, unordered_index_enum_e::third}));
}

} // namespace reflex::tests
