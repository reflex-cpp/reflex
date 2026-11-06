// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <ranges>

#if __cpp_lib_ranges_to_container >= 202202L

namespace reflex::ranges {
using std::ranges::to;
}

#else

// Based on the LLVM libc++ implementation of std::ranges::to:
// https://github.com/llvm/llvm-project/blob/13a628735fdb0023f1fdc477fed00dc68d7fdf11/libcxx/include/__ranges/to.h
// Original LLVM license header:
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

    #include <concepts>
    #include <cstddef>
    #include <functional>
    #include <iterator>
    #include <type_traits>
    #include <utility>

    #include <boost/stl_interfaces/view_adaptor.hpp>

namespace reflex::ranges {

namespace detail {

// Type that wraps an arbitrary function object and makes it into a range adaptor closure, i.e. something that can be
// called via the `x | f` notation.
template<class Fn>
struct pipeable : Fn, boost::stl_interfaces::range_adaptor_closure<pipeable<Fn>> {
    constexpr explicit pipeable(Fn &&f) : Fn(std::move(f)) {}
};

template<class Container>
constexpr bool reservable_container = std::ranges::sized_range<Container> &&
                                      requires(Container &c, std::ranges::range_size_t<Container> n) {
                                          c.reserve(n);
                                          { c.capacity() } -> std::same_as<decltype(n)>;
                                          { c.max_size() } -> std::same_as<decltype(n)>;
                                      };

template<class Container, class Ref>
constexpr bool container_appendable = requires(Container &c, Ref &&ref) {
    requires(
            requires { c.emplace_back(std::forward<Ref>(ref)); } || //
            requires { c.push_back(std::forward<Ref>(ref)); } ||
            requires { c.emplace(c.end(), std::forward<Ref>(ref)); } ||
            requires { c.insert(c.end(), std::forward<Ref>(ref)); });
};

// Note: making this a concept allows short-circuiting the second condition.
template<class Container, class Range>
concept try_non_recursive_conversion =
        !std::ranges::input_range<Container> ||
        std::convertible_to<std::ranges::range_reference_t<Range>, std::ranges::range_value_t<Container>>;

template<class Container, class Range, class... Args>
concept constructible_from_iter_pair =
        std::ranges::common_range<Range> &&
        requires { typename std::iterator_traits<std::ranges::iterator_t<Range>>::iterator_category; } &&
        std::derived_from<typename std::iterator_traits<std::ranges::iterator_t<Range>>::iterator_category,
                          std::input_iterator_tag> &&
        std::constructible_from<Container, std::ranges::iterator_t<Range>, std::ranges::sentinel_t<Range>, Args...>;

template<class>
concept always_false = false;

} // namespace detail

// `ranges::to` base template -- the `Container` type is a simple type template parameter.
template<class Container, std::ranges::input_range Range, class... Args>
    requires(!std::ranges::view<Container>)
[[nodiscard]] constexpr Container to(Range &&range, Args &&...args) {
    // Mandates: C is a cv-unqualified class type.
    static_assert(!std::is_const_v<Container>,
                  "The target container cannot be const-qualified, please remove the const");
    static_assert(!std::is_volatile_v<Container>,
                  "The target container cannot be volatile-qualified, please remove the volatile");
    static_assert(std::is_class_v<Container> || std::is_union_v<Container>,
                  "The target must be a class type or union type");
    // First see if the non-recursive case applies -- the conversion target is either:
    // - a range with a convertible value type;
    // - a non-range type which might support being created from the input argument(s) (e.g. an `optional`).
    if constexpr (detail::try_non_recursive_conversion<Container, Range>) {
        // Case 1 -- construct directly from the given range.
        if constexpr (std::constructible_from<Container, Range, Args...>) {
            return Container(std::forward<Range>(range), std::forward<Args>(args)...);
        }

        // Case 2 -- construct using the `from_range_t` tagged constructor.
        // else if constexpr (std::constructible_from<Container, std::from_range_t, Range, Args...>) {
        //     return Container(std::from_range, std::forward<Range>(range), std::forward<Args>(args)...);
        // }

        // Case 3 -- construct from a begin-end iterator pair.
        else if constexpr (detail::constructible_from_iter_pair<Container, Range, Args...>) {
            return Container(std::ranges::begin(range), std::ranges::end(range), std::forward<Args>(args)...);
        }

        // Case 4 -- default-construct (or construct from the extra arguments) and insert, reserving the size if
        // possible.
        else if constexpr (std::constructible_from<Container, Args...> &&
                           detail::container_appendable<Container, std::ranges::range_reference_t<Range>>) {
            Container result(std::forward<Args>(args)...);
            if constexpr (std::ranges::sized_range<Range> && detail::reservable_container<Container>) {
                result.reserve(static_cast<std::ranges::range_size_t<Container>>(std::ranges::size(range)));
            }

            for (auto &&ref : range) {
                using Ref = decltype(ref);
                if constexpr (requires { result.emplace_back(std::declval<Ref>()); }) {
                    result.emplace_back(std::forward<Ref>(ref));
                } else if constexpr (requires { result.push_back(std::declval<Ref>()); }) {
                    result.push_back(std::forward<Ref>(ref));
                } else if constexpr (requires { result.emplace(result.end(), std::declval<Ref>()); }) {
                    result.emplace(result.end(), std::forward<Ref>(ref));
                } else {
                    static_assert(requires { result.insert(result.end(), std::declval<Ref>()); });
                    result.insert(result.end(), std::forward<Ref>(ref));
                }
            }
            return result;
        } else {
            static_assert(detail::always_false<Container>,
                          "ranges::to: unable to convert to the given container type.");
        }

        // Try the recursive case.
    } else if constexpr (std::ranges::input_range<std::ranges::range_reference_t<Range>>) {
        return ranges::to<Container>(
                std::ranges::ref_view(range) | std::views::transform([](auto &&elem) {
                    return ranges::to<std::ranges::range_value_t<Container>>(std::forward<decltype(elem)>(elem));
                }),
                std::forward<Args>(args)...);
    } else {
        static_assert(detail::always_false<Container>, "ranges::to: unable to convert to the given container type.");
    }
}

namespace detail {

template<class Range>
struct minimal_input_iterator {
    using iterator_category = std::input_iterator_tag;
    using value_type = std::ranges::range_value_t<Range>;
    using difference_type = std::ptrdiff_t;
    using pointer = std::add_pointer_t<std::ranges::range_reference_t<Range>>;
    using reference = std::ranges::range_reference_t<Range>;

