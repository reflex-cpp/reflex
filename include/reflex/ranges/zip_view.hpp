// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <ranges>

namespace reflex::ranges::views {}
namespace reflex {
namespace views = ranges::views;
}

#if __cpp_lib_ranges_zip >= 202110L

namespace reflex::ranges {
using std::ranges::zip_view;
namespace views {
using std::views::zip;
}
} // namespace reflex::ranges

#else

// Based on the LLVM libc++ implementation of std::ranges::zip_view:
// https://github.com/llvm/llvm-project/blob/edc56e05f6616e3b5a797bb23b67d6e75f325a2d/libcxx/include/__ranges/zip_view.h
// Original LLVM license header:
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

    #include <algorithm>
    #include <concepts>
    #include <functional>
    #include <iterator>
    #include <tuple>
    #include <type_traits>
    #include <utility>

    #include <reflex/detail/config.hpp>
    #include <reflex/ranges/range_utils.hpp>

namespace reflex::ranges {

namespace detail {

template<class... Ranges>
concept zip_is_common = (sizeof...(Ranges) == 1 && (std::ranges::common_range<Ranges> && ...)) ||
                        (!(std::ranges::bidirectional_range<Ranges> && ...) &&
                         (std::ranges::common_range<Ranges> && ...)) ||
                        ((std::ranges::random_access_range<Ranges> && ...) &&
                         (std::ranges::sized_range<Ranges> && ...));

template<class Fun, class Tuple>
constexpr auto tuple_transform(Fun &&f, Tuple &&tuple) {
    return std::apply(
            [&]<class... Types>(Types &&...elements) {
                return std::tuple<std::invoke_result_t<Fun &, Types>...>(
                        std::invoke(f, std::forward<Types>(elements))...);
            },
            std::forward<Tuple>(tuple));
}

template<class Fun, class Tuple>
constexpr void tuple_for_each(Fun &&f, Tuple &&tuple) {
    std::apply(
            [&]<class... Types>(Types &&...elements) {
                (static_cast<void>(std::invoke(f, std::forward<Types>(elements))), ...);
            },
            std::forward<Tuple>(tuple));
}

template<class Fun, class Tuple1, class Tuple2, size_t... Indices>
constexpr std::tuple<std::invoke_result_t<Fun &,
                                          std::tuple_element_t<Indices, std::remove_cvref_t<Tuple1>>,
                                          std::tuple_element_t<Indices, std::remove_cvref_t<Tuple2>>>...>
tuple_zip_transform(Fun &&f, Tuple1 &&tuple1, Tuple2 &&tuple2, std::index_sequence<Indices...>) {
    return {std::invoke(f,
                        std::get<Indices>(std::forward<Tuple1>(tuple1)),
                        std::get<Indices>(std::forward<Tuple2>(tuple2)))...};
}

template<class Fun, class Tuple1, class Tuple2>
constexpr auto tuple_zip_transform(Fun &&f, Tuple1 &&tuple1, Tuple2 &&tuple2) {
    return detail::tuple_zip_transform(f,
                                       std::forward<Tuple1>(tuple1),
                                       std::forward<Tuple2>(tuple2),
                                       std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<Tuple1>>>());
}

template<class Fun, class Tuple1, class Tuple2, size_t... Indices>
constexpr void tuple_zip_for_each(Fun &&f, Tuple1 &&tuple1, Tuple2 &&tuple2, std::index_sequence<Indices...>) {
    (std::invoke(f, std::get<Indices>(std::forward<Tuple1>(tuple1)), std::get<Indices>(std::forward<Tuple2>(tuple2))),
     ...);
}

template<class Fun, class Tuple1, class Tuple2>
constexpr auto tuple_zip_for_each(Fun &&f, Tuple1 &&tuple1, Tuple2 &&tuple2) {
    return detail::tuple_zip_for_each(f,
                                      std::forward<Tuple1>(tuple1),
                                      std::forward<Tuple2>(tuple2),
                                      std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<Tuple1>>>());
}

template<class Tuple1, class Tuple2>
constexpr bool tuple_any_equals(const Tuple1 &tuple1, const Tuple2 &tuple2) {
    const auto equals = detail::tuple_zip_transform(std::equal_to<>(), tuple1, tuple2);
    return std::apply([](auto... bools) { return (bools || ...); }, equals);
}

// abs in cstdlib is not constexpr
template<class T>
constexpr T abs(T t) {
    return t < 0 ? -t : t;
}

} // namespace detail

template<std::ranges::input_range... Views>
    requires(std::ranges::view<Views> && ...) && (sizeof...(Views) > 0)
class zip_view : public std::ranges::view_interface<zip_view<Views...>> {
    REFLEX_NO_UNIQUE_ADDRESS std::tuple<Views...> views_;

