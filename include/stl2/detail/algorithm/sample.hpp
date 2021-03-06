// cmcstl2 - A concept-enabled C++ standard library
//
//  Copyright Eric Niebler 2015
//  Copyright Casey Carter 2016
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/caseycarter/cmcstl2
//
#ifndef RANGES_V3_ALGORITHM_SAMPLE_HPP
#define RANGES_V3_ALGORITHM_SAMPLE_HPP

#include <stl2/iterator.hpp>
#include <stl2/random.hpp>
#include <stl2/detail/fwd.hpp>
#include <stl2/detail/randutils.hpp>
#include <stl2/detail/algorithm/results.hpp>
#include <stl2/detail/concepts/algorithm.hpp>
#include <stl2/detail/concepts/core.hpp>

////////////////////////////////////////////////////////////////////////////////
// sample [Extension]
//
STL2_OPEN_NAMESPACE {
	namespace ext {
		template<class I, class O>
		using sample_result = __in_out_result<I, O>;

		template<class I, class S, class O, class Gen>
		META_CONCEPT __sample_constraint =
			InputIterator<I> && Sentinel<S, I> && WeaklyIncrementable<O> &&
			IndirectlyCopyable<I, O> &&
			UniformRandomNumberGenerator<std::remove_reference_t<Gen>>;

		struct __sample_fn : private __niebloid {
			template<class I, class S, class O, class Gen = detail::default_random_engine&>
			requires __sample_constraint<I, S, O, Gen> &&
				(ForwardIterator<I> || SizedSentinel<S, I> || RandomAccessIterator<O>)
			constexpr sample_result<I, O>
			operator()(I first, S last, O o, iter_difference_t<I> n,
				Gen&& gen = detail::get_random_engine()) const
			{
				if constexpr (ForwardIterator<I> || SizedSentinel<S, I>) {
					const auto k = distance(first, last);
					return sized_impl(std::move(first), std::move(last),
						k, std::move(o), n, gen);
				} else {
					if (n > 0) {
						for (iter_difference_t<I> i = 0; i < n && first != last; (void) ++i, (void) ++first) {
							o[i] = *first;
						}
						if (first != last) {
							std::uniform_int_distribution<iter_difference_t<I>> dist;
							using param_t = typename decltype(dist)::param_type;
							for (auto pop_size = n; first != last; (void)++first, ++pop_size) {
								const auto i = dist(gen, param_t{0, pop_size});
								if (i < n) {
									o[i] = *first;
								}
							}
							o += n;
						}
					}
					return {std::move(first), std::move(o)};
				}
			}

			template<class I, class S, class OR, class Gen = detail::default_random_engine&>
			requires __sample_constraint<I, S, iterator_t<OR>, Gen> &&
				(ForwardIterator<I> || SizedSentinel<S, I> || RandomAccessRange<OR>) &&
				(ForwardRange<OR> || SizedRange<OR>)
			constexpr sample_result<I, safe_iterator_t<OR>>
			operator()(I first, S last, OR&& o, Gen&& gen = detail::get_random_engine()) const {
				if constexpr (ForwardIterator<I> || SizedSentinel<S, I>) {
					auto k = distance(first, last);
					return sized_impl(std::move(first), std::move(last),
						k, begin(o), distance(o), gen);
				} else { // RandomAccessRange<OR>
					return (*this)(std::move(first), std::move(last),
						begin(o), distance(o), std::forward<Gen>(gen));
				}
			}

			template<Range R, class O, class Gen = detail::default_random_engine&>
			requires __sample_constraint<iterator_t<R>, sentinel_t<R>, O, Gen> &&
				(ForwardRange<R> || SizedRange<R> || RandomAccessIterator<O>)
			constexpr sample_result<safe_iterator_t<R>, O>
			operator()(R&& r, O o, iter_difference_t<iterator_t<R>> n,
				Gen&& gen = detail::get_random_engine()) const
			{
				if constexpr (ForwardRange<R> || SizedRange<R>) {
					return sized_impl(begin(r), end(r), distance(r), std::move(o),
						n, std::forward<Gen>(gen));
				} else { // RandomAccessIterator<O>
					return (*this)(begin(r), end(r), std::move(o), n, std::forward<Gen>(gen));
				}
			}

			template<Range IR, Range OR, class Gen = detail::default_random_engine&>
			requires __sample_constraint<iterator_t<IR>, sentinel_t<IR>, iterator_t<OR>, Gen> &&
				(ForwardRange<IR> || SizedRange<IR> || RandomAccessRange<OR>) &&
				(ForwardRange<OR> || SizedRange<OR>)
			constexpr sample_result<safe_iterator_t<IR>, safe_iterator_t<OR>>
			operator()(IR&& r, OR&& o, Gen&& gen = detail::get_random_engine()) const {
				if constexpr (ForwardRange<IR> || SizedRange<IR>) {
					return sized_impl(begin(r), end(r), distance(r), begin(o),
						distance(o), std::forward<Gen>(gen));
				} else { // RandomAccessRange<OR>
					return (*this)(begin(r), end(r),
						begin(o), distance(o), std::forward<Gen>(gen));
				}
			}
		private:
			template<class I, class S, class O, class Gen>
			requires __sample_constraint<I, S, O, Gen>
			static constexpr sample_result<I, O>
			sized_impl(I first, S last, iter_difference_t<I> pop_size,
				O o, iter_difference_t<I> n, Gen& gen)
			{
				std::uniform_int_distribution<iter_difference_t<I>> dist;
				using param_t = typename decltype(dist)::param_type;
				if (n > pop_size) {
					n = pop_size;
				}
				for (; n > 0 && first != last; ++first) {
					if (dist(gen, param_t{0, --pop_size}) < n) {
						--n;
						*o = *first;
						++o;
					}
				}
				return {std::move(first), std::move(o)};
			}
		};

		inline constexpr __sample_fn sample {};
	} // namespace ext
} STL2_CLOSE_NAMESPACE

#endif
