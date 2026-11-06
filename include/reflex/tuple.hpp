// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0 AND BSL-1.0

// The reflex::tuple implementation is largely based on KUMI (https://github.com/jfalcou/kumi).
/*
  KUMI - Compact Tuple Tools
  Copyright : KUMI Project Contributors
  SPDX-License-Identifier: BSL-1.0
*/

#pragma once

// #define REFLEX_TUPLE_USE_STD
#define REFLEX_TUPLE_OPTIMIZE_STRUCT

#ifdef REFLEX_TUPLE_USE_STD
    #undef REFLEX_TUPLE_USE_STD

    #include <cstddef>
    #include <tuple>

namespace reflex {

using std::tuple;
using std::get;
using std::apply;

} // namespace reflex
#else
    #include <cstddef>
    #include <type_traits>
    #include <utility>

    #include <reflex/detail/config.hpp>

namespace reflex {
namespace detail {

// Utilities

template<typename T0, typename... Ts>
concept all_same = (... && std::is_same_v<T0, Ts>);

    #if REFLEX_HAS_BUILTIN(__type_pack_element)
template<std::size_t I, typename... Ts>
using type_pack_element_t = __type_pack_element<I, Ts...>;
    #else
template<std::size_t, typename T>
struct indexed_type {
    using type = T;
};

template<typename, typename... Ts>
struct type_indexer;

template<std::size_t... Is, typename... Ts>
struct type_indexer<std::index_sequence<Is...>, Ts...> : indexed_type<Is, Ts>... {};

template<std::size_t I, typename T>
indexed_type<I, T> select_indexed_type(const indexed_type<I, T> &);

template<std::size_t I, typename... Ts>
using type_pack_element_t = //
        typename decltype(select_indexed_type<I>(type_indexer<std::index_sequence_for<Ts...>, Ts...>()))::type;
    #endif

template<typename T, typename... Ts>
inline constexpr std::size_t count_of_v = (... + (std::is_same_v<T, Ts> ? std::size_t{1} : std::size_t{0}));

template<typename, typename, typename...>
struct index_of_impl;

template<std::size_t... Is, typename T, typename... Ts>
struct index_of_impl<std::index_sequence<Is...>, T, Ts...>
    : std::integral_constant<std::size_t, (... + (std::is_same_v<T, Ts> ? Is : std::size_t{0}))> {
    static_assert(count_of_v<T, Ts...> > 0, "T not found within Ts...");
    static_assert(count_of_v<T, Ts...> < 2, "T must be unique within Ts...");
};

template<typename T, typename... Ts>
inline constexpr std::size_t index_of_v = index_of_impl<std::index_sequence_for<Ts...>, T, Ts...>::value;

// Generic implementation

template<std::size_t I, typename T>
struct leaf {
    REFLEX_NO_UNIQUE_ADDRESS T _value;
};

template<std::size_t I, typename T>
[[nodiscard]] constexpr T &get_leaf(leaf<I, T> &l) noexcept {
    return l._value;
}

template<std::size_t I, typename T>
[[nodiscard]] constexpr const T &get_leaf(const leaf<I, T> &l) noexcept {
    return l._value;
}

template<std::size_t I, typename T>
[[nodiscard]] constexpr T &&get_leaf(leaf<I, T> &&l) noexcept {
    return static_cast<T &&>(l._value);
}

template<std::size_t I, typename T>
[[nodiscard]] constexpr const T &&get_leaf(const leaf<I, T> &&l) noexcept {
    return static_cast<const T &&>(l._value);
}


template<typename ISeq, typename... Ts>
struct binder;

template<std::size_t... Is, typename... Ts>
struct binder<std::index_sequence<Is...>, Ts...> : leaf<Is, Ts>... {};


template<typename ISeq, typename... Ts>
struct make_binder {
    using type = binder<ISeq, Ts...>;
};

template<typename ISeq, typename... Ts>
using make_binder_t = typename make_binder<ISeq, Ts...>::type;


// Optimize for same types

template<typename T, std::size_t N>
struct array_binder {
    T _elements[N];
};

template<std::size_t... Is, typename T0, typename T1, typename... Ts>
    requires(all_same<T0, T1, Ts...> && !std::is_reference_v<T0>)
struct make_binder<std::index_sequence<Is...>, T0, T1, Ts...> {
    using type = array_binder<T0, 2 + sizeof...(Ts)>;
};

template<std::size_t I, typename T, std::size_t N>
[[nodiscard]] constexpr T &get_leaf(array_binder<T, N> &arr) noexcept {
    return arr._elements[I];
}

template<std::size_t I, typename T, std::size_t N>
[[nodiscard]] constexpr const T &get_leaf(const array_binder<T, N> &arr) noexcept {
    return arr._elements[I];
}

template<std::size_t I, typename T, std::size_t N>
[[nodiscard]] constexpr T &&get_leaf(array_binder<T, N> &&arr) noexcept {
    return std::move(arr._elements[I]);
}

template<std::size_t I, typename T, std::size_t N>
[[nodiscard]] constexpr const T &&get_leaf(const array_binder<T, N> &&arr) noexcept {
    return std::move(arr._elements[I]);
}

// clang-format off

#ifdef REFLEX_TUPLE_OPTIMIZE_STRUCT
#undef REFLEX_TUPLE_OPTIMIZE_STRUCT
// Optimize for 1-15 elements

#define MEM_I(i) \
    using _type##i = T##i; \
    REFLEX_NO_UNIQUE_ADDRESS T##i _##i;

template<typename T0>
struct binder<std::index_sequence<0>, T0> {
    MEM_I(0)
};

template<typename T0, typename T1>
struct binder<std::index_sequence<0, 1>,
              T0, T1> {
    MEM_I(0) MEM_I(1)
};

template<typename T0, typename T1, typename T2>
struct binder<std::index_sequence<0, 1, 2>,
              T0, T1, T2> {
    MEM_I(0) MEM_I(1) MEM_I(2)
};

template<typename T0, typename T1, typename T2, typename T3>
struct binder<std::index_sequence<0, 1, 2, 3>,
              T0, T1, T2, T3> {
    MEM_I(0) MEM_I(1) MEM_I(2) MEM_I(3)
};

template<typename T0, typename T1, typename T2, typename T3, typename T4>
struct binder<std::index_sequence<0, 1, 2, 3, 4>,
              T0, T1, T2, T3, T4> {
    MEM_I(0) MEM_I(1) MEM_I(2) MEM_I(3) MEM_I(4)
};

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
struct binder<std::index_sequence<0, 1, 2, 3, 4, 5>,
              T0, T1, T2, T3, T4, T5> {
    MEM_I(0) MEM_I(1) MEM_I(2) MEM_I(3) MEM_I(4) MEM_I(5)
};

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
struct binder<std::index_sequence<0, 1, 2, 3, 4, 5, 6>,
              T0, T1, T2, T3, T4, T5, T6> {
    MEM_I(0) MEM_I(1) MEM_I(2) MEM_I(3) MEM_I(4) MEM_I(5) MEM_I(6)
};

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
struct binder<std::index_sequence<0, 1, 2, 3, 4, 5, 6, 7>,
              T0, T1, T2, T3, T4, T5, T6, T7> {
    MEM_I(0) MEM_I(1) MEM_I(2) MEM_I(3) MEM_I(4) MEM_I(5) MEM_I(6) MEM_I(7)
};

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
         typename T8>
struct binder<std::index_sequence<0, 1, 2, 3, 4, 5, 6, 7, 8>,
              T0, T1, T2, T3, T4, T5, T6, T7, T8> {
    MEM_I(0) MEM_I(1) MEM_I(2) MEM_I(3) MEM_I(4) MEM_I(5) MEM_I(6) MEM_I(7)
    MEM_I(8)
};

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
         typename T8, typename T9>
struct binder<std::index_sequence<0, 1, 2, 3, 4, 5, 6, 7, 8, 9>,
              T0, T1, T2, T3, T4, T5, T6, T7, T8, T9> {
    MEM_I(0) MEM_I(1) MEM_I(2) MEM_I(3) MEM_I(4) MEM_I(5) MEM_I(6) MEM_I(7)
    MEM_I(8) MEM_I(9)
};

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
         typename T8, typename T9, typename T10>
struct binder<std::index_sequence<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10>,
              T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> {
    MEM_I(0) MEM_I(1) MEM_I(2) MEM_I(3) MEM_I(4) MEM_I(5) MEM_I(6) MEM_I(7)
    MEM_I(8) MEM_I(9) MEM_I(10)
};

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
         typename T8, typename T9, typename T10, typename T11>
struct binder<std::index_sequence<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11>,
              T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11> {
    MEM_I(0) MEM_I(1) MEM_I(2) MEM_I(3) MEM_I(4) MEM_I(5) MEM_I(6) MEM_I(7)
    MEM_I(8) MEM_I(9) MEM_I(10) MEM_I(11)
};

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
         typename T8, typename T9, typename T10, typename T11, typename T12>
struct binder<std::index_sequence<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12>,
              T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12> {
    MEM_I(0) MEM_I(1) MEM_I(2) MEM_I(3) MEM_I(4) MEM_I(5) MEM_I(6) MEM_I(7)
    MEM_I(8) MEM_I(9) MEM_I(10) MEM_I(11) MEM_I(12)
};

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
         typename T8, typename T9, typename T10, typename T11, typename T12, typename T13>
struct binder<std::index_sequence<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13>,
              T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13> {
    MEM_I(0) MEM_I(1) MEM_I(2) MEM_I(3) MEM_I(4) MEM_I(5) MEM_I(6) MEM_I(7)
    MEM_I(8) MEM_I(9) MEM_I(10) MEM_I(11) MEM_I(12) MEM_I(13)
};

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
         typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14>
struct binder<std::index_sequence<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14>,
              T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14> {
    MEM_I(0) MEM_I(1) MEM_I(2) MEM_I(3) MEM_I(4) MEM_I(5) MEM_I(6) MEM_I(7)
    MEM_I(8) MEM_I(9) MEM_I(10) MEM_I(11) MEM_I(12) MEM_I(13) MEM_I(14)
};

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
         typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15>
struct binder<std::index_sequence<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15>,
              T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15> {
    MEM_I(0) MEM_I(1) MEM_I(2) MEM_I(3) MEM_I(4) MEM_I(5) MEM_I(6) MEM_I(7)
    MEM_I(8) MEM_I(9) MEM_I(10) MEM_I(11) MEM_I(12) MEM_I(13) MEM_I(14) MEM_I(15)
};

#undef MEM_I

template<std::size_t I, typename Binder>
    requires requires { typename Binder::_type0; }
[[nodiscard]] constexpr auto &get_leaf(Binder &b) noexcept {
    #define RET_I(i) if constexpr (I == i) { return b._##i; }
    RET_I(0) RET_I(1) RET_I(2) RET_I(3) RET_I(4) RET_I(5) RET_I(6) RET_I(7)
    RET_I(8) RET_I(9) RET_I(10) RET_I(11) RET_I(12) RET_I(13) RET_I(14) RET_I(15)
    #undef RET_I
}

template<std::size_t I, typename Binder>
    requires requires { typename Binder::_type0; }
[[nodiscard]] constexpr const auto &get_leaf(const Binder &b) noexcept {
    #define RET_I(i) if constexpr (I == i) { return b._##i; }
    RET_I(0) RET_I(1) RET_I(2) RET_I(3) RET_I(4) RET_I(5) RET_I(6) RET_I(7)
    RET_I(8) RET_I(9) RET_I(10) RET_I(11) RET_I(12) RET_I(13) RET_I(14) RET_I(15)
    #undef RET_I
}

template<std::size_t I, typename Binder>
    requires requires { typename Binder::_type0; }
[[nodiscard]] constexpr auto &&get_leaf(Binder &&b) noexcept {
    #define RET_I(i) if constexpr (I == i) { return static_cast<Binder::_type##i &&>(b._##i); }
    RET_I(0) RET_I(1) RET_I(2) RET_I(3) RET_I(4) RET_I(5) RET_I(6) RET_I(7)
    RET_I(8) RET_I(9) RET_I(10) RET_I(11) RET_I(12) RET_I(13) RET_I(14) RET_I(15)
    #undef RET_I
}

template<std::size_t I, typename Binder>
    requires requires { typename Binder::_type0; }
[[nodiscard]] constexpr const auto &&get_leaf(const Binder &&b) noexcept {
    #define RET_I(i) if constexpr (I == i) { return static_cast<const Binder::_type##i &&>(b._##i); }
    RET_I(0) RET_I(1) RET_I(2) RET_I(3) RET_I(4) RET_I(5) RET_I(6) RET_I(7)
    RET_I(8) RET_I(9) RET_I(10) RET_I(11) RET_I(12) RET_I(13) RET_I(14) RET_I(15)
    #undef RET_I
}

#endif // clang-format on

} // namespace detail

/**
 * \brief A custom lightweight tuple class with a focus on compile times.
 * \warning This class is not fully compatible with \c std::tuple.
 */
template<typename... Ts>
struct tuple {
    detail::make_binder_t<std::make_index_sequence<sizeof...(Ts)>, Ts...> _impl;
};

template<>
struct tuple<> {};

template<typename... Ts>
tuple(Ts &&...) -> tuple<std::remove_cvref_t<Ts>...>;

} // namespace reflex

