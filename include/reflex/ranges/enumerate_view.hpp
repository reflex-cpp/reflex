// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <ranges>

namespace reflex::ranges::views {}
namespace reflex {
namespace views = ranges::views;
}

#if __cpp_lib_ranges_enumerate >= 202302L

namespace reflex::ranges {
using std::ranges::enumerate_view;
namespace views {
using std::views::enumerate;
}
} // namespace reflex::ranges

#else

// Based on the LLVM libc++ implementation of std::ranges::enumerate_view:
// https://github.com/llvm/llvm-project/blob/9525bcc3e06e6c0bff3a85f503e47b06a63d6dcc/libcxx/include/__ranges/enumerate_view.h
// Original LLVM license header:
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

    #include <compare>
    #include <concepts>
    #include <iterator>
    #include <tuple>
    #include <type_traits>
    #include <utility>

    #include <boost/stl_interfaces/view_adaptor.hpp>

    #include <reflex/ranges/range_utils.hpp>

namespace reflex::ranges {

namespace detail {

template<class R>
concept range_with_movable_references = std::ranges::input_range<R> &&
                                        std::move_constructible<std::ranges::range_reference_t<R>> &&
                                        std::move_constructible<std::ranges::range_rvalue_reference_t<R>>;

} // namespace detail

template<std::ranges::view View>
    requires detail::range_with_movable_references<View>
class enumerate_view : public std::ranges::view_interface<enumerate_view<View>> {
    View base_;

    template<bool Const>
    class _iterator;

    template<bool Const>
    class _sentinel;

public:
    enumerate_view()
        requires std::default_initializable<View>
    = default;

    constexpr explicit enumerate_view(View base) : base_(std::move(base)) {}

    [[nodiscard]] constexpr auto begin()
        requires(!detail::simple_view<View>)
    {
        return _iterator<false>(std::ranges::begin(base_), 0);
    }

    [[nodiscard]] constexpr auto begin() const
        requires detail::range_with_movable_references<const View>
    {
        return _iterator<true>(std::ranges::begin(base_), 0);
    }

    [[nodiscard]] constexpr auto end()
        requires(!detail::simple_view<View>)
    {
        if constexpr (std::ranges::forward_range<View> && std::ranges::common_range<View> &&
                      std::ranges::sized_range<View>) {
            return _iterator<false>(std::ranges::end(base_), std::ranges::distance(base_));
        } else {
            return _sentinel<false>(std::ranges::end(base_));
        }
    }

    [[nodiscard]] constexpr auto end() const
        requires detail::range_with_movable_references<const View>
    {
        if constexpr (std::ranges::forward_range<const View> && std::ranges::common_range<const View> &&
                      std::ranges::sized_range<const View>) {
            return _iterator<true>(std::ranges::end(base_), std::ranges::distance(base_));
        } else {
            return _sentinel<true>(std::ranges::end(base_));
        }
    }

    [[nodiscard]] constexpr auto size()
        requires std::ranges::sized_range<View>
    {
        return std::ranges::size(base_);
    }

    [[nodiscard]] constexpr auto size() const
        requires std::ranges::sized_range<const View>
    {
        return std::ranges::size(base_);
    }

    [[nodiscard]] constexpr View base() const &
        requires std::copy_constructible<View>
    {
        return base_;
    }

    [[nodiscard]] constexpr View base() & { return std::move(base_); }
};

template<class Range>
enumerate_view(Range &&) -> enumerate_view<std::views::all_t<Range>>;

namespace detail {

template<class Base>
static consteval auto get_enumerate_iterator_concept() {
    if constexpr (std::ranges::random_access_range<Base>) {
        return std::random_access_iterator_tag{};
    } else if constexpr (std::ranges::bidirectional_range<Base>) {
        return std::bidirectional_iterator_tag{};
    } else if constexpr (std::ranges::forward_range<Base>) {
        return std::forward_iterator_tag{};
    } else {
        return std::input_iterator_tag{};
    }
}

} // namespace detail

template<std::ranges::view View>
    requires detail::range_with_movable_references<View>
template<bool Const>
class enumerate_view<View>::_iterator {
    using Base = detail::maybe_const<Const, View>;

    friend class enumerate_view<View>;

public:
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = decltype(detail::get_enumerate_iterator_concept<Base>());
    using difference_type = std::ranges::range_difference_t<Base>;
    using value_type = std::tuple<difference_type, std::ranges::range_value_t<Base>>;

private:
    using reference_type = std::tuple<difference_type, std::ranges::range_reference_t<Base>>;

    std::ranges::iterator_t<Base> current_ = std::ranges::iterator_t<Base>();
    std::ranges::range_difference_t<Base> pos_ = 0;

    constexpr explicit _iterator(std::ranges::iterator_t<Base> current, std::ranges::range_difference_t<Base> pos)
        : current_(std::move(current)),
          pos_(pos) {}

public:
    _iterator()
        requires std::default_initializable<std::ranges::iterator_t<Base>>
    = default;

    constexpr _iterator(_iterator<!Const> i)
        requires Const && std::convertible_to<std::ranges::iterator_t<View>, std::ranges::iterator_t<Base>>
        : current_(std::move(i.current_)),
          pos_(i.pos_) {}

    [[nodiscard]] constexpr const std::ranges::iterator_t<Base> &base() const & noexcept { return current_; }

    [[nodiscard]] constexpr std::ranges::iterator_t<Base> base() && { return std::move(current_); }

    [[nodiscard]] constexpr std::ranges::range_difference_t<Base> index() const noexcept { return pos_; }

    [[nodiscard]] constexpr auto operator*() const { return reference_type(pos_, *current_); }

