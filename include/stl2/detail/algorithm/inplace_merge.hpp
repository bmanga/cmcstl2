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
#include <stl2/detail/algorithm/merge_move.hpp>
#include <stl2/detail/algorithm/lower_bound.hpp>
#include <stl2/detail/algorithm/upper_bound.hpp>
#include <stl2/detail/algorithm/rotate.hpp>
#include <stl2/detail/concepts/algorithm.hpp>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////
// inplace_merge [alg.merge]
//
// TODO: SizedRange overload; downgrade the enumerate call to a distance?
//
STL2_OPEN_NAMESPACE {
  namespace detail {
    struct merge_adaptive_fn {
    private:
      template <BidirectionalIterator I, class C, class P>
        requires Sortable<I, C, P>()
      static void impl(I begin, I middle, I end, DifferenceType<I> len1,
                       DifferenceType<I> len2, temporary_buffer<ValueType<I>>& buf, C &pred, P &proj) {
        temporary_vector<ValueType<I>> vec{buf};
        if(len1 <= len2)
          {
            __stl2::move(begin, middle, __stl2::back_inserter(vec));
            __stl2::merge_move(__stl2::begin(vec), __stl2::end(vec), __stl2::move(middle), __stl2::move(end),
                               __stl2::move(begin), __stl2::ref(pred), __stl2::ref(proj), __stl2::ref(proj));
          }
        else
          {
            __stl2::move(middle, end, __stl2::back_inserter(vec));
            using RBi = __stl2::reverse_iterator<I>;
            __stl2::merge_move(RBi{__stl2::move(middle)}, RBi{__stl2::move(begin)},
                               __stl2::rbegin(vec), __stl2::rend(vec), RBi{__stl2::move(end)},
                               not_fn(__stl2::ref(pred)), __stl2::ref(proj), __stl2::ref(proj));
          }
      }

    public:
      template <BidirectionalIterator I, class C, class P>
        requires Sortable<I, C, P>()
      void operator()(I begin, I middle, I end, DifferenceType<I> len1, DifferenceType<I> len2,
                      detail::temporary_buffer<ValueType<I>>& buf, C pred_, P proj_) const
      {
        using D = DifferenceType<I>;
        auto&& pred = __stl2::as_function(pred_);
        auto&& proj = __stl2::as_function(proj_);
        while (true) {
          // if middle == end, we're done
          if (len2 == 0) {
            return;
          }
          // shrink [begin, middle) as much as possible (with no moves), returning if it shrinks to 0
          for (; true; ++begin, --len1) {
            if (len1 == 0) {
              return;
            }
            if (pred(proj(*middle), proj(*begin))) {
              break;
            }
          }
          if (len1 <= buf.size() || len2 <= buf.size()) {
            merge_adaptive_fn::impl(__stl2::move(begin), __stl2::move(middle), __stl2::move(end),
                                    len1, len2, buf, pred, proj);
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
            m2 = __stl2::next(middle, len21);
            m1 = __stl2::upper_bound(begin, middle, proj(*m2), __stl2::ref(pred), __stl2::ref(proj));
            len11 = __stl2::distance(begin, m1);
          } else {
            if (len1 == 1) {
              // len1 >= len2 && len2 > 0, therefore len2 == 1
              // It is known *begin > *middle
              __stl2::iter_swap2(begin, middle);
              return;
            }
            // len1 >= 2, len2 >= 1
            len11 = len1 / 2;
            m1 = __stl2::next(begin, len11);
            m2 = __stl2::lower_bound(middle, end, proj(*m1), __stl2::ref(pred), __stl2::ref(proj));
            len21 = __stl2::distance(middle, m2);
          }
          D len12 = len1 - len11;  // distance(m1, middle)
          D len22 = len2 - len21;  // distance(m2, end)
          // [begin, m1) [m1, middle) [middle, m2) [m2, end)
          // swap middle two partitions
          middle = __stl2::rotate(m1, __stl2::move(middle), m2).begin();
          // len12 and len21 now have swapped meanings
          // merge smaller range with recursive call and larger with tail recursion elimination
          if(len11 + len21 < len12 + len22) {
            (*this)(__stl2::move(begin), __stl2::move(m1), middle, len11, len21, buf,
                    __stl2::ref(pred), __stl2::ref(proj));
            begin = __stl2::move(middle);
            middle = __stl2::move(m2);
            len1 = len12;
            len2 = len22;
          } else {
            (*this)(middle, __stl2::move(m2), __stl2::move(end), len12, len22, buf,
                    __stl2::ref(pred), __stl2::ref(proj));
            end = __stl2::move(middle);
            middle = __stl2::move(m1);
            len1 = len11;
            len2 = len21;
          }
        }
      }
    };

    namespace {
      constexpr auto&& merge_adaptive = static_const<merge_adaptive_fn>::value;
    }

    struct inplace_merge_no_buffer_fn
    {
      template <BidirectionalIterator I, class C = less<>, class P = identity>
        requires Sortable<I, C, P>()
      void operator()(I begin, I middle, I end, DifferenceType<I> len1,
                      DifferenceType<I> len2, C&& pred = C{}, P&& proj = P{}) const
      {
        temporary_buffer<ValueType<I>> no_buffer;
        merge_adaptive(__stl2::move(begin), __stl2::move(middle), __stl2::move(end),
          len1, len2, no_buffer, __stl2::forward<C>(pred), __stl2::forward<P>(proj));
      }
    };

    namespace {
      constexpr auto&& inplace_merge_no_buffer = static_const<inplace_merge_no_buffer_fn>::value;
    }
  }

  template <BidirectionalIterator I, Sentinel<I> S, class Comp = less<>,
            class Proj = identity>
    requires Sortable<I, Comp, Proj>()
  I inplace_merge(I first, I middle, S last,
                    Comp&& comp = Comp{}, Proj&& proj = Proj{}) {
    auto len1 = __stl2::distance(first, middle);
    auto len2_and_end = __stl2::ext::enumerate(middle, __stl2::move(last));
    auto buf_size = std::min(len1, len2_and_end.first);
    detail::temporary_buffer<ValueType<I>> buf;
    if (is_trivially_copy_assignable<ValueType<I>>() && 8 < buf_size) {
      buf = detail::temporary_buffer<ValueType<I>>{buf_size};
    }
    detail::merge_adaptive(__stl2::move(first), __stl2::move(middle), len2_and_end.second,
      len1, len2_and_end.first, buf, __stl2::forward<Comp>(comp), __stl2::forward<Proj>(proj));
    return len2_and_end.second;
  }

  template <BidirectionalRange Rng, class Comp = less<>, class Proj = identity>
    requires Sortable<IteratorType<Rng>, Comp, Proj>()
  safe_iterator_t<Rng>
  inplace_merge(Rng&& rng, IteratorType<Rng> middle, Comp&& comp = Comp{},
                Proj&& proj = Proj{}) {
    return __stl2::inplace_merge(__stl2::begin(rng), __stl2::move(middle),
      __stl2::end(rng), __stl2::forward<Comp>(comp), __stl2::forward<Proj>(proj));
  }
} STL2_CLOSE_NAMESPACE

#endif