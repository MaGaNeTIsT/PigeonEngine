#pragma once

#include "../../../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include "../../../../Core/Config/ErrorCaption.h"
#if 0
#include <rapidjson.h>
#include <vector>
#include <algorithm>
#include <random>
#include <array>
#endif
#include "../../../../../Development/Alert/DevelopmentDefines.h"
#include "Base/Math/Math.h"
#include "Base/Memory/Memory.h"

namespace PigeonEngine
{

#if 1

    template <typename _Ty,
        typename _TSizeType = INT32,
        typename _TAllocator = TDefaultAllocator<_Ty, _TSizeType>,
        TEnableIfType<TIsIntegral<_TSizeType>::value, _TSizeType> = 0>
    class TArray final
    {
    public:
        template<typename _TResultType = _TSizeType, TEnableIfType<TIsIntegral<_TResultType>::value, _TResultType> = 0>
        PE_CONSTEXPR_STATIC _TResultType ElementTypeSize = (_TResultType)(sizeof(_Ty));

        using TElementType          = _Ty;
        using TAllocatorType        = _TAllocator;
        using TIterator             = _Ty*;
        using TConstIterator        = const _Ty*;
        using TReverseIterator      = TReversePointerIterator<_Ty>;
        using TConstReverseIterator = TReversePointerIterator<const _Ty>;
    public:
        template<typename _TResultType = _TSizeType, TEnableIfType<TIsIntegral<_TResultType>::value, _TResultType> = 0>
        _TResultType Add(const TElementType& InElement)
        {
            if PE_CONSTEXPR(!(TIsScalar<TElementType>::value))
            {
                PushInternal(InElement);
            }
            else
            {
                PushScalarInternal(InElement);
            }
            Check((ElementNum > (_TSizeType)0));
            if PE_CONSTEXPR(TIsSame<_TResultType, _TSizeType>::value)
            {
                return (ElementNum - (_TSizeType)1);
            }
            else
            {
                return ((_TResultType)(ElementNum - (_TSizeType)1));
            }
        }
        template<typename _TResultType = _TSizeType, TEnableIfType<TIsIntegral<_TResultType>::value, _TResultType> = 0>
        _TResultType Add(TElementType&& InElement)
        {
            if PE_CONSTEXPR(!(TIsScalar<TElementType>::value))
            {
                PushInternal(EMemory::Forward<TElementType>(InElement));
            }
            else
            {
                PushScalarInternal(EMemory::Forward<TElementType>(InElement));
            }
            Check((ElementNum > (_TSizeType)0));
            if PE_CONSTEXPR(TIsSame<_TResultType, _TSizeType>::value)
            {
                return (ElementNum - (_TSizeType)1);
            }
            else
            {
                return ((_TResultType)(ElementNum - (_TSizeType)1));
            }
        }
        template<typename _TOtherIndexType, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        PE_NODISCARD PE_FORCEINLINE TElementType& operator[](_TOtherIndexType InIndex)
        {
            Check(((InIndex >= (_TOtherIndexType)0) && ((_TSizeType)(InIndex) < ElementNum)));
            return (GetData()[InIndex]);
        }
        template<typename _TOtherIndexType, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        PE_NODISCARD PE_FORCEINLINE const TElementType& operator[](_TOtherIndexType InIndex)const
        {
            Check(((InIndex >= (_TOtherIndexType)0) && ((_TSizeType)(InIndex) < ElementNum)));
            return (GetData()[InIndex]);
        }
        TElementType& AddDefaultGetRef()
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
        void Append(const TArray<TElementType, _TOtherSizeType, _TOtherAllocator>& Other)
        {
            const _TSizeType RestCapacity = Capacity() - ElementNum;
            const _TSizeType OtherNum = Other.Num<_TSizeType>();
            if (RestCapacity < OtherNum)
            {
                AppendCapacityInternal(OtherNum - RestCapacity);
            }
            if PE_CONSTEXPR(!(TIsScalar<TElementType>::value))
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
                    TElementType* TypeDataPtr = GetData();
                    EMemory::Memcpy((&(TypeDataPtr[ElementNum])), Other.GetData(), OtherNum);
                    ElementNum = ElementNum + OtherNum;
                }
            }
        }
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        void Append(TArray<TElementType, _TOtherSizeType, _TOtherAllocator>&& Other)
        {
            const _TSizeType RestCapacity = Capacity() - ElementNum;
            const _TSizeType OtherNum = Other.Num<_TSizeType>();
            if (RestCapacity < OtherNum)
            {
                AppendCapacityInternal(OtherNum - RestCapacity);
            }
            if (OtherNum > (_TSizeType)0)
            {
                TElementType* TypeDataPtr = GetData();
                EMemory::Memmov((&(TypeDataPtr[ElementNum])), Other.GetData(), OtherNum);
                ElementNum = ElementNum + OtherNum;

                Other.ElementNum = 0;
                Other.Allocator.Release();
            }
        }
        template<typename _TFirstIndexType, typename _TSecondIndexType
            , TEnableIfType<TIsIntegral<_TFirstIndexType>::value, _TFirstIndexType> = 0, TEnableIfType<TIsIntegral<_TSecondIndexType>::value, _TSecondIndexType> = 0>
        void Swap(_TFirstIndexType InFirstIndex, _TSecondIndexType InSecondIndex)
        {
            Check(((InFirstIndex >= (_TFirstIndexType)0) && (((_TSizeType)(InFirstIndex)) < ElementNum)));
            Check(((InSecondIndex >= (_TSecondIndexType)0) && (((_TSizeType)(InSecondIndex)) < ElementNum)));
            if PE_CONSTEXPR(!(TIsScalar<TElementType>::value))
            {
                UINT8 TempBuffer[TArray::ElementTypeSize<_TSizeType>];
                EMemory::Memmov(&(TempBuffer[0]), &(GetData()[InFirstIndex]), TArray::ElementTypeSize<_TSizeType>);
                EMemory::Memmov(&(GetData()[InFirstIndex]), &(GetData()[InSecondIndex]), TArray::ElementTypeSize<_TSizeType>);
                EMemory::Memmov(&(GetData()[InFirstIndex]), &(TempBuffer[0]), TArray::ElementTypeSize<_TSizeType>);
            }
            else
            {
                TElementType TempItem = (*this)[InFirstIndex];
                (*this)[InFirstIndex] = (*this)[InSecondIndex];
                (*this)[InSecondIndex] = TempItem;
            }
        }
        template<typename _TOtherIndexType, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        PE_NODISCARD PE_FORCEINLINE TElementType& GetRef(_TOtherIndexType InIndex)
        {
            return ((*this)[InIndex]);
        }
        template<typename _TOtherIndexType, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        PE_NODISCARD PE_FORCEINLINE const TElementType& GetRef(_TOtherIndexType InIndex)const
        {
            return ((*this)[InIndex]);
        }
        PE_NODISCARD PE_FORCEINLINE TElementType& FirstRef()
        {
            return ((*this)[0]);
        }
        PE_NODISCARD PE_FORCEINLINE const TElementType& FirstRef()const
        {
            return ((*this)[0]);
        }
        PE_NODISCARD PE_FORCEINLINE TElementType& LastRef()
        {
            return ((*this)[ElementNum - (_TSizeType)1]);
        }
        PE_NODISCARD PE_FORCEINLINE const TElementType& LastRef()const
        {
            return ((*this)[ElementNum - (_TSizeType)1]);
        }
        template<typename _TOtherIndexType, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        void RemoveAt(_TOtherIndexType InIndex)
        {
            Check(((InIndex >= (_TOtherIndexType)0) && (((_TSizeType)(InIndex)) < ElementNum)));
            const _TSizeType UsedIndex = (_TSizeType)(InIndex);
            const _TSizeType LastIndex = ElementNum - (_TSizeType)1;
            if (UsedIndex < LastIndex)
            {
                if PE_CONSTEXPR(!(TIsScalar<TElementType>::value))
                {
                    (*this)[UsedIndex].~TElementType();
                }
                TElementType* TypeDataPtr = GetData();
                // TODO : Maybe we need copy to a temp buffer for memory move
                EMemory::Memmov(&(TypeDataPtr[InIndex]), &(TypeDataPtr[InIndex + 1]), LastIndex - UsedIndex);
                ElementNum = ElementNum - (_TSizeType)1;
            }
            else
            {
                PopInternal();
            }
        }
        template<typename _TOtherIndexType, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        void RemoveAtSwap(_TOtherIndexType InIndex)
        {
            Check(((InIndex >= (_TOtherIndexType)0) && (((_TSizeType)(InIndex)) < ElementNum)));
            const _TSizeType UsedIndex = (_TSizeType)(InIndex);
            const _TSizeType LastIndex = ElementNum - (_TSizeType)1;
            if (UsedIndex != LastIndex)
            {
                Swap(UsedIndex, LastIndex);
            }
            PopInternal();
        }
        void Remove(const TElementType& InElement)
        {
            if (ElementNum > (_TSizeType)0)
            {
                for (_TSizeType i = ElementNum - (_TSizeType)1; i >= 0; i--)
                {
                    if (((*this)[i]) == InElement)
                    {
                        RemoveAt(i);
                        return;
                    }
                }
            }
        }
        template<typename _TOtherIndexType, typename _TConditionFunc, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        void Remove(_TConditionFunc&& InConditionFunc)
        {
            if (ElementNum > (_TSizeType)0)
            {
                for (_TOtherIndexType i = ((_TOtherIndexType)(ElementNum)) - (_TOtherIndexType)1; i >= 0; i--)
                {
                    if (InConditionFunc(i, (*this)[i]))
                    {
                        RemoveAt(i);
                        return;
                    }
                }
            }
        }
        template<typename _TOtherIndexType, typename _TConditionFunc, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        void Remove(const _TConditionFunc& InConditionFunc)
        {
            if (ElementNum > (_TSizeType)0)
            {
                for (_TOtherIndexType i = ((_TOtherIndexType)(ElementNum)) - (_TOtherIndexType)1; i >= 0; i--)
                {
                    if (InConditionFunc(i, (*this)[i]))
                    {
                        RemoveAt(i);
                        return;
                    }
                }
            }
        }
        void RemoveAll(const TElementType& InElement)
        {
            if (ElementNum > (_TSizeType)0)
            {
                for (_TSizeType i = ElementNum - (_TSizeType)1; i >= 0; i--)
                {
                    if (((*this)[i]) == InElement)
                    {
                        RemoveAt(i);
                    }
                }
            }
        }
        template<typename _TOtherIndexType, typename _TConditionFunc, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        void RemoveAll(_TConditionFunc&& InConditionFunc)
        {
            if (ElementNum > (_TSizeType)0)
            {
                for (_TOtherIndexType i = ((_TOtherIndexType)(ElementNum)) - (_TOtherIndexType)1; i >= 0; i--)
                {
                    if (InConditionFunc(i, (*this)[i]))
                    {
                        RemoveAt(i);
                    }
                }
            }
        }
        template<typename _TOtherIndexType, typename _TConditionFunc, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        void RemoveAll(const _TConditionFunc& InConditionFunc)
        {
            if (ElementNum > (_TSizeType)0)
            {
                for (_TOtherIndexType i = ((_TOtherIndexType)(ElementNum)) - (_TOtherIndexType)1; i >= 0; i--)
                {
                    if (InConditionFunc(i, (*this)[i]))
                    {
                        RemoveAt(i);
                    }
                }
            }
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
        void Empty(_TOtherSizeType InElementNum = (_TOtherSizeType)0)
        {
            EmptyInternal(InElementNum);
        }
        template<typename _TOtherIndexType = _TSizeType, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        PE_NODISCARD _TOtherIndexType Find(const TElementType& InElement)const
        {
            for (_TOtherIndexType i = (_TOtherIndexType)0, n = ((_TOtherIndexType)(ElementNum)); i < n; i++)
            {
                if (((*this)[i]) == InElement)
                {
                    return i;
                }
            }
            return ((_TOtherIndexType)(-1));
        }
        template<typename _TOtherIndexType, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        BOOL8 Find(const TElementType& InElement, _TOtherIndexType& OutIndex)const
        {
            for (_TOtherIndexType i = (_TOtherIndexType)0, n = ((_TOtherIndexType)(ElementNum)); i < n; i++)
            {
                if (((*this)[i]) == InElement)
                {
                    OutIndex = i;
                    return TRUE;
                }
            }
            return FALSE;
        }
        PE_NODISCARD BOOL8 Contains(const TElementType& InElement)const
        {
            for (_TSizeType i = (_TSizeType)0, n = ElementNum; i < n; i++)
            {
                if (((*this)[i]) == InElement)
                {
                    return TRUE;
                }
            }
            return FALSE;
        }
    public:
        void Shuffle()
        {
            //TODO
        }
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        TArray<TElementType, _TOtherSizeType, _TOtherAllocator> Reverse()const
        {
            TArray<TElementType, _TOtherSizeType, _TOtherAllocator> NewReverseArray;
            const _TOtherSizeType UsedElementNum = (_TOtherIndexType)(ElementNum);
            NewReverseArray.Reserve<_TOtherSizeType>(UsedElementNum);

            //TODO

            return NewReverseArray;
        }
        void DefaultSort()
        {
            //TODO
        }
        template<typename _TConditionFunc>
        void Sort(_TConditionFunc&& InConditionFunc)
        {
            //TODO
        }
        template<typename _TConditionFunc>
        void Sort(const _TConditionFunc& InConditionFunc)
        {
            //TODO
        }
        void DefaultStableSort()
        {
            //TODO
        }
        template<typename _TConditionFunc>
        void StableSort(_TConditionFunc&& InConditionFunc)
        {
            //TODO
        }
        template<typename _TConditionFunc>
        void StableSort(const _TConditionFunc& InConditionFunc)
        {
            //TODO
        }
    public:
        ~TArray()
        {
            EmptyInternal();
        }
        PE_FORCEINLINE TArray()noexcept
            : ElementNum((_TSizeType)0)
        {
        }
        template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        PE_FORCEINLINE TArray(_TOtherSizeType InElementNum)noexcept
            : ElementNum((_TSizeType)0)
        {
            AppendElementInternal(InElementNum);
        }
        TArray(TInitializerList<TElementType> InInitList)noexcept
            : ElementNum((_TSizeType)0)
        {
            const _TSizeType NumList = InitializerListSize<TElementType, _TSizeType>(InInitList);
            if (NumList > (_TSizeType)0)
            {
                AppendCapacityInternal(NumList);
                const TElementType* ListBegin = InitializerListBegin<TElementType>(InInitList);
                for (_TSizeType i = (_TSizeType)0; i < NumList; i++)
                {
                    if PE_CONSTEXPR(!(TIsScalar<TElementType>::value))
                    {
                        PushInternal(ListBegin[i]);
                    }
                    else
                    {
                        PushScalarInternal(ListBegin[i]);
                    }
                }
            }
        }
        template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        PE_FORCEINLINE TArray(const TElementType* InPtr, _TOtherSizeType InNum)
            : ElementNum((_TSizeType)0)
        {
            Allocator.CopyFrom(InPtr, TArray::ElementTypeSize<_TOtherSizeType> * InNum);
            ElementNum = (_TSizeType)(InNum);
        }
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        PE_FORCEINLINE explicit TArray(const TArray<TElementType, _TOtherSizeType, _TOtherAllocator>& Other)noexcept
            : ElementNum((_TSizeType)0)
        {
            CopyFromInternal<_TOtherSizeType, _TOtherAllocator>(Other);
        }
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        PE_FORCEINLINE explicit TArray(TArray<TElementType, _TOtherSizeType, _TOtherAllocator>&& Other)noexcept
            : ElementNum((_TSizeType)0)
        {
            MoveFromInternal<_TOtherSizeType, _TOtherAllocator>(EMemory::Forward<TArray<TElementType, _TOtherSizeType, _TOtherAllocator>>(Other));
        }
        PE_FORCEINLINE TArray(const TArray& Other)noexcept
            : ElementNum((_TSizeType)0)
        {
            CopyFromInternal<_TSizeType, TAllocatorType>(Other);
        }
        PE_FORCEINLINE TArray(TArray&& Other)noexcept
            : ElementNum((_TSizeType)0)
        {
            MoveFromInternal<_TSizeType, TAllocatorType>(EMemory::Forward<TArray>(Other));
        }
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        TArray& operator=(const TArray<TElementType, _TOtherSizeType, _TOtherAllocator>& Other)noexcept
        {
            CopyFrom<_TOtherSizeType, _TOtherAllocator>(Other);
            return (*this);
        }
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        TArray& operator=(TArray<TElementType, _TOtherSizeType, _TOtherAllocator>&& Other)noexcept
        {
            MoveFrom<_TOtherSizeType, _TOtherAllocator>(EMemory::Forward<TArray<TElementType, _TOtherSizeType, _TOtherAllocator>>(Other));
            return (*this);
        }
        TArray& operator=(const TArray& Other)noexcept
        {
            CopyFrom<_TSizeType, TAllocatorType>(Other);
            return (*this);
        }
        TArray& operator=(TArray&& Other)noexcept
        {
            MoveFrom<_TSizeType, TAllocatorType>(EMemory::Forward<TArray>(Other));
            return (*this);
        }
        TArray& operator=(TInitializerList<TElementType> InInitList)noexcept
        {
            EmptyInternal();
            const _TSizeType NumList = InitializerListSize<TElementType, _TSizeType>(InInitList);
            if (NumList > (_TSizeType)0)
            {
                AppendCapacityInternal(NumList);
                const TElementType* ListBegin = InitializerListBegin<TElementType>(InInitList);
                for (_TSizeType i = (_TSizeType)0; i < NumList; i++)
                {
                    if PE_CONSTEXPR(!(TIsScalar<TElementType>::value))
                    {
                        PushInternal(ListBegin[i]);
                    }
                    else
                    {
                        PushScalarInternal(ListBegin[i]);
                    }
                }
            }
            return (*this);
        }
    public:
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        void CopyFrom(const TArray<TElementType, _TOtherSizeType, _TOtherAllocator>& Other)
        {
            EmptyInternal();
            CopyFromInternal<_TOtherSizeType, _TOtherAllocator>(Other);
        }
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        void MoveFrom(TArray<TElementType, _TOtherSizeType, _TOtherAllocator>&& Other)
        {
            EmptyInternal();
            MoveFromInternal<_TOtherSizeType, _TOtherAllocator>(EMemory::Forward<TArray<TElementType, _TOtherSizeType, _TOtherAllocator>>(Other));
        }
        PE_NODISCARD BOOL8 IsEmpty()const
        {
            return (ElementNum == (_TSizeType)0);
        }
        template<typename _TOtherIndexType, TEnableIfType<TIsIntegral<_TOtherIndexType>::value, _TOtherIndexType> = 0>
        PE_NODISCARD PE_FORCEINLINE BOOL8 IsValidIndex(_TOtherIndexType InIndex)const
        {
            if PE_CONSTEXPR(TIsSame<_TOtherIndexType, _TSizeType>::value)
            {
                return ((InIndex >= (_TSizeType)0) && (InIndex < ElementNum));
            }
            else
            {
                return ((InIndex >= (_TOtherIndexType)0) && (InIndex < (_TOtherIndexType)ElementNum));
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
                return ElementNum;
            }
            else
            {
                return ((_TResultType)ElementNum);
            }
        }
        template<typename _TResultType = _TSizeType, TEnableIfType<TIsIntegral<_TResultType>::value, _TResultType> = 0>
        PE_NODISCARD PE_FORCEINLINE _TResultType AllocateSize()const
        {
            return (Allocator.HeapSize<_TResultType>());
        }
        template<typename _TResultType = _TSizeType, TEnableIfType<TIsIntegral<_TResultType>::value, _TResultType> = 0>
        PE_NODISCARD PE_FORCEINLINE _TResultType Capacity()const
        {
            return (Allocator.HeapSize<_TResultType>() / TArray::ElementTypeSize<_TResultType>);
        }
    private:
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        void CopyFromInternal(const TArray<TElementType, _TOtherSizeType, _TOtherAllocator>& Other)
        {
            EmptyInternal();
            if PE_CONSTEXPR(!(TIsScalar<TElementType>::value))
            {
                const _TOtherSizeType NumOther = Other.Num<_TOtherSizeType>();
                if (NumOther > (_TOtherSizeType)0)
                {
                    AppendCapacityInternal(NumOther);
                    for (_TOtherSizeType i = (_TOtherSizeType)0; i < NumOther; i++)
                    {
                        PushInternal(Other[i]);
                    }
                }
            }
            else
            {
                Allocator.CopyFrom<_TOtherSizeType, _TOtherSizeType, _TOtherAllocator::AllocateStepSize<_TOtherSizeType>>(Other.Allocator, Other.Num<_TOtherSizeType>() * TArray::ElementTypeSize<_TOtherSizeType>);
                ElementNum = Other.Num<_TSizeType>();
            }
        }
        template<typename _TOtherSizeType, typename _TOtherAllocator>
        void MoveFromInternal(TArray<TElementType, _TOtherSizeType, _TOtherAllocator>&& Other)
        {
            EmptyInternal();
            Allocator.MoveFrom<_TOtherSizeType, _TOtherAllocator::AllocateStepSize<_TOtherSizeType>>(EMemory::Move(Other.Allocator));
            ElementNum = Other.Num<_TSizeType>();
            Other.ElementNum = (_TOtherSizeType)0;
        }
        template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        void AppendCapacityInternal(_TOtherSizeType InElementNum)
        {
            Allocator.Append(InElementNum * TArray::ElementTypeSize<_TOtherSizeType>);
        }
        template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        void AppendElementInternal(_TOtherSizeType InElementNum)
        {
            const _TOtherSizeType HoldElementNum = (_TOtherSizeType)(ElementNum);
            AppendElementUninitializedInternal(InElementNum);
            if PE_CONSTEXPR(!(TIsScalar<TElementType>::value))
            {
                TElementType* TypeDataPtr = GetData();
                for (_TOtherSizeType i = (_TOtherSizeType)0; i < InElementNum; i++)
                {
                    new (&(TypeDataPtr[HoldElementNum + i])) TElementType();
                }
            }
        }
        template<typename _TOtherSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        void AppendElementUninitializedInternal(_TOtherSizeType InElementNum)
        {
            const _TOtherSizeType RestNum = Capacity<_TOtherSizeType>() - Num<_TOtherSizeType>();
            if (RestNum < InElementNum)
            {
                AppendCapacityInternal(InElementNum - RestNum);
            }
            ElementNum = ElementNum + (_TSizeType)(InElementNum);
        }
        template<typename _TOtherSizeType = _TSizeType, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        void EmptyInternal(_TOtherSizeType InElementNum = (_TOtherSizeType)0)
        {
            if PE_CONSTEXPR(!(TIsScalar<TElementType>::value))
            {
                TElementType* TypeDataPtr = GetData();
                for (_TSizeType i = (_TSizeType)0; i < ElementNum; i++)
                {
                    (TypeDataPtr[i]).~TElementType();
                }
            }
            Allocator.Reallocate(InElementNum * TArray::ElementTypeSize<_TOtherSizeType>);
            ElementNum = (_TSizeType)0;
        }
        TElementType& PushDefaultInternal()
        {
            if (ElementNum >= Capacity<_TSizeType>())
            {
                Allocator.Growth();
            }
            TElementType* NewItem = nullptr;
            if PE_CONSTEXPR(!(TIsScalar<TElementType>::value))
            {
                TElementType* TypeDataPtr = GetData();
                NewItem = new (&(TypeDataPtr[ElementNum])) TElementType();
            }
            else
            {
                TElementType* TypeDataPtr = GetData();
                NewItem = (&(TypeDataPtr[ElementNum]));
            }
            ElementNum = ElementNum + (_TSizeType)1;
            return (*NewItem);
        }
        template<typename... _TArguments>
        TElementType& PushInternal(_TArguments&&... _InArgs)
        {
            if (ElementNum >= Capacity<_TSizeType>())
            {
                Allocator.Growth();
            }
            TElementType* TypeDataPtr = GetData();
            TElementType* NewItem = new (&(TypeDataPtr[ElementNum])) TElementType(EMemory::Forward<_TArguments>(_InArgs)...);
            ElementNum = ElementNum + (_TSizeType)1;
            return (*NewItem);
        }
        TElementType& PushScalarInternal(const TElementType& InElement)
        {
            if (ElementNum >= Capacity<_TSizeType>())
            {
                Allocator.Growth();
            }
            TElementType* TypeDataPtr = GetData();
            TElementType* NewItem = (&(TypeDataPtr[ElementNum]));
            (*NewItem) = InElement;
            ElementNum = ElementNum + (_TSizeType)1;
            return (*NewItem);
        }
        TElementType& PushScalarInternal(TElementType&& InElement)
        {
            if (ElementNum >= Capacity<_TSizeType>())
            {
                Allocator.Growth();
            }
            TElementType* TypeDataPtr = GetData();
            TElementType* NewItem = (&(TypeDataPtr[ElementNum]));
            (*NewItem) = EMemory::Forward<TElementType>(InElement);
            ElementNum = ElementNum + (_TSizeType)1;
            return (*NewItem);
        }
        void PopInternal()
        {
            if PE_CONSTEXPR(!(TIsScalar<TElementType>::value))
            {
                TElementType* TypeDataPtr = GetData();
                (TypeDataPtr[ElementNum - (_TSizeType)1]).~TElementType();
            }
            ElementNum = ElementNum - (_TSizeType)1;
        }
        void ShrinkInternal()
        {
            const _TSizeType RestNum = Capacity() - ElementNum;
            const _TSizeType ShrinkBaseSlackNum = RestNum / TAllocatorType::AllocateStepSize<_TSizeType>;
            const _TSizeType SubNum = (ShrinkBaseSlackNum > (_TSizeType)0) ? (ShrinkBaseSlackNum * TAllocatorType::AllocateStepSize<_TSizeType>) : ((_TSizeType)0);
            Allocator.Substract(SubNum * TArray::ElementTypeSize<_TSizeType>);
        }
    public:
        /* NEVER USE DIRECTLY !!! STL-like iterators to enable range-based for loop support.*/
        PE_FORCEINLINE TIterator                begin()         { return (GetData()); }
        /* NEVER USE DIRECTLY !!! STL-like iterators to enable range-based for loop support.*/
        PE_FORCEINLINE TConstIterator           begin()const    { return (GetData()); }
        /* NEVER USE DIRECTLY !!! STL-like iterators to enable range-based for loop support.*/
        PE_FORCEINLINE TIterator                end()           { return (GetData() + Num<_TSizeType>()); }
        /* NEVER USE DIRECTLY !!! STL-like iterators to enable range-based for loop support.*/
        PE_FORCEINLINE TConstIterator           end()const      { return (GetData() + Num<_TSizeType>()); }
        /* NEVER USE DIRECTLY !!! STL-like iterators to enable range-based for loop support.*/
        PE_FORCEINLINE TReverseIterator         rbegin()        { return TReverseIterator(GetData() + Num<_TSizeType>()); }
        /* NEVER USE DIRECTLY !!! STL-like iterators to enable range-based for loop support.*/
        PE_FORCEINLINE TConstReverseIterator    rbegin()const   { return TConstReverseIterator(GetData() + Num<_TSizeType>()); }
        /* NEVER USE DIRECTLY !!! STL-like iterators to enable range-based for loop support.*/
        PE_FORCEINLINE TReverseIterator         rend()          { return TReverseIterator(GetData()); }
        /* NEVER USE DIRECTLY !!! STL-like iterators to enable range-based for loop support.*/
        PE_FORCEINLINE TConstReverseIterator    rend()const     { return TConstReverseIterator(GetData()); }
    private:
        _TSizeType      ElementNum;
        TAllocatorType  Allocator;
    };

