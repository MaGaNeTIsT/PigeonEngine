#include "InputType.h"

#include "Base/DataStructure/Container/Array.h"

namespace PigeonEngine
{

    const EKey EKeysBuiltIn::AnyKey("AnyKey", -1);

    // const EKey EKeysBuiltIn::MouseX("MouseX");
    // const EKey EKeysBuiltIn::MouseY("MouseY");
    // const EKey EKeysBuiltIn::Mouse2D("Mouse2D");
    // const EKey EKeysBuiltIn::MouseScrollUp("MouseScrollUp");
    // const EKey EKeysBuiltIn::MouseScrollDown("MouseScrollDown");
    // const EKey EKeysBuiltIn::MouseWheelAxis("MouseWheelAxis");

    const EKey EKeysBuiltIn::LeftMouseButton  ("LeftMouseButton", 0x01);
    const EKey EKeysBuiltIn::RightMouseButton ("RightMouseButton", 0x02);
    const EKey EKeysBuiltIn::MiddleMouseButton("MiddleMouseButton", 0x04);
    const EKey EKeysBuiltIn::ThumbMouseButton ("ThumbMouseButton", 0x05);
    const EKey EKeysBuiltIn::ThumbMouseButton2("ThumbMouseButton2", 0x06);

    const EKey EKeysBuiltIn::BackSpace("BackSpace", 0x08);
    const EKey EKeysBuiltIn::Tab      ("Tab", 0x09);
    const EKey EKeysBuiltIn::Enter    ("Enter", 0x0D);
    const EKey EKeysBuiltIn::Pause    ("Pause", 0x13);

    const EKey EKeysBuiltIn::CapsLock("CapsLock", 0x14);
    const EKey EKeysBuiltIn::Escape  ("Escape", 0x1B);
    const EKey EKeysBuiltIn::SpaceBar("SpaceBar", 0x20);
    const EKey EKeysBuiltIn::PageUp  ("PageUp", 0x21);
    const EKey EKeysBuiltIn::PageDown("PageDown", 0x22);
    const EKey EKeysBuiltIn::End     ("End", 0x23);
    const EKey EKeysBuiltIn::Home    ("Home", 0x24);

    const EKey EKeysBuiltIn::Left ("Left", 0x25);
    const EKey EKeysBuiltIn::Up   ("Up", 0x26);
    const EKey EKeysBuiltIn::Right("Right", 0x27);
    const EKey EKeysBuiltIn::Down ("Down", 0x28);

    const EKey EKeysBuiltIn::Insert("Insert", 0x2D);
    const EKey EKeysBuiltIn::Delete("Delete", 0x2E);

    const EKey EKeysBuiltIn::Zero ("Zero",  0x30);
    const EKey EKeysBuiltIn::One  ("One",   0x31);
    const EKey EKeysBuiltIn::Two  ("Two",   0x32);
    const EKey EKeysBuiltIn::Three("Three", 0x33);
    const EKey EKeysBuiltIn::Four ("Four",  0x34);
    const EKey EKeysBuiltIn::Five ("Five",  0x35);
    const EKey EKeysBuiltIn::Six  ("Six",   0x36);
    const EKey EKeysBuiltIn::Seven("Seven", 0x37);
    const EKey EKeysBuiltIn::Eight("Eight", 0x38);
    const EKey EKeysBuiltIn::Nine ("Nine",  0x39);

    const EKey EKeysBuiltIn::A("A", 0x41);
    const EKey EKeysBuiltIn::B("B", 0x42);
    const EKey EKeysBuiltIn::C("C", 0x43);
    const EKey EKeysBuiltIn::D("D", 0x44);
    const EKey EKeysBuiltIn::E("E", 0x45);
    const EKey EKeysBuiltIn::F("F", 0x46);
    const EKey EKeysBuiltIn::G("G", 0x47);
    const EKey EKeysBuiltIn::H("H", 0x48);
    const EKey EKeysBuiltIn::I("I", 0x49);
    const EKey EKeysBuiltIn::J("J", 0x4A);
    const EKey EKeysBuiltIn::K("K", 0x4B);
    const EKey EKeysBuiltIn::L("L", 0x4C);
    const EKey EKeysBuiltIn::M("M", 0x4D);
    const EKey EKeysBuiltIn::N("N", 0x4E);
    const EKey EKeysBuiltIn::O("O", 0x4F);
    const EKey EKeysBuiltIn::P("P", 0x50);
    const EKey EKeysBuiltIn::Q("Q", 0x51);
    const EKey EKeysBuiltIn::R("R", 0x52);
    const EKey EKeysBuiltIn::S("S", 0x53);
    const EKey EKeysBuiltIn::T("T", 0x54);
    const EKey EKeysBuiltIn::U("U", 0x55);
    const EKey EKeysBuiltIn::V("V", 0x56);
    const EKey EKeysBuiltIn::W("W", 0x57);
    const EKey EKeysBuiltIn::X("X", 0x58);
    const EKey EKeysBuiltIn::Y("Y", 0x59);
    const EKey EKeysBuiltIn::Z("Z", 0x5A);