    template<bool>
    class _iterator;

    template<bool>
    class _sentinel;

public:
    zip_view() = default;

    constexpr explicit zip_view(Views... views) : views_(std::move(views)...) {}

    constexpr auto begin()
        requires(!(detail::simple_view<Views> && ...))
    {
        return _iterator<false>(detail::tuple_transform(std::ranges::begin, views_));
    }

    constexpr auto begin() const
        requires(std::ranges::range<const Views> && ...)
    {
        return _iterator<true>(detail::tuple_transform(std::ranges::begin, views_));
    }

    constexpr auto end()
        requires(!(detail::simple_view<Views> && ...))
    {
        if constexpr (!detail::zip_is_common<Views...>) {
            return _sentinel<false>(detail::tuple_transform(std::ranges::end, views_));
        } else if constexpr ((std::ranges::random_access_range<Views> && ...)) {
            return begin() + std::iter_difference_t<_iterator<false>>(size());
        } else {
            return _iterator<false>(detail::tuple_transform(std::ranges::end, views_));
        }
    }

    constexpr auto end() const
        requires(std::ranges::range<const Views> && ...)
    {
        if constexpr (!detail::zip_is_common<const Views...>) {
            return _sentinel<true>(detail::tuple_transform(std::ranges::end, views_));
        } else if constexpr ((std::ranges::random_access_range<const Views> && ...)) {
            return begin() + std::iter_difference_t<_iterator<true>>(size());
        } else {
            return _iterator<true>(detail::tuple_transform(std::ranges::end, views_));
        }
    }

    constexpr auto size()
        requires(std::ranges::sized_range<Views> && ...)
    {
        return std::apply(
                [](auto... sizes) {
                    using CT = std::make_unsigned_t<std::common_type_t<decltype(sizes)...>>;
                    return std::ranges::min({CT(sizes)...});
                },
                detail::tuple_transform(std::ranges::size, views_));
    }

    constexpr auto size() const
        requires(std::ranges::sized_range<const Views> && ...)
    {
        return std::apply(
                [](auto... sizes) {
                    using CT = std::make_unsigned_t<std::common_type_t<decltype(sizes)...>>;
                    return std::ranges::min({CT(sizes)...});
                },
                detail::tuple_transform(std::ranges::size, views_));
    }
};

template<class... Ranges>
zip_view(Ranges &&...) -> zip_view<std::views::all_t<Ranges>...>;

namespace detail {

template<bool Const, class... Views>
concept zip_all_random_access = (std::ranges::random_access_range<maybe_const<Const, Views>> && ...);

template<bool Const, class... Views>
concept zip_all_bidirectional = (std::ranges::bidirectional_range<maybe_const<Const, Views>> && ...);

template<bool Const, class... Views>
concept zip_all_forward = (std::ranges::forward_range<maybe_const<Const, Views>> && ...);

template<bool Const, class... Views>
consteval auto get_zip_view_iterator_tag() {
    if constexpr (zip_all_random_access<Const, Views...>) {
        return std::random_access_iterator_tag();
    } else if constexpr (zip_all_bidirectional<Const, Views...>) {
        return std::bidirectional_iterator_tag();
    } else if constexpr (zip_all_forward<Const, Views...>) {
        return std::forward_iterator_tag();
    } else {
        return std::input_iterator_tag();
    }
}

template<bool Const, class... Views>
struct zip_view_iterator_category_base {};

template<bool Const, class... Views>
    requires zip_all_forward<Const, Views...>
struct zip_view_iterator_category_base<Const, Views...> {
    using iterator_category = std::input_iterator_tag;
};

} // namespace detail

template<std::ranges::input_range... Views>
    requires(std::ranges::view<Views> && ...) && (sizeof...(Views) > 0)
template<bool Const>
class zip_view<Views...>::_iterator : public detail::zip_view_iterator_category_base<Const, Views...> {
    std::tuple<std::ranges::iterator_t<detail::maybe_const<Const, Views>>...> current_;

    constexpr explicit _iterator(std::tuple<std::ranges::iterator_t<detail::maybe_const<Const, Views>>...> current)
        : current_(std::move(current)) {}

    template<bool>
    friend class zip_view<Views...>::_iterator;

    template<bool>
    friend class zip_view<Views...>::_sentinel;

