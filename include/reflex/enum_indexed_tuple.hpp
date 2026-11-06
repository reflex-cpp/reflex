// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>

#include <reflex/enum_indexing.hpp>

namespace reflex {

template<typename E, typename... Types>
class EnumIndexedTuple;

namespace detail {

template<typename E, template<E> typename MakeT, std::size_t... I>
EnumIndexedTuple<E, MakeT<static_cast<E>(I)>...> make_enum_indexed_tuple_impl(std::index_sequence<I...> indices);

template<typename T, typename... Args>
inline constexpr bool is_copy_list_initializable_from_v = requires(void (*fn)(T), Args &&...args) {
    fn({std::forward<Args>(args)...});
};

template<typename T>
inline constexpr bool is_enum_indexed_tuple_v = false;

template<typename E, typename... Types>
inline constexpr bool is_enum_indexed_tuple_v<EnumIndexedTuple<E, Types...>> = true;

} // namespace detail

template<typename E, typename... Types>
class EnumIndexedTuple {
    using traits_t = enum_indexing_traits<E>;
    static_assert(sizeof...(Types) == traits_t::count);
    using tuple_t = std::tuple<Types...>;

    template<std::size_t I, typename... UTypes>
    using pack_at = boost::mp11::mp_at_c<boost::mp11::mp_list<UTypes...>, I>;

    template<std::size_t I>
    using types_at = pack_at<I, Types...>;

    template<typename... UTypes>
    [[nodiscard]] static consteval bool can_convert_initialize_from() {
        if constexpr (sizeof...(UTypes) != sizeof...(Types) || sizeof...(Types) == 0) {
            return false;
        } else if constexpr (!(... && std::is_constructible_v<Types, UTypes>)) {
            return false;
        } else {
            using D = std::remove_cvref_t<pack_at<0, UTypes...>>;
            if constexpr (sizeof...(Types) == 1) {
                return !detail::is_enum_indexed_tuple_v<D>;
            } else if constexpr (sizeof...(Types) == 2 || sizeof...(Types) == 3) {
                return std::is_same_v<types_at<0>, std::allocator_arg_t> || !std::is_same_v<D, std::allocator_arg_t>;
            } else {
                return true;
            }
        }
    }

public:
    constexpr explicit(!detail::is_copy_list_initializable_from_v<tuple_t>) EnumIndexedTuple()
        requires std::is_default_constructible_v<tuple_t>
    = default;

    constexpr explicit(!(... && std::is_convertible_v<const Types &, Types>)) EnumIndexedTuple(const Types &...args) //
            noexcept((... && std::is_nothrow_copy_constructible_v<Types>)) //
        requires(sizeof...(Types) != 0 && (... && std::is_copy_constructible_v<Types>))
        : m_tuple(args...) {}

    // same as the converting constructor of std::tuple, except checks U0 for EnumIndexedTuple instead of std::tuple
    template<typename... UTypes>
        requires(can_convert_initialize_from<UTypes...>())
    constexpr explicit(!(... && std::is_convertible_v<UTypes, Types>)) EnumIndexedTuple(UTypes &&...args) //
            noexcept((... && std::is_nothrow_constructible_v<Types, UTypes>))
        : m_tuple(std::forward<UTypes>(args)...) {}

    template<typename... UTypes>
        requires(std::is_constructible_v<tuple_t, const std::tuple<UTypes...> &>)
    constexpr explicit(!std::is_convertible_v<const std::tuple<UTypes...> &, tuple_t>)
            EnumIndexedTuple(const EnumIndexedTuple<E, UTypes...> &other) //
            noexcept((... && std::is_nothrow_constructible_v<Types, const UTypes &>))
        : m_tuple(other.m_tuple) {}

    template<typename... UTypes>
        requires std::is_constructible_v<tuple_t, std::tuple<UTypes...>>
    constexpr explicit(!std::is_convertible_v<std::tuple<UTypes...>, tuple_t>)
            EnumIndexedTuple(EnumIndexedTuple<E, UTypes...> &&other) //
            noexcept((... && std::is_nothrow_constructible_v<Types, UTypes>))
        : m_tuple(std::move(other)) {}

    constexpr EnumIndexedTuple(const EnumIndexedTuple &other) = default;

    constexpr EnumIndexedTuple(EnumIndexedTuple &&other)
        requires std::is_move_constructible_v<tuple_t>
    = default;


    // The following constructors are the same as above, except each element is created by uses-allocator construction.

    template<typename Alloc>
        requires std::constructible_from<tuple_t, std::allocator_arg_t &, const Alloc &>
    constexpr explicit(!detail::is_copy_list_initializable_from_v<tuple_t>)
            EnumIndexedTuple(std::allocator_arg_t tag, const Alloc &a)
        : m_tuple(tag, a) {}

    template<typename Alloc>
        requires(sizeof...(Types) != 0 && (... && std::is_copy_constructible_v<Types>))
    constexpr explicit(!(... && std::is_convertible_v<const Types &, Types>))
            EnumIndexedTuple(std::allocator_arg_t tag, const Alloc &a, const Types &...args)
        : m_tuple(tag, a, args...) {}

    template<typename Alloc, typename... UTypes>
        requires(can_convert_initialize_from<UTypes...>())
    constexpr explicit(!(... && std::is_convertible_v<UTypes, Types>))
            EnumIndexedTuple(std::allocator_arg_t tag, const Alloc &a, UTypes &&...args)
        : m_tuple(tag, a, std::forward<UTypes>(args)...) {}

