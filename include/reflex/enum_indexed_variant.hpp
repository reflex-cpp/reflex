// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cassert>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <variant>

#include <reflex/enum_indexing.hpp>

namespace reflex {

template<typename E, typename... Types>
class EnumIndexedVariant;

namespace detail {

template<typename E, template<E> typename MakeT, std::size_t... I>
EnumIndexedVariant<E, MakeT<static_cast<E>(I)>...> make_enum_indexed_variant_impl(std::index_sequence<I...> indices);

} // namespace detail

template<auto E>
    requires std::is_enum_v<decltype(E)>
struct in_place_enum_t {
    constexpr explicit in_place_enum_t() = default;
};

template<auto E>
inline constexpr auto in_place_enum = in_place_enum_t<E>{};


template<typename E, typename... Types>
class EnumIndexedVariant : public std::variant<Types...> {
    using variant_t = std::variant<Types...>;

    [[nodiscard]] constexpr variant_t &base() & noexcept { return *this; }
    [[nodiscard]] constexpr const variant_t &base() const & noexcept { return *this; }
    [[nodiscard]] constexpr variant_t &&base() && noexcept { return std::move(*this); }
    [[nodiscard]] constexpr const variant_t &&base() const && noexcept { return std::move(*this); }

public:
    constexpr EnumIndexedVariant() //
            noexcept(std::is_nothrow_default_constructible_v<variant_t>) //
        requires std::is_default_constructible_v<variant_t>
    = default;

    constexpr EnumIndexedVariant(const EnumIndexedVariant &other) = default;

    constexpr EnumIndexedVariant(EnumIndexedVariant &&other)
        requires std::is_move_constructible_v<variant_t>
    = default;

    template<typename T> // check for cvref variant_t to avoid calling the variant_t's copy/move constructors
        requires(std::constructible_from<variant_t, T> && !std::same_as<std::remove_cvref_t<T>, variant_t>)
    constexpr EnumIndexedVariant(T &&value) noexcept(std::is_nothrow_constructible_v<variant_t, T>)
        : variant_t(std::forward<T>(value)) {}

    template<typename T, typename... Args>
        requires std::constructible_from<variant_t, std::in_place_type_t<T>, Args...>
    constexpr explicit EnumIndexedVariant(std::in_place_type_t<T> tag, Args &&...args) //
            noexcept(std::is_nothrow_constructible_v<variant_t, std::in_place_type_t<T>, Args...>)
        : variant_t(tag, std::forward<Args>(args)...) {}

    template<typename T, typename U, typename... Args>
        requires(std::constructible_from<variant_t, std::in_place_type_t<T>, std::initializer_list<U> &, Args...>)
    constexpr explicit EnumIndexedVariant(std::in_place_type_t<T> tag,
                                          std::initializer_list<U> il,
                                          Args &&...args) //
            noexcept(std::is_nothrow_constructible_v<variant_t,
                                                     std::in_place_type_t<T>,
                                                     std::initializer_list<U> &,
                                                     Args...>)
        : variant_t(tag, il, std::forward<Args>(args)...) {}

    template<E Index, typename... Args>
        requires(std::constructible_from<variant_t, std::in_place_index_t<static_cast<std::size_t>(Index)>, Args...>)
    constexpr explicit EnumIndexedVariant(in_place_enum_t<Index> /*tag*/, Args &&...args) //
            noexcept(std::is_nothrow_constructible_v<variant_t,
                                                     std::in_place_index_t<static_cast<std::size_t>(Index)>,
                                                     Args...>)
        : variant_t(std::in_place_index<static_cast<std::size_t>(Index)>, std::forward<Args>(args)...) {}

