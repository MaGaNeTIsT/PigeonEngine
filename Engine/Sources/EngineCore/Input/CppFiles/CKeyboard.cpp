#include "../../../../Entry/EngineMain.h"
#include "../Headers/CKeyboard.h"

BOOL CKeyboard::IsKeyPressed(unsigned char keycode) const 
{
	return KeyStates[keycode];;
}

std::optional<CKeyboard::Event> CKeyboard::ReadKey()
{
	if (KeyBuffer.size() > 0u)
	{
		CKeyboard::Event e = KeyBuffer.front();
		KeyBuffer.pop();
		return e;
	}
	return {};
}

BOOL CKeyboard::IsKeyEmpty() const 
{
	return KeyBuffer.empty();
}

void CKeyboard::FlushKey() 
{
	KeyBuffer = std::queue<Event>();
}

std::optional<char> CKeyboard::ReadChar() 
{
	if (CharBuffer.size() > 0u)
	{
		unsigned char charcode = CharBuffer.front();
		CharBuffer.pop();
		return charcode;
	}
	return {};
}

BOOL CKeyboard::IsCharEmpty() const 
{
	return CharBuffer.empty();
}

void CKeyboard::FlushChar() 
{
	CharBuffer = std::queue<char>();
}

void CKeyboard::Flush() 
{
	FlushKey();
	FlushChar();
}

void CKeyboard::EnableAutorepeat() 
{
	bAutorepeatEnabled = true;
}

void CKeyboard::DisableAutorepeat() 
{
	bAutorepeatEnabled = false;
}

BOOL CKeyboard::IsAutorepeatEnabled() const
{
	return bAutorepeatEnabled;
}

void CKeyboard::OnKeyPressed(unsigned char keycode)
{
	KeyStates[keycode] = true;
	KeyBuffer.push(CKeyboard::Event(CKeyboard::Event::EType::Press, keycode));
	TrimBuffer(KeyBuffer);
}

void CKeyboard::OnKeyReleased(unsigned char keycode)
{
	KeyStates[keycode] = false;
	KeyBuffer.push(CKeyboard::Event(CKeyboard::Event::EType::Release, keycode));
	TrimBuffer(KeyBuffer);
}

void CKeyboard::OnChar(char character)
{
	CharBuffer.push(character);
	TrimBuffer(CharBuffer);
}

void CKeyboard::ClearState()
{
	KeyStates.reset();
}

template<typename T>
inline void CKeyboard::TrimBuffer(std::queue<T>& buffer)
{
	while (buffer.size() > BufferSize)
	{
		buffer.pop();
	}
}