    template<typename Alloc, typename... UTypes>
        requires(std::is_constructible_v<tuple_t, std::allocator_arg_t &, const Alloc &, const std::tuple<UTypes...> &>)
    constexpr explicit(!std::is_convertible_v<const std::tuple<UTypes...> &, tuple_t>)
            EnumIndexedTuple(std::allocator_arg_t tag, const Alloc &a, const EnumIndexedTuple<E, UTypes...> &other)
        : m_tuple(tag, a, other.m_tuple) {}

    template<typename Alloc, typename... UTypes>
        requires std::is_constructible_v<tuple_t, std::allocator_arg_t &, const Alloc &, std::tuple<UTypes...>>
    constexpr explicit(!std::is_convertible_v<std::tuple<UTypes...>, tuple_t>)
            EnumIndexedTuple(std::allocator_arg_t tag, const Alloc &a, EnumIndexedTuple<E, UTypes...> &&other)
        : m_tuple(tag, a, std::move(other).m_tuple) {}

    template<typename Alloc>
        requires(... && std::is_copy_constructible_v<Types>)
    constexpr EnumIndexedTuple(std::allocator_arg_t tag, const Alloc &a, const EnumIndexedTuple &other)
        : m_tuple(tag, a, other.m_tuple) {}

    template<typename Alloc>
        requires(!(... && std::is_copy_constructible_v<Types>))
    EnumIndexedTuple(std::allocator_arg_t tag, const Alloc &a, const EnumIndexedTuple &other) = delete;

    template<typename Alloc>
        requires(... && std::is_move_constructible_v<Types>)
    constexpr EnumIndexedTuple(std::allocator_arg_t tag, const Alloc &a, EnumIndexedTuple &&other)
        : m_tuple(tag, a, std::move(other).m_tuple) {}


    constexpr EnumIndexedTuple &operator=(const EnumIndexedTuple &other) = default;

    constexpr EnumIndexedTuple &operator=(EnumIndexedTuple &&other)
        requires(std::is_move_assignable_v<tuple_t>)
    = default;

    template<typename... UTypes>
        requires(std::is_assignable_v<tuple_t &, const std::tuple<UTypes...> &>)
    constexpr EnumIndexedTuple &operator=(const EnumIndexedTuple<E, UTypes...> &other) //
            noexcept(std::is_nothrow_assignable_v<tuple_t &, const std::tuple<UTypes...> &>) {
        m_tuple = other.m_tuple;
        return *this;
    }

    template<typename... UTypes>
        requires std::is_assignable_v<tuple_t &, std::tuple<UTypes...>>
    constexpr EnumIndexedTuple &operator=(EnumIndexedTuple<E, UTypes...> &&other) //
            noexcept(std::is_nothrow_assignable_v<tuple_t &, std::tuple<UTypes...>>) {
        m_tuple = std::move(other).m_tuple;
        return *this;
    }


    template<E Index, typename Self>
        requires std::is_same_v<std::remove_cvref_t<Self>, EnumIndexedTuple>
    [[nodiscard]] friend constexpr decltype(auto) get(Self &&self) noexcept {
        constexpr auto index = static_cast<std::size_t>(Index);
        static_assert(index < sizeof...(Types));
        return std::get<index>(std::forward<Self>(self).m_tuple);
    }

    template<std::size_t Index, typename Self>
        requires std::is_same_v<std::remove_cvref_t<Self>, EnumIndexedTuple>
    [[nodiscard]] friend constexpr decltype(auto) get(Self &&self) noexcept {
        static_assert(Index < sizeof...(Types));
        return std::get<Index>(std::forward<Self>(self).m_tuple);
    }

    template<typename T, typename Self>
        requires std::is_same_v<std::remove_cvref_t<Self>, EnumIndexedTuple>
    [[nodiscard]] friend constexpr decltype(auto) get(Self &&self) noexcept {
        return std::get<T>(std::forward<Self>(self).m_tuple);
    }

    [[nodiscard]] friend constexpr bool operator==(const EnumIndexedTuple &lhs, const EnumIndexedTuple &rhs) = default;
    [[nodiscard]] friend constexpr auto operator<=>(const EnumIndexedTuple &lhs, const EnumIndexedTuple &rhs) = default;

    friend constexpr void swap(EnumIndexedTuple &lhs, EnumIndexedTuple &rhs) //
            noexcept(std::is_nothrow_swappable_v<tuple_t>) {
        lhs.m_tuple.swap(rhs.m_tuple);
    }

private:
    tuple_t m_tuple;

    template<typename EE, typename... UTypes>
    friend class EnumIndexedTuple;
};

template<typename E, template<E> typename MakeT>
using make_enum_indexed_tuple_t = decltype(detail::make_enum_indexed_tuple_impl<E, MakeT>(
        std::make_index_sequence<enum_indexing_traits<E>::count>{}));

} // namespace reflex

template<typename E, typename... Types>
struct std::tuple_size<reflex::EnumIndexedTuple<E, Types...>> : integral_constant<size_t, sizeof...(Types)> {};

template<std::size_t I, typename E, typename... Types>
struct std::tuple_element<I, reflex::EnumIndexedTuple<E, Types...>> {
    static_assert(I < sizeof...(Types));
    using type = boost::mp11::mp_at_c<boost::mp11::mp_list<Types...>, I>;
};

template<typename E, typename... Types, typename Alloc>
struct std::uses_allocator<reflex::EnumIndexedTuple<E, Types...>, Alloc> : true_type {};
