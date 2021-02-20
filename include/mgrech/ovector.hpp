// Copyright 2020-2021 Markus Grech
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// 	http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <algorithm>
#include <cassert>
#include <new>
#include <type_traits>

#if __cplusplus >= 201700
#  define OVECTOR_NODISCARD [[nodiscard]]
#else
#  define OVECTOR_NODISCARD
#endif

#ifdef _MSC_VER
#  define OVECTOR_FORCE_INLINE __forceinline
#else
#  define OVECTOR_FORCE_INLINE __attribute__((always_inline))
#endif

namespace mgrech { namespace detail
{

template <typename T>
OVECTOR_FORCE_INLINE
auto inlined_move(T& value) noexcept
{
	return static_cast<T&&>(value);
}

template <typename T>
OVECTOR_FORCE_INLINE
auto inlined_forward(typename std::remove_reference<T>::type& value) noexcept
{
	return value;
}

template <typename T>
OVECTOR_FORCE_INLINE
auto inlined_forward(typename std::remove_reference<T>::type&& value) noexcept
{
	return inlined_move(value);
}

template <typename T, typename U>
OVECTOR_FORCE_INLINE
T inlined_exchange(T& ref, U&& value) noexcept
{
	T tmp = inlined_move(ref);
	ref = inlined_forward<U>(value);
	return tmp;
}

template <typename T>
OVECTOR_FORCE_INLINE
void inlined_swap(T& lhs, T& rhs) noexcept
{
	static_assert(std::is_nothrow_move_assignable<T>::value && std::is_nothrow_move_constructible<T>::value,
		      "T must be nothrow move constructible and assignable");

	T tmp = inlined_move(lhs);
	lhs = inlined_move(rhs);
	rhs = inlined_move(tmp);
}

// let's not include an unnecessary header just for size_t
using size_type = decltype(sizeof 0);

void* guarded_alloc(size_type dataSize, size_type guardSize);
void guarded_dealloc(void* memory, size_type dataSize, size_type guardSize);

// RAII-style wrapper for the backing storage
template <typename T>
struct ovector_storage
{
	T* memory;
	size_type size;
	size_type max_size;

	ovector_storage(ovector_storage const&) = delete;
	ovector_storage& operator=(ovector_storage const&) = delete;

	OVECTOR_FORCE_INLINE
	ovector_storage() noexcept
		: memory(nullptr), size(0), max_size(0)
	{}

	OVECTOR_FORCE_INLINE
	explicit ovector_storage(size_type max_size) noexcept
		: memory((T*)guarded_alloc(max_size * sizeof(T), sizeof(T))),
		  size(0), max_size(memory ? max_size : 0)
	{}

	OVECTOR_FORCE_INLINE
	ovector_storage(ovector_storage&& other) noexcept
		: memory(inlined_exchange(other.memory, nullptr)),
		  size(inlined_exchange(other.size, 0)),
		  max_size(inlined_exchange(other.max_size, 0))
	{}

	OVECTOR_FORCE_INLINE
	ovector_storage& operator=(ovector_storage&& other) noexcept
	{
		deallocate();
		memory = inlined_exchange(other.memory, nullptr);
		size = inlined_exchange(other.size, 0);
		max_size = inlined_exchange(other.max_size, 0);
		return *this;
	}

	OVECTOR_FORCE_INLINE
	~ovector_storage() noexcept
	{
		deallocate();
	}

private:
	OVECTOR_FORCE_INLINE
	void deallocate() noexcept
	{
		if(memory)
			guarded_dealloc(memory, max_size * sizeof(T), sizeof(T));
	}
};

} // namespace detail

/**
 * @brief overcommit vector
 * @tparam T element type, should be nothrow-destructible
 * @details A dynamic array container that allocates all memory upfront to avoid the need for reallocations.
 * Modern operating systems back virtual memory with physical memory lazily so there is no excess memory usage until it
 * is actually accessed.
 *
 * An @c ovector is either backed by a fixed amount of or no virtual memory. If there is no backing memory, inserting
 * elements results in undefined behavior. An @c ovector is not backed by memory if any of these conditions apply:
 * <ul>
 * <li>It was default-constructed.
 * <li>It was created using @c with_max_size_or_null and the memory allocation failed.
 * <li>It is a moved-from object.
 * </ul>
 * For such a state, @c data() returns @c nullptr.
 */
template <typename T>
class ovector
{
	detail::ovector_storage<T> _storage;

	OVECTOR_FORCE_INLINE
	explicit ovector(detail::size_type max_size) noexcept
		: _storage(max_size)
	{}

	OVECTOR_FORCE_INLINE
	ovector& unconst() const noexcept
	{
		return *const_cast<ovector*>(this);
	}

	OVECTOR_FORCE_INLINE
	void clear_impl(std::true_type) noexcept
	{
		_storage.size = 0;
	}

