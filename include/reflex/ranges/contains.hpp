// Part of the Reflex project.
// Copyright © Reflex Project Contributors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>

#if __cpp_lib_ranges_contains >= 202207L

namespace reflex::ranges {
using std::ranges::contains;
}

#else

// Based on the LLVM libc++ implementation of std::ranges::contains:
// https://github.com/llvm/llvm-project/blob/13a628735fdb0023f1fdc477fed00dc68d7fdf11/libcxx/include/__algorithm/ranges_contains.h
// Original LLVM license header:
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

    #include <functional>
    #include <iterator>
    #include <ranges>

namespace reflex::ranges {

struct _contains {
    template<std::input_iterator Iter, std::sentinel_for<Iter> Sent, class Type, class Proj = std::identity>
        requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<Iter, Proj>, const Type *>
    [[nodiscard]]
    #if __cpp_static_call_operator >= 202207L
    static
    #endif
            constexpr bool operator()(Iter first, Sent last, const Type &value, Proj proj = {})
    #if __cpp_static_call_operator < 202207L
                    const
    #endif
    {
        return std::ranges::find(std::move(first), last, value, std::ref(proj)) != last;
    }

    template<std::ranges::input_range Range, class Type, class Proj = std::identity>
        requires std::indirect_binary_predicate<std::ranges::equal_to,
                                                std::projected<std::ranges::iterator_t<Range>, Proj>,
                                                const Type *>
    [[nodiscard]]
    #if __cpp_static_call_operator >= 202207L
    static
    #endif
            constexpr bool operator()(Range &&range, const Type &value, Proj proj = {})
    #if __cpp_static_call_operator < 202207L
                    const
    #endif
    {
        return std::ranges::find(std::ranges::begin(range), std::ranges::end(range), value, std::ref(proj)) !=
               std::ranges::end(range);
    }
};

inline namespace _cpo {
inline constexpr auto contains = _contains{};
}

} // namespace reflex::ranges

#endif