    template<E Index, typename U, typename... Args>
        requires(std::constructible_from<variant_t,
                                         std::in_place_index_t<static_cast<std::size_t>(Index)>,
                                         std::initializer_list<U> &,
                                         Args...>)
    constexpr explicit EnumIndexedVariant(in_place_enum_t<Index> /*tag*/,
                                          std::initializer_list<U> il,
                                          Args &&...args) //
            noexcept(std::is_nothrow_constructible_v<variant_t,
                                                     std::in_place_index_t<static_cast<std::size_t>(Index)>,
                                                     std::initializer_list<U> &,
                                                     Args...>)
        : variant_t(std::in_place_index<static_cast<std::size_t>(Index)>, il, std::forward<Args>(args)...) {}

    constexpr EnumIndexedVariant &operator=(const EnumIndexedVariant &rhs) = default;

    constexpr EnumIndexedVariant &operator=(EnumIndexedVariant &&rhs)
        requires std::is_move_assignable_v<variant_t>
    = default;

    template<typename T> // check for cvref variant_t to avoid calling variant_t's copy/move assignment operators
        requires(std::assignable_from<variant_t &, T> && !std::same_as<std::remove_cvref_t<T>, variant_t>)
    constexpr EnumIndexedVariant &operator=(T &&value) noexcept(std::is_nothrow_assignable_v<variant_t &, T>) {
        base() = std::forward<T>(value);
        return *this;
    }

    /**
     * \brief Identifies the current alternative.
     * \warning Unlike \c std::variant::index(), this function does not handle the valueless-by-exception state. This
     *          must be checked beforehand by calling \c valueless_by_exception().
     * \pre <tt>!valueless_by_exception()</tt>
     * \return The currently held alternative's identifier.
     */
    [[nodiscard]] constexpr E index() const noexcept {
        assert(!valueless_by_exception());
        return static_cast<E>(base().index());
    }

    using variant_t::valueless_by_exception;

    template<typename T, typename... Args>
        requires requires(variant_t &v, Args &&...args) { v.template emplace<T>(std::forward<Args>(args)...); }
    constexpr T &emplace(Args &&...args) noexcept(noexcept(base().template emplace<T>(std::forward<Args>(args)...))) {
        return base().template emplace<T>(std::forward<Args>(args)...);
    }

    template<typename T, typename U, typename... Args>
        requires requires(variant_t &v, std::initializer_list<U> il, Args &&...args) {
            v.template emplace<T>(il, std::forward<Args>(args)...);
        }
    constexpr T &emplace(std::initializer_list<U> il, Args &&...args) //
            noexcept(noexcept(base().template emplace<T>(il, std::forward<Args>(args)...))) {
        return base().template emplace<T>(il, std::forward<Args>(args)...);
    }

    template<E Index, typename... Args>
        requires requires(variant_t &v, Args &&...args) {
            v.template emplace<static_cast<std::size_t>(Index)>(std::forward<Args>(args)...);
        }
    constexpr auto &emplace(Args &&...args) //
            noexcept(noexcept(base().template emplace<static_cast<std::size_t>(Index)>(std::forward<Args>(args)...))) {
        return base().template emplace<static_cast<std::size_t>(Index)>(std::forward<Args>(args)...);
    }

    template<E Index, typename U, typename... Args>
        requires requires(variant_t &v, std::initializer_list<U> il, Args &&...args) {
            v.template emplace<static_cast<std::size_t>(Index)>(il, std::forward<Args>(args)...);
        }
    constexpr auto &emplace(std::initializer_list<U> il, Args &&...args) //
            noexcept(noexcept(base().template emplace<static_cast<std::size_t>(Index)>(il,
                                                                                       std::forward<Args>(args)...))) {
        return base().template emplace<static_cast<std::size_t>(Index)>(il, std::forward<Args>(args)...);
    }

    template<E Index, typename Self>
        requires std::same_as<std::remove_cvref_t<Self>, EnumIndexedVariant>
    [[nodiscard]] friend constexpr decltype(auto) get(Self &&self) {
        return std::get<static_cast<std::size_t>(Index)>(std::forward<Self>(self).base());
    }

