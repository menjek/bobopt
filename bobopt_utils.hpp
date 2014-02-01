#ifndef BOBOPT_UTILS_HPP_GUARD_
#define BOBOPT_UTILS_HPP_GUARD_

#include <bobopt_inline.hpp>
#include <bobopt_language.hpp>

#include <memory>
#include <type_traits>
#include <utility>

namespace bobopt
{

    // make_unique
    //==========================================================================

    // Implementation of make_unique based on paper N3656 by STL.
    // http://isocpp.org/files/papers/N3656.txt
    // Can't wait for C++14.

    /// \brief Traits for make_unique called on single object.
    template <typename T>
    struct unique_if
    {
        typedef std::unique_ptr<T> single_object;
    };

    /// \brief Traits for make_unique called on an array with uknown size.
    template <typename T>
    struct unique_if<T[]>
    {
        typedef std::unique_ptr<T[]> unknown_bound;
    };

    /// \brief Traits for make_unique called on an array with known size.
    template <typename T, size_t N>
    struct unique_if<T[N]>
    {
        typedef void known_bound;
    };

    /// \brief Definition for call on single object.
    /// Just forwards argument to constructor of object.
    template <typename T, typename... Args>
    typename unique_if<T>::single_object make_unique(Args&&... args)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    /// \brief Definition for call on an array with uknown size.
    /// Default construct n objects of array element type.
    template <typename T>
    typename unique_if<T>::unknown_bound make_unique(size_t n)
    {
        typedef typename std::remove_extent<T>::type U;
        return std::unique_ptr<T>(new U[n]());
    }

    /// \brief Delete definition for call on an array of known size.
    template <typename T, typename... Args>
    typename unique_if<T>::known_bound make_unique(Args&&...) BOBOPT_SPECIAL_DELETE;

    // value_distance.
    //==========================================================================
    template <typename T, typename U>
    auto value_distance(T a, U b) -> decltype((a > b) ? (a - b) : (b - a))
    {
        return (a > b) ? (a - b) : (b - a);
    }

} // namespace

#endif // guard