    const EKey EKeysBuiltIn::NumPadZero ("NumPadZero",  0x60);
    const EKey EKeysBuiltIn::NumPadOne  ("NumPadOne",   0x61);
    const EKey EKeysBuiltIn::NumPadTwo  ("NumPadTwo",   0x62);
    const EKey EKeysBuiltIn::NumPadThree("NumPadThree", 0x63);
    const EKey EKeysBuiltIn::NumPadFour ("NumPadFour",  0x64);
    const EKey EKeysBuiltIn::NumPadFive ("NumPadFive",  0x65);
    const EKey EKeysBuiltIn::NumPadSix  ("NumPadSix",   0x66);
    const EKey EKeysBuiltIn::NumPadSeven("NumPadSeven", 0x67);
    const EKey EKeysBuiltIn::NumPadEight("NumPadEight", 0x68);
    const EKey EKeysBuiltIn::NumPadNine ("NumPadNine",  0x69);

    const EKey EKeysBuiltIn::Multiply("Multiply", 0x6A);
    const EKey EKeysBuiltIn::Add     ("Add",      0x6B);
    const EKey EKeysBuiltIn::Subtract("Subtract", 0x6D);
    const EKey EKeysBuiltIn::Decimal ("Decimal",  0x6E);
    const EKey EKeysBuiltIn::Divide  ("Divide",   0x6F);

    const EKey EKeysBuiltIn::F1 ("F1",  0x70);
    const EKey EKeysBuiltIn::F2 ("F2",  0x71);
    const EKey EKeysBuiltIn::F3 ("F3",  0x72);
    const EKey EKeysBuiltIn::F4 ("F4",  0x73);
    const EKey EKeysBuiltIn::F5 ("F5",  0x74);
    const EKey EKeysBuiltIn::F6 ("F6",  0x75);
    const EKey EKeysBuiltIn::F7 ("F7",  0x76);
    const EKey EKeysBuiltIn::F8 ("F8",  0x77);
    const EKey EKeysBuiltIn::F9 ("F9",  0x78);
    const EKey EKeysBuiltIn::F10("F10", 0x79);
    const EKey EKeysBuiltIn::F11("F11", 0x7A);
    const EKey EKeysBuiltIn::F12("F12", 0x7B);

    const EKey EKeysBuiltIn::NumLock   ("NumLock",    0x90);
    const EKey EKeysBuiltIn::ScrollLock("ScrollLock", 0x91);

    const EKey EKeysBuiltIn::LeftShift   ("LeftShift",    0xA0);
    const EKey EKeysBuiltIn::RightShift  ("RightShift",   0xA1);
    const EKey EKeysBuiltIn::LeftControl ("LeftControl",  0xA2);
    const EKey EKeysBuiltIn::RightControl("RightControl", 0xA3);
    const EKey EKeysBuiltIn::LeftAlt     ("LeftAlt",      0xA4);
    const EKey EKeysBuiltIn::RightAlt    ("RightAlt",     0xA5);
    // const EKey EKeysBuiltIn::LeftCommand("LeftCommand");
    // const EKey EKeysBuiltIn::RightCommand("RightCommand");

