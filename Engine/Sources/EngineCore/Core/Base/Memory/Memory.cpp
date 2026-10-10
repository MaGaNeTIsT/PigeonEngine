#include "Memory.h"
#include <cstdlib>
#include <new>

void* operator new(std::size_t InSize)
{
	if (InSize == 0)
	{
		InSize = 1;
	}
	void* Ptr = std::malloc(InSize);
	if (!Ptr)
	{
		throw std::bad_alloc{};
	}
	return Ptr;
}

void* operator new[](std::size_t InSize)
{
	return operator new(InSize);
}

void operator delete(void* InPtr) noexcept
{
	std::free(InPtr);
}

void operator delete[](void* InPtr) noexcept
{
	operator delete(InPtr);
}

void* operator new(std::size_t InSize, std::align_val_t InAlign)
{
	if (InSize == 0)
	{
		InSize = 1;
	}
	void* Ptr = nullptr;
	std::size_t Alignment = static_cast<std::size_t>(InAlign);

#if defined(_MSC_VER)
	Ptr = ::_aligned_malloc(InSize, Alignment);
#elif defined(__APPLE__) || defined(__linux__)
	if (posix_memalign(&Ptr, Alignment, InSize) != 0)
	{
		Ptr = nullptr;
	}
#else
	std::size_t AdjustedSize = ((InSize + Alignment - 1) / Alignment) * Alignment;
	Ptr = std::aligned_alloc(Alignment, AdjustedSize);
#endif

	if (!Ptr)
	{
		throw std::bad_alloc{};
	}
	return Ptr;
}

void* operator new[](std::size_t InSize, std::align_val_t InAlign)
{
	return operator new(InSize, InAlign);
}

void operator delete(void* InPtr, std::align_val_t InAlign) noexcept
{
#if defined(_MSC_VER)
	::_aligned_free(InPtr);
#else
	std::free(InPtr);
#endif
}

void operator delete[](void* InPtr, std::align_val_t InAlign) noexcept
{
	operator delete(InPtr, InAlign);
}

void* operator new(std::size_t InSize, const std::nothrow_t&) noexcept
{
	if (InSize == 0)
	{
		InSize = 1;
	}
	return std::malloc(InSize);
}

void* operator new[](std::size_t InSize, const std::nothrow_t&) noexcept
{
	return operator new(InSize, std::nothrow);
}

void operator delete(void* InPtr, const std::nothrow_t&) noexcept
{
	std::free(InPtr);
}

void operator delete[](void* InPtr, const std::nothrow_t&) noexcept
{
	operator delete(InPtr, std::nothrow);
}

void* operator new(std::size_t InSize, std::align_val_t InAlign, const std::nothrow_t&) noexcept
{
	if (InSize == 0)
	{
		InSize = 1;
	}
	void* Ptr = nullptr;
	std::size_t Alignment = static_cast<std::size_t>(InAlign);

#if defined(_MSC_VER)
	Ptr = ::_aligned_malloc(InSize, Alignment);
#elif defined(__APPLE__) || defined(__linux__)
	if (posix_memalign(&Ptr, Alignment, InSize) != 0)
	{
		Ptr = nullptr;
	}
#else
	std::size_t AdjustedSize = ((InSize + Alignment - 1) / Alignment) * Alignment;
	Ptr = std::aligned_alloc(Alignment, AdjustedSize);
#endif

	return Ptr;
}

void* operator new[](std::size_t InSize, std::align_val_t InAlign, const std::nothrow_t&) noexcept
{
	return operator new(InSize, InAlign, std::nothrow);
}

void operator delete(void* InPtr, std::align_val_t InAlign, const std::nothrow_t&) noexcept
{
#if defined(_MSC_VER)
	::_aligned_free(InPtr);
#else
	std::free(InPtr);
#endif
}

void operator delete[](void* InPtr, std::align_val_t InAlign, const std::nothrow_t&) noexcept
{
	operator delete(InPtr, InAlign, std::nothrow);
}

namespace PigeonEngine
{

};
