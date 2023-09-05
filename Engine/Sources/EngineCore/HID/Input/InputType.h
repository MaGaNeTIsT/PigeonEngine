#pragma once
#include "Base/DataStructure/Text/String.h"

namespace PigeonEngine
{
    enum EKeyStateType : UINT8
    {
        PRESSED,
        REALEASED,
    };

	
    struct EKey
    {
        EKey() = default;
        EKey(EString InKeyName, const UINT8& InKeyCode) : KeyName(std::move(InKeyName)), KeyCode(InKeyCode){}

        PE_NODISCARD const EString& GetKeyName()const{return this->KeyName;}
        PE_NODISCARD const UINT8&   GetKeyCode()const{return this->KeyCode;}

    private:
        EString KeyName;
        UINT8   KeyCode = -1;
    };

    struct EKeysBuiltIn
    {
        static const EKey AnyKey;

		// static const EKey MouseX;
		// static const EKey MouseY;
		// static const EKey Mouse2D;
		// static const EKey MouseScrollUp;
		// static const EKey MouseScrollDown;
		// static const EKey MouseWheelAxis;

		static const EKey LeftMouseButton;
		static const EKey RightMouseButton;
		static const EKey MiddleMouseButton;
		static const EKey ThumbMouseButton;
		static const EKey ThumbMouseButton2;

		static const EKey BackSpace;
		static const EKey Tab;
		static const EKey Enter;
		static const EKey Pause;

		static const EKey CapsLock;
		static const EKey Escape;
		static const EKey SpaceBar;
		static const EKey PageUp;
		static const EKey PageDown;
		static const EKey End;
		static const EKey Home;

		static const EKey Left;
		static const EKey Up;
		static const EKey Right;
		static const EKey Down;

		static const EKey Insert;
		static const EKey Delete;

		static const EKey Zero;
		static const EKey One;
		static const EKey Two;
		static const EKey Three;
		static const EKey Four;
		static const EKey Five;
		static const EKey Six;
		static const EKey Seven;
		static const EKey Eight;
		static const EKey Nine;

		static const EKey A;
		static const EKey B;
		static const EKey C;
		static const EKey D;
		static const EKey E;
		static const EKey F;
		static const EKey G;
		static const EKey H;
		static const EKey I;
		static const EKey J;
		static const EKey K;
		static const EKey L;
		static const EKey M;
		static const EKey N;
		static const EKey O;
		static const EKey P;
		static const EKey Q;
		static const EKey R;
		static const EKey S;
		static const EKey T;
		static const EKey U;
		static const EKey V;
		static const EKey W;
		static const EKey X;
		static const EKey Y;
		static const EKey Z;

		static const EKey NumPadZero;
		static const EKey NumPadOne;
		static const EKey NumPadTwo;
		static const EKey NumPadThree;
		static const EKey NumPadFour;
		static const EKey NumPadFive;
		static const EKey NumPadSix;
		static const EKey NumPadSeven;
		static const EKey NumPadEight;
		static const EKey NumPadNine;

		static const EKey Multiply;
		static const EKey Add;
		static const EKey Subtract;
		static const EKey Decimal;
		static const EKey Divide;

		static const EKey F1;
		static const EKey F2;
		static const EKey F3;
		static const EKey F4;
		static const EKey F5;
		static const EKey F6;
		static const EKey F7;
		static const EKey F8;
		static const EKey F9;
		static const EKey F10;
		static const EKey F11;
		static const EKey F12;

		static const EKey NumLock;

		static const EKey ScrollLock;

		static const EKey LeftShift;
		static const EKey RightShift;
		static const EKey LeftControl;
		static const EKey RightControl;
		static const EKey LeftAlt;
		static const EKey RightAlt;
		// static const EKey LeftCommand;
		// static const EKey RightCommand;

		// static const EKey Semicolon;
		// static const EKey Equals;
		// static const EKey Comma;
		// static const EKey Underscore;
		// static const EKey Hyphen;
		// static const EKey Period;
		// static const EKey Slash;
		// static const EKey Tilde;
		// static const EKey LeftBracket;
		// static const EKey Backslash;
		// static const EKey RightBracket;
		// static const EKey Apostrophe;
		//
		// static const EKey Ampersand;
		// static const EKey Asterix;
		// static const EKey Caret;
		// static const EKey Colon;
		// static const EKey Dollar;
		// static const EKey Exclamation;
		// static const EKey LeftParantheses;
		// static const EKey RightParantheses;
		// static const EKey Quote;
		//
		// static const EKey A_AccentGrave;
		// static const EKey E_AccentGrave;
		// static const EKey E_AccentAigu;
		// static const EKey C_Cedille;
		// static const EKey Section;

    	static const EKey& GetKeyByKeyCode(const UINT8& KeyCode);
    	
    };
}


