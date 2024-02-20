#pragma once

#include <assert.h> // [TODO]: This should be removed once custom error-handling is in-place
#include <utility>
#include <type_traits>

namespace leap
{
	namespace internal
	{
		template<typename T>
		struct DefaultDeleter final
		{
			void operator()(T*& pData) const
			{
				if (pData)
				{
					if constexpr (std::is_array_v<T>)
					{
						delete[] pData;
					}
					else
					{
						delete pData;
					}

					pData = nullptr;
				}
			}
		};
	}

	template<typename T, typename Deleter = internal::DefaultDeleter<T>>
	class UniquePtr
	{
		static_assert(std::is_invocable_v<Deleter, T*&>, "UniquePtr's Deleter must be invocable and take T*& as parameter");

	public:
		UniquePtr();
		UniquePtr(T* const pPointer);

		~UniquePtr();

		UniquePtr(const UniquePtr&) = delete;
		UniquePtr(UniquePtr&& other);
		UniquePtr& operator=(const UniquePtr&) = delete;
		UniquePtr& operator=(UniquePtr&&);

		void Reset(T* pPointer);

		T* Get();
		const T* Get() const;

		T* operator->();
		const T* operator->() const;

		T& operator*();
		const T& operator*() const;

		explicit operator bool() const;

		bool operator==(const UniquePtr& other) const;
		bool operator!=(const UniquePtr& other) const;
		bool operator==(std::nullptr_t) const;
		bool operator!=(std::nullptr_t) const;

	private:
		T* m_pData;
		Deleter m_Deleter;
	};

	template<typename T, typename Deleter>
	class UniquePtr<T[], Deleter>
	{
		static_assert(std::is_invocable_v<Deleter, T*&>, "UniquePtr's Deleter must be invocable and take T*& as parameter");

	public:
		UniquePtr();
		UniquePtr(T* const pPointer);

		~UniquePtr();

		UniquePtr(const UniquePtr&) = delete;
		UniquePtr(UniquePtr&& other);
		UniquePtr& operator=(const UniquePtr&) = delete;
		UniquePtr& operator=(UniquePtr&&);

		void Reset(T* pPointer);

		T* Get();
		const T* Get() const;

		T* operator->();
		const T* operator->() const;

		T& operator*();
		const T& operator*() const;

		explicit operator bool() const;

		bool operator==(const UniquePtr& other) const;
		bool operator!=(const UniquePtr& other) const;
		bool operator==(std::nullptr_t) const;
		bool operator!=(std::nullptr_t) const;

	private:
		T* m_pData;
		Deleter m_Deleter;
	};

	/// ----------------------------------------
	/// ----------------------------------------
	/// MakeUnique<T> and MakeUnique<T[]> Implementation
	/// ----------------------------------------
	/// ----------------------------------------

	template
	<
		typename T,
		typename Deleter = leap::internal::DefaultDeleter<T>,
		typename ... Ts,
		std::enable_if_t<!std::is_array_v<T>, bool> = true
	>
	[[nodiscard]] UniquePtr<T, Deleter> MakeUnique(Ts&& ... args)
	{
		return UniquePtr<T>(new T{ std::forward<Ts>(args)... });
	}

	template
	<
		typename T,
		typename Deleter = leap::internal::DefaultDeleter<T>,
		std::enable_if_t<std::is_array_v<T> && std::extent_v<T> == 0, bool> = true
	>
	[[nodiscard]] inline UniquePtr<T, Deleter> MakeUnique(const int nrOfElements)
	{
		return UniquePtr<T>(new std::remove_extent_t<T>[nrOfElements] {});
	}

	template
	<
		typename T,
		typename Deleter = leap::internal::DefaultDeleter<T>,
		typename ... Ts,
		std::enable_if_t<std::extent_v<T> != 0, bool> = true
	>
	void MakeUnique(Ts&& ...) = delete;

	/// ----------------------------------------
	/// ----------------------------------------
	/// UniquePtr<T> Implementation
	/// ----------------------------------------
	/// ----------------------------------------

	template<typename T, typename Deleter>
	UniquePtr<T, Deleter>::UniquePtr()
		: m_pData{}
		, m_Deleter{}
	{}

	template<typename T, typename Deleter>
	UniquePtr<T, Deleter>::UniquePtr(T* const pPointer)
		: m_pData{ pPointer }
		, m_Deleter{}
	{}

	template<typename T, typename Deleter>
	UniquePtr<T, Deleter>::~UniquePtr()
	{
		m_Deleter(m_pData);
	}

	template<typename T, typename Deleter>
	UniquePtr<T, Deleter>::UniquePtr(UniquePtr&& other)
		: m_pData{ std::move(other.m_pData) }
		, m_Deleter{ std::move(other.m_Deleter) }
	{
		other.m_pData = nullptr;
	}

	template<typename T, typename Deleter>
	UniquePtr<T, Deleter>& UniquePtr<T, Deleter>::operator=(UniquePtr&& other)
	{
		m_Deleter(m_pData);

		m_pData = std::move(other.m_pData);
		m_Deleter = std::move(other.m_Deleter);

		other.m_pData = nullptr;

		return *this;
	}

