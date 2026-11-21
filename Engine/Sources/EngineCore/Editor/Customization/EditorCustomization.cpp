#include <Editor/Customization/EditorCustomization.h>

#if ENGINE_EDITOR_ONLY

#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/ActorComponent.h>
#include <Editor/Panels/PropertyEditor.h>

namespace PigeonEngine
{
	namespace
	{
		TMap<size_t, EEditorCustomization::FDrawActorDetails>& GetActorEditorMap()
		{
			static TMap<size_t, EEditorCustomization::FDrawActorDetails> Map;
			return Map;
		}

		TMap<size_t, EEditorCustomization::FDrawComponentDetails>& GetComponentEditorMap()
		{
			static TMap<size_t, EEditorCustomization::FDrawComponentDetails> Map;
			return Map;
		}

		// Root-first class chain: the leaf appends first, so the array is reversed in place
		// to make base registrations draw before derived ones.
		void BuildClassChainRootFirst(const size_t& InClassHash, TArray<size_t>& OutChain)
		{
			for (size_t ClassHash = InClassHash; ClassHash != static_cast<size_t>(-1); ClassHash = GetClassParentHash(ClassHash))
			{
				OutChain.Add(ClassHash);
			}
			for (UINT32 Left = 0u, Right = (OutChain.Num<UINT32>() > 0u) ? (OutChain.Num<UINT32>() - 1u) : 0u; Left < Right; Left++, Right--)
			{
				size_t Temp = OutChain[Left];
				OutChain[Left] = OutChain[Right];
				OutChain[Right] = Temp;
			}
		}
	}

	namespace EEditorCustomization
	{
		void RegisterActorCustomEditor(const size_t& InClassHash, const FDrawActorDetails& InDraw)
		{
			GetActorEditorMap().Add(InClassHash, InDraw);
		}

		void RegisterComponentCustomEditor(const size_t& InClassHash, const FDrawComponentDetails& InDraw)
		{
			GetComponentEditorMap().Add(InClassHash, InDraw);
		}

		BOOL32 DrawActorDetails(PActor* InActor, PWorld* InWorld)
		{
			if ((!InActor) || (!InWorld))
			{
				return FALSE;
			}
			EPropertyEditor::DrawProperties(InActor, GetClassHashCode(InActor));
			TArray<size_t> Chain;
			BuildClassChainRootFirst(GetClassHashCode(InActor), Chain);
			for (UINT32 Index = 0u, Num = Chain.Num<UINT32>(); Index < Num; Index++)
			{
				if (const FDrawActorDetails* Draw = GetActorEditorMap().FindValueAsPtr(Chain[Index]))
				{
					(*Draw)(InActor, InWorld);
				}
			}
			return TRUE;
		}

		BOOL32 DrawComponentDetails(PActorComponent* InComponent, PWorld* InWorld)
		{
			if ((!InComponent) || (!InWorld))
			{
				return FALSE;
			}
			EPropertyEditor::DrawProperties(InComponent, GetClassHashCode(InComponent));
			TArray<size_t> Chain;
			BuildClassChainRootFirst(GetClassHashCode(InComponent), Chain);
			for (UINT32 Index = 0u, Num = Chain.Num<UINT32>(); Index < Num; Index++)
			{
				if (const FDrawComponentDetails* Draw = GetComponentEditorMap().FindValueAsPtr(Chain[Index]))
				{
					(*Draw)(InComponent, InWorld);
				}
			}
			return TRUE;
		}

		UINT32 CountActorEditorsForClass(const size_t& InClassHash)
		{
			UINT32 Count = 0u;
			for (size_t ClassHash = InClassHash; ClassHash != static_cast<size_t>(-1); ClassHash = GetClassParentHash(ClassHash))
			{
				if (GetActorEditorMap().FindValueAsPtr(ClassHash))
				{
					Count += 1u;
				}
			}
			return Count;
		}

		UINT32 CountComponentEditorsForClass(const size_t& InClassHash)
		{
			UINT32 Count = 0u;
			for (size_t ClassHash = InClassHash; ClassHash != static_cast<size_t>(-1); ClassHash = GetClassParentHash(ClassHash))
			{
				if (GetComponentEditorMap().FindValueAsPtr(ClassHash))
				{
					Count += 1u;
				}
			}
			return Count;
		}
	}
}

#endif
