#pragma once
#include <queue>
#include <optional>
class CMouse
{
public:

	struct RawDelta
	{
		INT x, y;
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
		BOOL bLeftIsPressed = false;
		BOOL bRightIsPressed = false;
		INT x;
		INT y;

	public:

		Event(EType INType, const CMouse& Parent)
			:
			Type(INType),
			bLeftIsPressed()
		{

		}
		EType GetType() const
		{
			return Type;
		}
		std::pair<INT, INT> GetPos() const 
		{
			return{ x,y };
		}
		INT GetPosX() const 
		{
			return x;
		}
		INT GetPosY() const 
		{
			return y;
		}
		BOOL LeftIsPressed() const 
		{
			return bLeftIsPressed;
		}
		BOOL RightIsPressed() const 
		{
			return bRightIsPressed;
		}
	};

public:

	CMouse() = default;
	CMouse(const CMouse&) = delete;
	CMouse& operator=(const CMouse&) = delete;
	std::pair<INT, INT> GetPos() const ;
	std::optional<RawDelta> ReadRawDelta() ;
	INT GetPosX() const ;
	INT GetPosY() const ;
	BOOL IsInWindow() const ;
	BOOL LeftIsPressed() const ;
	BOOL RightIsPressed() const ;
	std::optional<CMouse::Event> Read() ;
	BOOL IsEmpty() const 
	{
		return Buffer.empty();
	}
	void Flush() ;
	void EnableRaw() ;
	void DisableRaw() ;
	BOOL IsRawEnabled() const ;

public:

	void OnMouseMove(INT x, INT y) ;
	void OnMouseLeave() ;
	void OnMouseEnter() ;
	void OnRawDelta(INT dx, INT dy) ;
	void OnLeftPressed(INT x, INT y) ;
	void OnLeftReleased(INT x, INT y) ;
	void OnRightPressed(INT x, INT y) ;
	void OnRightReleased(INT x, INT y) ;
	void OnWheelUp(INT x, INT y) ;
	void OnWheelDown(INT x, INT y) ;
	void TrimBuffer() ;
	void TrimRawInputBuffer() ;
	void OnWheelDelta(INT x, INT y, INT delta) ;

private:  

	const UINT BufferSize = 16u;
	INT x;
	INT y;
	BOOL bLeftIsPressed = false;
	BOOL bRightIsPressed = false;
	BOOL bIsInWindow = false;
	INT WheelDeltaCarry = 0;
	BOOL RawEnabled = true;
	std::queue<Event> Buffer;
	std::queue<RawDelta> RawDeltaBuffer;
};

