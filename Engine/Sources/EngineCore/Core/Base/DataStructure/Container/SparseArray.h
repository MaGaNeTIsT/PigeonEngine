#pragma once

#include <Main/Main.h>
#include <Base/DataStructure/BuiltInType.h>
#include <Base/Math/Math.h>
#include <Core/Config/ErrorCaption.h>
#include <Development/Alert/DevelopmentDefines.h>
#include <vector>
#include <algorithm>
#include <random>
#include <array>

namespace PigeonEngine
{

    template <typename _TDataType>
    class TSparseArray final
    {
    public:
        constexpr const UINT32 MaxID = static_cast<UINT32>(-1);
    private:
        struct EDataStructure
        {
            EDataStructure()
                : PrevID(TSparseArray::MaxID), NextID(TSparseArray::MaxID), Data(nullptr)
            {
            }
            EDataStructure(const EDataStructure& Other)
                : PrevID(Other.PrevID), NextID(Other.NextID), Data(nullptr)
            {
                if (!!(Other.Data))
                {
                    Data = new _TDataType(*(Other.Data));
                }
            }
            EDataStructure& operator=(const EDataStructure& Other)
            {
                if (!!Data)
                {
                    delete Data;
                    Data = nullptr;
                }
                PrevID = Other.PrevID;
                NextID = Other.NextID;
                if (!!(Other.Data))
                {
                    Data = new _TDataType(*(Other.Data));
                }
                return (*this);
            }
            void Clear()
            {
                if (!!Data)
                {
                    delete Data;
                    Data = nullptr;
                }
                PrevID = TSparseArray::MaxID;
                NextID = TSparseArray::MaxID;
            }
            void Reset(const UINT32 InPrevID, const UINT32 InNextID)
            {
                if (!!Data)
                {
                    delete Data;
                    Data = nullptr;
                }
                PrevID = InPrevID;
                NextID = InNextID;
            }
            UINT32          PrevID;
            UINT32          NextID;
            _TDataType*     Data;
        };
    public:
        TSparseArray()
            : StartID(TSparseArray::MaxID), EmptyID(0u), DataSize(0u)
        {
        }
        ~TSparseArray()
        {
            Empty();
        }
        TSparseArray(const TSparseArray& Other)
            : StartID(Other.StartID), EmptyID(Other.EmptyID), DataSize(Other.DataSize)
        {
            Empty();
            if (Other.Datas.size() > 0u)
            {
                Datas.resize(Other.Datas.size());
                for (size_t i = 0u, n = Other.Datas.size(); i < n; i++)
                {
                    Datas[i] = Other.Datas[i];
                }
            }
        }
    public:
        UINT32 Add(const _TDataType& InData)
        {
            const UINT32 Result = EmptyID;
            const UINT32 TotalLength = Length();
            if (EmptyID < TotalLength)
            {
                Check((Datas[EmptyID].PrevID == TSparseArray::MaxID));
                Check((!(Datas[EmptyID].Data)));
                const UINT32 NextEmptyID = Datas[EmptyID].NextID;

                Datas[EmptyID].PrevID = (EmptyID > 0u) ? (EmptyID - 1u) : TSparseArray::MaxID;
                Datas[EmptyID].NextID = TSparseArray::MaxID;
                {
                    const UINT32 TempPrevID = Datas[EmptyID].PrevID;
                    if ((TempPrevID < TotalLength) && (TempPrevID != TSparseArray::MaxID))
                    {
                        const UINT32 TempNextID = Datas[TempPrevID].NextID;
                        if ((TempNextID < TotalLength) && (TempNextID != TSparseArray::MaxID))
                        {
                            Datas[TempNextID].PrevID = EmptyID;
                        }
                        Datas[TempPrevID].NextID = EmptyID;
                        Datas[EmptyID].NextID = TempNextID;
                    }
                }
                Datas[EmptyID].Data = new _TDataType(InData);

                Datas[NextEmptyID].PrevID = TSparseArray::MaxID;
                EmptyID = NextEmptyID;
            }
            else
            {
                Check((EmptyID == DataSize));
                Check((DataSize == TotalLength));
                EDataStructure TempData;
                TempData.PrevID = (DataSize > 0u) ? (DataSize - 1u) : TSparseArray::MaxID;
                TempData.NextID = DataSize + 1u;
                {
                    const UINT32 TempPrevID = TempData.PrevID;
                    if ((TempPrevID < TotalLength) && (TempPrevID != TSparseArray::MaxID))
                    {
                        Datas[TempPrevID].NextID = EmptyID;
                    }
                }
                TempData.Data = new _TDataType(InData);
                Datas.push_back(std::move(TempData));
                EmptyID++;
            }
            Check((EmptyID != 0u));
            DataSize++;
            Check(DataSize <= static_cast<UINT32>(Datas.size()));
            StartID = 0u;
            return Result;
        }
        UINT32 Add(_TDataType&& InData)
        {
            const UINT32 Result = EmptyID;
            const UINT32 TotalLength = Length();
            if (EmptyID < TotalLength)
            {
                Check((Datas[EmptyID].PrevID == TSparseArray::MaxID));
                Check((!(Datas[EmptyID].Data)));
                const UINT32 NextEmptyID = Datas[EmptyID].NextID;

                Datas[EmptyID].PrevID = (EmptyID > 0u) ? (EmptyID - 1u) : TSparseArray::MaxID;
                Datas[EmptyID].NextID = TSparseArray::MaxID;
                {
                    const UINT32 TempPrevID = Datas[EmptyID].PrevID;
                    if ((TempPrevID < TotalLength) && (TempPrevID != TSparseArray::MaxID))
                    {
                        const UINT32 TempNextID = Datas[TempPrevID].NextID;
                        if ((TempNextID < TotalLength) && (TempNextID != TSparseArray::MaxID))
                        {
                            Datas[TempNextID].PrevID = EmptyID;
                        }
                        Datas[TempPrevID].NextID = EmptyID;
                        Datas[EmptyID].NextID = TempNextID;
                    }
                }
                Datas[EmptyID].Data = new _TDataType(std::forward(InData));

                Datas[NextEmptyID].PrevID = TSparseArray::MaxID;
                EmptyID = NextEmptyID;
            }
            else
            {
                Check((EmptyID == DataSize));
                Check((DataSize == TotalLength));
                EDataStructure TempData;
                TempData.PrevID = (DataSize > 0u) ? (DataSize - 1u) : TSparseArray::MaxID;
                TempData.NextID = DataSize + 1u;
                {
                    const UINT32 TempPrevID = TempData.PrevID;
                    if ((TempPrevID < TotalLength) && (TempPrevID != TSparseArray::MaxID))
                    {
                        Datas[TempPrevID].NextID = EmptyID;
                    }
                }
                TempData.Data = new _TDataType(std::forward(InData));
                Datas.push_back(std::move(TempData));
                EmptyID++;
            }
            Check((EmptyID != 0u));
            DataSize++;
            Check(DataSize <= static_cast<UINT32>(Datas.size()));
            StartID = 0u;
            return Result;
        }
        void Remove(const UINT32 InIndex)
        {
            const UINT32 TotalLength = Length();
            if (InIndex >= TotalLength || (!(Datas[InIndex].Data)))
            {
                return;
            }

            Check((EmptyID != InIndex));
            Check((Datas[InIndex].NextID != TSparseArray::MaxID));

            {
                const UINT32 TempPrevID = Datas[InIndex].PrevID;
                const UINT32 TempNextID = Datas[InIndex].NextID;
                if ((TempPrevID < TotalLength) && (TempPrevID != TSparseArray::MaxID))
                {
                    Datas[TempPrevID].NextID = TempNextID;
                }
                if ((TempNextID < TotalLength) && (TempNextID != TSparseArray::MaxID))
                {
                    Datas[TempNextID].PrevID = TempPrevID;
                }
                if ((StartID == InIndex) && (StartID != TSparseArray::MaxID))
                {
                    StartID = TempNextID;
                }
                Check((DataSize > 0u));
                DataSize--;
            }

            Datas[InIndex].Clear();

            if ((EmptyID < TotalLength) && (EmptyID != TSparseArray::MaxID))
            {
                if (InIndex < EmptyID)
                {
                    Datas[EmptyID].PrevID = InIndex;
                    Datas[InIndex].NextID = EmptyID;
                    EmptyID = InIndex;
                }
                else
                {
                    UINT32 LeftID = EmptyID;
                    UINT32 RightID = Datas[LeftID].NextID;
                    while (TRUE)
                    {
                        if ((LeftID >= TotalLength) || (LeftID == TSparseArray::MaxID))
                        {
                            Check((FALSE));
                            return;
                        }
                        if ((LeftID < InIndex) && (InIndex < RightID))
                        {
                            if ((LeftID < TotalLength) && (LeftID != TSparseArray::MaxID))
                            {
                                Datas[LeftID].NextID = InIndex;
                            }
                            if ((RightID < TotalLength) && (RightID != TSparseArray::MaxID))
                            {
                                Datas[RightID].PrevID = InIndex;
                            }
                            return;
                        }
                        LeftID = RightID;
                        RightID = Datas[LeftID].NextID;
                    }
                }
            }
        }
        void Empty(const UINT32 InHoldNum = 0u)
        {
            for (size_t i = 0u, n = Datas.size(); i < n; i++)
            {
                Datas[i].Clear();
            }
            if (InHoldNum > 0u)
            {
                Datas.resize(static_cast<size_t>(InHoldNum));
                for (size_t i = 0u, n = Datas.size(); i < n; i++)
                {
                    Datas[i].Reset(((i > 0u) ? (i - 1u) : TSparseArray::MaxID), (i + 1u));
                }
            }
            else
            {
                Datas.clear();
            }
            StartID = TSparseArray::MaxID;
            EmptyID = 0u;
            DataSize = 0u;
        }
        PE_NODISCARD UINT32 DataLength()const
        {
            return DataSize;
        }
        template<typename _TOtherSizeType = INT32, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        PE_NODISCARD _TOtherSizeType Num()const
        {
            return static_cast<_TOtherSizeType>(Datas.size());
        }
        PE_NODISCARD _TDataType& operator[](const UINT32 InIndex)
        {
            Check((InIndex < Length()));
            Check((!!(Datas[InIndex].Data)));
            return (*(Datas[InIndex].Data));
        }
        PE_NODISCARD const _TDataType& operator[](const UINT32 InIndex)
        {
            Check((InIndex < Length()));
            Check((!!(Datas[InIndex].Data)));
            return (*(Datas[InIndex].Data));
        }
    private:
        UINT32                      StartID;
        UINT32                      EmptyID;
        UINT32                      DataSize;
        std::vector<EDataStructure> Datas;
    };

};