	template<typename T, typename Deleter>
	void UniquePtr<T, Deleter>::Reset(T* pPointer)
	{
		T* pOldPointer{ m_pData };
		m_pData = pPointer;

		m_Deleter(pOldPointer);
	}

	template<typename T, typename Deleter>
	T* UniquePtr<T, Deleter>::Get()
	{
		return m_pData;
	}

	template<typename T, typename Deleter>
	const T* UniquePtr<T, Deleter>::Get() const
	{
		return m_pData;
	}

	template<typename T, typename Deleter>
	T* UniquePtr<T, Deleter>::operator->()
	{
		return m_pData;
	}

	template<typename T, typename Deleter>
	const T* UniquePtr<T, Deleter>::operator->() const
	{
		return m_pData;
	}

	template<typename T, typename Deleter>
	T& UniquePtr<T, Deleter>::operator*()
	{
		assert(m_pData != nullptr);

		return *m_pData;
	}

	template<typename T, typename Deleter>
	const T& UniquePtr<T, Deleter>::operator*() const
	{
		assert(m_pData != nullptr);

		return *m_pData;
	}

	template<typename T, typename Deleter>
	UniquePtr<T, Deleter>::operator bool() const
	{
		return m_pData != nullptr;
	}

	template<typename T, typename Deleter>
	bool UniquePtr<T, Deleter>::operator==(const UniquePtr& other) const
	{
		return Get() == other.Get();
	}

	template<typename T, typename Deleter>
	bool UniquePtr<T, Deleter>::operator!=(const UniquePtr& other) const
	{
		return Get() != other.Get();
	}

	template<typename T, typename Deleter>
	bool UniquePtr<T, Deleter>::operator==(std::nullptr_t) const
	{
		return m_pData == nullptr;
	}

	template<typename T, typename Deleter>
	bool UniquePtr<T, Deleter>::operator!=(std::nullptr_t) const
	{
		return m_pData != nullptr;
	}

	/// ----------------------------------------
	/// ----------------------------------------
	/// UniquePtr<T[]> Specialisation Implementation
	/// ----------------------------------------
	/// ----------------------------------------

	template<typename T, typename Deleter>
	UniquePtr<T[], Deleter>::UniquePtr()
		: m_pData{}
		, m_Deleter{}
	{}

	template<typename T, typename Deleter>
	UniquePtr<T[], Deleter>::UniquePtr(T* const pPointer)
		: m_pData{ pPointer }
		, m_Deleter{}
	{}

	template<typename T, typename Deleter>
	UniquePtr<T[], Deleter>::~UniquePtr()
	{
		m_Deleter(m_pData);
	}

	template<typename T, typename Deleter>
	UniquePtr<T[], Deleter>::UniquePtr(UniquePtr&& other)
		: m_pData{ std::move(other.m_pData) }
		, m_Deleter{ std::move(other.m_Deleter) }
	{
		other.m_pData = nullptr;
	}

	template<typename T, typename Deleter>
	UniquePtr<T[], Deleter>& UniquePtr<T[], Deleter>::operator=(UniquePtr&& other)
	{
		m_Deleter(m_pData);

		m_pData = std::move(other.m_pData);
		m_Deleter = std::move(other.m_Deleter);

		other.m_pData = nullptr;

		return *this;
	}

	template<typename T, typename Deleter>
	void UniquePtr<T[], Deleter>::Reset(T* pPointer)
	{
		T* pOldPointer{ m_pData };
		m_pData = pPointer;

		m_Deleter(pOldPointer);
	}

	template<typename T, typename Deleter>
	T* UniquePtr<T[], Deleter>::Get()
	{
		return m_pData;
	}

	template<typename T, typename Deleter>
	const T* UniquePtr<T[], Deleter>::Get() const
	{
		return m_pData;
	}

	template<typename T, typename Deleter>
	T* UniquePtr<T[], Deleter>::operator->()
	{
		return m_pData;
	}

	template<typename T, typename Deleter>
	const T* UniquePtr<T[], Deleter>::operator->() const
	{
		return m_pData;
	}

	template<typename T, typename Deleter>
	T& UniquePtr<T[], Deleter>::operator*()
	{
		assert(m_pData != nullptr);

		return *m_pData;
	}

	template<typename T, typename Deleter>
	const T& UniquePtr<T[], Deleter>::operator*() const
	{
		assert(m_pData != nullptr);

		return *m_pData;
	}

	template<typename T, typename Deleter>
	UniquePtr<T[], Deleter>::operator bool() const
	{
		return m_pData != nullptr;
	}

	template<typename T, typename Deleter>
	bool UniquePtr<T[], Deleter>::operator==(const UniquePtr& other) const
	{
		return Get() == other.Get();
	}

	template<typename T, typename Deleter>
	bool UniquePtr<T[], Deleter>::operator!=(const UniquePtr& other) const
	{
		return Get() != other.Get();
	}

	template<typename T, typename Deleter>
	bool UniquePtr<T[], Deleter>::operator==(std::nullptr_t) const
	{
		return m_pData == nullptr;
	}

	template<typename T, typename Deleter>
	bool UniquePtr<T[], Deleter>::operator!=(std::nullptr_t) const
	{
		return m_pData != nullptr;
	}