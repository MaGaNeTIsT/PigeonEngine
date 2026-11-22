#include <Editor/Manager/EditorMenuRegistry.h>

#if ENGINE_EDITOR_ONLY

#include <Development/EditorLogManager.h>
#include <imgui.h>

namespace PigeonEngine
{
	namespace
	{
		struct FEditorMenuItem
		{
			EString				Root;
			EString				Group;
			EString				DefaultLabel;
			FEditorMenuItemDesc	Desc;
		};

		// Heap nodes keep each descriptor at a stable address while the registry grows;
		// Clear deletes them. The menu callbacks outlive the registration pass, so nothing
		// here may point into a relocating container.
		TArray<FEditorMenuItem*>	Items;
		TArray<EString>			RootOrder;

		BOOL32 SplitPath(const EString& InPath, TArray<EString>& OutSegments)
		{
			OutSegments.Empty();
			if (InPath.Length() == 0u)
			{
				return FALSE;
			}
			EString Segment;
			for (UINT32 Index = 0u; Index < InPath.Length(); Index++)
			{
				const CHAR Char = InPath[Index];
				if (Char == '/')
				{
					if (Segment.Length() == 0u)
					{
						return FALSE;
					}
					OutSegments.Add(Segment);
					Segment = EString();
				}
				else
				{
					Segment += EString::FromChar(Char);
				}
			}
			if (Segment.Length() == 0u)
			{
				return FALSE;
			}
			OutSegments.Add(Segment);
			return TRUE;
		}

		BOOL32 IsItemVisible(const FEditorMenuItemDesc& InDesc)
		{
			return InDesc.IsVisible ? InDesc.IsVisible() : TRUE;
		}

		void DrawItem(const FEditorMenuItem& InItem, const INT32 InIndex)
		{
			const FEditorMenuItemDesc& Desc = InItem.Desc;

			EString Label;
			if (Desc.GetLabel)
			{
				Label = Desc.GetLabel();
			}
			if (Label.Length() == 0u)
			{
				Label = InItem.DefaultLabel;
			}
			const CHAR* Shortcut = (Desc.Shortcut.Length() > 0u) ? *Desc.Shortcut : nullptr;
			const bool bEnabled = Desc.IsEnabled ? (Desc.IsEnabled() != FALSE) : true;

			ImGui::PushID(InIndex);
			if (Desc.bStatusOnly != FALSE)
			{
				// Same look as the existing Build status rows.
				ImGui::MenuItem(*Label, Shortcut, false, false);
			}
			else if (Desc.IsChecked)
			{
				bool bChecked = (Desc.IsChecked() != FALSE) ? true : false;
				if (ImGui::MenuItem(*Label, Shortcut, &bChecked, bEnabled))
				{
					if (Desc.Execute)
					{
						Desc.Execute();
					}
				}
			}
			else
			{
				if (ImGui::MenuItem(*Label, Shortcut, false, bEnabled))
				{
					if (Desc.Execute)
					{
						Desc.Execute();
					}
				}
			}
			ImGui::PopID();
		}

		void DrawRoot(const EString& InRoot)
		{
			EString CurrentGroup;
			BOOL32 bGroupHeaderPending = FALSE;
			for (INT32 Index = 0, ItemNum = Items.Num<INT32>(); Index < ItemNum; Index++)
			{
				const FEditorMenuItem& Item = *Items[Index];
				if (Item.Root != InRoot)
				{
					continue;
				}
				if (IsItemVisible(Item.Desc) == FALSE)
				{
					continue;
				}

				if (Item.Group != CurrentGroup)
				{
					CurrentGroup = Item.Group;
					bGroupHeaderPending = (CurrentGroup.Length() > 0u) ? TRUE : FALSE;
				}
				if (bGroupHeaderPending != FALSE)
				{
					// A section label, not a nested menu: every entry stays one click away.
					ImGui::SeparatorText(*CurrentGroup);
					bGroupHeaderPending = FALSE;
				}

				DrawItem(Item, Index);
			}
		}
	}

	namespace EEditorMenuRegistry
	{
		void RegisterItem(const FEditorMenuItemDesc& InItem)
		{
			TArray<EString> Segments;
			if (SplitPath(InItem.Path, Segments) == FALSE)
			{
				PE_LOG_ERROR(EString("EditorMenuRegistry: invalid menu path: ") + InItem.Path);
				return;
			}
			if (Segments.Num<INT32>() < 2)
			{
				PE_LOG_ERROR(EString("EditorMenuRegistry: a menu path needs a root and an item: ") + InItem.Path);
				return;
			}

			FEditorMenuItem* Item = new FEditorMenuItem();
			Item->Root = Segments[0];
			Item->DefaultLabel = Segments[Segments.Num<INT32>() - 1];
			for (INT32 Index = 1, GroupEnd = Segments.Num<INT32>() - 1; Index < GroupEnd; Index++)
			{
				if (Item->Group.Length() > 0u)
				{
					Item->Group += EString(" / ");
				}
				Item->Group += Segments[Index];
			}
			Item->Desc = InItem;
			Items.Add(Item);

			if (RootOrder.Find(Item->Root) == static_cast<UINT32>(-1))
			{
				RootOrder.Add(Item->Root);
			}
		}

		void DrawMainMenuBar()
		{
			if (!ImGui::BeginMainMenuBar())
			{
				return;
			}
			for (INT32 Index = 0, RootNum = RootOrder.Num<INT32>(); Index < RootNum; Index++)
			{
				const EString& Root = RootOrder[Index];
				if (ImGui::BeginMenu(*Root))
				{
					DrawRoot(Root);
					ImGui::EndMenu();
				}
			}
			ImGui::EndMainMenuBar();
		}

		void Clear()
		{
			for (INT32 Index = 0, ItemNum = Items.Num<INT32>(); Index < ItemNum; Index++)
			{
				delete Items[Index];
			}
			Items.Empty();
			RootOrder.Empty();
		}
	}
}

#endif
