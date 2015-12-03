// cmcstl2 - A concept-enabled C++ standard library
//
//  Copyright Casey Carter 2015
//  Copyright Eric Niebler 2014
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/caseycarter/cmcstl2
//
#ifndef STL2_DETAIL_ITERATOR_BASIC_ITERATOR_HPP
#define STL2_DETAIL_ITERATOR_BASIC_ITERATOR_HPP

#include <stl2/type_traits.hpp>
#include <stl2/detail/ebo_box.hpp>
#include <stl2/detail/fwd.hpp>
#include <stl2/detail/meta.hpp>
#include <stl2/detail/concepts/fundamental.hpp>
#include <stl2/detail/concepts/object.hpp>
#include <stl2/detail/iterator/common_iterator.hpp>
#include <stl2/detail/iterator/concepts.hpp>
#include <stl2/detail/iterator/default_sentinel.hpp>

// TODO:
// * Name cursor_access type aliases consistently with the iterator type aliases
// * Determine if the code complexity incurred by not having basic_sentinel in the
//   design is actually enabling anything useful.
// * Proxies

STL2_OPEN_NAMESPACE {
  template <Destructible T>
  class basic_mixin : protected detail::ebo_box<T> {
    using box_t = detail::ebo_box<T>;
  public:
    constexpr basic_mixin()
    noexcept(is_nothrow_default_constructible<box_t>::value)
    requires DefaultConstructible<T>()
    : box_t{}
    {}
    constexpr basic_mixin(const T& t)
    noexcept(is_nothrow_constructible<box_t, const T&>::value)
    requires CopyConstructible<T>()
    : box_t(t)
    {}
    constexpr basic_mixin(T&& t)
    noexcept(is_nothrow_constructible<box_t, T&&>::value)
    requires MoveConstructible<T>()
    : box_t(__stl2::move(t))
    {}
  };

  class cursor_access {
    template <class T>
    struct mixin_base {
      using type = basic_mixin<T>;
    };
    template <class T>
    requires requires { typename T::mixin; }
    struct mixin_base<T> {
      using type = typename T::mixin;
    };

  public:
    // Not a bool variable template due to GCC PR68666.
    template <class>
    struct single_pass : false_type {};
    template <class C>
    requires
      requires {
        C::single_pass;
        requires bool(C::single_pass);
      } ||
      requires {
        typename C::single_pass;
        requires bool(C::single_pass::value);
      }
    struct single_pass<C> : true_type {};

    // Not a bool variable template due to GCC PR68666.
    template <class> struct contiguous : false_type {};
    template <class C>
    requires
      (requires {
         typename C::contiguous;
         requires bool(C::contiguous::value);
       } || requires {
        C::contiguous;
        requires bool(C::contiguous);
      }) &&
      requires (const C& c) {
        c.current();
        requires _Is<decltype(c.current()), is_reference>;
      }
    struct contiguous<C> : true_type {};

    template <class T>
    using mixin_t = meta::_t<mixin_base<T>>;

    template <class>
    struct difference_type {
      using type = std::ptrdiff_t;
    };
    template <detail::MemberDifferenceType C>
    struct difference_type<C> {
      using type = typename C::difference_type;
    };
    template <class C>
    requires
      !detail::MemberDifferenceType<C> &&
      requires (const C& lhs, const C& rhs) {
        STL2_DEDUCTION_CONSTRAINT(rhs.distance_to(lhs), SignedIntegral);
      }
    struct difference_type<C> {
      using type = decltype(declval<const C&>().distance_to(declval<const C&>()));
    };
    template <class C>
    using DifferenceType = meta::_t<difference_type<C>>;

    template <class C>
    struct value_type {};
    template <detail::MemberValueType C>
    struct value_type<C> {
      using type = typename C::value_type;
    };
    template <class C>
    requires
      !detail::MemberValueType<C> &&
      requires (const C& c) { STL2_DEDUCE_AUTO_REF_REF(c.current()); }
    struct value_type<C> {
      using type = decay_t<decltype(declval<const C&>().current())>;
    };
    template <class C>
    using ValueType = meta::_t<value_type<C>>;

    template <class I>
    requires
      requires (I&& i) { STL2_DEDUCE_AUTO_REF_REF(((I&&)i).pos()); }
    static constexpr auto&& cursor(I&& i)
    STL2_NOEXCEPT_RETURN(__stl2::forward<I>(i).pos())

    template <class Cursor>
    requires
      requires (Cursor& c) { c.current(); }
    static constexpr decltype(auto) current(Cursor& c)
    STL2_NOEXCEPT_RETURN(c.current())

    template <class Cursor>
    requires
      requires (Cursor& c) { c.arrow(); }
    static constexpr decltype(auto) arrow(Cursor& c)
    STL2_NOEXCEPT_RETURN(c.arrow())

    template <class Cursor, class T>
    requires
      requires (Cursor& c, T&& t) { c.write(__stl2::forward<T>(t)); }
    static constexpr void write(Cursor& c, T&& t)
    STL2_NOEXCEPT_RETURN((void)c.write(__stl2::forward<T>(t)))

    template <class Cursor>
    requires
      requires (Cursor& c) { c.next(); }
    static constexpr void next(Cursor& c)
    STL2_NOEXCEPT_RETURN((void)c.next())

    template <class Cursor>
    requires
      requires (Cursor& c) { c.prev(); }
    static constexpr void prev(Cursor& c)
    STL2_NOEXCEPT_RETURN((void)c.prev())

    template <class Cursor, class Other>
    requires
      requires (const Cursor& lhs, const Other& rhs) {
        { lhs.equal(rhs) } -> bool;
      }
    static constexpr bool equal(const Cursor& lhs, const Other& rhs)
    STL2_NOEXCEPT_RETURN(bool(lhs.equal(rhs)))

    template <class Cursor>
    requires
      requires (Cursor& c, DifferenceType<Cursor> n) { c.advance(n); }
    static constexpr void advance(Cursor& c, DifferenceType<Cursor> n)
    STL2_NOEXCEPT_RETURN((void)c.advance(n))

    template <class Cursor, class Other>
    requires
      requires (const Cursor& lhs, const Other& rhs) {
        { lhs.distance_to(rhs) } -> DifferenceType<Cursor>;
      }
    static constexpr DifferenceType<Cursor>
    distance(const Cursor& lhs, const Other& rhs)
    STL2_NOEXCEPT_RETURN(DifferenceType<Cursor>(lhs.distance_to(rhs)))
  };

  namespace detail {
    template <class C>
    concept bool CursorCurrent = requires (C& c) {
      cursor_access::current(c);
    };
    template <class C>
    concept bool CursorArrow = requires (C& c) {
      cursor_access::arrow(c);
    };
    template <class C>
    concept bool CursorNext = requires (C& c) {
      cursor_access::next(c);
    };
    template <class C>
    concept bool CursorPrev = requires (C& c) {
      cursor_access::prev(c);
    };
    template <class C, class O>
    concept bool CursorEqual =
      requires (const C& l, const O& r) {
        cursor_access::equal(l, r);
      };
    template <class C>
    concept bool CursorAdvance =
      requires (C& c, cursor_access::DifferenceType<C> n) {
        cursor_access::advance(c, n);
      };
    template <class C, class O>
    concept bool CursorDistance =
      requires (const C& l, const O& r) {
        cursor_access::distance(l, r);
      };
    template <class C, class T>
    concept bool CursorWrite =
      requires (C& c, T&& t) {
        cursor_access::write(c, __stl2::forward<T>(t));
      };

    template <class C>
    concept bool Cursor =
      Semiregular<C>() && Semiregular<cursor_access::mixin_t<C>>();
    template <class C>
    concept bool WeakInputCursor =
      Cursor<C> && CursorCurrent<C> &&
      CursorNext<C> && requires {
        typename cursor_access::ValueType<C>;
      };
    template <class C>
    concept bool ForwardCursor =
      WeakInputCursor<C> && CursorEqual<C, C>;
    template <class C>
    concept bool InputCursor =
      ForwardCursor<C> && cursor_access::single_pass<C>::value;
    template <class C>
    concept bool BidirectionalCursor =
      ForwardCursor<C> && CursorPrev<C>;
    template <class C>
    concept bool RandomAccessCursor =
      BidirectionalCursor<C> && CursorAdvance<C> && CursorDistance<C, C>;
    template <class C>
    concept bool ContiguousCursor =
      RandomAccessCursor<C> && cursor_access::contiguous<C>::value;

    template <class>
    struct cursor_category {};
    template <WeakInputCursor C>
    struct cursor_category<C> {
      using type = weak_input_iterator_tag;
    };
    template <InputCursor C>
    struct cursor_category<C> {
      using type = input_iterator_tag;
    };
    template <ForwardCursor C>
    struct cursor_category<C> {
      using type = forward_iterator_tag;
    };
    template <BidirectionalCursor C>
    struct cursor_category<C> {
      using type = bidirectional_iterator_tag;
    };
    template <RandomAccessCursor C>
    struct cursor_category<C> {
      using type = random_access_iterator_tag;
    };
    template <ContiguousCursor C>
    struct cursor_category<C> {
      using type = ext::contiguous_iterator_tag;
    };
    template <class C>
    using CursorCategory = meta::_t<cursor_category<C>>;
  }

  detail::Cursor{C}
  class basic_iterator : public cursor_access::mixin_t<C> {
    friend cursor_access;
    using base_t = cursor_access::mixin_t<C>;

    constexpr C& pos() & noexcept { return base_t::get(); }
    constexpr const C& pos() const& noexcept { return base_t::get(); }
    constexpr C&& pos() && noexcept { return base_t::get(); }

  public:
    using difference_type = cursor_access::DifferenceType<C>;

    basic_iterator() = default;
    using base_t::base_t;

    constexpr basic_iterator& operator*() noexcept {
      return *this;
    }
    constexpr basic_iterator& operator++() & noexcept {
      return *this;
    }
    constexpr basic_iterator& operator++() &
    noexcept(noexcept(cursor_access::next(declval<C&>())))
    requires detail::CursorNext<C>
    {
      cursor_access::next(pos());
      return *this;
    }
    constexpr basic_iterator& operator++(int) & noexcept {
      return *this;
    }
    constexpr basic_iterator operator++(int) &
    noexcept(is_nothrow_copy_constructible<basic_iterator>::value &&
             noexcept(++declval<basic_iterator&>()))
    requires detail::CursorNext<C>
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    template <class T>
    requires
      !Same<decay_t<T>, basic_iterator>() &&
      detail::CursorWrite<C, T>
    constexpr basic_iterator& operator=(T&& t) &
    noexcept(noexcept(
      cursor_access::write(declval<C&>(), __stl2::forward<T>(t))))
    {
      cursor_access::write(pos(), __stl2::forward<T>(t));
      return *this;
    }
  };

  detail::WeakInputCursor{C}
  class basic_iterator<C> : public cursor_access::mixin_t<C> {
    friend cursor_access;
    using base_t = cursor_access::mixin_t<C>;

    constexpr C& pos() & noexcept { return base_t::get(); }
    constexpr const C& pos() const& noexcept { return base_t::get(); }
    constexpr C&& pos() && noexcept { return base_t::get(); }

  public:
    using difference_type = cursor_access::DifferenceType<C>;
    using value_type = cursor_access::ValueType<C>;
    using iterator_category = detail::CursorCategory<C>;
    using reference = decltype(cursor_access::current(declval<const C&>()));

    basic_iterator() = default;
    using base_t::base_t;

    constexpr reference operator*() const
    noexcept(noexcept(cursor_access::current(declval<const C&>())))
    requires detail::CursorCurrent<const C>
    {
      return cursor_access::current(pos());
    }

    constexpr decltype(auto) operator->() const
    noexcept(noexcept(cursor_access::arrow(declval<const C&>())))
    requires detail::CursorArrow<const C>
    {
      return cursor_access::arrow(pos());
    }

    constexpr basic_iterator& operator++() &
    noexcept(noexcept(cursor_access::next(declval<C&>())))
    {
      cursor_access::next(pos());
      return *this;
    }

    constexpr basic_iterator operator++(int) &
    noexcept(is_nothrow_copy_constructible<basic_iterator>::value &&
             is_nothrow_move_constructible<basic_iterator>::value &&
             noexcept(cursor_access::next(declval<C&>())))
    {
      auto tmp = *this;
      cursor_access::next(pos());
      return tmp;
    }

    constexpr basic_iterator& operator--() &
    noexcept(noexcept(cursor_access::prev(declval<C&>())))
    requires detail::CursorPrev<C>
    {
      cursor_access::prev(pos());
      return *this;
    }
    constexpr basic_iterator operator--(int) &
    noexcept(is_nothrow_copy_constructible<basic_iterator>::value &&
             is_nothrow_move_constructible<basic_iterator>::value &&
             noexcept(cursor_access::prev(declval<C&>())))
    requires detail::CursorPrev<C>
    {
      auto tmp = *this;
      cursor_access::prev(pos());
      return *this;
    }

    constexpr basic_iterator& operator+=(difference_type n) &
    noexcept(noexcept(cursor_access::advance(declval<C&>(), n)))
    requires detail::CursorAdvance<C>
    {
      cursor_access::advance(pos(), n);
      return *this;
    }
    constexpr basic_iterator& operator-=(difference_type n) &
    noexcept(noexcept(cursor_access::advance(declval<C&>(), -n)))
    requires  detail::CursorAdvance<C>
    {
      cursor_access::advance(pos(), -n);
      return *this;
    }

    friend constexpr basic_iterator
    operator+(const basic_iterator& i, difference_type n)
    noexcept(is_nothrow_copy_constructible<basic_iterator>::value &&
             is_nothrow_move_constructible<basic_iterator>::value &&
             noexcept(cursor_access::advance(declval<C&>(), n)))
    requires detail::CursorAdvance<C>
    {
      auto tmp = i;
      cursor_access::advance(tmp.pos(), n);
      return tmp;
    }
    friend constexpr basic_iterator
    operator+(difference_type n, const basic_iterator& i)
    noexcept(noexcept(i + n))
    requires detail::CursorAdvance<C>
    {
      return i + n;
    }
    friend constexpr basic_iterator
    operator-(const basic_iterator& i, difference_type n)
    noexcept(noexcept(i + -n))
    requires detail::CursorAdvance<C>
    {
      return i + -n;
    }

    constexpr decltype(auto) operator[](difference_type n) const
      noexcept(noexcept(*(declval<basic_iterator&>() + n)))
      requires detail::CursorAdvance<C> {
      return *(*this + n);
    }
  };

  template <class C>
  requires detail::CursorEqual<C, C>
  constexpr bool operator==(const basic_iterator<C>& lhs,
                            const basic_iterator<C>& rhs)
  STL2_NOEXCEPT_RETURN(
    cursor_access::equal(cursor_access::cursor(lhs),
                         cursor_access::cursor(rhs))
  )

  template <class C, class Other>
  requires detail::CursorEqual<C, Other>
  constexpr bool operator==(const basic_iterator<C>& lhs, const Other& rhs)
  STL2_NOEXCEPT_RETURN(
    cursor_access::equal(cursor_access::cursor(lhs), rhs)
  )

  template <class C, class Other>
  requires detail::CursorEqual<C, Other>
  constexpr bool operator==(const Other& lhs, const basic_iterator<C>& rhs)
  STL2_NOEXCEPT_RETURN(
    cursor_access::equal(cursor_access::cursor(rhs), lhs)
  )

  template <class C>
  requires detail::CursorEqual<C, C>
  constexpr bool operator!=(const basic_iterator<C>& lhs,
                            const basic_iterator<C>& rhs)
  STL2_NOEXCEPT_RETURN(
    !cursor_access::equal(cursor_access::cursor(lhs),
                          cursor_access::cursor(rhs))
  )

  template <class C, class Other>
  requires detail::CursorEqual<C, Other>
  constexpr bool operator!=(const basic_iterator<C>& lhs, const Other& rhs)
  STL2_NOEXCEPT_RETURN(
    !cursor_access::equal(cursor_access::cursor(lhs), rhs)
  )

  template <class C, class Other>
  requires detail::CursorEqual<C, Other>
  constexpr bool operator!=(const Other& lhs, const basic_iterator<C>& rhs)
  STL2_NOEXCEPT_RETURN(
    !cursor_access::equal(cursor_access::cursor(rhs), lhs)
  )

  template <class C>
  requires detail::CursorDistance<C, C>
  constexpr cursor_access::DifferenceType<C>
  operator-(const basic_iterator<C>& lhs, const basic_iterator<C>& rhs)
  STL2_NOEXCEPT_RETURN(
    cursor_access::distance(cursor_access::cursor(rhs),
                            cursor_access::cursor(lhs))
  )

  template <class C, class Other>
  requires detail::CursorDistance<C, Other>
  constexpr cursor_access::DifferenceType<C>
  operator-(const basic_iterator<C>& lhs, const Other& rhs)
  STL2_NOEXCEPT_RETURN(
    -cursor_access::distance(cursor_access::cursor(lhs), rhs)
  )

  template <class C, class Other>
  requires detail::CursorDistance<C, Other>
  constexpr cursor_access::DifferenceType<C>
  operator-(const Other& lhs, const basic_iterator<C>& rhs)
  STL2_NOEXCEPT_RETURN(
    cursor_access::distance(cursor_access::cursor(rhs), lhs)
  )

  template <class C>
  requires detail::CursorDistance<C, C>
  constexpr bool operator<(const basic_iterator<C>& lhs,
                           const basic_iterator<C>& rhs)
  STL2_NOEXCEPT_RETURN(
    cursor_access::distance(cursor_access::cursor(rhs),
                            cursor_access::cursor(lhs)) > 0
  )

  template <class C>
  requires detail::CursorEqual<C, default_sentinel>
  constexpr bool operator<(const basic_iterator<C>& lhs,
                           default_sentinel rhs)
  STL2_NOEXCEPT_RETURN(
    !cursor_access::equal(cursor_access::cursor(lhs), rhs)
  )

  template <class C>
  requires detail::CursorEqual<C, default_sentinel>
  constexpr bool operator<(default_sentinel,
                           const basic_iterator<C>&) noexcept {
    return false;
  }

  template <class C>
  requires detail::CursorDistance<C, C>
  constexpr bool operator>(const basic_iterator<C>& lhs,
                           const basic_iterator<C>& rhs)
  STL2_NOEXCEPT_RETURN(
    rhs < lhs
  )

  template <class C>
  requires detail::CursorEqual<C, default_sentinel>
  constexpr bool operator>(default_sentinel lhs,
                           const basic_iterator<C>& rhs)
  STL2_NOEXCEPT_RETURN(
    !cursor_access::equal(cursor_access::cursor(rhs), lhs)
  )

  template <class C>
  requires detail::CursorEqual<C, default_sentinel>
  constexpr bool operator>(const basic_iterator<C>&,
                           default_sentinel) noexcept {
    return false;
  }

  template <class C>
  requires detail::CursorDistance<C, C>
  constexpr bool operator<=(const basic_iterator<C>& lhs,
                            const basic_iterator<C>& rhs)
  STL2_NOEXCEPT_RETURN(
    !(rhs < lhs)
  )

  template <class C>
  requires detail::CursorEqual<C, default_sentinel>
  constexpr bool operator<=(default_sentinel lhs,
                            const basic_iterator<C>& rhs)
  STL2_NOEXCEPT_RETURN(
    cursor_access::equal(cursor_access::cursor(rhs), lhs)
  )

  template <class C>
  requires detail::CursorEqual<C, default_sentinel>
  constexpr bool operator<=(const basic_iterator<C>&,
                            default_sentinel) noexcept {
    return true;
  }

  template <class C>
  requires detail::CursorDistance<C, C>
  constexpr bool operator>=(const basic_iterator<C>& lhs,
                            const basic_iterator<C>& rhs)
  STL2_NOEXCEPT_RETURN(
    !(lhs < rhs)
  )

  template <class C>
  requires detail::CursorEqual<C, default_sentinel>
  constexpr bool operator>=(default_sentinel,
                            const basic_iterator<C>&) noexcept {
    return true;
  }

  template <class C>
  requires detail::CursorEqual<C, default_sentinel>
  constexpr bool operator>=(const basic_iterator<C>& lhs,
                            default_sentinel rhs)
  STL2_NOEXCEPT_RETURN(
    cursor_access::equal(cursor_access::cursor(lhs), rhs)
  )

  template <class C, class S>
  requires
    detail::CursorEqual<C, S> &&
    !_Valid<__cond, basic_iterator<C>, S>
  struct common_type<basic_iterator<C>, S> {
    using type = common_iterator<basic_iterator<C>, S>;
  };
  template <class C, class S>
  requires
    detail::CursorEqual<C, S> &&
    !_Valid<__cond, basic_iterator<C>, S>
  struct common_type<S, basic_iterator<C>> {
    using type = common_iterator<basic_iterator<C>, S>;
  };

} STL2_CLOSE_NAMESPACE

#endif
