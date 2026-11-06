// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>

#include <reflex/enum_indexing.hpp>
#include <reflex/ranges/zip_view.hpp>

namespace reflex {

template<typename T, typename E>
class EnumIndexedArray {
    using traits_t = enum_indexing_traits<E>;
    using array_t = std::array<T, traits_t::count>;

public:
    using value_type = typename array_t::value_type;
    using size_type = typename array_t::size_type;
    using difference_type = typename array_t::difference_type;
    using reference = typename array_t::reference;
    using const_reference = typename array_t::const_reference;
    using pointer = typename array_t::pointer;
    using const_pointer = typename array_t::const_pointer;
    using iterator = typename array_t::iterator;
    using const_iterator = typename array_t::const_iterator;
    using reverse_iterator = typename array_t::reverse_iterator;
    using const_reverse_iterator = typename array_t::const_reverse_iterator;

    [[nodiscard]] constexpr T &operator[](const E e) noexcept {
        const auto index = static_cast<std::size_t>(e);
        assert(index < size());
        return _arr[index];
    }

    [[nodiscard]] constexpr const T &operator[](const E e) const noexcept {
        const auto index = static_cast<std::size_t>(e);
        assert(index < size());
        return _arr[index];
    }

    [[nodiscard]] constexpr T &front() noexcept {
        assert(!empty());
        return _arr.front();
    }

    [[nodiscard]] constexpr const T &front() const noexcept {
        assert(!empty());
        return _arr.front();
    }

    [[nodiscard]] constexpr T &back() noexcept {
        assert(!empty());
        return _arr.back();
    }

    [[nodiscard]] constexpr const T &back() const noexcept {
        assert(!empty());
        return _arr.back();
    }

    [[nodiscard]] constexpr T *data() noexcept { return _arr.data(); }
    [[nodiscard]] constexpr const T *data() const noexcept { return _arr.data(); }


    [[nodiscard]] constexpr iterator begin() noexcept { return _arr.begin(); }
    [[nodiscard]] constexpr const_iterator begin() const noexcept { return _arr.begin(); }
    [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return _arr.cbegin(); }

    [[nodiscard]] constexpr iterator end() noexcept { return _arr.end(); }
    [[nodiscard]] constexpr const_iterator end() const noexcept { return _arr.end(); }
    [[nodiscard]] constexpr const_iterator cend() const noexcept { return _arr.cend(); }

    [[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return _arr.rbegin(); }
    [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return _arr.rbegin(); }
    [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return _arr.crbegin(); }

    [[nodiscard]] constexpr reverse_iterator rend() noexcept { return _arr.rend(); }
    [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return _arr.rend(); }
    [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return _arr.crend(); }

    [[nodiscard]] constexpr bool empty() const noexcept { return traits_t::count == 0; }
    [[nodiscard]] constexpr size_type size() const noexcept { return traits_t::count; }
    [[nodiscard]] constexpr size_type max_size() const noexcept { return traits_t::count; }

    constexpr void swap(EnumIndexedArray &other) noexcept(noexcept(_arr.swap(other._arr))) { _arr.swap(other._arr); }

    friend constexpr void swap(EnumIndexedArray &lhs, EnumIndexedArray &rhs) noexcept(noexcept(lhs.swap(rhs))) {
        lhs.swap(rhs);
    }

    [[nodiscard]] friend constexpr bool operator==(const EnumIndexedArray &lhs, const EnumIndexedArray &rhs) = default;
    [[nodiscard]] friend constexpr auto operator<=>(const EnumIndexedArray &lhs, const EnumIndexedArray &rhs) = default;

    template<E Index, typename Self>
        requires std::is_same_v<std::remove_cvref_t<Self>, EnumIndexedArray>
    [[nodiscard]] friend constexpr decltype(auto) get(Self &&arr) noexcept {
        constexpr auto index = static_cast<std::size_t>(Index);
        static_assert(index < traits_t::count);
        return std::get<index>(std::forward<Self>(arr)._arr);
    }

    template<std::size_t Index, typename Self>
        requires std::is_same_v<std::remove_cvref_t<Self>, EnumIndexedArray>
    [[nodiscard]] friend constexpr decltype(auto) get(Self &&arr) noexcept {
        static_assert(Index < traits_t::count);
        return std::get<Index>(std::forward<Self>(arr)._arr);
    }

    [[nodiscard]] constexpr auto enumerate() & noexcept { return views::zip(index_enumerators_of<E>(), *this); }
    [[nodiscard]] constexpr auto enumerate() const & noexcept { return views::zip(index_enumerators_of<E>(), *this); }

    // Cannot take a view of a temporary
    constexpr auto enumerate() && = delete;
    constexpr auto enumerate() const && = delete;

    array_t _arr;
};

} // namespace reflex

template<typename T, typename E>
struct std::tuple_size<reflex::EnumIndexedArray<T, E>>
    : integral_constant<size_t, reflex::enum_indexing_traits<E>::count> {};

template<std::size_t I, typename T, typename E>
struct std::tuple_element<I, reflex::EnumIndexedArray<T, E>> {
    static_assert(I < reflex::enum_indexing_traits<E>::count);
    using type = T;
};
