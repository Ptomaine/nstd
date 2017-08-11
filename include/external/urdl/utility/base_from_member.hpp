#pragma once

//  Base-from-member class template  -----------------------------------------//

// Helper to initialize a base object so a derived class can use this
// object in the initialization of another base class.  Used by
// Dietmar Kuehl from ideas by Ron Klatcho to solve the problem of a
// base class needing to be initialized by a member.

// Contributed by Daryle Walker

#include <type_traits>

namespace urdl
{

template <typename MemberType, int UniqueID = 0>
class base_from_member
{
protected:
    MemberType member;

    template <typename ...T>
    explicit constexpr base_from_member(T&& ...x) noexcept
        : member(static_cast<T&&>(x)...)
        {}
};

template < typename MemberType, int UniqueID >
class base_from_member<MemberType&, UniqueID>
{
protected:
    MemberType& member;

    explicit constexpr base_from_member(MemberType& x) noexcept
        : member(x)
        {}
};

}