    friend class zip_view<Views...>;

public:
    using iterator_concept = decltype(detail::get_zip_view_iterator_tag<Const, Views...>());
    using value_type = std::tuple<std::ranges::range_value_t<detail::maybe_const<Const, Views>>...>;
    using difference_type = std::common_type_t<std::ranges::range_difference_t<detail::maybe_const<Const, Views>>...>;

    _iterator() = default;

    constexpr _iterator(_iterator<!Const> i)
        requires Const && (std::convertible_to<std::ranges::iterator_t<Views>,
                                               std::ranges::iterator_t<detail::maybe_const<Const, Views>>> &&
                           ...)
        : current_(std::move(i.current_)) {}

    constexpr auto operator*() const {
        return detail::tuple_transform([](auto &i) -> decltype(auto) { return *i; }, current_);
    }

    constexpr _iterator &operator++() {
        detail::tuple_for_each([](auto &i) { ++i; }, current_);
        return *this;
    }

    constexpr void operator++(int) { ++*this; }

    constexpr _iterator operator++(int)
        requires detail::zip_all_forward<Const, Views...>
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr _iterator &operator--()
        requires detail::zip_all_bidirectional<Const, Views...>
    {
        detail::tuple_for_each([](auto &i) { --i; }, current_);
        return *this;
    }

    constexpr _iterator operator--(int)
        requires detail::zip_all_bidirectional<Const, Views...>
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    constexpr _iterator &operator+=(difference_type x)
        requires detail::zip_all_random_access<Const, Views...>
    {
        detail::tuple_for_each([&]<class Iter>(Iter &i) { i += std::iter_difference_t<Iter>(x); }, current_);
        return *this;
    }

    constexpr _iterator &operator-=(difference_type x)
        requires detail::zip_all_random_access<Const, Views...>
    {
        detail::tuple_for_each([&]<class Iter>(Iter &i) { i -= std::iter_difference_t<Iter>(x); }, current_);
        return *this;
    }

    constexpr auto operator[](difference_type n) const
        requires detail::zip_all_random_access<Const, Views...>
    {
        return detail::tuple_transform(
                [&]<class Iter>(Iter &i) -> decltype(auto) { return i[std::iter_difference_t<Iter>(n)]; },
                current_);
    }

    friend constexpr bool operator==(const _iterator &x, const _iterator &y)
        requires(std::equality_comparable<std::ranges::iterator_t<detail::maybe_const<Const, Views>>> && ...)
    {
        if constexpr (detail::zip_all_bidirectional<Const, Views...>) {
            return x.current_ == y.current_;
        } else {
            return detail::tuple_any_equals(x.current_, y.current_);
        }
    }

    friend constexpr auto operator<=>(const _iterator &x, const _iterator &y)
        requires detail::zip_all_random_access<Const, Views...>
    {
        return x.current_ <=> y.current_;
    }

    friend constexpr _iterator operator+(const _iterator &i, difference_type n)
        requires detail::zip_all_random_access<Const, Views...>
    {
        auto r = i;
        r += n;
        return r;
    }

    friend constexpr _iterator operator+(difference_type n, const _iterator &i)
        requires detail::zip_all_random_access<Const, Views...>
    {
        return i + n;
    }

    friend constexpr _iterator operator-(const _iterator &i, difference_type n)
        requires detail::zip_all_random_access<Const, Views...>
    {
        auto r = i;
        r -= n;
        return r;
    }

    friend constexpr difference_type operator-(const _iterator &x, const _iterator &y)
        requires(std::sized_sentinel_for<std::ranges::iterator_t<detail::maybe_const<Const, Views>>,
                                         std::ranges::iterator_t<detail::maybe_const<Const, Views>>> &&
                 ...)
    {
        const auto diffs = detail::tuple_zip_transform(std::minus<>(), x.current_, y.current_);
        return std::apply(
                [](auto... ds) {
                    return std::ranges::min({difference_type(ds)...},
                                            [](auto a, auto b) { return detail::abs(a) < detail::abs(b); });
                },
                diffs);
    }

    friend constexpr auto iter_move(const _iterator &i) noexcept(
            (noexcept(std::ranges::iter_move(
                     std::declval<const std::ranges::iterator_t<detail::maybe_const<Const, Views>> &>())) &&
             ...) &&
            (std::is_nothrow_move_constructible_v<
                     std::ranges::range_rvalue_reference_t<detail::maybe_const<Const, Views>>> &&
             ...)) {
        return detail::tuple_transform(std::ranges::iter_move, i.current_);
    }

