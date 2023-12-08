#include "Keyboard.h"
#include "Input.h"

namespace PigeonEngine
{
	BOOL32 IKeyboard::IsKeyPressed(BYTE keycode) const
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
	BOOL32 IKeyboard::IsKeyEmpty() const
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
			BYTE charcode = CharBuffer.front();
			CharBuffer.pop();
			return charcode;
		}
		return {};
	}
	BOOL32 IKeyboard::IsCharEmpty() const
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
		bAutorepeatEnabled = TRUE;
	}
	void IKeyboard::DisableAutorepeat()
	{
		bAutorepeatEnabled = FALSE;
	}
	BOOL32 IKeyboard::IsAutorepeatEnabled() const
	{
		return bAutorepeatEnabled;
	}
	void IKeyboard::OnKeyPressed(BYTE keycode)
	{
		KeyStates[keycode] = TRUE;
		KeyBuffer.push(IKeyboard::Event(IKeyboard::Event::EType::Press, keycode));
		TrimBuffer(KeyBuffer);
		EInput::KeyEvent.Broadcast(IKeyboard::Event::EType::Press, keycode);
	}
	void IKeyboard::OnKeyReleased(BYTE keycode)
	{
		KeyStates[keycode] = FALSE;
		KeyBuffer.push(IKeyboard::Event(IKeyboard::Event::EType::Release, keycode));
		TrimBuffer(KeyBuffer);
		EInput::KeyEvent.Broadcast(IKeyboard::Event::EType::Release, keycode);
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

};