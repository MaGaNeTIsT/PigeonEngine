#pragma once

#include "../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <Base/DataStructure/BaseType.h>
#include <queue>
#include <optional>
#include "Base/Delegate/Delegate.h"
namespace PigeonEngine
{

	class IMouse
	{
	public:
		struct RawDelta
		{
			INT32 x, y;
		};
		class Event
		{
		public:
			enum class EType
			{
				LPress,
				LRelease,
				RPress,
				RRelease,
				WheelUp,
				WheelDown,
				Move,
				Enter,
				Leave,
				Invalid
			};

		private:
			EType Type;
			BOOL32 bLeftIsPressed = FALSE;
			BOOL32 bRightIsPressed = FALSE;
			INT32 x;
			INT32 y;

		public:
			Event(EType INType, const IMouse& Parent)
				:
				Type(INType),
				bLeftIsPressed(), x(0), y(0)
			{
			}
			EType GetType() const
			{
				return Type;
			}
			std::pair<INT32, INT32> GetPos() const
			{
				return{ x,y };
			}
			INT32 GetPosX() const
			{
				return x;
			}
			INT32 GetPosY() const
			{
				return y;
			}
			BOOL32 LeftIsPressed() const
			{
				return bLeftIsPressed;
			}
			BOOL32 RightIsPressed() const
			{
				return bRightIsPressed;
			}
		};

	public:
		IMouse() = default;
		IMouse(const IMouse&) = delete;
		IMouse& operator=(const IMouse&) = delete;
		std::pair<INT32, INT32> GetPos() const;
		std::optional<RawDelta> ReadRawDelta();
		INT32 GetPosX() const;
		INT32 GetPosY() const;
		BOOL32 IsInWindow() const;
		BOOL32 LeftIsPressed() const;
		BOOL32 RightIsPressed() const;
		std::optional<IMouse::Event> Read();
		BOOL32 IsEmpty() const
		{
			return Buffer.empty();
		}
		void Flush();
		void EnableRaw();
		void DisableRaw();
		BOOL32 IsRawEnabled() const;

	public:
		void OnMouseMove(INT32 InX, INT32 InY);
		void OnMouseLeave();
		void OnMouseEnter();
		void OnRawDelta(INT32 dx, INT32 dy);
		void OnLeftPressed(INT32 InX, INT32 InY);
		void OnLeftReleased(INT32 InX, INT32 InY);
		void OnRightPressed(INT32 InX, INT32 InY);
		void OnRightReleased(INT32 InX, INT32 InY);
		void OnWheelUp(INT32 InX, INT32 InY);
		void OnWheelDown(INT32 InX, INT32 InY);
		void TrimBuffer();
		void TrimRawInputBuffer();
		void OnWheelDelta(INT32 InX, INT32 InY, INT32 delta);
		
	
	private:
		const UINT32 BufferSize = 16u;
		INT32 x = 0;
		INT32 y = 0;
		BOOL32 bLeftIsPressed = FALSE;
		BOOL32 bRightIsPressed = FALSE;
		BOOL32 bIsInWindow = FALSE;
		INT32 WheelDeltaCarry = 0;
		BOOL32 RawEnabled = TRUE;
		std::queue<Event> Buffer;
		std::queue<RawDelta> RawDeltaBuffer;
	};

};