#pragma once

#include "../../../Main/Main.h"
#include "../DataStructure/BuiltInType.h"
#include <Config/ErrorCaption.h>
#include "../../../../Development/Alert/DevelopmentDefines.h"
#include "Base/Math/Math.h"
#include <memory>

namespace PigeonEngine
{

#if 1

#define SMART_PTR_GET(__PtrType) \
	template<typename _Ty>\
	static PE_FORCEINLINE PE_NODISCARD TRemoveExtentType<_Ty>* GetPtr(__PtrType<_Ty>&& _In)\
	{\
		return (_In.get());\
	}\
	template<typename _Ty>\
	static PE_FORCEINLINE PE_NODISCARD TRemoveExtentType<_Ty>* GetPtr(__PtrType<_Ty>& _In)\
	{\
		return (_In.get());\
	}\
	template<typename _Ty>\
	static PE_FORCEINLINE PE_NODISCARD const TRemoveExtentType<_Ty>* GetPtr(const __PtrType<_Ty>& _In)\
	{\
		return (_In.get());\
	}\

	template<typename _Ty>
	using TUniquePtr = std::unique_ptr<_Ty>;
	template<typename _Ty>
	using TSharedPtr = std::shared_ptr<_Ty>;
	template<typename _Ty>
	using TWeakPtr = std::weak_ptr<_Ty>;

	template <typename _Ty, typename _TSizeType>
	PE_NODISCARD PE_CONSTEXPR _TSizeType InitializerListSize(TInitializerList<_Ty> _List)
	{
		return ((_TSizeType)(_List.size()));
	}
	template<typename _Ty>
	PE_NODISCARD PE_CONSTEXPR const _Ty* InitializerListBegin(TInitializerList<_Ty> _List)
	{
		return (_List.begin());
	}
	template<typename _Ty>
	PE_NODISCARD PE_CONSTEXPR const _Ty* InitializerListEnd(TInitializerList<_Ty> _List)
	{
		return (_List.end());
	}

	template<typename _Ty>
	struct TReversePointerIterator
	{
	public:
		PE_CONSTEXPR explicit TReversePointerIterator(_Ty* InPtr)
			: Ptr(InPtr)
		{
		}
		PE_CONSTEXPR PE_INLINE _Ty& operator*()const
		{
			return (*(Ptr - 1));
		}
		PE_CONSTEXPR PE_INLINE TReversePointerIterator& operator++()const
		{
			--Ptr;
			return (*this);
		}
		PE_CONSTEXPR PE_INLINE TReversePointerIterator& operator--()const
		{
			++Ptr;
			return (*this);
		}
		PE_CONSTEXPR PE_INLINE BOOL8 operator!=(const TReversePointerIterator& _In)const
		{
			return (Ptr != (_In.Ptr));
		}
		PE_CONSTEXPR PE_INLINE BOOL8 operator==(const TReversePointerIterator& _In)const
		{
			return (Ptr == (_In.Ptr));
		}
	private:
		_Ty* Ptr;
	};

	class EMemory final
	{
	public:
		static PE_FORCEINLINE PE_NODISCARD void* Allocate(size_t size)
		{
			return std::malloc(size);
		}

		static PE_FORCEINLINE void Deallocate(void* ptr)
		{
			std::free(ptr);
		}

		template<typename _Ty, typename... Args>
		static PE_FORCEINLINE PE_NODISCARD _Ty* Construct(Args&&... args)
		{
			void* ptr = Allocate(sizeof(_Ty));
			return ::new (ptr) _Ty(static_cast<Args&&>(args)...);
		}

		template<typename _Ty>
		static PE_FORCEINLINE void Destroy(_Ty* ptr)
		{
			if (ptr)
			{
				ptr->~_Ty();
				Deallocate(ptr);
			}
		}

	public:
		template <typename _TSizeType = UINT32>
		static PE_FORCEINLINE void Memcpy(void* _Dst, void const* _Src, _TSizeType _Size)
		{
			std::memcpy(_Dst, _Src, static_cast<size_t>(_Size));
		}

		template <typename _TSizeType = UINT32>
		static PE_FORCEINLINE void Memset(void* _Dst, UINT8 _Val, _TSizeType _Size)
		{
			std::memset(_Dst, _Val, static_cast<size_t>(_Size));
		}

