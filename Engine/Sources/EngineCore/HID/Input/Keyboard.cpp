#include "./Keyboard.h"

BOOL IKeyboard::IsKeyPressed(unsigned char keycode) const
{
	return KeyStates[keycode];;
}
std::optional<IKeyboard::Event> IKeyboard::ReadKey()
{
	if (KeyBuffer.size() > 0u)
	{
		IKeyboard::Event e = KeyBuffer.front();
		KeyBuffer.pop();
		return e;
	}
	return {};
}
BOOL IKeyboard::IsKeyEmpty() const
{
	return KeyBuffer.empty();
}
void IKeyboard::FlushKey()
{
	KeyBuffer = std::queue<Event>();
}
std::optional<char> IKeyboard::ReadChar()
{
	if (CharBuffer.size() > 0u)
	{
		unsigned char charcode = CharBuffer.front();
		CharBuffer.pop();
		return charcode;
	}
	return {};
}
BOOL IKeyboard::IsCharEmpty() const
{
	return CharBuffer.empty();
}
void IKeyboard::FlushChar()
{
	CharBuffer = std::queue<char>();
}
void IKeyboard::Flush()
{
	FlushKey();
	FlushChar();
}
void IKeyboard::EnableAutorepeat()
{
	bAutorepeatEnabled = true;
}
void IKeyboard::DisableAutorepeat()
{
	bAutorepeatEnabled = false;
}
BOOL IKeyboard::IsAutorepeatEnabled() const
{
	return bAutorepeatEnabled;
}
void IKeyboard::OnKeyPressed(unsigned char keycode)
{
	KeyStates[keycode] = true;
	KeyBuffer.push(IKeyboard::Event(IKeyboard::Event::EType::Press, keycode));
	TrimBuffer(KeyBuffer);
}
void IKeyboard::OnKeyReleased(unsigned char keycode)
{
	KeyStates[keycode] = false;
	KeyBuffer.push(IKeyboard::Event(IKeyboard::Event::EType::Release, keycode));
	TrimBuffer(KeyBuffer);
}
void IKeyboard::OnChar(char character)
{
	CharBuffer.push(character);
	TrimBuffer(CharBuffer);
}
void IKeyboard::ClearState()
{
	KeyStates.reset();
}
template<typename T>
inline void IKeyboard::TrimBuffer(std::queue<T>& buffer)
{
	while (buffer.size() > BufferSize)
	{
		buffer.pop();
	}
}