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
#ifndef STL2_DETAIL_ALGORITHM_INPLACE_MERGE_HPP
#define STL2_DETAIL_ALGORITHM_INPLACE_MERGE_HPP

#include <stl2/functional.hpp>
#include <stl2/iterator.hpp>
#include <stl2/type_traits.hpp>
#include <stl2/detail/fwd.hpp>
#include <stl2/detail/temporary_vector.hpp>
#include <stl2/detail/algorithm/move.hpp>
#include <stl2/detail/algorithm/merge.hpp>
#include <stl2/detail/algorithm/lower_bound.hpp>
#include <stl2/detail/algorithm/upper_bound.hpp>
#include <stl2/detail/algorithm/rotate.hpp>
#include <stl2/detail/concepts/algorithm.hpp>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////
// inplace_merge [alg.merge]
//
// TODO:
// * SizedRange overload; downgrade the enumerate call to a distance?
// * Forward ranges.
//
STL2_OPEN_NAMESPACE {
	namespace detail {
		struct merge_adaptive_fn {
		private:
			template<BidirectionalIterator I, class C, class P>
			requires
				Sortable<I, C, P>
			static void impl(I first, I middle, I last, iter_difference_t<I> len1,
				iter_difference_t<I> len2, temporary_buffer<iter_value_t<I>>& buf,
				C& pred, P& proj)
			{
				STL2_EXPENSIVE_ASSERT(len1 == distance(first, midddle));
				STL2_EXPENSIVE_ASSERT(len2 == distance(middle, last));
				temporary_vector<iter_value_t<I>> vec{buf};
				if (len1 <= len2) {
					move(first, middle, __stl2::back_inserter(vec));
					merge(
						__stl2::make_move_iterator(begin(vec)),
						__stl2::make_move_iterator(end(vec)),
						__stl2::make_move_iterator(std::move(middle)),
						__stl2::make_move_iterator(std::move(last)),
						std::move(first), __stl2::ref(pred),
						__stl2::ref(proj), __stl2::ref(proj));
				} else {
					move(middle, last, __stl2::back_inserter(vec));
					using RBi = reverse_iterator<I>;
					merge(
						__stl2::make_move_iterator(RBi{std::move(middle)}),
						__stl2::make_move_iterator(RBi{std::move(first)}),
						__stl2::make_move_iterator(rbegin(vec)),
						__stl2::make_move_iterator(rend(vec)),
						RBi{std::move(last)},
						__stl2::not_fn(__stl2::ref(pred)),
						__stl2::ref(proj), __stl2::ref(proj));
				}
			}

		public:
			template<BidirectionalIterator I, class C, class P>
			requires
				Sortable<I, __f<C>, __f<P>>
			void operator()(I begin, I middle, I end, iter_difference_t<I> len1, iter_difference_t<I> len2,
				detail::temporary_buffer<iter_value_t<I>>& buf, C pred, P proj) const
			{
				// Pre: len1 == distance(begin, midddle)
				// Pre: len2 == distance(middle, end)
				using D = iter_difference_t<I>;
				while (true) {
					// if middle == end, we're done
					if (len2 == 0) {
						return;
					}
					// shrink [begin, middle) as much as possible (with no moves),
					// returning if it shrinks to 0
					for (; true; ++begin, --len1) {
						if (len1 == 0) {
							return;
						}
						if (__stl2::invoke(pred, __stl2::invoke(proj, *middle), __stl2::invoke(proj, *begin))) {
							break;
						}
					}
					if (len1 <= buf.size() || len2 <= buf.size()) {
						merge_adaptive_fn::impl(std::move(begin), std::move(middle),
							std::move(end), len1, len2, buf, pred, proj);
						return;
					}
					// begin < middle < end
					// *begin > *middle
					// partition [begin, m1) [m1, middle) [middle, m2) [m2, end) such that
					//     all elements in:
					//         [begin, m1)  <= [middle, m2)
					//         [middle, m2) <  [m1, middle)
					//         [m1, middle) <= [m2, end)
					//     and m1 or m2 is in the middle of its range
					I m1;  // "median" of [begin, middle)
					I m2;  // "median" of [middle, end)
					D len11;      // distance(begin, m1)
					D len21;      // distance(middle, m2)
					// binary search smaller range
					if (len1 < len2) {
						// len >= 1, len2 >= 2
						len21 = len2 / 2;
						m2 = next(middle, len21);
						m1 = __stl2::upper_bound(begin, middle, __stl2::invoke(proj, *m2),
							__stl2::ref(pred), __stl2::ref(proj));
						len11 = distance(begin, m1);
					} else {
						if (len1 == 1) {
							// len1 >= len2 && len2 > 0, therefore len2 == 1
							// It is known *begin > *middle
							iter_swap(begin, middle);
							return;
						}
						// len1 >= 2, len2 >= 1
						len11 = len1 / 2;
						m1 = next(begin, len11);
						m2 = __stl2::lower_bound(middle, end, __stl2::invoke(proj, *m1),
							__stl2::ref(pred), __stl2::ref(proj));
						len21 = distance(middle, m2);
					}
					D len12 = len1 - len11;  // distance(m1, middle)
					D len22 = len2 - len21;  // distance(m2, end)
					// [begin, m1) [m1, middle) [middle, m2) [m2, end)
					// swap middle two partitions
					middle = rotate(m1, std::move(middle), m2).begin();
					// len12 and len21 now have swapped meanings
					// merge smaller range with recursive call and larger with tail recursion elimination
					if(len11 + len21 < len12 + len22) {
						(*this)(std::move(begin), std::move(m1), middle, len11, len21, buf,
										__stl2::ref(pred), __stl2::ref(proj));
						begin = std::move(middle);
						middle = std::move(m2);
						len1 = len12;
						len2 = len22;
					} else {
						(*this)(middle, std::move(m2), std::move(end), len12, len22, buf,
										__stl2::ref(pred), __stl2::ref(proj));
						end = std::move(middle);
						middle = std::move(m1);
						len1 = len11;
						len2 = len21;
					}
				}
			}
		};

		inline constexpr merge_adaptive_fn merge_adaptive {};

		struct inplace_merge_no_buffer_fn
		{
			template<BidirectionalIterator I, class C = less, class P = identity>
			requires
				Sortable<I, __f<C>, __f<P>>
			void operator()(I begin, I middle, I end, iter_difference_t<I> len1,
				iter_difference_t<I> len2, C pred = {}, P proj = {}) const
			{
				temporary_buffer<iter_value_t<I>> no_buffer;
				merge_adaptive(std::move(begin), std::move(middle), std::move(end),
					len1, len2, no_buffer, __stl2::ref(pred), __stl2::ref(proj));
			}
		};

		inline constexpr inplace_merge_no_buffer_fn inplace_merge_no_buffer {};
	}

	template<BidirectionalIterator I, Sentinel<I> S, class Comp = less,
		class Proj = identity>
	requires Sortable<I, Comp, Proj>
	I inplace_merge(I first, I middle, S last, Comp comp = {}, Proj proj = {})
	{
		auto len1 = distance(first, middle);
		auto len2_and_end = __stl2::ext::enumerate(middle, std::move(last));
		auto buf_size = std::min(len1, len2_and_end.count);
		detail::temporary_buffer<iter_value_t<I>> buf;
		if (std::is_trivially_move_assignable_v<iter_value_t<I>> && 8 < buf_size) {
			buf = detail::temporary_buffer<iter_value_t<I>>{buf_size};
		}
		detail::merge_adaptive(std::move(first), std::move(middle), len2_and_end.end,
			len1, len2_and_end.count, buf, __stl2::ref(comp), __stl2::ref(proj));
		return len2_and_end.end;
	}

	template<BidirectionalRange Rng, class Comp = less, class Proj = identity>
	requires Sortable<iterator_t<Rng>, Comp, Proj>
	safe_iterator_t<Rng>
	inplace_merge(Rng&& rng, iterator_t<Rng> middle, Comp comp = {}, Proj proj = {})
	{
		return __stl2::inplace_merge(begin(rng), std::move(middle),
			end(rng), __stl2::ref(comp), __stl2::ref(proj));
	}
} STL2_CLOSE_NAMESPACE

#endif
