// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <boost/mp11/list.hpp>

namespace reflex {

namespace detail {

template<bool IsNoexcept, typename R, typename... Args>
struct function_traits_impl {
    static constexpr auto is_noexcept = IsNoexcept;
    using result_type = R;
    using argument_types = boost::mp11::mp_list<Args...>;
};

template<typename R, typename... Args>
auto get_function_traits(R (&)(Args...) noexcept(true), int /* higher priority */)
        -> function_traits_impl<true, R, Args...>;

template<typename R, typename... Args>
auto get_function_traits(R (&)(Args...) noexcept(false), ... /* lower priority */)
        -> function_traits_impl<false, R, Args...>;

template<bool Const, typename Class, typename ReturnType, typename... Args>
struct member_function_traits_impl;

template<typename Class, typename ReturnType, typename... Args>
struct member_function_traits_impl<false, Class, ReturnType, Args...> {
    using class_type = Class;
    using self_type = Class;
    using result_type = ReturnType;
    using argument_types = boost::mp11::mp_list<Args...>;

    ReturnType (Class::*ptr)(Args...);
};

template<typename Class, typename ReturnType, typename... Args>
struct member_function_traits_impl<true, Class, ReturnType, Args...> {
    using class_type = Class;
    using self_type = const Class;
    using result_type = ReturnType;
    using argument_types = boost::mp11::mp_list<Args...>;

    ReturnType (Class::*ptr)(Args...) const;
};

template<typename Class, typename ReturnType, typename... Args>
explicit member_function_traits_impl(ReturnType (Class::*ptr)(Args...))
        -> member_function_traits_impl<false, Class, ReturnType, Args...>;


template<typename Class, typename ReturnType, typename... Args>
explicit member_function_traits_impl(ReturnType (Class::*ptr)(Args...) const)
        -> member_function_traits_impl<true, Class, ReturnType, Args...>;

} // namespace detail

template<auto &Function>
using function_traits = decltype(detail::get_function_traits(Function, 0));

template<auto MemberFunctionPtr>
using member_function_traits = decltype(detail::member_function_traits_impl{MemberFunctionPtr});

} // namespace reflex