		template <typename _TSizeType = UINT32>
		static PE_FORCEINLINE void Memmov(void* _Dst, void const* _Src, _TSizeType _Size)
		{
			std::memmove(_Dst, _Src, static_cast<size_t>(_Size));
		}
	public:
		template <typename _Ty>
		static PE_FORCEINLINE PE_NODISCARD PE_CONSTEXPR TRemoveRefType<_Ty>&& Move(_Ty&& _In)noexcept
		{
			return std::move<_Ty>(std::forward<_Ty>(_In));
		}

		template <typename _Ty>
		static PE_FORCEINLINE PE_NODISCARD PE_CONSTEXPR _Ty&& Forward(TRemoveRefType<_Ty>& _In)noexcept
		{
			return std::forward<_Ty>(_In);
		}

		template <typename _Ty>
		static PE_FORCEINLINE PE_NODISCARD PE_CONSTEXPR _Ty&& Forward(TRemoveRefType<_Ty>&& _In)noexcept
		{
			return std::forward<_Ty>(_In);
		}

		template <typename _Ty, typename... _TArguments>
		static PE_NODISCARD PE_FORCEINLINE TUniquePtr<_Ty> MakeUnique(_TArguments&&... _InArgs)
		{
			return std::make_unique<_Ty, _TArguments...>(std::forward<_TArguments>(_InArgs)...);
		}

		template <typename _Ty, typename _TSizeType = UINT32>
		static PE_NODISCARD PE_FORCEINLINE TUniquePtr<_Ty> MakeUnique(const _TSizeType _InSize)
		{
			return std::make_unique<_Ty>(static_cast<size_t>(_InSize));
		}

		template <typename _Ty, typename... _TArguments>
		static PE_FORCEINLINE PE_NODISCARD TSharedPtr<_Ty> MakeShared(_TArguments&&... _InArgs)
		{
			return std::make_shared<_Ty, _TArguments...>(std::forward<_TArguments>(_InArgs)...);
		}

		SMART_PTR_GET(TUniquePtr)
		SMART_PTR_GET(TSharedPtr)
		SMART_PTR_GET(TWeakPtr)

	private:
		EMemory()noexcept {}
		EMemory(const EMemory&)noexcept {}
		EMemory(EMemory&)noexcept {}
		~EMemory() {}
		EMemory& operator=(const EMemory&)noexcept { return (*this); }
		EMemory& operator=(EMemory&&)noexcept { return (*this); }
	public:
		static EMemory* GetSingleton()
		{
			static EMemory _MemorySingletonObject;
			return (&_MemorySingletonObject);
		}
	};

	// New with arguments
	template<typename _Ty, typename... Args>
	PE_FORCEINLINE PE_NODISCARD _Ty*  New(Args&&... args)
	{
		return EMemory::Construct<_Ty>(static_cast<Args&&>(args)...);
	}

	// New
	template<typename _Ty>
	PE_FORCEINLINE PE_NODISCARD _Ty* New()
	{
		return EMemory::Construct<_Ty>();
	}

	// New Array
	template<typename _Ty>
	PE_FORCEINLINE PE_NODISCARD _Ty* NewArray(size_t count)
	{
		if (count == 0) return nullptr;

		void* ptr = EMemory::Allocate(sizeof(_Ty) * count);
		_Ty* array = static_cast<_Ty*>(ptr);

		// Construct each element in the array
		for (size_t i = 0; i < count; ++i)
		{
			::new (&array[i]) _Ty();
		}

		return array;
	}

	// Delete
	template<typename _Ty>
	PE_FORCEINLINE void Delete(_Ty* ptr)
	{
		EMemory::Destroy(ptr);
	}

	// Delete Array
	template<typename _Ty>
	PE_FORCEINLINE void DeleteArray(_Ty* ptr, size_t count)
	{
		if (ptr && count > 0)
		{
			for (size_t i = 0; i < count; ++i)
			{
				ptr[i].~_Ty();
			}
			EMemory::Deallocate(ptr);
		}
	}

	using THeapArrayType = UINT8[];

