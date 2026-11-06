// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string_view>

#include <reflex/meta.hpp>
#include <reflex/tuple.hpp>

namespace reflex {

template<reflected_enum E>
[[nodiscard]] constexpr std::optional<std::string_view> enumerator_name(const E e) noexcept {
    const std::string_view *result;
    const auto check_enumerator = [&](const E value, const std::string_view *const name) {
        if (e == value) {
            result = name;
            return true;
        }
        return false;
    };
    const auto found = reflex::apply(
            [&](const auto &...info) { return (... || check_enumerator(info.value, &info.name)); },
            meta<E>::enumerators);
    if (!found) [[unlikely]] {
        return std::nullopt;
    }
    return *result;
}

template<reflected_enum E>
[[nodiscard]] constexpr std::string_view enum_to_string(const E e) noexcept {
    return enumerator_name(e).value_or("<unknown>");
}

} // namespace reflex