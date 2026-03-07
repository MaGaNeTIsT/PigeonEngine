#pragma once

#include "../../../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include "../../../../Core/Config/ErrorCaption.h"
#include "../../../../../Development/Alert/DevelopmentDefines.h"
#include "Base/Math/Math.h"
#include "Base/Memory/Memory.h"

namespace PigeonEngine
{
#if 1
    struct EBitReference
    {
    public:
        PE_FORCEINLINE explicit EBitReference(UINT32& InData, UINT32 InMask)
            : Data(InData), Mask(InMask)
        {
        }
        PE_FORCEINLINE operator BOOL8()const
        {
            return ((Data & Mask) != 0u);
        }
        PE_FORCEINLINE void operator=(const BOOL8 InValue)
        {
            if (InValue)
            {
                Data |= Mask;
            }
            else
            {
                Data &= (~Mask);
            }
        }
        PE_FORCEINLINE void operator|=(const BOOL8 InValue)
        {
            if (InValue)
            {
                Data |= Mask;
            }
        }
        PE_FORCEINLINE void operator&=(const BOOL8 InValue)
        {
            if (!InValue)
            {
                Data &= (~Mask);
            }
        }
        PE_FORCEINLINE EBitReference& operator=(const EBitReference& Other)
        {
            (*this) = (BOOL8)Other;
            return (*this);
        }
    private:
        UINT32& Data;
        UINT32 Mask;
    };

    struct EConstBitReference
    {
    public:
        PE_FORCEINLINE explicit EConstBitReference(const UINT32& InData, UINT32 InMask)
            : Data(InData), Mask(InMask)
        {
        }
        PE_FORCEINLINE operator BOOL8()const
        {
            return ((Data & Mask) != 0u);
        }
    private:
        const UINT32& Data;
        UINT32 Mask;
    };

    struct ERelativeBitReference
    {
    public:
        PE_CONSTEXPR static UINT32 BitZeroPoint = 0x80000000u;
        PE_CONSTEXPR static UINT32 Uint32BitLength = 0x5u;
    public:
        PE_FORCEINLINE explicit ERelativeBitReference(UINT32 InIndex)
            : PartIndex(InIndex >> Uint32BitLength), Mask(BitZeroPoint >> ((InIndex)& (31u)))
        {
        }
        PE_FORCEINLINE BOOL8 operator==(const ERelativeBitReference& Other)const
        {
            return ((PartIndex == Other.PartIndex) && (Mask == Other.Mask));
        }
        PE_FORCEINLINE BOOL8 operator!=(const ERelativeBitReference& Other)const
        {
            return (!((*this) == Other));
        }
    public:
        UINT32 PartIndex;
        UINT32 Mask;
    };

    template <typename _TSizeType = INT32,
        typename _TAllocator = TDefaultAllocator<UINT32, _TSizeType>,
        TEnableIfType<TIsIntegral<_TSizeType>::value, _TSizeType> = 0,
        TEnableIfType<TIsSame<_TAllocator::TObjectType, UINT32>::value, _TSizeType> = 0>
    class TBitArray final
    {
    public:
        template<typename _TResultType = _TSizeType, TEnableIfType<TIsIntegral<_TResultType>::value, _TResultType> = 0>
        PE_CONSTEXPR_STATIC PE_FORCEINLINE _TResultType ElementNumBits = (_TResultType)32;
        template<typename _TResultType = _TSizeType, TEnableIfType<TIsIntegral<_TResultType>::value, _TResultType> = 0>
        PE_CONSTEXPR_STATIC PE_FORCEINLINE _TResultType ElementTypeSize = (_TResultType)(sizeof(UINT32));

        using TElementType      = UINT32;
        using TAllocatorType    = _TAllocator;