	template<typename _TSizeType,
		_TSizeType _AllocateSize,
		TEnableIfType<TIsIntegral<_TSizeType>::value, _TSizeType> = 0>
	class THeapBase final
	{
	public:
		PE_INLINE void Growth()
		{
			Append(_AllocateSize);
		}
		template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
		PE_INLINE void Reallocate(_TOtherSizeType InSize)
		{
			if (HeapSize == ((_TSizeType)(InSize)))
			{
				return;
			}
			Release();
			HeapSize = (_TSizeType)(InSize);
			if (HeapSize > (_TSizeType)0)
			{
				HeapPtr = EMemory::MakeUnique<THeapArrayType>(HeapSize);
			}
		}
		PE_INLINE void Release()
		{
			if (HeapPtr)
			{
				HeapPtr = nullptr;
			}
			HeapSize = (_TSizeType)0;
		}
		template<typename _TOtherSizeType, _TOtherSizeType _OtherAllocateSize>
		PE_INLINE void Append(const THeapBase<_TOtherSizeType, _OtherAllocateSize>& Other)
		{
			if (Other.HeapSize > (_TOtherSizeType)0)
			{
				_TSizeType NewSize = ((_TSizeType)(Other.HeapSize)) + HeapSize;
				Check((NewSize > (_TOtherSizeType)0));
				TUniquePtr<THeapArrayType> NewHeapPtr = EMemory::MakeUnique<THeapArrayType>(NewSize);
				UINT8* RawDataPtr = EMemory::GetPtr(NewHeapPtr);
				if (HeapSize > (_TSizeType)0)
				{
					EMemory::Memmov(RawDataPtr, Data(), HeapSize);
					RawDataPtr = &(RawDataPtr[HeapSize]);
				}
				EMemory::Memcpy(RawDataPtr, Other.Data(), Other.HeapSize);
				HeapPtr = EMemory::Move(NewHeapPtr);
				HeapSize = NewSize;
			}
		}
		template<typename _TOtherSizeType, _TOtherSizeType _OtherAllocateSize>
		PE_INLINE void Append(THeapBase<_TOtherSizeType, _OtherAllocateSize>&& Other)
		{
			if (Other.HeapSize > (_TOtherSizeType)0)
			{
				_TSizeType NewSize = ((_TSizeType)(Other.HeapSize)) + HeapSize;
				Check((NewSize > (_TOtherSizeType)0));
				TUniquePtr<THeapArrayType> NewHeapPtr = EMemory::MakeUnique<THeapArrayType>(NewSize);
				UINT8* RawDataPtr = EMemory::GetPtr(NewHeapPtr);
				if (HeapSize > (_TSizeType)0)
				{
					EMemory::Memmov(RawDataPtr, Data(), HeapSize);
					RawDataPtr = &(RawDataPtr[HeapSize]);
				}
				EMemory::Memmov(RawDataPtr, Other.Data(), Other.HeapSize);
				HeapPtr = EMemory::Move(NewHeapPtr);
				HeapSize = NewSize;
				Other.Release();
			}
		}
		template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
		PE_INLINE void Append(_TOtherSizeType InExtentSize)
		{
			if (InExtentSize > (_TOtherSizeType)0)
			{
				_TSizeType NewSize = ((_TSizeType)(InExtentSize)) + HeapSize;
				Check((NewSize > (_TOtherSizeType)0));
				TUniquePtr<THeapArrayType> NewHeapPtr = EMemory::MakeUnique<THeapArrayType>(NewSize);
				if (HeapSize > (_TSizeType)0)
				{
					UINT8* RawDataPtr = EMemory::GetPtr(NewHeapPtr);
					EMemory::Memmov(RawDataPtr, Data(), HeapSize);
				}
				HeapPtr = EMemory::Move(NewHeapPtr);
				HeapSize = NewSize;
			}
		}
		template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
		PE_INLINE void Substract(_TOtherSizeType InExtentSize)
		{
			if (InExtentSize > (_TOtherSizeType)0)
			{
				_TSizeType RestSize = (HeapSize >= ((_TSizeType)(InExtentSize))) ? (HeapSize - ((_TSizeType)(InExtentSize))) : ((_TSizeType)0);
				TUniquePtr<THeapArrayType> NewHeapPtr = nullptr;
				if (RestSize > (_TSizeType)0)
				{
					NewHeapPtr = EMemory::MakeUnique<THeapArrayType>(RestSize);
					UINT8* RawDataPtr = EMemory::GetPtr(NewHeapPtr);
					EMemory::Memmov(RawDataPtr, Data(), RestSize);
				}
				HeapPtr = NewHeapPtr ? EMemory::Move(NewHeapPtr) : nullptr;
				HeapSize = RestSize;
			}
		}
		template<typename _TOtherSizeType, _TOtherSizeType _OtherAllocateSize>
		PE_INLINE void Copy(const THeapBase<_TOtherSizeType, _OtherAllocateSize>& Other)
		{
			if ((Other.Size<_TOtherSizeType>() > (_TOtherSizeType)0))
			{
				if (HeapSize < Other.Size<_TSizeType>())
				{
					Reallocate(Other.Size<_TSizeType>());
				}
				EMemory::Memcpy(Data(), Other.Data(), Other.Size<_TOtherSizeType>());
			}
		}
		template<typename _TTruncSizeType, typename _TOtherSizeType, _TOtherSizeType _OtherAllocateSize
			, TEnableIfType<TIsIntegral<_TTruncSizeType>::value, _TTruncSizeType> = 0, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
		PE_INLINE void Copy(const THeapBase<_TOtherSizeType, _OtherAllocateSize>& Other, _TTruncSizeType InTruncSize)
		{
			if ((Other.Size<_TOtherSizeType>() > (_TOtherSizeType)0) &&
				(InTruncSize > (_TTruncSizeType)0) &&
				(InTruncSize <= Other.Size<_TTruncSizeType>()))
			{
				if (HeapSize < ((_TSizeType)(InTruncSize)))
				{
					Reallocate(InTruncSize);
				}
				EMemory::Memcpy(Data(), Other.Data(), InTruncSize);
			}
		}
		template<typename _TOtherSizeType, _TOtherSizeType _OtherAllocateSize>
		PE_INLINE void Move(THeapBase<_TOtherSizeType, _OtherAllocateSize>&& Other)
		{
			if ((Other.Size<_TOtherSizeType>() > (_TOtherSizeType)0))
			{
				Release();
				HeapPtr = EMemory::Move(Other.HeapPtr);
				HeapSize = (_TSizeType)(Other.HeapSize);
				Other.Release();
			}
		}
		PE_INLINE PE_NODISCARD void* Data()
		{
			return ((void*)(EMemory::GetPtr(HeapPtr)));
		}
		PE_INLINE PE_NODISCARD const void* Data()const
		{
			return ((const void*)(EMemory::GetPtr(HeapPtr)));
		}
		template<typename _TResultType = _TSizeType, TEnableIfType<TIsIntegral<_TResultType>::value, _TResultType> = 0>
		PE_INLINE PE_NODISCARD _TResultType Size()const
		{
			if PE_CONSTEXPR(TIsSame<_TResultType, _TSizeType>::value)
			{
				return (HeapSize);
			}
			else
			{
				return ((_TResultType)HeapSize);
			}
		}
	public:
		~THeapBase()
		{
			Release();
		}
		PE_FORCEINLINE THeapBase()noexcept
			: HeapPtr(nullptr), HeapSize((_TSizeType)0)
		{
			Reallocate(_AllocateSize);
		}
		template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
		PE_FORCEINLINE THeapBase(_TOtherSizeType InInitSize)noexcept
			: HeapPtr(nullptr), HeapSize((_TSizeType)0)
		{
			Reallocate(InInitSize);
		}
		template<typename _TOtherSizeType, _TOtherSizeType _OtherAllocateSize>
		PE_FORCEINLINE explicit THeapBase(const THeapBase<_TOtherSizeType, _OtherAllocateSize>& Other)noexcept
			: HeapPtr(nullptr), HeapSize((_TSizeType)0)
		{
			Copy(Other);
		}
		template<typename _TOtherSizeType, _TOtherSizeType _OtherAllocateSize>
		PE_FORCEINLINE explicit THeapBase(THeapBase<_TOtherSizeType, _OtherAllocateSize>&& Other)noexcept
			: HeapPtr(nullptr), HeapSize((_TSizeType)0)
		{
			Move(EMemory::Forward<THeapBase<_TOtherSizeType, _OtherAllocateSize>>(Other));
		}
		PE_FORCEINLINE THeapBase(const THeapBase& Other)noexcept
			: HeapPtr(nullptr), HeapSize((_TSizeType)0)
		{
			Copy(Other);
		}
		PE_FORCEINLINE THeapBase(THeapBase&& Other)noexcept
			: HeapPtr(nullptr), HeapSize((_TSizeType)0)
		{
			Move(EMemory::Forward<THeapBase>(Other));
		}
		template<typename _TOtherSizeType, _TOtherSizeType _OtherAllocateSize>
		THeapBase& operator=(const THeapBase<_TOtherSizeType, _OtherAllocateSize>& Other)noexcept
		{
			Release();
			if (Other.HeapSize > (_TOtherSizeType)0)
			{
				Reallocate(Other.HeapSize);
				Copy(Other);
			}
			return (*this);
		}
		template<typename _TOtherSizeType, _TOtherSizeType _OtherAllocateSize>
		THeapBase& operator=(THeapBase<_TOtherSizeType, _OtherAllocateSize>&& Other)noexcept
		{
			Release();
			if (Other.HeapSize > (_TOtherSizeType)0)
			{
				Move(EMemory::Forward<THeapBase<_TOtherSizeType, _OtherAllocateSize>>(Other));
			}
			return (*this);
		}
		THeapBase& operator=(const THeapBase& Other)noexcept
		{
			Release();
			if (Other.HeapSize > (_TSizeType)0)
			{
				Reallocate(Other.HeapSize);
				Copy(Other);
			}
			return (*this);
		}
		THeapBase& operator=(THeapBase&& Other)noexcept
		{
			Release();
			if (Other.HeapSize > (_TSizeType)0)
			{
				Move(EMemory::Forward<THeapBase>(Other));
			}
			return (*this);
		}
	private:
		TUniquePtr<THeapArrayType>	HeapPtr;
		_TSizeType					HeapSize;
	};

