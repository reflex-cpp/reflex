// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace reflex {

namespace detail {

#if defined __clang__ || defined __GNUC__
    #define REFLEX_DETAIL_PRETTY_FUNCTION __PRETTY_FUNCTION__
inline constexpr char type_name_prefix = '=';
inline constexpr char type_name_suffix = ']';
#elif defined _MSC_VER
    #define REFLEX_DETAIL_PRETTY_FUNCTION __FUNCSIG__
inline constexpr char type_name_prefix = '<';
inline constexpr char type_name_suffix = '>';
#else
    #error reflex::type_name is not implemented for this compiler
#endif

template<typename T>
[[nodiscard]] consteval auto get_type_name() noexcept { // the auto return type is required for MSVC
    const auto pretty_function = std::string_view{REFLEX_DETAIL_PRETTY_FUNCTION};
    const auto prefix_pos = pretty_function.find_first_of(type_name_prefix);
    const auto begin_pos = pretty_function.find_first_not_of(' ', prefix_pos + 1);
    const auto end_pos = pretty_function.find_last_of(type_name_suffix);
    return pretty_function.substr(begin_pos, end_pos - begin_pos);
}

#undef REFLEX_DETAIL_PRETTY_FUNCTION

template<std::size_t N>
[[nodiscard]] consteval std::array<char, N> make_char_array(const char *const str) noexcept {
    std::array<char, N> arr;
    for (auto i = 0uz; i != N; ++i) {
        arr[i] = str[i];
    }
    return arr;
}

// This array contains only the type name, excluding the get_type_name<T>() function name from the binary.
template<typename T>
inline constexpr auto type_name_arr = detail::make_char_array<detail::get_type_name<T>().size()>(
        detail::get_type_name<T>().data());

} // namespace detail

template<typename T>
inline constexpr std::string_view type_name = {detail::type_name_arr<T>.data(), detail::type_name_arr<T>.size()};

} // namespace reflex