        struct TIterator : public ERelativeBitReference
        {
        public:
            PE_FORCEINLINE explicit TIterator(TBitArray<TAllocatorType>& InArray, UINT32 InStartIndex = 0u)
                : ERelativeBitReference(InStartIndex), Array(InArray), Index(InStartIndex)
            {
            }
            PE_FORCEINLINE TIterator& operator++()
            {
                ++Index;
                Mask >>= 0x1u;
                if (Mask == 0u)
                {
                    Mask = ERelativeBitReference::BitZeroPoint;
                    ++PartIndex;
                }
                return (*this);
            }
            PE_FORCEINLINE EBitReference operator*()const
            {
                return GetValue();
            }
            PE_FORCEINLINE explicit operator BOOL8()const
            {
                return (Index < (Array.Num<UINT32>()));
            }
            PE_FORCEINLINE BOOL8 operator!()const
            {
                return (!((BOOL8)(*this)));
            }
            PE_FORCEINLINE EBitReference GetValue()const
            {
                return EBitReference(Array.GetData()[PartIndex], Mask);
            }
            PE_FORCEINLINE UINT32 GetIndex()const
            {
                return Index;
            }
        private:
            TBitArray<TAllocatorType>& Array;
            UINT32 Index;
        };

        struct TConstIterator : public ERelativeBitReference
        {
        public:
            PE_FORCEINLINE explicit TConstIterator(const TBitArray<TAllocatorType>& InArray, UINT32 InStartIndex = 0u)
                : ERelativeBitReference(InStartIndex), Array(InArray), Index(InStartIndex)
            {
            }
            PE_FORCEINLINE TConstIterator& operator++()
            {
                ++Index;
                Mask >>= 0x1u;
                if (Mask == 0u)
                {
                    Mask = ERelativeBitReference::BitZeroPoint;
                    ++PartIndex;
                }
                return (*this);
            }
            PE_FORCEINLINE EConstBitReference operator*()const
            {
                return GetValue();
            }
            PE_FORCEINLINE explicit operator BOOL8()const
            {
                return (Index < (Array.Num<UINT32>()));
            }
            PE_FORCEINLINE BOOL8 operator!()const
            {
                return (!((BOOL8)(*this)));
            }
            PE_FORCEINLINE EConstBitReference GetValue()const
            {
                return EConstBitReference(Array.GetData()[PartIndex], Mask);
            }
            PE_FORCEINLINE UINT32 GetIndex()const
            {
                return Index;
            }
        private:
            const TBitArray<TAllocatorType>& Array;
            UINT32 Index;
        };