	template<typename _Ty, typename _TSizeType, _TSizeType _AllocateStepSize, TEnableIfType<TIsIntegral<_TSizeType>::value, _TSizeType> = 0>
	class TAllocatorBase
	{
	public:
		template<typename _TResultType = _TSizeType, TEnableIfType<TIsIntegral<_TResultType>::value, _TResultType> = 0>
		PE_CONSTEXPR_STATIC _TResultType AllocateStepSize = (_TResultType)_AllocateStepSize;
		template<typename _TResultType = _TSizeType, TEnableIfType<TIsIntegral<_TResultType>::value, _TResultType> = 0>
		PE_CONSTEXPR_STATIC _TResultType AllocateHeapStepSize = ((_TResultType)_AllocateStepSize) * sizeof(TObjectType);

		using TObjectType	= _Ty;
		using THeapType		= THeapBase<_TSizeType, _AllocateStepSize * sizeof(TObjectType)>;
	public:
		PE_INLINE void ZeroHeapMemory()
		{
			EMemory::Memset(Heap.Data(), 0x0u, Heap.Size<_TSizeType>());
		}
		PE_INLINE void Growth()
		{
			Heap.Growth();
		}
		template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
		PE_INLINE void Reallocate(_TOtherSizeType InSize)
		{
			Heap.Reallocate(InSize);
		}
		PE_INLINE void Release()
		{
			Heap.Release();
		}
		template<typename _TOtherSizeType, _TOtherSizeType _OtherAllocateStepSize>
		PE_INLINE void Append(const TAllocatorBase<TObjectType, _TOtherSizeType, _OtherAllocateStepSize>& Other)
		{
			Heap.Append(Other.Heap);
		}
		template<typename _TOtherSizeType, _TOtherSizeType _OtherAllocateStepSize>
		PE_INLINE void Append(TAllocatorBase<TObjectType, _TOtherSizeType, _OtherAllocateStepSize>&& Other)
		{
			Heap.Append(EMemory::Forward<TAllocatorBase<TObjectType, _TOtherSizeType, _OtherAllocateStepSize>::THeapType>(Other.Heap));
		}
		template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
		PE_INLINE void Append(_TOtherSizeType InExtentSize)
		{
			Heap.Append(InExtentSize);
		}
		template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
		PE_INLINE void Substract(_TOtherSizeType InExtentSize)
		{
			Heap.Substract(InExtentSize);
		}
		template<typename _TOtherSizeType, _TOtherSizeType _OtherAllocateStepSize>
		PE_INLINE void CopyFrom(const TAllocatorBase<TObjectType, _TOtherSizeType, _OtherAllocateStepSize>& Other)
		{
			Heap.Copy(Other.Heap);
		}
		template<typename _TTruncSizeType, typename _TOtherSizeType, _TOtherSizeType _OtherAllocateStepSize
			, TEnableIfType<TIsIntegral<_TTruncSizeType>::value, _TTruncSizeType> = 0, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
		PE_INLINE void CopyFrom(const TAllocatorBase<TObjectType, _TOtherSizeType, _OtherAllocateStepSize>& Other, _TTruncSizeType InTruncSize)
		{
			Heap.Copy(Other.Heap, InTruncSize);
		}
		template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
		PE_INLINE void CopyFrom(const void* InPtr, _TOtherSizeType InMemorySize)
		{
			if (Heap.Size<_TOtherSizeType>() < InMemorySize)
			{
				Heap.Reallocate(InMemorySize);
			}
			Check((InMemorySize > (_TOtherSizeType)0));
			EMemory::Memcpy(Heap.Data(), InPtr, InMemorySize);
		}
		template<typename _TOtherSizeType, _TOtherSizeType _OtherAllocateStepSize>
		PE_INLINE void MoveFrom(TAllocatorBase<TObjectType, _TOtherSizeType, _OtherAllocateStepSize>&& Other)
		{
			Release();
			if (Other.HeapSize<_TOtherSizeType>() > (_TOtherSizeType)0)
			{
				Heap.Move(EMemory::Forward<TAllocatorBase<TObjectType, _TOtherSizeType, _OtherAllocateStepSize>::THeapType>(Other.Heap));
			}
		}
		PE_INLINE PE_NODISCARD TObjectType* HeapData()
		{
			return ((TObjectType*)(Heap.Data()));
		}
		PE_INLINE PE_NODISCARD const TObjectType* HeapData()const
		{
			return ((const TObjectType*)(Heap.Data()));
		}
		template<typename _TResultType = _TSizeType, TEnableIfType<TIsIntegral<_TResultType>::value, _TResultType> = 0>
		PE_INLINE PE_NODISCARD _TResultType HeapSize()const
		{
			return (Heap.Size<_TResultType>());
		}
	public:
		virtual ~TAllocatorBase()
		{
			Release();
		}
		PE_FORCEINLINE TAllocatorBase()noexcept
		{
		}
		template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
		PE_FORCEINLINE TAllocatorBase(_TOtherSizeType InInitSize)noexcept
			: Heap(InInitSize)
		{
		}
	public:
		template<typename _TOtherSizeType, _TOtherSizeType _OtherAllocateStepSize>
		PE_FORCEINLINE TAllocatorBase(const TAllocatorBase<TObjectType, _TOtherSizeType, _OtherAllocateStepSize>& Other)noexcept = delete;
		template<typename _TOtherSizeType, _TOtherSizeType _OtherAllocateStepSize>
		PE_FORCEINLINE TAllocatorBase(TAllocatorBase<TObjectType, _TOtherSizeType, _OtherAllocateStepSize>&& Other)noexcept = delete;
		TAllocatorBase(const TAllocatorBase& Other)noexcept = delete;
		TAllocatorBase(TAllocatorBase&& Other)noexcept = delete;

		template<typename _TOtherSizeType, _TOtherSizeType _OtherAllocateStepSize>
		TAllocatorBase& operator=(const TAllocatorBase<TObjectType, _TOtherSizeType, _OtherAllocateStepSize>& Other)noexcept = delete;
		template<typename _TOtherSizeType, _TOtherSizeType _OtherAllocateStepSize>
		TAllocatorBase& operator=(TAllocatorBase<TObjectType, _TOtherSizeType, _OtherAllocateStepSize>&& Other)noexcept = delete;
		TAllocatorBase& operator=(const TAllocatorBase& Other)noexcept = delete;
		TAllocatorBase& operator=(TAllocatorBase&& Other)noexcept = delete;
	private:
		THeapType Heap;
	};

	template<typename _Ty, typename _TSizeType = INT32, _TSizeType _DefaultAllocateSize = (_TSizeType)8>
	using TDefaultAllocator = TAllocatorBase<_Ty, _TSizeType, _DefaultAllocateSize>;

#endif

};