    template<E Index>
    [[nodiscard]] friend constexpr decltype(auto) get_if(EnumIndexedVariant *const pv) noexcept {
        return pv ? std::get_if<static_cast<std::size_t>(Index)>(&pv->base()) : nullptr;
    }

    template<E Index>
    [[nodiscard]] friend constexpr decltype(auto) get_if(const EnumIndexedVariant *const pv) noexcept {
        return pv ? std::get_if<static_cast<std::size_t>(Index)>(&pv->base()) : nullptr;
    }

    [[nodiscard]] friend constexpr bool operator==(const EnumIndexedVariant &lhs,
                                                   const EnumIndexedVariant &rhs) = default;

    [[nodiscard]] friend constexpr auto operator<=>(const EnumIndexedVariant &lhs,
                                                    const EnumIndexedVariant &rhs) = default;

    friend constexpr void swap(EnumIndexedVariant &lhs, EnumIndexedVariant &rhs) //
            noexcept(std::is_nothrow_swappable_v<variant_t>) {
        lhs.base().swap(rhs.base());
    }

private:
    template<typename T>
    friend struct std::hash;
};

namespace detail {

template<typename... Ts>
[[nodiscard]] constexpr auto &&as_std_variant(std::variant<Ts...> &var) noexcept {
    return var;
}

template<typename... Ts>
[[nodiscard]] constexpr auto &&as_std_variant(const std::variant<Ts...> &var) noexcept {
    return var;
}

template<typename... Ts>
[[nodiscard]] constexpr auto &&as_std_variant(std::variant<Ts...> &&var) noexcept {
    return std::move(var);
}

template<typename... Ts>
[[nodiscard]] constexpr auto &&as_std_variant(const std::variant<Ts...> &&var) noexcept {
    return std::move(var);
}

template<typename Variant>
concept has_unambiguous_std_variant_base = requires(Variant &&var) { as_std_variant(std::forward<Variant>(var)); };

} // namespace detail

// This version of visit() implements P2162 (std::visit() for classes derived from std::variant).
// The defect report was applied to C++17 and above, but as of writing it is not available everywhere.
// Notably, it would require GCC 11.3 on Linux.
#if __cpp_lib_variant < 202102L // must be guarded, otherwise the call to visit() is ambiguous
template<typename Visitor, typename... Variants>
    requires(... && detail::has_unambiguous_std_variant_base<Variants>)
constexpr decltype(auto) visit(Visitor &&vis, Variants &&...vars) {
    return std::visit(std::forward<Visitor>(vis), detail::as_std_variant(std::forward<Variants>(vars))...);
}
#endif

template<auto Index, typename T>
struct variant_alternative;

template<auto Index, typename T>
using variant_alternative_t = typename variant_alternative<Index, T>::type;

template<typename E, typename... Types, E Index>
struct variant_alternative<Index, EnumIndexedVariant<E, Types...>> {
    using type = std::variant_alternative_t<static_cast<std::size_t>(Index), std::variant<Types...>>;
};

template<auto Index, typename T>
struct variant_alternative : variant_alternative<Index, const T> {
    using type = std::add_const_t<variant_alternative_t<Index, T>>;
};

template<typename E, template<E> typename MakeT>
using make_enum_indexed_variant_t = decltype(detail::make_enum_indexed_variant_impl<E, MakeT>(
        std::make_index_sequence<enum_indexing_traits<E>::count>{}));

} // namespace reflex

template<typename E, typename... Types>
struct std::variant_size<reflex::EnumIndexedVariant<E, Types...>> : std::integral_constant<size_t, sizeof...(Types)> {};


template<typename E, typename... Types>
struct std::hash<reflex::EnumIndexedVariant<E, Types...>> {
private:
    using argument_type = reflex::EnumIndexedVariant<E, Types...>;

public:
    [[nodiscard]] size_t operator()(const argument_type &v) {
        return std::hash<typename argument_type::variant_t>{}(v.base());
    }
};
