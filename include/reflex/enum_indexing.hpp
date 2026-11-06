// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <limits>
#include <ranges>
#include <type_traits>
#include <utility>

#include <reflex/meta.hpp>
#include <reflex/tuple.hpp>

namespace reflex {

namespace detail {

template<typename E>
[[nodiscard]] consteval bool do_enumerators_form_index_sequence() noexcept {
    constexpr auto count = std::tuple_size_v<decltype(meta<E>::enumerators)>;
    auto enumerators = []<std::size_t... Index>(std::index_sequence<Index...> /*indices*/) {
        return std::array<E, count>{reflex::get<Index>(meta<E>::enumerators).value...};
    }(std::make_index_sequence<count>{});
    constexpr auto to_underlying = [](const E e) { return std::to_underlying(e); };
    std::ranges::sort(enumerators, std::ranges::less{}, to_underlying);
    // the first enumerator, if present, must be 0
    if (!enumerators.empty() && std::to_underlying(enumerators.front()) != 0) {
        return false;
    }
    // all enumerators must be in the range [0..ptrdiff_t::max) (0 is already checked above)
    constexpr auto ptrdiff_max = std::numeric_limits<std::ptrdiff_t>::max();
    if (std::ranges::any_of(
                enumerators,
                [](const auto val) { return std::cmp_greater(val, ptrdiff_max); },
                to_underlying)) {
        return false;
    }
    // enumerators must form an index sequence
    const auto first_nonsequential = std::ranges::adjacent_find(
            enumerators,
            [](const auto lhs, const auto rhs) { return rhs <= lhs || rhs != (lhs + 1); },
            to_underlying);
    return first_nonsequential == enumerators.end();
}

} // namespace detail

template<typename E>
struct enum_indexing_traits {
    static_assert(reflected_enum<E>, "E must be an enumeration, reflected using [[reflex::reflect]]");
    static_assert(detail::do_enumerators_form_index_sequence<E>(),
                  "The values of E's enumerators must form a continuous index sequence starting with 0");

    static constexpr std::size_t count = enumerator_count<E>;
};

/**
 * \brief Creates a view of the enumerators of \c E in indexing order.
 * @tparam E A reflected enumeration type, whose enumerators form an index sequence.
 * @return A sequence of E's enumerators.
 */
template<typename E>
[[nodiscard]] consteval auto index_enumerators_of() noexcept {
    using traits_t = enum_indexing_traits<E>;
    // Avoid using std::size_t if possible because it causes iota's difference_type to be 128 bits on 64-bit systems.
    using iota_base_t =
            std::conditional_t<(traits_t::count < std::numeric_limits<unsigned>::max()), unsigned, std::size_t>;
    return std::views::iota(iota_base_t{0}, static_cast<iota_base_t>(traits_t::count)) |
           std::views::transform([](const iota_base_t index) noexcept -> E { return static_cast<E>(index); });
}

template<typename E>
constexpr void enum_indexed_for_each(auto lambda) {
    [&lambda]<std::size_t... I>(std::index_sequence<I...> /* indices */) {
        (..., lambda.template operator()<static_cast<E>(I)>());
    }(std::make_index_sequence<enum_indexing_traits<E>::count>{});
}

} // namespace reflex
