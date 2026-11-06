// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace reflex::tests {

enum class [[reflex::reflect]] index_enum_e {
    first,
    second,
    third,
};

enum class [[reflex::reflect]] unordered_index_enum_e {
    second = 1,
    third = 2,
    first = 0,
};

struct uvec2_s {
    std::uint32_t x;
    std::uint32_t y;
    uvec2_s(const std::uint32_t a, const std::uint32_t b) : x(a), y(b) {}
};

struct [[reflex::reflect]] S {
    uvec2_s base = {2, 3};
};

template<index_enum_e Id>
struct [[reflex::reflect]] structure_s {
    static constexpr auto I = Id;
};

using first_structure_s [[reflex::reflect]] = structure_s<index_enum_e::first>;

[[reflex::reflect]] inline first_structure_s get_enum_struct() { return {}; }

template<typename T, bool is_optional>
struct maybe_optional;

template<typename T>
struct maybe_optional<T, true> {
    using type = std::optional<T>;
};

template<typename T>
struct maybe_optional<T, false> {
    using type = T;
};

template<typename T, bool is_optional>
using maybe_optional_t = typename maybe_optional<T, is_optional>::type;

template<bool use_optional>
struct [[reflex::reflect]] struct_w_optional_s {
    maybe_optional_t<std::vector<int>, use_optional> value{};
};

using with_optional [[reflex::reflect]] = struct_w_optional_s<true>;
using without_optional [[reflex::reflect]] = struct_w_optional_s<false>;

template<bool use_optional>
struct [[reflex::reflect]] nested_dependent_s {
    struct_w_optional_s<use_optional> value{};
};

using nested_dependent_s_false [[reflex::reflect]] = nested_dependent_s<false>;

[[reflex::reflect]] inline nested_dependent_s<false> dependent_result_function() { return {}; }


template<bool use_optional>
struct [[reflex::reflect]] base_t {
    int color{};
    void do_nothing() {}
};

struct [[reflex::reflect]] inherited_s : base_t<true> {};

} // namespace reflex::tests

#ifndef __REFLEX_TOOL__
    #include "test_types.meta.inc"
#endif
