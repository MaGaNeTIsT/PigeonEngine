#pragma once

#include <CoreMinimal.h>
#include <queue>
#include <bitset>
#include <optional>

namespace PigeonEngine
{

	class IKeyboard
	{
		friend class IController;
	public:
		class Event
		{
		public:
			enum class EType
			{
				Press,
				Release,
			};
		private:
			EType Type;
			unsigned char Code;
		public:
			Event(EType InType, unsigned char InCode)
				:
				Type(InType),
				Code(InCode)
			{}
			BOOL IsPress() const
			{
				return Type == EType::Press;
			}
			BOOL IsRelease() const
			{
				return Type == EType::Release;
			}
			unsigned char GetCode() const
			{
				return Code;
			}
		};
	public:
		IKeyboard() = default;
		IKeyboard(const IKeyboard&) = delete;
		IKeyboard& operator=(const IKeyboard&) = delete;

	public:
		// key event stuff
		BOOL IsKeyPressed(unsigned char keycode) const;
		std::optional<Event> ReadKey();
		BOOL IsKeyEmpty() const;
		void FlushKey();
		// char event stuff
		std::optional<char> ReadChar();
		BOOL IsCharEmpty() const;
		void FlushChar();
		void Flush();
		// autorepeat control
		void EnableAutorepeat();
		void DisableAutorepeat();
		BOOL IsAutorepeatEnabled() const;

	private:
		void OnKeyPressed(unsigned char keycode);
		void OnKeyReleased(unsigned char keycode);
		void OnChar(char character);
		void ClearState();
		template<typename T>
		static void TrimBuffer(std::queue<T>& buffer);

	private:
		static constexpr UINT nKeys = 256u;
		static constexpr UINT BufferSize = 16u;
		BOOL bAutorepeatEnabled = false;
		std::bitset<nKeys> KeyStates;
		std::queue<Event> KeyBuffer;
		std::queue<char> CharBuffer;
	};

};