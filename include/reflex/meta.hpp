// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string_view>
#include <type_traits>
#include <utility>

#include <reflex/detail/config.hpp>
#include <reflex/tuple.hpp>

namespace reflex {

template<typename E>
concept enumeration = std::is_enum_v<E>;

template<enumeration E, typename... Properties>
struct enumerator_info {
    REFLEX_NO_UNIQUE_ADDRESS E value;
    REFLEX_NO_UNIQUE_ADDRESS std::string_view name;
    REFLEX_NO_UNIQUE_ADDRESS tuple<Properties...> properties;
};

template<enumeration E, typename... Properties>
explicit enumerator_info(E value, std::string_view name, tuple<Properties...> properties)
        -> enumerator_info<E, Properties...>;

template<typename Class, typename T, typename... Properties>
struct data_member_info {
    using type = T;

    REFLEX_NO_UNIQUE_ADDRESS T Class::*ptr;
    REFLEX_NO_UNIQUE_ADDRESS std::string_view name;
    REFLEX_NO_UNIQUE_ADDRESS std::remove_cv_t<T> (*initializer)();
    REFLEX_NO_UNIQUE_ADDRESS tuple<Properties...> properties;
};

template<typename Class, typename T, typename... Properties>
explicit data_member_info(T Class::*ptr,
                          std::string_view name,
                          std::convertible_to<std::remove_cv_t<T> (*)()> auto initializer,
                          tuple<Properties...> properties) -> data_member_info<Class, T, Properties...>;


template<typename T, typename... Properties>
struct function_parameter_info {
    using type = T;

    REFLEX_NO_UNIQUE_ADDRESS std::string_view name;
    REFLEX_NO_UNIQUE_ADDRESS std::remove_cv_t<T> (*default_value)();
    REFLEX_NO_UNIQUE_ADDRESS tuple<Properties...> properties;
};


template<typename T>
struct meta
#ifndef __REFLEX_TOOL__
{
}
#endif
;


template<typename T>
concept reflected_type = requires { meta<T>::properties; };

template<typename T>
concept reflected_enum = reflected_type<T> && std::is_enum_v<T>;

template<typename T>
concept reflected_class = reflected_type<T> && std::is_class_v<T>;


template<typename Property, typename Properties>
[[nodiscard]] constexpr bool has_property() noexcept {
    return []<typename... P>(const tuple<P...> * /* properties */) {
        return (... || std::is_same_v<Property, P>);
    }(static_cast<const Properties *>(nullptr));
}


template<auto &Function>
struct func_meta
#ifndef __REFLEX_TOOL__
{
}
#endif
;


template<auto MemberFunctionPtr>
struct member_func_meta
#ifndef __REFLEX_TOOL__
{
}
#endif
;

template<reflected_enum E>
inline constexpr auto enumerator_count = std::tuple_size_v<decltype(meta<E>::enumerators)>;

} // namespace reflex