	void clear_impl(std::false_type) noexcept
	{
		// we do not need to pull variables out for a smart compiler, but we are assuming
		// very little (i.e. only debug-safe and relatively fast) optimizations which may not
		// be able to prove that the loads are redundant.

		auto p = _storage.memory;

		if(p)
		{
			auto s = _storage.size;

			for(size_type i = 0; i != s; ++i)
				p[i].~T();

			_storage.size = 0;
		}
	}

public:
	static_assert(std::is_nothrow_destructible<T>::value, "T cannot have throwing dtor");

	// STL compatibility
	using value_type = T;
	using reference = T&;
	using const_reference = T const&;
	using iterator = T*;
	using const_iterator = T const*;
	using size_type = detail::size_type;
	using difference_type = decltype(iterator() - iterator());

	ovector(ovector const&) = delete;
	ovector& operator=(ovector const&) = delete;

	/**
	 * Construct an @c ovector without backing storage.
	 * @post @code data() == nullptr @endcode
	 * @post @code size() == 0 @endcode
	 * @post @code max_size() == 0 @endcode
	 * @note You cannot insert elements into this ovector for efficiency reasons: Either @c emplace_back
	 *       would need to check if the @c ovector is backed by storage or the default constructor would need
	 *       to allocate some pre-defined amount of space. If you want to insert elements, construct the
	 *       @c ovector using the @c with_max_size_* factory functions instead.
	 */
	ovector() noexcept = default;

	/**
	 * Construct an @c ovector from another by moving its contents. After this operation the moved-from @c ovector
	 * is not backed by storage.
	 */
	ovector(ovector&&) noexcept = default;

	OVECTOR_FORCE_INLINE
	ovector& operator=(ovector&& other) noexcept
	{
		clear();
		_storage = detail::inlined_move(other._storage);
		return *this;
	}

	/**
	 * Create a new @c ovector with given capacity.
	 * @param max_size The number of elements that the @c ovector should have storage capacity for.
	 * @return The newly created @c ovector. @c data() returns @c nullptr if the allocation failed.
	 */
	OVECTOR_NODISCARD
	OVECTOR_FORCE_INLINE
	static
	ovector with_max_size_or_null(size_type max_size) noexcept
	{
		return ovector(max_size);
	}

	OVECTOR_FORCE_INLINE
	~ovector() noexcept
	{
		clear();
	}

	explicit operator bool() const noexcept
	{
		return data() != nullptr;
	}

	/**
	 * Get direct access to the underlying storage.
	 * @return A pointer to the internal storage, or @c nullptr if this @c ovector is not backed by storage.
	 */
	OVECTOR_NODISCARD
	OVECTOR_FORCE_INLINE
	T* data() noexcept
	{
		return _storage.memory;
	}

	/**
	 * @copydoc T* data() noexcept
	 */
	OVECTOR_NODISCARD
	OVECTOR_FORCE_INLINE
	T const* data() const noexcept
	{
		return unconst().data();
	}

	/**
	 * Get number of elements.
	 * @return The number of elements currently held by this @c ovector.
	 */
	OVECTOR_NODISCARD
	OVECTOR_FORCE_INLINE
	size_type size() const noexcept
	{
		return _storage.size;
	}

	/**
	 * Check whether this @c ovector is empty.
	 * @return @c true if the @c ovector is empty, @c false otherwise.
	 */
	OVECTOR_NODISCARD
	OVECTOR_FORCE_INLINE
	bool empty() const noexcept
	{
		return size() == 0;
	}

	/**
	 * Get the maximum size of this @c ovector.
	 * @return The number of elements that fit into the currently allocated storage of this @c ovector,
	 * or @c 0 if this @c ovector is not backed by storage.
	 */
	OVECTOR_NODISCARD
	OVECTOR_FORCE_INLINE
	size_type max_size() const noexcept
	{
		return _storage.max_size;
	}

	OVECTOR_NODISCARD
	OVECTOR_FORCE_INLINE
	T* begin() noexcept
	{
		return data();
	}

	OVECTOR_NODISCARD
	OVECTOR_FORCE_INLINE
	T const* begin() const noexcept
	{
		return unconst().begin();
	}

	OVECTOR_NODISCARD
	OVECTOR_FORCE_INLINE
	T* end() noexcept
	{
		return data() + size();
	}

	OVECTOR_NODISCARD
	OVECTOR_FORCE_INLINE
	T const* end() const noexcept
	{
		return unconst().end();
	}

	OVECTOR_NODISCARD
	OVECTOR_FORCE_INLINE
	T const* cbegin() const noexcept
	{
		return begin();
	}

	OVECTOR_NODISCARD
	OVECTOR_FORCE_INLINE
	T const* cend() const noexcept
	{
		return end();
	}

	OVECTOR_NODISCARD
	OVECTOR_FORCE_INLINE
	T& operator[](size_type index) noexcept
	{
		assert(index < _storage.size);
		return data()[index];
	}

	OVECTOR_NODISCARD
	OVECTOR_FORCE_INLINE
	T const& operator[](size_type index) const noexcept
	{
		return unconst()[index];
	}

