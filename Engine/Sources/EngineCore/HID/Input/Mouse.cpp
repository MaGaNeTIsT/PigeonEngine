#include "Mouse.h"

namespace PigeonEngine
{

	std::pair<INT32, INT32> IMouse::GetPos() const
	{
		return { x,y };
	}
	std::optional<IMouse::RawDelta> IMouse::ReadRawDelta()
	{
		if (RawDeltaBuffer.empty())
		{
			return std::nullopt;
		}
		const RawDelta d = RawDeltaBuffer.front();
		RawDeltaBuffer.pop();
		return d;
	}
	INT32 IMouse::GetPosX() const
	{
		return x;
	}
	INT32 IMouse::GetPosY() const
	{
		return y;
	}
	BOOL32 IMouse::IsInWindow() const
	{
		return bIsInWindow;
	}
	BOOL32 IMouse::LeftIsPressed() const
	{
		return bLeftIsPressed;
	}
	BOOL32 IMouse::RightIsPressed() const
	{
		return bRightIsPressed;
	}
	std::optional<IMouse::Event> IMouse::Read()
	{
		if (Buffer.size() > 0u)
		{
			IMouse::Event e = Buffer.front();
			Buffer.pop();
			return e;
		}
		return {};
	}
	void IMouse::Flush()
	{
		Buffer = std::queue<Event>();
	}
	void IMouse::EnableRaw()
	{
		RawEnabled = true;
	}
	void IMouse::DisableRaw()
	{
		RawEnabled = false;
	}
	BOOL32 IMouse::IsRawEnabled() const
	{
		return RawEnabled;
	}
	void IMouse::OnMouseMove(INT32 newx, INT32 newy)
	{
		x = newx;
		y = newy;

		Buffer.push(IMouse::Event(IMouse::Event::EType::Move, *this));
		TrimBuffer();
	}
	void IMouse::OnMouseLeave()
	{
		bIsInWindow = false;
		Buffer.push(IMouse::Event(IMouse::Event::EType::Leave, *this));
		TrimBuffer();
	}
	void IMouse::OnMouseEnter()
	{
		bIsInWindow = true;
		Buffer.push(IMouse::Event(IMouse::Event::EType::Enter, *this));
		TrimBuffer();
	}
	void IMouse::OnRawDelta(INT32 dx, INT32 dy)
	{
		RawDeltaBuffer.push({ dx,dy });
		TrimBuffer();
	}
	void IMouse::OnLeftPressed(INT32 x, INT32 y)
	{
		bLeftIsPressed = true;

		Buffer.push(IMouse::Event(IMouse::Event::EType::LPress, *this));
		TrimBuffer();
	}
	void IMouse::OnLeftReleased(INT32 x, INT32 y)
	{
		bLeftIsPressed = false;

		Buffer.push(IMouse::Event(IMouse::Event::EType::LRelease, *this));
		TrimBuffer();
	}
	void IMouse::OnRightPressed(INT32 x, INT32 y)
	{
		bRightIsPressed = true;

		Buffer.push(IMouse::Event(IMouse::Event::EType::RPress, *this));
		TrimBuffer();
	}
	void IMouse::OnRightReleased(INT32 x, INT32 y)
	{
		bRightIsPressed = false;

		Buffer.push(IMouse::Event(IMouse::Event::EType::RRelease, *this));
		TrimBuffer();
	}
	void IMouse::OnWheelUp(INT32 x, INT32 y)
	{
		Buffer.push(IMouse::Event(IMouse::Event::EType::WheelUp, *this));
		TrimBuffer();
	}
	void IMouse::OnWheelDown(INT32 x, INT32 y)
	{
		Buffer.push(IMouse::Event(IMouse::Event::EType::WheelDown, *this));
		TrimBuffer();
	}
	void IMouse::TrimBuffer()
	{
		while (Buffer.size() > BufferSize)
		{
			Buffer.pop();
		}
	}
	void IMouse::TrimRawInputBuffer()
	{
		while (RawDeltaBuffer.size() > BufferSize)
		{
			RawDeltaBuffer.pop();
		}
	}
	void IMouse::OnWheelDelta(INT32 x, INT32 y, INT32 delta)
	{
		WheelDeltaCarry += delta;
		// generate events for every 120 
		while (WheelDeltaCarry >= WHEEL_DELTA)
		{
			WheelDeltaCarry -= WHEEL_DELTA;
			OnWheelUp(x, y);
		}
		while (WheelDeltaCarry <= -WHEEL_DELTA)
		{
			WheelDeltaCarry += WHEEL_DELTA;
			OnWheelDown(x, y);
		}
	}

};