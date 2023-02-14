#include "../../../../Entry/EngineMain.h"
#include "../Headers/CMouse.h"



std::pair<INT, INT> CMouse::GetPos() const 
{
	return { x,y };
}

std::optional<CMouse::RawDelta> CMouse::ReadRawDelta() 
{
	if (RawDeltaBuffer.empty())
	{
		return std::nullopt;
	}
	const RawDelta d = RawDeltaBuffer.front();
	RawDeltaBuffer.pop();
	return d;
}

INT CMouse::GetPosX() const 
{
	return x;
}

INT CMouse::GetPosY() const 
{
	return y;
}

BOOL CMouse::IsInWindow() const 
{
	return bIsInWindow;
}

BOOL CMouse::LeftIsPressed() const 
{
	return bLeftIsPressed;
}

BOOL CMouse::RightIsPressed() const 
{
	return bRightIsPressed;
}

std::optional<CMouse::Event> CMouse::Read() 
{
	if (Buffer.size() > 0u)
	{
		CMouse::Event e = Buffer.front();
		Buffer.pop();
		return e;
	}
	return {};
}

void CMouse::Flush() 
{
	Buffer = std::queue<Event>();
}

void CMouse::EnableRaw() 
{
	RawEnabled = true;
}

void CMouse::DisableRaw() 
{
	RawEnabled = false;
}

BOOL CMouse::IsRawEnabled() const 
{
	return RawEnabled;
}

void CMouse::OnMouseMove(INT newx, INT newy) 
{
	x = newx;
	y = newy;

	Buffer.push(CMouse::Event(CMouse::Event::EType::Move, *this));
	TrimBuffer();
}

void CMouse::OnMouseLeave() 
{
	bIsInWindow = false;
	Buffer.push(CMouse::Event(CMouse::Event::EType::Leave, *this));
	TrimBuffer();
}

void CMouse::OnMouseEnter() 
{
	bIsInWindow = true;
	Buffer.push(CMouse::Event(CMouse::Event::EType::Enter, *this));
	TrimBuffer();
}

void CMouse::OnRawDelta(INT dx, INT dy) 
{
	RawDeltaBuffer.push({ dx,dy });
	TrimBuffer();
}

void CMouse::OnLeftPressed(INT x, INT y) 
{
	bLeftIsPressed = true;

	Buffer.push(CMouse::Event(CMouse::Event::EType::LPress, *this));
	TrimBuffer();
}

void CMouse::OnLeftReleased(INT x, INT y) 
{
	bLeftIsPressed = false;

	Buffer.push(CMouse::Event(CMouse::Event::EType::LRelease, *this));
	TrimBuffer();
}

void CMouse::OnRightPressed(INT x, INT y) 
{
	bRightIsPressed = true;

	Buffer.push(CMouse::Event(CMouse::Event::EType::RPress, *this));
	TrimBuffer();
}

void CMouse::OnRightReleased(INT x, INT y) 
{
	bRightIsPressed = false;

	Buffer.push(CMouse::Event(CMouse::Event::EType::RRelease, *this));
	TrimBuffer();
}

void CMouse::OnWheelUp(INT x, INT y) 
{
	Buffer.push(CMouse::Event(CMouse::Event::EType::WheelUp, *this));
	TrimBuffer();
}

void CMouse::OnWheelDown(INT x, INT y) 
{
	Buffer.push(CMouse::Event(CMouse::Event::EType::WheelDown, *this));
	TrimBuffer();
}

void CMouse::TrimBuffer() 
{
	while (Buffer.size() > BufferSize)
	{
		Buffer.pop();
	}
}

void CMouse::TrimRawInputBuffer() 
{
	while (RawDeltaBuffer.size() > BufferSize)
	{
		RawDeltaBuffer.pop();
	}
}

void CMouse::OnWheelDelta(INT x, INT y, INT delta) 
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