    // const EKey EKeysBuiltIn::Semicolon("Semicolon", 0xBA);
    // const EKey EKeysBuiltIn::Equals("Equals");
    // const EKey EKeysBuiltIn::Comma("Comma", 0xBC);
    // const EKey EKeysBuiltIn::Underscore("Underscore");
    // const EKey EKeysBuiltIn::Hyphen("Hyphen");
    // const EKey EKeysBuiltIn::Period("Period", 0xBE);
    // const EKey EKeysBuiltIn::Slash("Slash");
    // const EKey EKeysBuiltIn::Tilde("Tilde");
    // const EKey EKeysBuiltIn::LeftBracket("LeftBracket");
    // const EKey EKeysBuiltIn::LeftParantheses("LeftParantheses");
    // const EKey EKeysBuiltIn::Backslash("Backslash");
    // const EKey EKeysBuiltIn::RightBracket("RightBracket");
    // const EKey EKeysBuiltIn::RightParantheses("RightParantheses");
    // const EKey EKeysBuiltIn::Apostrophe("Apostrophe");
    // const EKey EKeysBuiltIn::Quote("Quote");
    //
    // const EKey EKeysBuiltIn::Asterix("Asterix");
    // const EKey EKeysBuiltIn::Ampersand("Ampersand");
    // const EKey EKeysBuiltIn::Caret("Caret");
    // const EKey EKeysBuiltIn::Dollar("Dollar");
    // const EKey EKeysBuiltIn::Exclamation("Exclamation");
    // const EKey EKeysBuiltIn::Colon("Colon");

