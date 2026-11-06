// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <concepts>

#include <catch2/catch_test_macros.hpp>

#include <reflex/meta.hpp>

#include "test_types.hpp"

namespace reflex::tests {

TEST_CASE("meta traits contains correct information for structure specialization") {
    using traits = meta<first_structure_s>;
    STATIC_REQUIRE(std::tuple_size_v<decltype(traits::data_members)> == 0u);
    STATIC_REQUIRE(std::tuple_size_v<decltype(traits::properties)> == 0u);
    STATIC_REQUIRE(std::same_as<traits::member_functions_t, boost::mp11::mp_list<>>);
}

TEST_CASE("meta traits contains initializer info for inherited structure") {
    using traits = meta<inherited_s>;
    STATIC_REQUIRE(reflex::get<0>(traits::data_members).initializer != nullptr);
}

TEST_CASE("meta traits contains info for inherited member function") {
    using traits = meta<inherited_s>;

    STATIC_REQUIRE(boost::mp11::mp_size<traits::member_functions_t>::value == 1u);
}

} // namespace reflex::tests