    constexpr _iterator &operator++() {
        ++current_;
        ++pos_;
        return *this;
    }

    constexpr void operator++(int) { return ++*this; }

    constexpr _iterator operator++(int)
        requires std::ranges::forward_range<Base>
    {
        auto temp = *this;
        ++*this;
        return temp;
    }

    constexpr _iterator &operator--()
        requires std::ranges::bidirectional_range<Base>
    {
        --current_;
        --pos_;
        return *this;
    }

    constexpr _iterator operator--(int)
        requires std::ranges::bidirectional_range<Base>
    {
        auto temp = *this;
        --*this;
        return *temp;
    }

    constexpr _iterator &operator+=(difference_type n)
        requires std::ranges::random_access_range<Base>
    {
        current_ += n;
        pos_ += n;
        return *this;
    }

    constexpr _iterator &operator-=(difference_type n)
        requires std::ranges::random_access_range<Base>
    {
        current_ -= n;
        pos_ -= n;
        return *this;
    }

    [[nodiscard]] constexpr auto operator[](difference_type n) const
        requires std::ranges::random_access_range<Base>
    {
        return reference_type(pos_ + n, current_[n]);
    }

    friend constexpr bool operator==(const _iterator &x, const _iterator &y) noexcept { return x.pos_ == y.pos_; }

    friend constexpr std::strong_ordering operator<=>(const _iterator &x, const _iterator &y) noexcept {
        return x.pos_ <=> y.pos_;
    }

    [[nodiscard]] friend constexpr _iterator operator+(const _iterator &i, difference_type n)
        requires std::ranges::random_access_range<Base>
    {
        auto temp = i;
        temp += n;
        return temp;
    }

    [[nodiscard]] friend constexpr _iterator operator+(difference_type n, const _iterator &i)
        requires std::ranges::random_access_range<Base>
    {
        return i + n;
    }

    [[nodiscard]] friend constexpr _iterator operator-(const _iterator &i, difference_type n)
        requires std::ranges::random_access_range<Base>
    {
        auto temp = i;
        temp -= n;
        return temp;
    }

    [[nodiscard]] friend constexpr difference_type operator-(const _iterator &x, const _iterator &y) noexcept {
        return x.pos_ - y.pos_;
    }

    [[nodiscard]] friend constexpr auto iter_move(const _iterator &i) //
            noexcept(noexcept(std::ranges::iter_move(i.current_)) &&
                     std::is_nothrow_move_constructible_v<std::ranges::range_rvalue_reference_t<Base>>) {
        return std::tuple<difference_type, std::ranges::range_rvalue_reference_t<Base>>(
                i.pos_,
                std::ranges::iter_move(i.current_));
    }
};

template<std::ranges::view View>
    requires detail::range_with_movable_references<View>
template<bool Const>
class enumerate_view<View>::_sentinel {
    using Base = detail::maybe_const<Const, View>;

    std::ranges::sentinel_t<Base> end_ = std::ranges::sentinel_t<Base>();

    constexpr explicit _sentinel(std::ranges::sentinel_t<Base> end) : end_(std::move(end)) {}

    friend class enumerate_view<View>;

public:
    _sentinel() = default;

    constexpr _sentinel(_sentinel<!Const> other)
        requires Const && std::convertible_to<std::ranges::sentinel_t<View>, std::ranges::sentinel_t<Base>>
        : end_(std::move(other.end_)) {}

    [[nodiscard]] constexpr std::ranges::sentinel_t<Base> base() const { return end_; }

    template<bool OtherConst>
        requires std::sentinel_for<std::ranges::sentinel_t<Base>,
                                   std::ranges::iterator_t<detail::maybe_const<OtherConst, View>>>
    friend constexpr bool operator==(const _iterator<OtherConst> &x, const _sentinel &y) {
        return x.current_ == y.end_;
    }

    template<bool OtherConst>
        requires std::sized_sentinel_for<std::ranges::sentinel_t<Base>,
                                         std::ranges::iterator_t<detail::maybe_const<OtherConst, View>>>
    [[nodiscard]] friend constexpr std::ranges::range_difference_t<detail::maybe_const<OtherConst, View>> operator-(
            const _iterator<OtherConst> &x,
            const _sentinel &y) {
        return x.current_ - y.end_;
    }

    template<bool OtherConst>
        requires std::sized_sentinel_for<std::ranges::sentinel_t<Base>,
                                         std::ranges::iterator_t<detail::maybe_const<OtherConst, View>>>
    [[nodiscard]] friend constexpr std::ranges::range_difference_t<detail::maybe_const<OtherConst, View>> operator-(
            const _sentinel &x,
            const _iterator<OtherConst> &y) {
        return x.end_ - y.current_;
    }
};

namespace views {

namespace _enumerate {
struct _fn : boost::stl_interfaces::range_adaptor_closure<_fn> {
    template<class Range>
    [[nodiscard]]
    #if __cpp_static_call_operator >= 202207L
    static
    #endif
            constexpr auto operator()(Range &&range)
    #if __cpp_static_call_operator < 202207L
                    const
    #endif
            noexcept(noexcept(enumerate_view<std::views::all_t<Range>>(range)))
                    -> decltype(enumerate_view<std::views::all_t<Range>>(range)) {
        return enumerate_view<std::views::all_t<Range>>(range);
    }
};
} // namespace _enumerate

inline namespace _cpo {
inline constexpr auto enumerate = _enumerate::_fn{};
}

} // namespace views
} // namespace reflex::ranges

template<class View>
inline constexpr bool std::ranges::enable_borrowed_range<reflex::ranges::enumerate_view<View>> =
        std::ranges::enable_borrowed_range<View>;

#endif