    // const EKey EKeysBuiltIn::A_AccentGrave("A_AccentGrave");
    // const EKey EKeysBuiltIn::E_AccentGrave("E_AccentGrave");
    // const EKey EKeysBuiltIn::E_AccentAigu("E_AccentAigu");
    // const EKey EKeysBuiltIn::C_Cedille("C_Cedille");
    // const EKey EKeysBuiltIn::Section("Section");
    const EKey& EKeysBuiltIn::GetKeyByKeyCode(const UINT8& KeyCode)
    {
        TArray<const EKey*> AllKeys;
        AllKeys.Add(&EKeysBuiltIn::LeftMouseButton);
        AllKeys.Add(&EKeysBuiltIn::LeftMouseButton);
        AllKeys.Add(&EKeysBuiltIn::RightMouseButton);
        AllKeys.Add(&EKeysBuiltIn::MiddleMouseButton);
        AllKeys.Add(&EKeysBuiltIn::ThumbMouseButton);
        AllKeys.Add(&EKeysBuiltIn::ThumbMouseButton2);
        AllKeys.Add(&EKeysBuiltIn::BackSpace);
        AllKeys.Add(&EKeysBuiltIn::Tab);
        AllKeys.Add(&EKeysBuiltIn::Enter);
        AllKeys.Add(&EKeysBuiltIn::Pause);
        AllKeys.Add(&EKeysBuiltIn::CapsLock);
        AllKeys.Add(&EKeysBuiltIn::Escape);
        AllKeys.Add(&EKeysBuiltIn::SpaceBar);
        AllKeys.Add(&EKeysBuiltIn::PageUp);
        AllKeys.Add(&EKeysBuiltIn::PageDown);
        AllKeys.Add(&EKeysBuiltIn::End);
        AllKeys.Add(&EKeysBuiltIn::Home);
        AllKeys.Add(&EKeysBuiltIn::Left);
        AllKeys.Add(&EKeysBuiltIn::Up);
        AllKeys.Add(&EKeysBuiltIn::Right);
        AllKeys.Add(&EKeysBuiltIn::Down);
        AllKeys.Add(&EKeysBuiltIn::Insert);
        AllKeys.Add(&EKeysBuiltIn::Delete);
        AllKeys.Add(&EKeysBuiltIn::Zero);
        AllKeys.Add(&EKeysBuiltIn::One);
        AllKeys.Add(&EKeysBuiltIn::Two);
        AllKeys.Add(&EKeysBuiltIn::Three);
        AllKeys.Add(&EKeysBuiltIn::Four);
        AllKeys.Add(&EKeysBuiltIn::Five);
        AllKeys.Add(&EKeysBuiltIn::Six);
        AllKeys.Add(&EKeysBuiltIn::Seven);
        AllKeys.Add(&EKeysBuiltIn::Eight);
        AllKeys.Add(&EKeysBuiltIn::Nine);
        AllKeys.Add(&EKeysBuiltIn::A);
        AllKeys.Add(&EKeysBuiltIn::B);
        AllKeys.Add(&EKeysBuiltIn::C);
        AllKeys.Add(&EKeysBuiltIn::D);
        AllKeys.Add(&EKeysBuiltIn::E);
        AllKeys.Add(&EKeysBuiltIn::F);
        AllKeys.Add(&EKeysBuiltIn::G);
        AllKeys.Add(&EKeysBuiltIn::H);
        AllKeys.Add(&EKeysBuiltIn::I);
        AllKeys.Add(&EKeysBuiltIn::J);
        AllKeys.Add(&EKeysBuiltIn::K);
        AllKeys.Add(&EKeysBuiltIn::L);
        AllKeys.Add(&EKeysBuiltIn::M);
        AllKeys.Add(&EKeysBuiltIn::N);
        AllKeys.Add(&EKeysBuiltIn::O);
        AllKeys.Add(&EKeysBuiltIn::P);
        AllKeys.Add(&EKeysBuiltIn::Q);
        AllKeys.Add(&EKeysBuiltIn::R);
        AllKeys.Add(&EKeysBuiltIn::S);
        AllKeys.Add(&EKeysBuiltIn::T);
        AllKeys.Add(&EKeysBuiltIn::U);
        AllKeys.Add(&EKeysBuiltIn::V);
        AllKeys.Add(&EKeysBuiltIn::W);
        AllKeys.Add(&EKeysBuiltIn::X);
        AllKeys.Add(&EKeysBuiltIn::Y);
        AllKeys.Add(&EKeysBuiltIn::Z);
        AllKeys.Add(&EKeysBuiltIn::NumPadZero);
        AllKeys.Add(&EKeysBuiltIn::NumPadOne);
        AllKeys.Add(&EKeysBuiltIn::NumPadTwo);
        AllKeys.Add(&EKeysBuiltIn::NumPadThree);
        AllKeys.Add(&EKeysBuiltIn::NumPadFour);
        AllKeys.Add(&EKeysBuiltIn::NumPadFive);
        AllKeys.Add(&EKeysBuiltIn::NumPadSix);
        AllKeys.Add(&EKeysBuiltIn::NumPadSeven);
        AllKeys.Add(&EKeysBuiltIn::NumPadEight);
        AllKeys.Add(&EKeysBuiltIn::NumPadNine);
        AllKeys.Add(&EKeysBuiltIn::Multiply);
        AllKeys.Add(&EKeysBuiltIn::Add);
        AllKeys.Add(&EKeysBuiltIn::Subtract);
        AllKeys.Add(&EKeysBuiltIn::Decimal);
        AllKeys.Add(&EKeysBuiltIn::Divide);
        AllKeys.Add(&EKeysBuiltIn::F1);
        AllKeys.Add(&EKeysBuiltIn::F2);
        AllKeys.Add(&EKeysBuiltIn::F3);
        AllKeys.Add(&EKeysBuiltIn::F4);
        AllKeys.Add(&EKeysBuiltIn::F5);
        AllKeys.Add(&EKeysBuiltIn::F6);
        AllKeys.Add(&EKeysBuiltIn::F7);
        AllKeys.Add(&EKeysBuiltIn::F8);
        AllKeys.Add(&EKeysBuiltIn::F9);
        AllKeys.Add(&EKeysBuiltIn::F10);
        AllKeys.Add(&EKeysBuiltIn::F11);
        AllKeys.Add(&EKeysBuiltIn::F12);
        AllKeys.Add(&EKeysBuiltIn::NumLock);
        AllKeys.Add(&EKeysBuiltIn::ScrollLock);
        AllKeys.Add(&EKeysBuiltIn::LeftShift);
        AllKeys.Add(&EKeysBuiltIn::RightShift);
        AllKeys.Add(&EKeysBuiltIn::LeftControl);
        AllKeys.Add(&EKeysBuiltIn::RightControl);
        AllKeys.Add(&EKeysBuiltIn::LeftAlt);
        AllKeys.Add(&EKeysBuiltIn::RightAlt);

        for(const auto& elem : AllKeys)
        {
            if(elem->GetKeyCode() == KeyCode)
            {
                return *elem;
            }
        }

        return EKeysBuiltIn::LeftMouseButton;
    }
}