template<std::size_t I, typename... Ts>
struct std::tuple_element<I, reflex::tuple<Ts...>> {
    using type = reflex::detail::type_pack_element_t<I, Ts...>;
};

namespace reflex {

template<std::size_t I, typename... Ts>
[[nodiscard]] constexpr decltype(auto) get(tuple<Ts...> &t) noexcept {
    return detail::get_leaf<I>(t._impl);
}

template<std::size_t I, typename... Ts>
[[nodiscard]] constexpr decltype(auto) get(const tuple<Ts...> &t) noexcept {
    return detail::get_leaf<I>(t._impl);
}

template<std::size_t I, typename... Ts>
[[nodiscard]] constexpr decltype(auto) get(tuple<Ts...> &&t) noexcept {
    return detail::get_leaf<I>(std::move(t._impl));
}

template<std::size_t I, typename... Ts>
[[nodiscard]] constexpr decltype(auto) get(const tuple<Ts...> &&t) noexcept {
    return detail::get_leaf<I>(std::move(t._impl));
}


template<typename T, typename... Ts>
[[nodiscard]] constexpr T &get(tuple<Ts...> &t) noexcept {
    return detail::get_leaf<detail::index_of_v<T, Ts...>>(t._impl);
}

template<typename T, typename... Ts>
[[nodiscard]] constexpr const T &get(const tuple<Ts...> &t) noexcept {
    return detail::get_leaf<detail::index_of_v<T, Ts...>>(t._impl);
}

template<typename T, typename... Ts>
[[nodiscard]] constexpr T &&get(tuple<Ts...> &&t) noexcept {
    return detail::get_leaf<detail::index_of_v<T, Ts...>>(std::move(t._impl));
}

template<typename T, typename... Ts>
[[nodiscard]] constexpr const T &&get(const tuple<Ts...> &&t) noexcept {
    return detail::get_leaf<detail::index_of_v<T, Ts...>>(std::move(t._impl));
}

} // namespace reflex

template<typename... Ts>
struct std::tuple_size<reflex::tuple<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};

template<typename... Ts>
inline constexpr std::size_t std::tuple_size_v<reflex::tuple<Ts...>> = sizeof...(Ts);

namespace reflex {

namespace detail {

template<typename Fn, typename Tuple, std::size_t... Indices>
constexpr decltype(auto) apply_impl(Fn &&fn, Tuple &&tuple, std::index_sequence<Indices...>) noexcept(
        noexcept(std::forward<Fn>(fn)(detail::get_leaf<Indices>(std::forward<Tuple>(tuple)._impl)...))) {
    return std::forward<Fn>(fn)(detail::get_leaf<Indices>(std::forward<Tuple>(tuple)._impl)...);
}

} // namespace detail

template<typename Fn, typename Tuple>
constexpr decltype(auto) apply(Fn &&fn, Tuple &&tuple) noexcept(
        noexcept(detail::apply_impl(std::forward<Fn>(fn),
                                    std::forward<Tuple>(tuple),
                                    std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{}))) {
    return detail::apply_impl(std::forward<Fn>(fn),
                              std::forward<Tuple>(tuple),
                              std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
}

} // namespace reflex

#endif