        struct TConstReverseIterator : public ERelativeBitReference
        {
        public:
            PE_FORCEINLINE explicit TConstReverseIterator(const TBitArray<TAllocatorType>& InArray)
                : ERelativeBitReference(InArray.Num<UINT32>() - 1u), Array(InArray), Index(InArray.Num<UINT32>() - 1u)
            {
            }
            PE_FORCEINLINE TIterator& operator++()
            {
                --Index;
                Mask <<= 0x1u;
                if (Mask == 0u)
                {
                    Mask = 0x1u;
                    --PartIndex;
                }
                return (*this);
            }
            PE_FORCEINLINE explicit operator BOOL8()const
            {
                return (Index >= 0u);
            }
            PE_FORCEINLINE BOOL8 operator!()const
            {
                return (!((BOOL8)(*this)));
            }
            PE_FORCEINLINE EBitReference GetValue()const
            {
                return EBitReference(Array.GetData()[PartIndex], Mask);
            }
            PE_FORCEINLINE UINT32 GetIndex()const
            {
                return Index;
            }
        private:
            const TBitArray<TAllocatorType>& Array;
            UINT32 Index;
        };
    public:
        template<typename _TResultType = _TSizeType, TEnableIfType<TIsIntegral<_TResultType>::value, _TResultType> = 0>
        _TResultType Add(const _Ty& InElement)
        {
            if PE_CONSTEXPR(!(TIsScalar<_Ty>::value))
            {
                PushInternal(InElement);
            }
            else
            {
                PushScalarInternal(InElement);
            }
            Check((ElementNum > (_TSizeType)0));
            return ((_TResultType)(ElementNum - (_TSizeType)1));
        }
        template<typename _TResultType = _TSizeType, TEnableIfType<TIsIntegral<_TResultType>::value, _TResultType> = 0>
        _TResultType Add(_Ty&& InElement)
        {
            if PE_CONSTEXPR(!(TIsScalar<_Ty>::value))
            {
                PushInternal(EMemory::Forward<_Ty>(InElement));
            }
            else
            {
                PushScalarInternal(EMemory::Forward<_Ty>(InElement));
            }
            Check((ElementNum > (_TSizeType)0));
            return ((_TResultType)(ElementNum - (_TSizeType)1));
        }
        template<typename _TOtherIndexType, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        PE_NODISCARD EBitReference operator[](_TOtherIndexType InIndex)
        {
            const UINT32 UsedIndex = (UINT32)InIndex;
            Check(((UsedIndex >= 0u) && (InIndex < (UINT32)NumBits)));
            return EBitReference(GetData()[UsedIndex / TBitArray::ElementNumBits<UINT32>], ERelativeBitReference::BitZeroPoint >> (UsedIndex % TBitArray::ElementNumBits<UINT32>));
        }
        template<typename _TOtherIndexType, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        PE_NODISCARD EConstBitReference operator[](_TOtherIndexType InIndex)const
        {
            const UINT32 UsedIndex = (UINT32)InIndex;
            Check(((UsedIndex >= 0u) && (InIndex < (UINT32)NumBits)));
            return EConstBitReference(GetData()[UsedIndex / TBitArray::ElementNumBits<UINT32>], ERelativeBitReference::BitZeroPoint >> (UsedIndex % TBitArray::ElementNumBits<UINT32>));
        }
        EBitReference AddDefaultGetRef()
        {
            return PushDefaultInternal();
        }
        template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        void SetNum(_TOtherSizeType InElementNum)
        {
            const _TSizeType NewElementNum = (_TSizeType)(InElementNum);
            if (ElementNum < NewElementNum)
            {
                const _TSizeType AddElementNum = NewElementNum - ElementNum;
                AppendElementInternal(AddElementNum);
            }
            if (ElementNum > NewElementNum)
            {
                for (_TSizeType i = ElementNum - (_TSizeType)1; i >= NewElementNum; i--)
                {
                    PopInternal();
                }
            }
        }
        template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        void Reserve(_TOtherSizeType InElementNum)
        {
            const _TSizeType CurrentCapacity = Capacity();
            if (((_TSizeType)(InElementNum)) <= CurrentCapacity)
            {
                return;
            }
            AppendCapacityInternal(((_TSizeType)(InElementNum)) - CurrentCapacity);
        }
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        void Append(const TArray<_Ty, _TOtherSizeType, _TOtherAllocator>& Other)
        {
            const _TSizeType RestCapacity = Capacity() - ElementNum;
            const _TSizeType OtherNum = Other.Num<_TSizeType>();
            if (RestCapacity < OtherNum)
            {
                AppendCapacityInternal(OtherNum - RestCapacity);
            }
            if PE_CONSTEXPR(!(TIsScalar<_Ty>::value))
            {
                for (_TSizeType i = (_TSizeType)0; i < OtherNum; i++)
                {
                    PushInternal(Other[i]);
                }
            }
            else
            {
                if (OtherNum > (_TSizeType)0)
                {
                    EMemory::Memcpy((&(GetData()[ElementNum])), Other.GetData(), OtherNum);
                    ElementNum = ElementNum + OtherNum;
                }
            }
        }
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        void Append(TBitArray<_TOtherSizeType, _TOtherAllocator>&& Other)
        {
            const _TSizeType RestElementNum = ElementCapacity() - ElementNum();
            const _TSizeType OtherNum = Other.Num<_TSizeType>();
            if (RestCapacity < OtherNum)
            {
                AppendCapacityInternal(OtherNum - RestCapacity);
            }
            if (OtherNum > (_TSizeType)0)
            {
                EMemory::Memmov((&(GetData()[ElementNum])), Other.GetData(), OtherNum);
                ElementNum = ElementNum + OtherNum;

                Other.ElementNum = 0;
                Other.Allocator.Release();
            }
        }
        template<typename _TFirstIndexType, typename _TSecondIndexType
            , TEnableIfType<TIsIntegral<_TFirstIndexType>::value, _TFirstIndexType> = 0, TEnableIfType<TIsIntegral<_TSecondIndexType>::value, _TSecondIndexType> = 0>
        void Swap(_TFirstIndexType InFirstIndex, _TSecondIndexType InSecondIndex)
        {
            Check(((InFirstIndex >= (_TFirstIndexType)0) && (((_TSizeType)(InFirstIndex)) < NumBits)));
            Check(((InSecondIndex >= (_TSecondIndexType)0) && (((_TSizeType)(InSecondIndex)) < NumBits)));
            const BOOL8 TempItem = (BOOL8)((*this)[InFirstIndex]);
            (*this)[InFirstIndex] = (BOOL8)((*this)[InSecondIndex]);
            (*this)[InSecondIndex] = TempItem;
        }
        template<typename _TOtherIndexType, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        PE_NODISCARD EBitReference GetRef(_TOtherIndexType InIndex)
        {
            return ((*this)[InIndex]);
        }
        template<typename _TOtherIndexType, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        PE_NODISCARD EConstBitReference GetRef(_TOtherIndexType InIndex)const
        {
            return ((*this)[InIndex]);
        }
        PE_NODISCARD EBitReference FirstRef()
        {
            return ((*this)[0]);
        }
        PE_NODISCARD EConstBitReference FirstRef()const
        {
            return ((*this)[0]);
        }
        PE_NODISCARD EBitReference LastRef()
        {
            return ((*this)[NumBits - (_TSizeType)1]);
        }
        PE_NODISCARD EConstBitReference LastRef()const
        {
            return ((*this)[NumBits - (_TSizeType)1]);
        }
        template<typename _TOtherIndexType, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        void RemoveAtSwap(_TOtherIndexType InBitIndex)
        {
            Check(((InBitIndex >= (_TOtherIndexType)0) && (((_TSizeType)(InBitIndex)) < NumBits)));
            const _TSizeType UsedIndex = (_TSizeType)(InBitIndex);
            const _TSizeType LastIndex = NumBits - (_TSizeType)1;
            if (UsedIndex != LastIndex)
            {
                Swap(UsedIndex, LastIndex);
            }
            PopInternal();
        }
        void Pop()
        {
            Check((ElementNum > (_TSizeType)0));
            PopInternal();
        }
        void Reset()
        {
            EmptyInternal(Capacity());
        }
        template<typename _TOtherSizeType = _TSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        void Empty(_TOtherSizeType InBitNum = (_TOtherSizeType)0)
        {
            EmptyInternal(InBitNum);
        }
        //TODO ^^^^^^^^^
        template<typename _TOtherIndexType = _TSizeType, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        PE_NODISCARD _TOtherIndexType FindFirst(BOOL8 InValue)const
        {
            for (_TOtherIndexType i = (_TOtherIndexType)0, n = (_TOtherIndexType)(NumBits); i < n; i++)
            {
                if (((BOOL8)((*this)[i])) == InValue)
                {
                    return i;
                }
            }
            return ((_TOtherIndexType)(-1));
        }
        template<typename _TOtherIndexType, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        BOOL8 FindFirst(BOOL8 InValue, _TOtherIndexType& OutIndex)const
        {
            for (_TOtherIndexType i = (_TOtherIndexType)0, n = (_TOtherIndexType)(NumBits); i < n; i++)
            {
                if (((BOOL8)((*this)[i])) == InValue)
                {
                    OutIndex = i;
                    return TRUE;
                }
            }
            return FALSE;
        }
        PE_NODISCARD BOOL8 Contains(BOOL8 InValue)const
        {
            Check((TIsUnsigned<TElementType>::value));
            TElementType* DataPtr = GetData();
            const _TSizeType ElemNum = (ElementNum<_TSizeType>() > (_TSizeType)0) ? (ElementNum<_TSizeType>() - (_TSizeType)1) : (_TSizeType)0;
            for (_TSizeType i = (_TSizeType)0; i < ElemNum; i++)
            {
                if (InValue && (DataPtr[i] > (TElementType)0))
                {
                    return TRUE;
                }
                else if ((!InValue) && (DataPtr[i] < ((TElementType)(-1))))
                {
                    return TRUE;
                }
            }
            for (_TSizeType i = ElemNum * sizeof(TElementType) * (_TSizeType)8, n = NumBits; i < n; i++)
            {
                if (((*this)[i]) == InValue)
                {
                    return TRUE;
                }
            }
            return FALSE;
        }
    public:
        ~TBitArray()
        {
            EmptyInternal();
        }
        PE_FORCEINLINE TBitArray()noexcept
            : NumBits((_TSizeType)0)
        {
        }
        template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        PE_FORCEINLINE TBitArray(_TOtherSizeType InBitNum)noexcept
            : NumBits((_TSizeType)0)
        {
            AppendElementInternal(InBitNum);
        }
        TBitArray(TInitializerList<BOOL8> InInitList)noexcept
            : NumBits((_TSizeType)0)
        {
            const _TSizeType NumList = InitializerListSize<BOOL8, _TSizeType>(InInitList);
            if (NumList > (_TSizeType)0)
            {
                const BOOL8* ListBegin = InitializerListBegin<BOOL8>(InInitList);
                AppendElementUninitializedInternal(NumList);
                for (_TSizeType i = (_TSizeType)0; i < NumList; i++)
                {
                    (*this)[i] = ListBegin[i];
                }
            }
        }
        template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        PE_FORCEINLINE TBitArray(const BOOL8* InPtr, _TOtherSizeType InNum)
            : NumBits((_TSizeType)0)
        {
            const _TSizeType NumList = (_TSizeType)InNum;
            if (NumList > (_TSizeType)0)
            {
                Check((!!InPtr));
                AppendElementUninitializedInternal(NumList);
                for (_TSizeType i = (_TSizeType)0; i < NumList; i++)
                {
                    (*this)[i] = InPtr[i];
                }
            }
        }
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        PE_FORCEINLINE explicit TBitArray(const TBitArray<_TOtherSizeType, _TOtherAllocator>& Other)noexcept
            : NumBits((_TSizeType)0)
        {
            CopyFromInternal<_TOtherSizeType, _TOtherAllocator>(Other);
        }
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        PE_FORCEINLINE explicit TBitArray(TBitArray<_TOtherSizeType, _TOtherAllocator>&& Other)noexcept
            : NumBits((_TSizeType)0)
        {
            MoveFromInternal<_TOtherSizeType, _TOtherAllocator>(EMemory::Forward<TBitArray<_TOtherSizeType, _TOtherAllocator>>(Other));
        }
        PE_FORCEINLINE TBitArray(const TBitArray& Other)noexcept
            : NumBits((_TSizeType)0)
        {
            CopyFromInternal<_TSizeType, TAllocatorType>(Other);
        }
        PE_FORCEINLINE TBitArray(TBitArray&& Other)noexcept
            : NumBits((_TSizeType)0)
        {
            MoveFromInternal<_TSizeType, TAllocatorType>(EMemory::Forward<TBitArray>(Other));
        }
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        TBitArray& operator=(const TBitArray<_Ty, _TOtherSizeType, _TOtherAllocator>& Other)noexcept
        {
            CopyFrom<_TOtherSizeType, _TOtherAllocator>(Other);
            return (*this);
        }
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        TBitArray& operator=(TBitArray<_TOtherSizeType, _TOtherAllocator>&& Other)noexcept
        {
            MoveFrom<_TOtherSizeType, _TOtherAllocator>(EMemory::Forward<TBitArray<_TOtherSizeType, _TOtherAllocator>>(Other));
            return (*this);
        }
        TBitArray& operator=(const TBitArray& Other)noexcept
        {
            CopyFrom<_TSizeType, TAllocatorType>(Other);
            return (*this);
        }
        TBitArray& operator=(TBitArray&& Other)noexcept
        {
            MoveFrom<_TSizeType, TAllocatorType>(EMemory::Forward<TBitArray>(Other));
            return (*this);
        }
        TBitArray& operator=(TInitializerList<BOOL8> InInitList)noexcept
        {
            EmptyInternal();
            const _TSizeType NumList = InitializerListSize<BOOL8, _TSizeType>(InInitList);
            if (NumList > (_TSizeType)0)
            {
                const BOOL8* ListBegin = InitializerListBegin<BOOL8>(InInitList);
                AppendElementUninitializedInternal(NumList);
                for (_TSizeType i = (_TSizeType)0; i < NumList; i++)
                {
                    (*this)[i] = ListBegin[i];
                }
            }
            return (*this);
        }
    public:
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        void CopyFrom(const TBitArray<_TOtherSizeType, _TOtherAllocator>& Other)
        {
            EmptyInternal();
            CopyFromInternal<_TOtherSizeType, _TOtherAllocator>(Other);
        }
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        void MoveFrom(TBitArray<_TOtherSizeType, _TOtherAllocator>&& Other)
        {
            EmptyInternal();
            MoveFromInternal<_TOtherSizeType, _TOtherAllocator>(EMemory::Forward<TBitArray<_TOtherSizeType, _TOtherAllocator>>(Other));
        }
        PE_NODISCARD BOOL8 IsEmpty()const
        {
            return (NumBits == (_TSizeType)0);
        }
        template<typename _TOtherIndexType, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        PE_NODISCARD PE_FORCEINLINE BOOL8 IsValidIndex(_TOtherIndexType InIndex)const
        {
            if PE_CONSTEXPR(TIsSame<_TOtherIndexType, _TSizeType>::value)
            {
                return ((InIndex >= (_TSizeType)0) && (InIndex < NumBits));
            }
            else
            {
                return ((InIndex >= (_TOtherIndexType)0) && (InIndex < (_TOtherIndexType)NumBits));
            }
        }
        PE_NODISCARD PE_FORCEINLINE TElementType* GetData()
        {
            return (Allocator.HeapData());
        }
        PE_NODISCARD PE_FORCEINLINE const TElementType* GetData()const
        {
            return (Allocator.HeapData());
        }
        template<typename _TResultType = _TSizeType, TEnableIfType<TIsIntegral<_TResultType>::value, _TResultType> = 0>
        PE_NODISCARD PE_FORCEINLINE _TResultType Num()const
        {
            if PE_CONSTEXPR(TIsSame<_TResultType, _TSizeType>::value)
            {
                return NumBits;
            }
            else
            {
                return ((_TResultType)NumBits);
            }
        }
        template<typename _TResultType = _TSizeType, TEnableIfType<TIsIntegral<_TResultType>::value, _TResultType> = 0>
        PE_NODISCARD PE_FORCEINLINE _TResultType ElementNum()const
        {
            const _TSizeType BaseNum = NumBits / TBitArray::ElementNumBits<_TSizeType>;
            return (BaseNum + (((NumBits - (BaseNum * TBitArray::ElementNumBits<_TSizeType>)) > (_TSizeType)0) ? ((_TSizeType)1) : ((_TSizeType)0)));
        }
        template<typename _TResultType = _TSizeType, TEnableIfType<TIsIntegral<_TResultType>::value, _TResultType> = 0>
        PE_NODISCARD PE_FORCEINLINE _TResultType AllocateSize()const
        {
            return (Allocator.HeapSize<_TResultType>());
        }
        template<typename _TResultType = _TSizeType, TEnableIfType<TIsIntegral<_TResultType>::value, _TResultType> = 0>
        PE_NODISCARD PE_FORCEINLINE _TResultType Capacity()const
        {
            // We assum allocate heap is always 1 byte alignment
            return (Allocator.HeapSize<_TResultType>() * (_TResultType)8);
        }
        template<typename _TResultType = _TSizeType, TEnableIfType<TIsIntegral<_TResultType>::value, _TResultType> = 0>
        PE_NODISCARD PE_FORCEINLINE _TResultType ElementCapacity()const
        {
            Check(((Allocator.HeapSize<_TSizeType>() % TBitArray::ElementTypeSize<_TSizeType>) == (_TSizeType)0));
            return (Allocator.HeapSize<_TResultType>() / TBitArray::ElementTypeSize<_TResultType>);
        }
    private:

        template<typename _TOtherSizeType, typename _TOtherAllocator>
        void CopyFromInternal(const TBitArray<_TOtherSizeType, _TOtherAllocator>& Other)
        {
            EmptyInternal();
            Allocator.CopyFrom<_TOtherSizeType, _TOtherSizeType, _TOtherAllocator::AllocateStepSize<_TOtherSizeType>>(Other.Allocator, Other.ElementNum<_TOtherSizeType>() * TBitArray<_TOtherSizeType, _TOtherAllocator>::ElementTypeSize<_TOtherSizeType>);
            NumBits = Other.Num<_TSizeType>();
        }
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        void MoveFromInternal(TBitArray<_TOtherSizeType, _TOtherAllocator>&& Other)
        {
            EmptyInternal();
            Allocator.MoveFrom<_TOtherSizeType, _TOtherAllocator::AllocateStepSize<_TOtherSizeType>>(EMemory::Move(Other.Allocator));
            NumBits = Other.Num<_TSizeType>();
        }
        template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        void AppendCapacityInternal(_TOtherSizeType InBitNum)
        {
            const _TOtherSizeType BaseNum = InBitNum / TBitArray::ElementNumBits<_TOtherSizeType>;
            const _TOtherSizeType AppendElementNum = (BaseNum + (((InBitNum - (BaseNum * TBitArray::ElementNumBits<_TOtherSizeType>)) > (_TOtherSizeType)0) ? ((_TOtherSizeType)1) : ((_TOtherSizeType)0)));
            Allocator.Append(AppendElementNum * TBitArray::ElementTypeSize<_TOtherSizeType>);
        }
        template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        void AppendElementInternal(_TOtherSizeType InBitNum)
        {
            AppendElementUninitializedInternal(InBitNum);
        }
        template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        void AppendElementUninitializedInternal(_TOtherSizeType InBitNum)
        {
            const _TOtherSizeType RestNum = Capacity<_TOtherSizeType>() - Num<_TOtherSizeType>();
            if (RestNum < InBitNum)
            {
                AppendCapacityInternal(InBitNum - RestNum);
            }
            NumBits = NumBits + (_TSizeType)InBitNum;
        }
        template<typename _TOtherSizeType = _TSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        void EmptyInternal(_TOtherSizeType InBitNum = (_TOtherSizeType)0)
        {
            const _TOtherSizeType BaseNum = InBitNum / TBitArray::ElementNumBits<_TOtherSizeType>;
            const _TOtherSizeType AppendElementNum = (BaseNum + (((InBitNum - (BaseNum * TBitArray::ElementNumBits<_TOtherSizeType>)) > (_TOtherSizeType)0) ? ((_TOtherSizeType)1) : ((_TOtherSizeType)0)));
            Allocator.Reallocate(AppendElementNum * TBitArray::ElementTypeSize<_TOtherSizeType>);
            NumBits = (_TSizeType)0;
        }
        EBitReference PushDefaultInternal()
        {
            if (NumBits >= Capacity<_TSizeType>())
            {
                Allocator.Growth();
            }
            EBitReference BitRef(GetData()[((UINT32)NumBits) / TBitArray::ElementNumBits<UINT32>], ERelativeBitReference::BitZeroPoint >> (((UINT32)NumBits) % TBitArray::ElementNumBits<UINT32>));
            NumBits = NumBits + (_TSizeType)1;
            return BitRef;
        }
        EBitReference PushInternal(BOOL8 InValue)
        {
            if (NumBits >= Capacity<_TSizeType>())
            {
                Allocator.Growth();
            }
            EBitReference BitRef(GetData()[((UINT32)NumBits) / TBitArray::ElementNumBits<UINT32>], ERelativeBitReference::BitZeroPoint >> (((UINT32)NumBits) % TBitArray::ElementNumBits<UINT32>));
            BitRef = InValue;
            NumBits = NumBits + (_TSizeType)1;
            return BitRef;
        }
        void PopInternal()
        {
            NumBits = NumBits - (_TSizeType)1;
        }
        void ShrinkInternal()
        {
            const _TSizeType RestNum = ElementCapacity<_TSizeType>() - ElementNum<_TSizeType>();
            const _TSizeType ShrinkBaseSlackNum = RestNum / TAllocatorType::AllocateStepSize<_TSizeType>;
            const _TSizeType SubNum = (ShrinkBaseSlackNum > (_TSizeType)0) ? (ShrinkBaseSlackNum * TAllocatorType::AllocateStepSize<_TSizeType>) : ((_TSizeType)0);
            Allocator.Substract(SubNum * TArray::ElementTypeSize<_TSizeType>);
        }
    public:
        /* NEVER USE DIRECTLY !!! STL-like iterators to enable range-based for loop support.*/
        PE_FORCEINLINE TIterator        begin()         { return TIterator(*this); }
        /* NEVER USE DIRECTLY !!! STL-like iterators to enable range-based for loop support.*/
        PE_FORCEINLINE TConstIterator   begin()const    { return TConstIterator(*this); }
        /* NEVER USE DIRECTLY !!! STL-like iterators to enable range-based for loop support.*/
        PE_FORCEINLINE TIterator        end()           { return TIterator(*this, Num<UINT32>()); }
        /* NEVER USE DIRECTLY !!! STL-like iterators to enable range-based for loop support.*/
        PE_FORCEINLINE TConstIterator   end()const      { return TConstIterator(*this, Num<UINT32>()); }
    private:
        _TSizeType      NumBits;
        TAllocatorType  Allocator;
    };
#endif
};
