// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <atomic>
#include <concepts>
#include <memory_resource>
#include <type_traits>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include <reflex/enum_indexed_tuple.hpp>

#include "test_types.hpp"

namespace reflex::tests {

namespace {

class NonDefaultConstructable {
    NonDefaultConstructable(int);
};

template<index_enum_e Value>
struct tuple_element_for_t {
    static constexpr auto value = Value;
};

using test_tuple_t = make_enum_indexed_tuple_t<index_enum_e, tuple_element_for_t>;

} // namespace

TEST_CASE("Check EnumIndexedTuple's basic properties") {
    STATIC_REQUIRE(std::is_default_constructible_v<test_tuple_t>);
    STATIC_REQUIRE(std::is_copy_constructible_v<test_tuple_t>);
    STATIC_REQUIRE(std::is_move_constructible_v<test_tuple_t>);
    STATIC_REQUIRE(std::is_constructible_v<test_tuple_t,
                                           tuple_element_for_t<index_enum_e::first>,
                                           tuple_element_for_t<index_enum_e::second>,
                                           tuple_element_for_t<index_enum_e::third>>);

    STATIC_REQUIRE(std::is_copy_assignable_v<test_tuple_t>);
    STATIC_REQUIRE(std::is_move_assignable_v<test_tuple_t>);
}

TEST_CASE("EnumIndexedTuple is default-constructible if all it's elements are") {
    STATIC_REQUIRE(std::default_initializable<reflex::EnumIndexedTuple<index_enum_e, int, float, std::string>>);
    STATIC_REQUIRE(!std::default_initializable<
                   reflex::EnumIndexedTuple<index_enum_e, int, NonDefaultConstructable, std::string>>);
}

TEST_CASE("EnumIndexedTuple is directly constructible from it's elements' types") {
    STATIC_REQUIRE(std::constructible_from<reflex::EnumIndexedTuple<index_enum_e, int, std::vector<float>, std::string>,
                                           const int &,
                                           const std::vector<float> &,
                                           const std::string &>);
    STATIC_REQUIRE(!std::constructible_from<reflex::EnumIndexedTuple<index_enum_e, int, std::atomic<int>, std::string>,
                                            const int &,
                                            const std::atomic<int> &,
                                            const std::string &>);
}

TEST_CASE("EnumIndexedTuple is constructible with per-element conversions") {
    STATIC_REQUIRE(std::constructible_from<reflex::EnumIndexedTuple<index_enum_e, int, std::vector<float>, std::string>,
                                           short,
                                           std::vector<float>,
                                           const char *>);
    STATIC_REQUIRE(
            !std::constructible_from<reflex::EnumIndexedTuple<index_enum_e, int, std::vector<float>, std::string>,
                                     int,
                                     std::vector<float>,
                                     bool>);
}

TEST_CASE("EnumIndexedTuple is copy-constructible from another specialization with the same enum") {
    STATIC_REQUIRE(std::constructible_from<
                   reflex::EnumIndexedTuple<index_enum_e, int, std::vector<float>, std::string>,
                   const reflex::EnumIndexedTuple<index_enum_e, int, const std::vector<float> &, std::string_view> &>);
    STATIC_REQUIRE(!std::constructible_from<
                   reflex::EnumIndexedTuple<index_enum_e, int, std::vector<float>, std::string>,
                   const reflex::EnumIndexedTuple<unordered_index_enum_e, int, std::vector<float>, std::string> &>);
}

TEST_CASE("EnumIndexedTuple is move-constructible from another specialization with the same enum") {
    STATIC_REQUIRE(std::constructible_from<
                   reflex::EnumIndexedTuple<index_enum_e, int, std::vector<float>, std::string>,
                   reflex::EnumIndexedTuple<index_enum_e, short, std::vector<float> &&, std::string> &&>);
    STATIC_REQUIRE(!std::constructible_from<
                   reflex::EnumIndexedTuple<index_enum_e, int, std::vector<float>, std::string>,
                   reflex::EnumIndexedTuple<unordered_index_enum_e, int, std::vector<float>, std::string> &&>);
}

TEST_CASE("EnumIndexedTuple supports uses-allocator construction") {
    auto external_resource = std::pmr::monotonic_buffer_resource{};
    auto tuple_resource = std::pmr::monotonic_buffer_resource{};
    auto vec = std::pmr::vector<int>({1, 2, 3}, &external_resource);
    auto tuple = reflex::EnumIndexedTuple<index_enum_e, int, std::pmr::vector<int>, std::string>(
            std::allocator_arg,
            std::pmr::polymorphic_allocator<>{&tuple_resource},
            42,
            std::move(vec),
            "Hello");
    const auto &[first, second, third] = tuple;
    REQUIRE(first == 42);
    REQUIRE(std::ranges::equal(second, std::array{1, 2, 3}));
    REQUIRE(second.get_allocator().resource() == &tuple_resource);
    REQUIRE(third == "Hello");
}

} // namespace reflex::tests

