#pragma once

#include "../../../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include "../../../../Core/Config/ErrorCaption.h"
#include "../../../../../Development/Alert/DevelopmentDefines.h"
#include "Base/Math/Math.h"
#include "Base/Memory/Memory.h"

namespace PigeonEngine
{

#if 0
	template<typename _Ty1, typename _Ty2>
	class TPair final
	{
	public:
		_Ty1		First;
		_Ty2		Second;

		TPair() noexcept
			: First{}
			, Second{}
		{
		}
		TPair(const _Ty1& InFirst, const _Ty2& InSecond) noexcept
			: First{ InFirst }
			, Second{ InSecond }
		{
		}
		TPair(_Ty1&& InFirst, _Ty2&& InSecond) noexcept
			: First{ EMemory::Forward<_Ty1>(InFirst) }
			, Second{ EMemory::Forward<_Ty2>(InSecond) }
		{
		}
		TPair& operator=(const TPair& Other) noexcept
		{
			First = Other.First;
			Second = Other.Second;
			return (*this);
		}
		TPair& operator=(TPair&& Other) noexcept
		{
			First = EMemory::Move(Other.First);
			Second = EMemory::Move(Other.Second);
			return (*this);
		}
	};
#endif

};
