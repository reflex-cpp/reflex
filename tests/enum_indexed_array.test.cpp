// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <array>
#include <concepts>
#include <ranges>
#include <type_traits>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include <reflex/enum_indexed_array.hpp>

#include "test_types.hpp"

namespace reflex::tests {

using test_array_t = EnumIndexedArray<int, index_enum_e>;

TEST_CASE("EnumIndexedArray is an aggregate") {
    STATIC_REQUIRE(std::is_aggregate_v<test_array_t>);
    STATIC_REQUIRE(std::is_trivial_v<test_array_t>);

    {
        auto arr = test_array_t{};
        REQUIRE(std::ranges::all_of(arr, [](const int x) { return x == 0; }));
    }
    {
        auto arr = test_array_t{1, 2, 3};
        REQUIRE(std::ranges::equal(arr, std::array{1, 2, 3}));
    }
}

TEST_CASE("EnumIndexedArray is a contiguous range") {
    using array_t = EnumIndexedArray<float, index_enum_e>;
    STATIC_REQUIRE(std::ranges::contiguous_range<array_t>);
    STATIC_REQUIRE(std::ranges::output_range<array_t, float>);
    STATIC_REQUIRE(!std::ranges::output_range<const array_t, float>);
}

TEST_CASE("Indexing into an EnumIndexedArray") {
    auto arr = test_array_t{};
    REQUIRE(arr[index_enum_e::first] == 0);
    arr[index_enum_e::first] = 10;
    REQUIRE(std::as_const(arr)[index_enum_e::first] == 10);
}

TEST_CASE("EnumIndexedArray::enumerate() enumerates the array") {
    auto arr = test_array_t{0, 1, 2};
    for (const auto [e, val] : arr.enumerate()) {
        STATIC_REQUIRE(std::same_as<decltype(e), const index_enum_e>);
        STATIC_REQUIRE(std::same_as<decltype(val), int &>);
        REQUIRE(std::to_underlying(e) == val);
    }

#if __cpp_lib_ranges_as_rvalue >= 202207L
    for (const auto [e, val] : arr.enumerate() | std::views::as_rvalue) {
        STATIC_REQUIRE(std::same_as<decltype(e), const index_enum_e>);
        STATIC_REQUIRE(std::same_as<decltype(val), int &&>);
        REQUIRE(std::to_underlying(e) == val);
    }
#endif
}

} // namespace reflex::tests

// the following two tests are outside of namespace reflex to test the argument dependent lookup of get()

TEST_CASE("get<E>(EnumIndexedArray) returns forwarded references") {
    using reflex::tests::index_enum_e;
    using array_t = reflex::EnumIndexedArray<int, index_enum_e>;
    auto arr = array_t{0, 1, 2};
    STATIC_REQUIRE(std::same_as<decltype(get<index_enum_e::second>(arr)), int &>);
    STATIC_REQUIRE(std::same_as<decltype(get<index_enum_e::second>(std::move(arr))), int &&>);
    STATIC_REQUIRE(std::same_as<decltype(get<index_enum_e::second>(std::as_const(arr))), const int &>);
    STATIC_REQUIRE(std::same_as<decltype(get<index_enum_e::second>(std::move(std::as_const(arr)))), const int &&>);
    REQUIRE(get<index_enum_e::second>(arr) == 1);
}

TEST_CASE("get<I>(EnumIndexedArray) returns forwarded references") {
    using array_t = reflex::EnumIndexedArray<int, reflex::tests::index_enum_e>;
    auto arr = array_t{0, 1, 2};
    STATIC_REQUIRE(std::same_as<decltype(get<0>(arr)), int &>);
    STATIC_REQUIRE(std::same_as<decltype(get<0>(std::move(arr))), int &&>);
    STATIC_REQUIRE(std::same_as<decltype(get<0>(std::as_const(arr))), const int &>);
    STATIC_REQUIRE(std::same_as<decltype(get<0>(std::move(std::as_const(arr)))), const int &&>);
// MSVC bug: https://developercommunity.visualstudio.com/t/Overload-resolution-fails-for-enum-non-t/10398088
#if !defined _MSC_VER || defined __clang__
    REQUIRE(get<1>(arr) == 1);
#endif
}

namespace reflex::tests {

TEST_CASE("EnumIndexedArray can be used in a structured binding") {
    auto arr = EnumIndexedArray<float, index_enum_e>{};
    const auto &[first, second, third] = arr;
    STATIC_REQUIRE(std::same_as<decltype(first), const float>);
}

} // namespace reflex::tests
