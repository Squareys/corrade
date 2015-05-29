#ifndef Corrade_Containers_Array_h
#define Corrade_Containers_Array_h
/*
    This file is part of Corrade.

    Copyright © 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

/** @file
 * @brief Class @ref Corrade::Containers::Array
 */

#include <type_traits>
#include <utility>

#include "Corrade/compatibility.h"
#include "Corrade/configure.h"
#include "Corrade/Containers/ArrayView.h"

#ifdef CORRADE_BUILD_DEPRECATED
#include "Corrade/Utility/Macros.h"
#endif

namespace Corrade { namespace Containers {

/**
@brief Array wrapper with size information

Provides movable RAII wrapper around plain C array. Main use case is storing
binary data of unspecified type, where addition/removal of elements is not
needed or harmful.

However, the class is usable also as lighter non-copyable alternative to
`std::vector`, in STL algorithms in the same way as plain C array and
additionally also in range-based for cycle.

Usage example:
@code
// Create default-initialized array with 5 integers and set them to some value
Containers::Array<int> a(5);
int b = 0;
for(auto& i: a) i = b++; // a = {0, 1, 2, 3, 4}

// Create array from given values
auto b = Containers::Array<int>::from(3, 18, -157, 0);
b[3] = 25; // b = {3, 18, -157, 25}
@endcode

@todo Something like ArrayTuple to create more than one array with single
    allocation and proper alignment for each type? How would non-POD types be
    constructed in that? Will that be useful in more than one place?
*/
template<class T> class Array {
    public:
        typedef T Type;     /**< @brief Element type */

        /**
         * @brief Create array from given values
         *
         * Zero argument count creates `nullptr` array.
         */
        template<class ...U> static Array<T> from(U&&... values) {
            return fromInternal(std::forward<U>(values)...);
        }

        /**
         * @brief Create zero-initialized array
         *
         * Creates array of given size, the values are value-initialized
         * (i.e. builtin types are zero-initialized). For other than builtin
         * types this is the same as @ref Array(std::size_t). If the size is
         * zero, no allocation is done.
         */
        static Array<T> zeroInitialized(std::size_t size) {
            if(!size) {
                #ifndef CORRADE_GCC45_COMPATIBILITY
                return nullptr;
                #else
                return {};
                #endif
            }

            Array<T> array;
            array._data = new T[size]();
            array._size = size;
            return array;
        }

        #ifndef CORRADE_GCC45_COMPATIBILITY
        /** @brief Conversion from nullptr */
        /*implicit*/ Array(std::nullptr_t) noexcept: _data(nullptr), _size(0) {}
        #endif

        /**
         * @brief Default constructor
         *
         * Creates zero-sized array. Move array with nonzero size onto the
         * instance to make it useful.
         */
        /*implicit*/ Array() noexcept: _data(nullptr), _size(0) {}

        /**
         * @brief Constructor
         *
         * Creates array of given size, the values are default-initialized
         * (i.e. builtin types are not initialized). If the size is zero, no
         * allocation is done.
         * @note Due to ambiguity you can't call directly `Array(0)` because
         *      it conflicts with Array(std::nullptr_t). You should call
         *      `Array(nullptr)` instead, which is also `noexcept`.
         * @see @ref zeroInitialized()
         */
        explicit Array(std::size_t size): _data(size ? new T[size] : nullptr), _size(size) {}

        ~Array() { delete[] _data; }

        /** @brief Copying is not allowed */
        /* Special "copymove" constructor to work around issues with std::map */
        #ifndef CORRADE_GCC45_COMPATIBILITY
        Array(const Array<T>&) = delete;
        #else
        Array(const Array<T>& other);
        #endif

        /** @brief Move constructor */
        Array(Array<T>&& other) noexcept;

        /** @brief Copying is not allowed */
        Array<T>& operator=(const Array<T>&) = delete;

        /** @brief Move assignment */
        Array<T>& operator=(Array<T>&&) noexcept;

        #if !defined(CORRADE_GCC44_COMPATIBILITY) && !defined(CORRADE_MSVC2013_COMPATIBILITY)
        /* Disabled on GCC 4.4 to avoid ambiguity with operator T*() (no
           explicit conversion operators). Disabled on MSVC 2013 to avoid
           ambiguous operator+() when doing pointer arithmetic. */
        /** @brief Whether the array is non-empty */
        constexpr explicit operator bool() const { return _data; }
        #endif

        /* `char* a = Containers::Array<char>(5); a[3] = 5;` would result in
           instant segfault, disallowing it in the following conversion
           operators */

        /** @brief Conversion to array type */
        /*implicit*/ operator T*()
        #if !defined(CORRADE_GCC47_COMPATIBILITY) && !defined(CORRADE_MSVC2013_COMPATIBILITY)
        &
        #endif
        { return _data; }

        /** @overload */
        /*implicit*/ operator const T*() const { return _data; }

        /** @brief Array data */
        T* data() { return _data; }
        const T* data() const { return _data; }         /**< @overload */

        /** @brief Array size */
        std::size_t size() const { return _size; }

        /** @brief Whether the array is empty */
        bool empty() const { return !_size; }

        /** @brief Pointer to first element */
        T* begin() { return _data; }
        const T* begin() const { return _data; }        /**< @overload */
        const T* cbegin() const { return _data; }       /**< @overload */

        /** @brief Pointer to (one item after) last element */
        T* end() { return _data+_size; }
        const T* end() const { return _data+_size; }    /**< @overload */
        const T* cend() const { return _data+_size; }   /**< @overload */

        /**
         * @brief Reference to array slice
         *
         * Equivalent to @ref ArrayView::slice().
         */
        ArrayView<T> slice(T* begin, T* end) {
            return ArrayView<T>{*this}.slice(begin, end);
        }
        /** @overload */
        ArrayView<const T> slice(const T* begin, const T* end) const {
            return ArrayView<const T>{*this}.slice(begin, end);
        }
        /** @overload */
        ArrayView<T> slice(std::size_t begin, std::size_t end) {
            return slice(_data + begin, _data + end);
        }
        /** @overload */
        ArrayView<const T> slice(std::size_t begin, std::size_t end) const {
            return slice(_data + begin, _data + end);
        }

        /**
         * @brief Array prefix
         *
         * Equivalent to @ref ArrayView::prefix().
         */
        ArrayView<T> prefix(T* end) {
            return ArrayView<T>{*this}.prefix(end);
        }
        /** @overload */
        ArrayView<const T> prefix(const T* end) const {
            return ArrayView<const T>{*this}.prefix(end);
        }
        ArrayView<T> prefix(std::size_t end) { return prefix(_data + end); } /**< @overload */
        ArrayView<const T> prefix(std::size_t end) const { return prefix(_data + end); } /**< @overload */

        /**
         * @brief Array suffix
         *
         * Equivalent to @ref ArrayView::suffix().
         */
        ArrayView<T> suffix(T* begin) {
            return ArrayView<T>{*this}.suffix(begin);
        }
        /** @overload */
        ArrayView<const T> suffix(const T* begin) const {
            return ArrayView<const T>{*this}.suffix(begin);
        }
        ArrayView<T> suffix(std::size_t begin) { return suffix(_data + begin); } /**< @overload */
        ArrayView<const T> suffix(std::size_t begin) const { return suffix(_data + begin); } /**< @overload */

        /**
         * @brief Release data storage
         *
         * Returns the data pointer and resets internal state to default.
         * Deleting the returned array is user responsibility.
         */
        T* release();

    private:
        template<class ...U> static Array<T> fromInternal(U&&... values) {
            Array<T> array;
            array._size = sizeof...(values);
            array._data = new T[sizeof...(values)] { T(std::forward<U>(values))... };
            return array;
        }
        /* Specialization for zero argument count */
        static Array<T> fromInternal() {
            #ifndef CORRADE_GCC45_COMPATIBILITY
            return nullptr;
            #else
            return {};
            #endif
        }

        T* _data;
        std::size_t _size;
};

#ifdef CORRADE_BUILD_DEPRECATED
/** @copybrief ArrayView
 * @deprecated Use @ref ArrayView.h and @ref ArrayView instead.
 */
#ifndef CORRADE_GCC46_COMPATIBILITY
template<class T> using ArrayReference CORRADE_DEPRECATED("use ArrayView.h and ArrayView instead") = ArrayView<T>;
#else
/* Uh. Just copied over and delegating to ArrayView constructors */
template<class T> class CORRADE_DEPRECATED("use ArrayView.h and ArrayView instead") ArrayReference: public ArrayView<T> {
    public:
        #ifndef CORRADE_GCC45_COMPATIBILITY
        constexpr /*implicit*/ ArrayReference(std::nullptr_t) noexcept: ArrayView<T>{nullptr} {}
        #endif
        constexpr /*implicit*/ ArrayReference() noexcept {}
        constexpr /*implicit*/ ArrayReference(T* data, std::size_t size) noexcept: ArrayView<T>{data, size} {}
        #ifdef CORRADE_GCC46_COMPATIBILITY
        #define size size_ /* With GCC 4.6 it conflicts with size(). WTF. */
        #endif
        template<std::size_t size> constexpr /*implicit*/ ArrayReference(T(&data)[size]) noexcept: ArrayView<T>{data} {}
        #ifdef CORRADE_GCC46_COMPATIBILITY
        #undef size
        #endif
        constexpr /*implicit*/ ArrayReference(Array<T>& array) noexcept: ArrayView<T>{array} {}
        template<class U, class V = typename std::enable_if<std::is_same<const U, T>::value>::type> constexpr /*implicit*/ ArrayReference(const Array<U>& array) noexcept: ArrayView<T>{array} {}
        template<class U, class V = typename std::enable_if<std::is_same<const U, T>::value>::type> constexpr /*implicit*/ ArrayReference(const ArrayView<U>& array) noexcept: ArrayView<T>{array} {}
};
template<> class CORRADE_DEPRECATED("use ArrayView.h and ArrayView instead") ArrayReference<const void>: public ArrayView<const void> {
    public:
        #ifndef CORRADE_GCC45_COMPATIBILITY
        constexpr /*implicit*/ ArrayReference(std::nullptr_t) noexcept: ArrayView<const void>{nullptr} {}
        #endif
        constexpr /*implicit*/ ArrayReference() noexcept {}
        constexpr /*implicit*/ ArrayReference(const void* data, std::size_t size) noexcept: ArrayView<const void>{data, size} {}
        template<class T> constexpr /*implicit*/ ArrayReference(const T* data, std::size_t size) noexcept: ArrayView<const void>{data, size} {}
        #ifdef CORRADE_GCC46_COMPATIBILITY
        #define size size_ /* With GCC 4.6 it conflicts with size(). WTF. */
        #endif
        template<class T, std::size_t size> constexpr /*implicit*/ ArrayReference(T(&data)[size]) noexcept: ArrayView<const void>{data} {}
        #ifdef CORRADE_GCC46_COMPATIBILITY
        #undef size
        #endif
        template<class T> constexpr /*implicit*/ ArrayReference(const Array<T>& array) noexcept: ArrayView<const void>{array} {}
        template<class T> constexpr /*implicit*/ ArrayReference(const ArrayView<T>& array) noexcept: ArrayView<const void>{array} {}
};
#endif
#endif

template<class T> inline Array<T>::Array(Array<T>&& other) noexcept: _data(other._data), _size(other._size) {
    other._data = nullptr;
    other._size = 0;
}

/* Special "copymove" constructor to work around issues with std::map */
#ifdef CORRADE_GCC45_COMPATIBILITY
template<class T> inline Array<T>::Array(const Array<T>& other): _data(other._data), _size(other._size) {
    const_cast<Array<T>&>(other)._data = nullptr;
    const_cast<Array<T>&>(other)._size = 0;
}
#endif

template<class T> inline Array<T>& Array<T>::operator=(Array<T>&& other) noexcept {
    using std::swap;
    swap(_data, other._data);
    swap(_size, other._size);
    return *this;
}

template<class T> inline T* Array<T>::release() {
    /** @todo I need `std::exchange` NOW. */
    T* const data = _data;
    _data = nullptr;
    _size = 0;
    return data;
}

}}

#endif
