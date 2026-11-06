// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <concepts>
#include <ranges>
#include <type_traits>

namespace reflex::ranges::detail {

template<bool Const, class T>
using maybe_const = std::conditional_t<Const, const T, T>;

template<class Range>
concept simple_view = std::ranges::view<Range> && std::ranges::range<const Range> &&
                      std::same_as<std::ranges::iterator_t<Range>, std::ranges::iterator_t<const Range>> &&
                      std::same_as<std::ranges::sentinel_t<Range>, std::ranges::sentinel_t<const Range>>;

} // namespace reflex::ranges::detail