// The following two tests are in the global namespace to test the argument dependent lookup of get<...>(t).

TEST_CASE("get<E>(EnumIndexedTuple) gets the corresponding element") {
    using reflex::tests::index_enum_e;
    using reflex::tests::tuple_element_for_t;
    auto t = reflex::tests::test_tuple_t{};
    STATIC_REQUIRE(std::same_as<decltype(get<index_enum_e::first>(t)), tuple_element_for_t<index_enum_e::first> &>);
    STATIC_REQUIRE(std::same_as<decltype(get<index_enum_e::second>(std::as_const(t))),
                                const tuple_element_for_t<index_enum_e::second> &>);
    STATIC_REQUIRE(std::same_as<decltype(get<index_enum_e::third>(std::move(t))),
                                tuple_element_for_t<index_enum_e::third> &&>);
    STATIC_REQUIRE(std::same_as<decltype(get<index_enum_e::third>(std::move(std::as_const(t)))),
                                const tuple_element_for_t<index_enum_e::third> &&>);
}

// MSVC bug: https://developercommunity.visualstudio.com/t/Overload-resolution-fails-for-enum-non-t/10398088
#if !defined _MSC_VER || defined __clang__
TEST_CASE("get<std::size_t>(EnumIndexedTuple) gets the corresponding element") {
    using reflex::tests::index_enum_e;
    using reflex::tests::tuple_element_for_t;
    auto t = reflex::tests::test_tuple_t{};
    STATIC_REQUIRE(std::same_as<decltype(get<0>(t)), tuple_element_for_t<index_enum_e::first> &>);
    STATIC_REQUIRE(std::same_as<decltype(get<1>(std::as_const(t))), const tuple_element_for_t<index_enum_e::second> &>);
    STATIC_REQUIRE(std::same_as<decltype(get<2>(std::move(t))), tuple_element_for_t<index_enum_e::third> &&>);
    STATIC_REQUIRE(std::same_as<decltype(get<2>(std::move(std::as_const(t)))),
                                const tuple_element_for_t<index_enum_e::third> &&>);
}
#endif

namespace reflex::tests {

TEST_CASE("get<T>(EnumIndexedTuple)") {
    auto t = test_tuple_t{};
    STATIC_REQUIRE(std::same_as<decltype(get<tuple_element_for_t<index_enum_e::second>>(std::move(std::as_const(t)))),
                                const tuple_element_for_t<index_enum_e::second> &&>);
}

// MSVC bug: https://developercommunity.visualstudio.com/t/Overload-resolution-fails-for-enum-non-t/10398088
#if !defined _MSC_VER || defined __clang__
TEST_CASE("EnumIndexedTuple can be used in a structured binding") {
    auto t = test_tuple_t{};
    auto &[first, second, third] = t;
    STATIC_REQUIRE(std::same_as<decltype(first), tuple_element_for_t<index_enum_e::first>>);
    STATIC_REQUIRE(std::same_as<decltype(second), tuple_element_for_t<index_enum_e::second>>);
    STATIC_REQUIRE(std::same_as<decltype(third), tuple_element_for_t<index_enum_e::third>>);
}
#endif

} // namespace reflex::tests