    friend constexpr void iter_swap(const _iterator &l, const _iterator &r) noexcept(
            (noexcept(std::ranges::iter_swap(
                     std::declval<const std::ranges::iterator_t<detail::maybe_const<Const, Views>> &>(),
                     std::declval<const std::ranges::iterator_t<detail::maybe_const<Const, Views>> &>())) &&
             ...))
        requires(std::indirectly_swappable<std::ranges::iterator_t<detail::maybe_const<Const, Views>>> && ...)
    {
        detail::tuple_zip_for_each(std::ranges::iter_swap, l.current_, r.current_);
    }
};

template<std::ranges::input_range... Views>
    requires(std::ranges::view<Views> && ...) && (sizeof...(Views) > 0)
template<bool Const>
class zip_view<Views...>::_sentinel {
    std::tuple<std::ranges::sentinel_t<detail::maybe_const<Const, Views>>...> end_;

    constexpr explicit _sentinel(std::tuple<std::ranges::sentinel_t<detail::maybe_const<Const, Views>>...> end)
        : end_(end) {}

    friend class zip_view<Views...>;

    // hidden friend cannot access private member of iterator because they are friends of friends
    template<bool OtherConst>
    static constexpr decltype(auto) iter_current(zip_view<Views...>::_iterator<OtherConst> const &it) {
        return (it.current_);
    }

public:
    _sentinel() = default;

    constexpr _sentinel(_sentinel<!Const> i)
        requires Const && (std::convertible_to<std::ranges::sentinel_t<Views>,
                                               std::ranges::sentinel_t<detail::maybe_const<Const, Views>>> &&
                           ...)
        : end_(std::move(i.end_)) {}

    template<bool OtherConst>
        requires(std::sentinel_for<std::ranges::sentinel_t<detail::maybe_const<Const, Views>>,
                                   std::ranges::iterator_t<detail::maybe_const<OtherConst, Views>>> &&
                 ...)
    friend constexpr bool operator==(const _iterator<OtherConst> &x, const _sentinel &y) {
        return detail::tuple_any_equals(iter_current(x), y.end_);
    }

    template<bool OtherConst>
        requires(std::sized_sentinel_for<std::ranges::sentinel_t<detail::maybe_const<Const, Views>>,
                                         std::ranges::iterator_t<detail::maybe_const<OtherConst, Views>>> &&
                 ...)
    friend constexpr std::common_type_t<std::ranges::range_difference_t<detail::maybe_const<OtherConst, Views>>...>
    operator-(const _iterator<OtherConst> &x, const _sentinel &y) {
        const auto diffs = detail::tuple_zip_transform(std::minus<>(), iter_current(x), y.end_);
        return std::apply(
                [](auto... ds) {
                    using Diff = std::common_type_t<
                            std::ranges::range_difference_t<detail::maybe_const<OtherConst, Views>>...>;
                    return std::ranges::min({Diff(ds)...},
                                            [](auto a, auto b) { return detail::abs(a) < detail::abs(b); });
                },
                diffs);
    }

    template<bool OtherConst>
        requires(std::sized_sentinel_for<std::ranges::sentinel_t<detail::maybe_const<Const, Views>>,
                                         std::ranges::iterator_t<detail::maybe_const<OtherConst, Views>>> &&
                 ...)
    friend constexpr std::common_type_t<std::ranges::range_difference_t<detail::maybe_const<OtherConst, Views>>...>
    operator-(const _sentinel &y, const _iterator<OtherConst> &x) {
        return -(x - y);
    }
};

namespace views {
namespace _zip {

struct _fn {
    [[nodiscard]]
    #if __cpp_static_call_operator >= 202207L
    static
    #endif
            constexpr auto operator()()
    #if __cpp_static_call_operator < 202207L
                    const
    #endif
            noexcept {
        return std::ranges::empty_view<std::tuple<>>{};
    }

    template<class... Ranges>
    [[nodiscard]]
    #if __cpp_static_call_operator >= 202207L
    static
    #endif
            constexpr auto operator()(Ranges &&...rs)
    #if __cpp_static_call_operator < 202207L
                    const
    #endif
            noexcept(noexcept(zip_view<std::views::all_t<Ranges &&>...>(std::forward<Ranges>(rs)...)))
                    -> decltype(zip_view<std::views::all_t<Ranges &&>...>(std::forward<Ranges>(rs)...)) {
        return zip_view<std::views::all_t<Ranges>...>(std::forward<Ranges>(rs)...);
    }
};

} // namespace _zip
inline namespace _cpo {
inline constexpr auto zip = _zip::_fn{};
}
} // namespace views

} // namespace reflex::ranges

template<class... Views>
inline constexpr bool std::ranges::enable_borrowed_range<reflex::ranges::zip_view<Views...>> =
        (std::ranges::enable_borrowed_range<Views> && ...);

#endif
