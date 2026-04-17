#pragma once

#include "../../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <Base/Memory/Memory.h>
#include <tuple>

namespace PigeonEngine
{
    template<SIZE_T... Index>
    using TIndexSequence = std::index_sequence<Index...>;

    template<SIZE_T Size>
    using TMakeIndexSequence = std::make_index_sequence<Size>;

    template<SIZE_T Index, typename TTupleType>
    decltype(auto) TupleGet(TTupleType&& InTuple);

    namespace ETupleHelper
    {
        template<typename TFunctionType, typename TTupleType, SIZE_T... Index>
        decltype(auto) ApplyImpl(TFunctionType&& InFunction, TTupleType&& InTuple, TIndexSequence<Index...>)
        {
            return EMemory::Forward<TFunctionType>(InFunction)(TupleGet<Index>(EMemory::Forward<TTupleType>(InTuple))...);
        }
    }

    template<typename... TTypes>
    using TTuple = std::tuple<TTypes...>;

    template<typename TTupleType>
    using TTupleSize = std::tuple_size<TTupleType>;

    template<typename TTupleType>
    PE_CONSTEXPR_STATIC SIZE_T TTupleSizeValue = std::tuple_size<TTupleType>::value;

    template<SIZE_T Index, typename TTupleType>
    using TTupleElement = std::tuple_element<Index, TTupleType>;

    template<SIZE_T Index, typename TTupleType>
    using TTupleElementType = typename std::tuple_element<Index, TTupleType>::type;

    template<SIZE_T Index, typename TTupleType>
    decltype(auto) TupleGet(TTupleType&& InTuple)
    {
        return std::get<Index>(EMemory::Forward<TTupleType>(InTuple));
    }

    template<typename... TTypes>
    PE_INLINE TTuple<TDecayType<TTypes>...> MakeTuple(TTypes&&... InValues)
    {
      return TTuple<TDecayType<TTypes>...>(EMemory::Forward<TTypes>(InValues)...);
    }

    template<typename TFunctionType, typename TTupleType>
    decltype(auto) TupleApply(TFunctionType&& InFunction, TTupleType&& InTuple)
    {
      using FTupleType = TRemoveRefType<TTupleType>;
        return ETupleHelper::ApplyImpl(
            EMemory::Forward<TFunctionType>(InFunction),
            EMemory::Forward<TTupleType>(InTuple),
            TMakeIndexSequence<TTupleSizeValue<FTupleType>>());
    }
};