	OVECTOR_NODISCARD
	OVECTOR_FORCE_INLINE
	T& front() noexcept
	{
		return *_storage.memory;
	}

	OVECTOR_NODISCARD
	OVECTOR_FORCE_INLINE
	T const& front() const noexcept
	{
		return unconst().front();
	}

	OVECTOR_NODISCARD
	OVECTOR_FORCE_INLINE
	T& back() noexcept
	{
		return _storage.memory[_storage.size - 1];
	}

	OVECTOR_NODISCARD
	OVECTOR_FORCE_INLINE
	T const& back() const noexcept
	{
		return unconst().back();
	}

	/**
	 * Construct by copy at the back and obtain pointer to inserted element.
	 * @see @c emplace_back
	 */
	OVECTOR_FORCE_INLINE
	T* push_back(T const& value) noexcept(noexcept(emplace_back(value)))
	{
		return emplace_back(value);
	}

	/**
	 * Construct by move at the back and obtain pointer to inserted element.
	 * @see @c emplace_back
	 */
	OVECTOR_FORCE_INLINE
	T* push_back(T&& value) noexcept(noexcept(emplace_back(detail::inlined_move(value))))
	{
		return emplace_back(detail::inlined_move(value));
	}

	/**
	 * Construct a new element at the back with given arguments.
	 * @return A pointer to the new element.
	 * @throw Any exception thrown by the constructor.
	 * @pre @code data() != nullptr @endcode
	 * @pre @code max_size() - size() > 0 @endcode
	 * @post The size increases by 1.
	 * @post The new element resides at the back of this @c ovector.
	 * @note Complexity: O(1).
	 * @note If the max_size is exceeded the behavior is undefined.
	 */
	template <typename... Args>
	OVECTOR_FORCE_INLINE
	T* emplace_back(Args&&... args) noexcept(noexcept(T(detail::inlined_forward<Args>(args)...)))
	{
		auto base = _storage.memory + _storage.size;
		auto p = new(base) T(detail::inlined_forward<Args>(args)...);
		// strong exception safety: update size after attempting to construct element
		uninitialized_grow_back_by(1);
		return p;
	}

	/**
	 * Remove the last element and invoke its destructor.
	 * @pre @code size() != 0 @endcode
	 * @post new_size = old_size - 1
	 */
	OVECTOR_FORCE_INLINE
	void pop_back() noexcept
	{
		uninitialized_shrink_back_by(1)->~T();
	}

	/**
	 * Remove all elements.
	 * @post @code size() == 0 @endcode
	 * @note Complexity: O(1) if T is trivially destructible, O(n) otherwise.
	 */
	OVECTOR_FORCE_INLINE
	void clear() noexcept
	{
		// for trivially-destructible types (such as int) we do not actually need to call dtors. instead of
		// relying on the compiler realizing that the dtor is a no-op and removing everything, we have two
		// versions: one which calls dtors and one which does not.
		clear_impl(std::integral_constant<bool, std::is_trivially_destructible<T>::value>());
	}

	/**
	 * Grow at back without constructing elements.
	 * @param n Number of elements to grow by.
	 * @pre @code size() + n &lt;= max_size()  @endcode
	 * @note Complexity: O(1).
	 * @note This function does not return a pointer to the element storage because increasing the size
	 * before attempting to construct elements violates the strong exception guarantee. Construct elements first,
	 * then invoke @c uninitialized_grow_back_by.
	 */
	OVECTOR_FORCE_INLINE
	void uninitialized_grow_back_by(size_type n) noexcept
	{
		_storage.size += n;
	}

	/**
	 * Shrink at back without calling destructors.
	 * @param n Number of elements to shrink by.
	 * @return A pointer to the storage previously occupied by the elements.
	 * @pre @code size() >= n @endcode
	 * @post @code new_size = old_size - n @endcode
	 * @note Complexity: O(1).
	 */
	OVECTOR_FORCE_INLINE
	T* uninitialized_shrink_back_by(size_type n) noexcept
	{
		_storage.size -= n;
		return _storage.memory + _storage.size;
	}

	OVECTOR_FORCE_INLINE
	void swap(ovector& other) noexcept
	{
		detail::inlined_swap(_storage.memory, other._storage.memory);
		detail::inlined_swap(_storage.size, other._storage.size);
		detail::inlined_swap(_storage.max_size, other._storage.max_size);
	}
};

template <typename T>
OVECTOR_FORCE_INLINE
void swap(ovector<T>& lhs, ovector<T>& rhs) noexcept
{
	lhs.swap(rhs);
}

template <typename T>
OVECTOR_NODISCARD
bool operator==(ovector<T> const& lhs, ovector<T> const& rhs) noexcept
{
	return lhs.size() == rhs.size() && std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
}

template <typename T>
OVECTOR_NODISCARD
OVECTOR_FORCE_INLINE
bool operator!=(ovector<T> const& lhs, ovector<T> const& rhs) noexcept
{
	return !(lhs == rhs);
}

} // namespace mgrech