#else

    // -------------------   Declarations   -----------------------------
    template <typename T>
    class TArray
    {
    public:
        using TIterator         = typename std::vector<T>::iterator;
        using TConstIterator    = typename std::vector<T>::const_iterator;
        using SizeType = UINT32;
    public:
        TArray();
        TArray(const TArray<T>& Other);

        explicit TArray(const std::vector<T>& Other);
        TArray(TArray<T>&& Other) noexcept;
        explicit TArray(std::vector<T>&& Other);
        
        ~TArray() = default;
        
        T&          operator[](const UINT32& Index);
        const T&    operator[](const UINT32& Index)const;
        TArray<T>&  operator= (const TArray<T>& Other);
        TArray<T>&  operator= (TArray<T>&& Other) noexcept;
        TArray<T>&  operator= (std::vector<T>&& Other);
        
        UINT32        Add       (const T& Element);
        UINT32        Add       (T&& Element);

        T&          Add_Default_GetRef()
        {
            T Temp;
            Elements.push_back(std::move(Temp));
            return Elements[Elements.size() - 1u];
        }
        
        T&          GetRef    (const UINT32& Index);
        const T&    Get       (const UINT32& Index)const;
        const T&    Last      ();
        // Find the index of given Element, return array's Length if the Element doesn't exist.
        UINT32 Find      (const T& Element) const;
        BOOL32 Contains  (const T& Element) const;
        void Resize    (const UINT32& NewSize);
        void Recapacity(const UINT32& NewCapacity);
        void RemoveAt  (const UINT32& Index);
        void Remove    (const T& Element);
        void Pop       ();
        void Clear     ();

        T*          RawData();
        const T*    RawData() const;

        void Append(const TArray<T>& Other);

        void       Sort   ();
        void       Sort   (BOOL32 Predicate(const T&, const T&));
        void       Shuffle();
        TArray<T>  Reverse() const;
        
        typename TIterator Begin()
        {
            return Elements.begin();
        }
        typename TConstIterator Begin()const
        {
            return Elements.begin();
        }
        typename TIterator End()
        {
            return Elements.end();
        }
        typename TConstIterator End()const
        {
            return Elements.end();
        }
        typename TIterator Rbegin()
        {
            return Elements.rbegin();
        }
        typename TIterator Rend()
        {
            return Elements.rend();
        }

        /**
         * DO NOT USE DIRECTLY
         * STL-like iterators to enable range-based for loop support.
         */
        typename TConstIterator begin()const{return Elements.begin();}
        typename TConstIterator end()const{return Elements.end();}
        typename TIterator      begin(){return Elements.begin();}
        typename TIterator      end(){return Elements.end();}
        
        PE_NODISCARD UINT32 Length()const
        {
            return static_cast<UINT32>(this->Elements.size());
        }
        PE_NODISCARD UINT32 LastIndex    ()const;

    
    private:
        class std::vector<T> Elements;
    };


    // -------------------   Implementations   -----------------------------
    template <typename T>
    TArray<T>::TArray()
    {
    }

    template <typename T>
    TArray<T>::TArray(const TArray<T>& Other)
        :
    Elements(Other.Elements)
    {
        
    }

    template <typename T>
    TArray<T>::TArray(const std::vector<T>& Other)
        :
    Elements(Other)
    {
    }

    template <typename T>
    TArray<T>::TArray(TArray<T>&& Other) noexcept
        :
    Elements(std::move(Other.Elements))
    {
        Other.Clear();
    }

    template <typename T>
    TArray<T>::TArray(std::vector<T>&& Other)
        :
    Elements(std::move(Other))
    {
        Other.clear();
    }

    template <typename T>
    T& TArray<T>::operator[](const UINT32& Index)
    {
#if _EDITOR_ONLY
        UINT32 NumElements = Length();
        PE_CHECK(ENGINE_ARRAY_ERROR, "Array has no this index", (Index < NumElements));
#endif
        return Elements[Index];
    }

    template <typename T>
    const T& TArray<T>::operator[](const UINT32& Index)const
    {
#if _EDITOR_ONLY
        UINT32 NumElements = Length();
        PE_CHECK(ENGINE_ARRAY_ERROR, "Array has no this index", (Index < NumElements));
#endif
        return Elements[Index];
    }

    template <typename T>
    TArray<T>& TArray<T>::operator=(const TArray<T>& Other)
    {
         Elements = Other.Elements;
         return *this;
    }

    template <typename T>
    TArray<T>& TArray<T>::operator=(TArray<T>&& Other) noexcept
    {
        Elements = std::move(Other.Elements);
        Other.Clear();
        return *this;
    }

    template <typename T>
    TArray<T>& TArray<T>::operator=(std::vector<T>&& Other)
    {
         Elements = std::move(Other);
         Other.clear();
         return *this;
    }
    
    template <typename T>
    UINT32 TArray<T>::LastIndex() const
    {
        return Length() > 0 ? Length() - 1 : 0;
    }

    template <typename T>
    UINT32 TArray<T>::Add(const T& Element)
    {
         this->Elements.push_back(Element);
         return LastIndex();
    }

    template <typename T>
    UINT32 TArray<T>::Add(T&& Element)
    {
        this->Elements.push_back(EMemory::Move(Element));
        return LastIndex();
    }

    template <typename T>
    T& TArray<T>::GetRef(const UINT32& Index)
    {
#if _EDITOR_ONLY
        UINT32 NumElements = Length();
        PE_CHECK(ENGINE_ARRAY_ERROR, "Array has no this index", (Index < NumElements));
#endif
        return this->Elements[Index];
    }

    template <typename T>
    const T& TArray<T>::Get(const UINT32& Index)const
    {
#if _EDITOR_ONLY
        UINT32 NumElements = Length();
        PE_CHECK(ENGINE_ARRAY_ERROR, "Array has no this index", (Index < NumElements));
#endif
        return this->Elements[Index];
    }

    template <typename T>
    const T& TArray<T>::Last()
    {
        return Elements[LastIndex()];
    }

    template <typename T>
    void TArray<T>::RemoveAt(const UINT32& Index)
    {
        if(Index >= Length())
        {
            return;
        }
        this->Elements.erase(Elements.begin() + Index);
    }

    template <typename T>
    void TArray<T>::Remove(const T& Element)
    {
        const UINT32& i = Find(Element);
        if(i != Length())
        {
            RemoveAt(i);
        }
    }

    template <typename T>
    void TArray<T>::Pop()
    {
        Elements.pop_back();
    }

    template <typename T>
    void TArray<T>::Clear()
    {
        Elements.clear();
    }

    template <typename T>
    T* TArray<T>::RawData()
    {
        return Elements.data();
    }

    template <typename T>
    const T* TArray<T>::RawData() const
    {
        return Elements.data();
    }

    template <typename T>
    void TArray<T>::Append(const TArray<T>& Other)
    {
         const UINT32 Size = Length() + Other.Length();
         Recapacity(Size * 2);
         for(const auto& elem:Other)
         {
             Add(elem);
         }
    }

    template <typename T>
    void TArray<T>::Sort()
    {
        std::sort(Elements.begin(), Elements.end());
    }

    template <typename T>
    void TArray<T>::Sort(BOOL32 Predicate(const T&, const T&))
    {
        std::sort(Elements.begin(), Elements.end(),Predicate);
    }
    
    template <typename T>
    void TArray<T>::Shuffle()
    {
        std::random_device rd;
        std::mt19937 rng(rd());
        std::shuffle(Elements.begin(), Elements.end(),rng);
    }

    template <typename T>
    TArray<T> TArray<T>::Reverse() const
    {
        std::vector<T> Temp = Elements;
        std::reverse(Temp.begin(), Temp.end());
        return TArray<T>(Temp);
    }

    template <typename T>
    UINT32 TArray<T>::Find(const T& Element) const
    {
        for(UINT32 i = 0; i < Elements.size();++i)
        {
            if(Elements[i] == Element)
            {
                return i;
            }
        }
        return Length();
    }

    template <typename T>
    BOOL32 TArray<T>::Contains(const T& Element) const
    {
         for(const auto& elem : Elements)
         {
             if(elem == Element)
             {
                 return TRUE;
             }
         }
         return FALSE;
    }

    template <typename T>
    void TArray<T>::Resize(const UINT32& NewSize)
    {
        if ((NewSize > 0u) && (this->Elements.size() != NewSize))
        {
            this->Elements.resize(NewSize);
        }
    }

    template <typename T>
    void TArray<T>::Recapacity(const UINT32& NewCapacity)
    {
         if ((NewCapacity > 0u) && (this->Elements.capacity() != NewCapacity))
         {
             this->Elements.reserve(NewCapacity);
         }
    }

#endif

};
