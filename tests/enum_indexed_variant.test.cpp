// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <catch2/catch_test_macros.hpp>

#include <reflex/enum_indexed_variant.hpp>

#include "test_types.hpp"

namespace reflex::tests {

namespace {

template<index_enum_e Value>
struct variant_alternative_for_t {
    static constexpr auto value = Value;

    friend constexpr bool operator==(variant_alternative_for_t lhs, variant_alternative_for_t rhs) noexcept = default;
};

using test_variant_t = make_enum_indexed_variant_t<index_enum_e, variant_alternative_for_t>;

} // namespace

TEST_CASE("Check EnumIndexedVariant properties") {
    STATIC_REQUIRE(std::is_default_constructible_v<test_variant_t>);

    auto v = test_variant_t{};
    REQUIRE(v.index() == index_enum_e::first);
}

TEST_CASE("EnumIndexedVariant::emplace<T>(...) initializes T") {
    auto v = test_variant_t{};
    v.emplace<variant_alternative_for_t<index_enum_e::second>>();
    REQUIRE(v.index() == index_enum_e::second);
}

TEST_CASE("EnumIndexedVariant::emplace<E>(...) initializes MakeT<E>") {
    auto v = test_variant_t{};
    v.emplace<index_enum_e::second>();
    REQUIRE(v.index() == index_enum_e::second);
}

TEST_CASE("visit(EnumIndexedVariant)") {
    auto v = test_variant_t{in_place_enum<index_enum_e::third>};
    const auto e = visit(
            []<typename T>(T &&val) {
                STATIC_REQUIRE(std::same_as<decltype(val), variant_alternative_for_t<index_enum_e::first> &&> ||
                               std::same_as<decltype(val), variant_alternative_for_t<index_enum_e::second> &&> ||
                               std::same_as<decltype(val), variant_alternative_for_t<index_enum_e::third> &&>);
                return T::value;
            },
            std::move(v));
    REQUIRE(e == index_enum_e::third);
}

TEST_CASE("holds_alternative<T>(EnumIndexedVariant)") {
    auto v = test_variant_t{in_place_enum<index_enum_e::first>};
    REQUIRE(holds_alternative<variant_alternative_for_t<index_enum_e::first>>(v));
}

TEST_CASE("get<E>(EnumIndexedArray)") {
    auto v = test_variant_t{in_place_enum<index_enum_e::second>};
    STATIC_REQUIRE(std::same_as<decltype(get<index_enum_e::second>(std::move(v))),
                                variant_alternative_for_t<index_enum_e::second> &&>);
    REQUIRE(get<index_enum_e::second>(v) == variant_alternative_for_t<index_enum_e::second>{});
    REQUIRE_THROWS_AS(get<index_enum_e::first>(v), std::bad_variant_access);
}

TEST_CASE("get<T>(EnumIndexedVariant)") {
    auto v = test_variant_t{in_place_enum<index_enum_e::third>};
    STATIC_REQUIRE(std::same_as<decltype(get<variant_alternative_for_t<index_enum_e::third>>(std::move(v))),
                                variant_alternative_for_t<index_enum_e::third> &&>);
    REQUIRE(get<variant_alternative_for_t<index_enum_e::third>>(v) == variant_alternative_for_t<index_enum_e::third>{});
    REQUIRE_THROWS_AS(get<variant_alternative_for_t<index_enum_e::second>>(v), std::bad_variant_access);
}

TEST_CASE("get_if<E>(EnumIndexedVariant)") {
    auto v = test_variant_t{in_place_enum<index_enum_e::first>};
    auto *const null_first = get_if<index_enum_e::first>(static_cast<test_variant_t *>(nullptr));
    REQUIRE(null_first == nullptr);
    auto *const first = get_if<index_enum_e::first>(&v);
    STATIC_REQUIRE(std::same_as<decltype(first), variant_alternative_for_t<index_enum_e::first> *const>);
    REQUIRE(first == &get<index_enum_e::first>(v));
    auto *const second = get_if<index_enum_e::second>(&std::as_const(v));
    STATIC_REQUIRE(std::same_as<decltype(second), const variant_alternative_for_t<index_enum_e::second> *const>);
    REQUIRE(second == nullptr);
}

TEST_CASE("get_if<T>(EnumIndexedVariant)") {
    auto v = test_variant_t{in_place_enum<index_enum_e::second>};
    REQUIRE(get_if<variant_alternative_for_t<index_enum_e::second>>(&v) ==
            &get<variant_alternative_for_t<index_enum_e::second>>(v));
}

} // namespace reflex::tests