    reference operator*() const;
    pointer operator->() const;
    minimal_input_iterator &operator++();
    minimal_input_iterator operator++(int);
    bool operator==(const minimal_input_iterator &) const;
};

// Deduces the full type of the container from the given template template parameter.
template<template<class...> class Container, std::ranges::input_range Range, class... Args>
struct Deducer {
    static constexpr auto deduce_func() {
        using InputIter = minimal_input_iterator<Range>;

        // Case 1 -- can construct directly from the given range.
        if constexpr (requires { Container(std::declval<Range>(), std::declval<Args>()...); }) {
            using Result = decltype(Container(std::declval<Range>(), std::declval<Args>()...));
            return std::type_identity<Result>{};
        }

        // Case 2 -- can construct from the given range using the `from_range_t` tagged constructor.
        // else if constexpr (requires { Container(std::from_range, std::declval<Range>(), std::declval<Args>()...); })
        // {
        //     using Result = decltype(Container(std::from_range, std::declval<Range>(), std::declval<Args>()...));
        //     return std::type_identity<Result>{};
        // }

        // Case 3 -- can construct from a begin-end iterator pair.
        else if constexpr ( //
                requires {
                    Container(std::declval<InputIter>(), std::declval<InputIter>(), std::declval<Args>()...);
                }) {
            using Result = //
                    decltype(Container(std::declval<InputIter>(), std::declval<InputIter>(), std::declval<Args>()...));
            return std::type_identity<Result>{};

        } else {
            static_assert(always_false<Range>,
                          "ranges::to: unable to deduce the container type from the template template argument.");
        }
    }

    using type = decltype(deduce_func())::type;
};

} // namespace detail

// `ranges::to` specialization -- `Container` is a template template parameter requiring deduction to figure out the
// container element type.
template<template<class...> class Container, std::ranges::input_range Range, class... Args>
[[nodiscard]] constexpr auto to(Range &&range, Args &&...args) {
    using DeducedExpr = detail::Deducer<Container, Range, Args...>::type;
    return ranges::to<DeducedExpr>(std::forward<Range>(range), std::forward<Args>(args)...);
}

// Range adaptor closure object 1 -- wrapping the `ranges::to` version where `Container` is a simple type template
// parameter.
template<class Container, class... Args>
    requires(!std::ranges::view<Container>)
[[nodiscard]] constexpr auto to(Args &&...args) {
    // Mandates: C is a cv-unqualified class type.
    static_assert(!std::is_const_v<Container>,
                  "The target container cannot be const-qualified, please remove the const");
    static_assert(!std::is_volatile_v<Container>,
                  "The target container cannot be volatile-qualified, please remove the volatile");
    static_assert(std::is_class_v<Container> || std::is_union_v<Container>,
                  "The target must be a class type or union type");
    auto to_func = []<std::ranges::input_range Range, class... Tail>(Range &&range, Tail &&...tail)
    #if __cpp_static_call_operator >= 202207L
                           static
    #endif
        requires requires { ranges::to<Container>(std::forward<Range>(range), std::forward<Tail>(tail)...); }
    { return ranges::to<Container>(std::forward<Range>(range), std::forward<Tail>(tail)...); };

    return detail::pipeable(boost::stl_interfaces::bind_back(to_func, std::forward<Args>(args)...));
}

// Range adaptor closure object 2 -- wrapping the `ranges::to` version where `Container` is a template template
// parameter.
template<template<class...> class Container, class... Args>
[[nodiscard]] constexpr auto to(Args &&...args) {
    auto to_func = []<std::ranges::input_range Range,
                      class... Tail,
                      class DeducedExpr = detail::Deducer<Container, Range, Tail...>::type> //
            (Range &&range, Tail &&...tail)
    #if __cpp_static_call_operator >= 202207L
                    static
    #endif
        requires requires { ranges::to<DeducedExpr>(std::forward<Range>(range), std::forward<Tail>(tail)...); }
    { return ranges::to<DeducedExpr>(std::forward<Range>(range), std::forward<Tail>(tail)...); };

    return detail::pipeable(boost::stl_interfaces::bind_back(to_func, std::forward<Args>(args)...));
}

} // namespace reflex::ranges

#endif
