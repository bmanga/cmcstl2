// cmcstl2 - A concept-enabled C++ standard library
//
//  Copyright Eric Niebler 2014
//  Copyright Casey Carter 2015
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/caseycarter/cmcstl2
//
//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
#ifndef STL2_DETAIL_ALGORITHM_IS_HEAP_UNTIL_HPP
#define STL2_DETAIL_ALGORITHM_IS_HEAP_UNTIL_HPP

#include <stl2/functional.hpp>
#include <stl2/iterator.hpp>
#include <stl2/detail/fwd.hpp>
#include <stl2/detail/concepts/callable.hpp>

///////////////////////////////////////////////////////////////////////////
// is_heap_until [is.heap]
//
STL2_OPEN_NAMESPACE {
	namespace detail {
		template<RandomAccessIterator I, class Comp = less, class Proj = identity>
		requires
			IndirectStrictWeakOrder<
				Comp, projected<I, Proj>>
		I is_heap_until_n(I first, const iter_difference_t<I> n,
			Comp comp = {}, Proj proj = {})
		{
			STL2_EXPECT(0 <= n);
			iter_difference_t<I> p = 0, c = 1;
			I pp = first;
			while (c < n) {
				I cp = first + c;
				if (__stl2::invoke(comp, __stl2::invoke(proj, *pp), __stl2::invoke(proj, *cp))) {
					return cp;
				}
				++c;
				++cp;
				if (c == n || __stl2::invoke(comp, __stl2::invoke(proj, *pp), __stl2::invoke(proj, *cp))) {
					return cp;
				}
				++p;
				++pp;
				c = 2 * p + 1;
			}
			return first + n;
		}
	}

	template<RandomAccessIterator I, Sentinel<I> S, class Comp = less,
		class Proj = identity>
	requires
		IndirectStrictWeakOrder<
			Comp, projected<I, Proj>>
	I is_heap_until(I first, S last, Comp comp = {}, Proj proj = {})
	{
		auto n = distance(first, std::move(last));
		return detail::is_heap_until_n(std::move(first), n,
			__stl2::ref(comp), __stl2::ref(proj));
	}

	template<RandomAccessRange Rng, class Comp = less, class Proj = identity>
	requires
		IndirectStrictWeakOrder<
			Comp, projected<iterator_t<Rng>, Proj>>
	safe_iterator_t<Rng>
	is_heap_until(Rng&& rng, Comp comp = {}, Proj proj = {})
	{
		return detail::is_heap_until_n(begin(rng), distance(rng),
			__stl2::ref(comp), __stl2::ref(proj));
	}
} STL2_CLOSE_NAMESPACE

#endif
