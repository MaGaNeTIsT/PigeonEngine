#pragma once

#if _EDITOR_ONLY

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Array.h>
#include <imgui.h>
#include <cstring>

namespace PigeonEngine
{

	/*
	* RAII wrapper for one ImGui ID stack level. Editor functions have many early returns
	* after a rebuild, so pairing PushID/PopID manually is error-prone; this keeps the
	* stack balanced even when a commit invalidates the live asset mid-frame.
	*/
	struct FImGuiIdScope
	{
	public:
		explicit FImGuiIdScope(const CHAR* InId)
		{
			ImGui::PushID(InId);
		}
		explicit FImGuiIdScope(INT32 InId)
		{
			ImGui::PushID(InId);
		}
		~FImGuiIdScope()
		{
			ImGui::PopID();
		}
	public:
		FImGuiIdScope(const FImGuiIdScope&) = delete;
		FImGuiIdScope& operator=(const FImGuiIdScope&) = delete;
	};

	/*
	* Drag handle between two vertically stacked areas. The caller owns the first height
	* and passes the minimums; the splitter keeps the second area reachable.
	*/
	struct FEditorSplitter
	{
	public:
		static constexpr FLOAT Thickness = 12.0f;
		static FLOAT GetCost()
		{
			return Thickness + ImGui::GetStyle().ItemSpacing.y * 2.0f;
		}
		static BOOL32 Horizontal(const CHAR* InId, FLOAT& InOutFirstHeight
			, FLOAT InMinFirstHeight, FLOAT InMinSecondHeight, FLOAT InTotalHeight)
		{
			const FLOAT Width = EMath::Max(ImGui::GetContentRegionAvail().x, 120.0f);
			ImGui::Button(InId, ImVec2(Width, Thickness));
			const BOOL32 bHovered = ImGui::IsItemHovered();
			const BOOL32 bActive = ImGui::IsItemActive();
			if ((bHovered != FALSE) || (bActive != FALSE))
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
			}
			const ImVec2 Min = ImGui::GetItemRectMin();
			const ImVec2 Max = ImGui::GetItemRectMax();
			const ImU32 Color = (bActive != FALSE)
				? IM_COL32(245, 210, 90, 255) : ((bHovered != FALSE)
					? IM_COL32(180, 185, 200, 255) : IM_COL32(70, 72, 80, 255));
			const FLOAT CenterY = (Min.y + Max.y) * 0.5f;
			ImGui::GetWindowDrawList()->AddLine(ImVec2(Min.x, CenterY)
				, ImVec2(Max.x, CenterY), Color, 2.0f);
			// A small grip in the middle makes the boundary discoverable.
			ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(Min.x + Width * 0.42f, CenterY - 3.0f)
				, ImVec2(Min.x + Width * 0.58f, CenterY + 3.0f), Color, 1.0f);
			if (bActive != FALSE)
			{
				const FLOAT MaxFirst = EMath::Max(InMinFirstHeight
					, InTotalHeight - InMinSecondHeight - GetCost());
				const FLOAT NewHeight = EMath::Clamp(InOutFirstHeight + ImGui::GetIO().MouseDelta.y
					, InMinFirstHeight, MaxFirst);
				if (NewHeight != InOutFirstHeight)
				{
					InOutFirstHeight = NewHeight;
					return TRUE;
				}
			}
			return FALSE;
		}

		/*
		* Same drag handle, but adjusts the height of the area below it instead. Used for a
		* bottom dock such as Source/Diagnostics while the content above keeps the rest.
		*/
		static BOOL32 HorizontalInverted(const CHAR* InId, FLOAT& InOutSecondHeight
			, FLOAT InMinSecondHeight, FLOAT InMinFirstHeight, FLOAT InTotalHeight)
		{
			const FLOAT Width = EMath::Max(ImGui::GetContentRegionAvail().x, 120.0f);
			ImGui::Button(InId, ImVec2(Width, Thickness));
			const BOOL32 bHovered = ImGui::IsItemHovered();
			const BOOL32 bActive = ImGui::IsItemActive();
			if ((bHovered != FALSE) || (bActive != FALSE))
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
			}
			const ImVec2 Min = ImGui::GetItemRectMin();
			const ImVec2 Max = ImGui::GetItemRectMax();
			const ImU32 Color = (bActive != FALSE)
				? IM_COL32(245, 210, 90, 255) : ((bHovered != FALSE)
					? IM_COL32(180, 185, 200, 255) : IM_COL32(70, 72, 80, 255));
			const FLOAT CenterY = (Min.y + Max.y) * 0.5f;
			ImGui::GetWindowDrawList()->AddLine(ImVec2(Min.x, CenterY)
				, ImVec2(Max.x, CenterY), Color, 2.0f);
			ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(Min.x + Width * 0.42f, CenterY - 3.0f)
				, ImVec2(Min.x + Width * 0.58f, CenterY + 3.0f), Color, 1.0f);
			if (bActive != FALSE)
			{
				const FLOAT MaxSecond = EMath::Max(InMinSecondHeight
					, InTotalHeight - InMinFirstHeight - GetCost());
				const FLOAT NewHeight = EMath::Clamp(InOutSecondHeight - ImGui::GetIO().MouseDelta.y
					, InMinSecondHeight, MaxSecond);
				if (NewHeight != InOutSecondHeight)
				{
					InOutSecondHeight = NewHeight;
					return TRUE;
				}
			}
			return FALSE;
		}
	};

	/*
	* ImGui multiline text buffer that grows on demand. The old editors used a fixed 8-24 KB
	* headroom and silently truncated pasted JSON once it was exhausted.
	*/
	class FGrowableTextBuffer
	{
	public:
		FGrowableTextBuffer()
		{
			SetText(EString());
		}
		void SetText(const EString& InText)
		{
			TextLength = InText.Length();
			const INT32 Capacity = TextLength + 4096 + 1;
			Buffer.SetNum(Capacity);
			::memset(Buffer.GetData(), 0, static_cast<size_t>(Capacity));
			if (TextLength > 0u)
			{
				::memcpy_s(Buffer.GetData(), static_cast<rsize_t>(Capacity), *InText, TextLength);
			}
		}
		PE_NODISCARD EString ToString()const
		{
			return EString(Buffer.GetData());
		}
		PE_NODISCARD CHAR* GetData()
		{
			return Buffer.GetData();
		}
		PE_NODISCARD const CHAR* GetData()const
		{
			return Buffer.GetData();
		}
		PE_NODISCARD INT32 GetCapacity()const
		{
			return Buffer.Num();
		}
		PE_NODISCARD INT32 GetTextLength()const
		{
			return TextLength;
		}
		void Resize(INT32 InTextLength)
		{
			TextLength = InTextLength;
			const INT32 RequiredCapacity = TextLength + 1;
			if (Buffer.Num() < RequiredCapacity)
			{
				Buffer.SetNum(TextLength + 4096 + 1);
			}
			else if (Buffer.Num() > 0)
			{
				Buffer[TextLength] = static_cast<CHAR>(0);
			}
		}
		BOOL32 InputText(const CHAR* InLabel, ImGuiInputTextFlags InFlags = ImGuiInputTextFlags_None)
		{
			if (Buffer.Num() <= 0)
			{
				SetText(EString());
			}
			const bool bChanged = ImGui::InputText(InLabel, Buffer.GetData()
				, static_cast<size_t>(Buffer.Num())
				, InFlags | ImGuiInputTextFlags_CallbackResize, &FGrowableTextBuffer::ResizeCallback, this);
			if (bChanged)
			{
				TextLength = static_cast<INT32>(::strlen(Buffer.GetData()));
			}
			return (bChanged ? TRUE : FALSE);
		}
		BOOL32 InputTextMultiline(const CHAR* InLabel, const ImVec2& InSize
			, ImGuiInputTextFlags InFlags = ImGuiInputTextFlags_None)
		{
			if (Buffer.Num() <= 0)
			{
				SetText(EString());
			}
			const bool bChanged = ImGui::InputTextMultiline(InLabel, Buffer.GetData()
				, static_cast<size_t>(Buffer.Num()), InSize
				, InFlags | ImGuiInputTextFlags_CallbackResize, &FGrowableTextBuffer::ResizeCallback, this);
			if (bChanged)
			{
				TextLength = static_cast<INT32>(::strlen(Buffer.GetData()));
			}
			return (bChanged ? TRUE : FALSE);
		}
	private:
		static int ResizeCallback(ImGuiInputTextCallbackData* InData)
		{
			if (InData->EventFlag == ImGuiInputTextFlags_CallbackResize)
			{
				FGrowableTextBuffer* TextBuffer = static_cast<FGrowableTextBuffer*>(InData->UserData);
				if (TextBuffer)
				{
					TextBuffer->Resize(InData->BufTextLen);
					InData->Buf = TextBuffer->GetData();
				}
			}
			return 0;
		}
	private:
		TArray<CHAR> Buffer;
		INT32 TextLength = 0;
	};

}

#endif
