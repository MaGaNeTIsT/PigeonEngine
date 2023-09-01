#pragma once

#include "../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <Base/DataStructure/BaseType.h>
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
			BOOL32 IsPress() const
			{
				return Type == EType::Press;
			}
			BOOL32 IsRelease() const
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
		BOOL32 IsKeyPressed(unsigned char keycode) const;
		std::optional<Event> ReadKey();
		BOOL32 IsKeyEmpty() const;
		void FlushKey();
		// char event stuff
		std::optional<char> ReadChar();
		BOOL32 IsCharEmpty() const;
		void FlushChar();
		void Flush();
		// autorepeat control
		void EnableAutorepeat();
		void DisableAutorepeat();
		BOOL32 IsAutorepeatEnabled() const;

	private:
		void OnKeyPressed(unsigned char keycode);
		void OnKeyReleased(unsigned char keycode);
		void OnChar(char character);
		void ClearState();
		template<typename T>
		static void TrimBuffer(std::queue<T>& buffer);

	private:
		static constexpr UINT32 nKeys = 256u;
		static constexpr UINT32 BufferSize = 16u;
		BOOL32 bAutorepeatEnabled = false;
		std::bitset<nKeys> KeyStates;
		std::queue<Event> KeyBuffer;
		std::queue<char> CharBuffer;
	};

};