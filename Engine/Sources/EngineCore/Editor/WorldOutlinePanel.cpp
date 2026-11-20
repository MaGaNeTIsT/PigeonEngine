#include <Editor/WorldOutlinePanel.h>

#if ENGINE_EDITOR_ONLY

#include <imgui.h>

#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Actor/LevelActor.h>
#include <PigeonBase/Object/Actor/LevelCharacter.h>
#include <PigeonBase/Object/Controller/Controller.h>
#include <PigeonBase/Object/Component/ActorComponent.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/Component/Character/CharacterInputComponent.h>
#include <PigeonBase/Object/Component/Character/MovementComponent.h>
#include <PigeonBase/Object/World/World.h>
#include <Editor/WorldEditorCommand.h>
#include <Editor/EditorWorldContext.h>

namespace PigeonEngine
{
	namespace
	{
		// Actor rename state: one rename at a time, kept alive between frames.
		PActor* g_RenameActor = nullptr;
		CHAR g_RenameBuffer[256] = {};

		BOOL32 IsClassDerivedFrom(const size_t& InClassHash, const size_t& InBaseHash)
		{
			for (size_t ClassHash = InClassHash; ClassHash != static_cast<size_t>(-1); ClassHash = GetClassParentHash(ClassHash))
			{
				if (ClassHash == InBaseHash)
				{
					return TRUE;
				}
			}
			return FALSE;
		}

		void GenerateActorNode(PActor* Actor, PWorld* World)
		{
			BOOL8 bSelectedActor	= EEditorWorldContext::GetSelectedActor(World) == Actor;
			ImGuiTreeNodeFlags TreeNodeFlag = (Actor->GetAllActorsAttached().Num() > 0 ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf);

			ImGui::PushID((void*)Actor);
			BOOL8 bTreeNodeExpand	= ImGui::TreeNodeEx("##node", TreeNodeFlag);
			ImGui::SameLine();
			BOOL8 bSelected		= ImGui::Selectable(*(POBJ_DEBUGNAME_GET(Actor)), &bSelectedActor);

			// Hierarchy editing: dragging this row carries the actor, dropping a dragged actor
			// here re-parents it below this one (KEEP_WORLD, undoable through the command stack).
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				PActor* DraggedActor = Actor;
				ImGui::TextUnformatted(*(POBJ_DEBUGNAME_GET(Actor)));
				ImGui::SetDragDropPayload("PWorldActorDrag", &DraggedActor, sizeof(PActor*));
				ImGui::EndDragDropSource();
			}
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("PWorldActorDrag"))
				{
					PActor* DraggedActor = ((Payload->Data) && (Payload->DataSize == sizeof(PActor*))) ? (*static_cast<PActor* const*>(Payload->Data)) : nullptr;
					if (DraggedActor && EEditorWorldContext::GetEditorCommandStack(World))
					{
						PushReparentActorCommand(*EEditorWorldContext::GetEditorCommandStack(World), *World, DraggedActor, Actor);
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (ImGui::BeginPopupContextItem("##ActorContext"))
			{
				if (ImGui::MenuItem("Rename"))
				{
					g_RenameActor = Actor;
					const EString& ActorName = POBJ_DEBUGNAME_GET(Actor);
					const UINT32 NameLength = EMath::Min(ActorName.Length(), static_cast<UINT32>(sizeof(g_RenameBuffer) - 1u));
					memset(g_RenameBuffer, 0, sizeof(g_RenameBuffer));
					if (NameLength > 0u)
					{
						memcpy(g_RenameBuffer, *ActorName, NameLength);
					}
				}
				FWorldEditorCommandStack* Commands = EEditorWorldContext::GetEditorCommandStack(World);
				if (Commands && ImGui::MenuItem("Delete"))
				{
					PushDeleteActorCommand(*Commands, *World, Actor);
					ImGui::EndPopup();
					ImGui::PopID();
					return;
				}
				if (Commands && ImGui::BeginMenu("Add Component"))
				{
					std::vector<ERTTIManager::ESerializableClassInfo> ClassInfos;
					ERTTIManager::GetManagerSingleton()->GetSerializableClassInfos(ClassInfos);
					const size_t ComponentBaseHash = GetClassHashCode<PActorComponent>();
					const size_t MovementComponentHash = GetClassHashCode<PMovementComponent>();
					const size_t CharacterInputComponentHash = GetClassHashCode<PCharacterInputComponent>();
					for (UINT32 Index = 0u, Num = static_cast<UINT32>(ClassInfos.size()); Index < Num; Index++)
					{
						const ERTTIManager::ESerializableClassInfo& ClassInfo = ClassInfos[Index];
						if ((!ClassInfo.bHasFactory) ||
							(!IsClassDerivedFrom(ClassInfo.ClassHash, ComponentBaseHash)) ||
							(ClassInfo.ClassHash == MovementComponentHash) ||
							(ClassInfo.ClassHash == CharacterInputComponentHash))
						{
							continue;
						}
						if (ImGui::MenuItem(ClassInfo.StableName.c_str()))
						{
							PActorComponent* NewComponent = CreateClassObjectByStableName<PActorComponent>(ClassInfo.StableName.c_str());
							if (NewComponent)
							{
								POBJ_DEBUGNAME_SET(NewComponent, EString(ClassInfo.StableName.c_str()));
								Actor->AddComponent(NewComponent);
								EEditorWorldContext::SetSelectedComponent(World, NewComponent);
								PushAddComponentCommand(*Commands, *World, Actor, NewComponent);
							}
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}
			ImGui::PopID();

			if (g_RenameActor == Actor)
			{
				ImGui::SetNextItemWidth(220.0f);
				ImGui::InputText("##ActorRename", g_RenameBuffer, sizeof(g_RenameBuffer));
				if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsKeyPressed(ImGuiKey_Enter))
				{
					POBJ_DEBUGNAME_SET(Actor, EString(g_RenameBuffer));
					g_RenameActor = nullptr;
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
				{
					g_RenameActor = nullptr;
				}
			}

			if (bTreeNodeExpand)
			{
				// Snapshot: a context menu inside a child can delete it (and its subtree) while
				// this loop is still running, which mutates the child list.
				const TArray<PActor*> ChildSnapshot = Actor->GetAllActorsAttached();
				for (PActor* Element : ChildSnapshot)
				{
					// Contains only compares pointer values, so it is safe to ask after the child
					// was destroyed and unlinked.
					if (Actor->GetAllActorsAttached().Contains(Element))
					{
						GenerateActorNode(Element, World);
					}
				}
				ImGui::TreePop();
			}

			if (bSelected)
			{
				EEditorWorldContext::SetSelectedActor(World, Actor);
			}
		}

		void GenerateComponentNode(PSceneComponent* Component, PWorld* World)
		{
			BOOL8 bSelectedComponent = EEditorWorldContext::GetSelectedComponent(World) == Component;
			const TArray<PSceneComponent*> Children = Component->GetChildrenComponents();

			const ImGuiTreeNodeFlags TreeNodeFlag = (Children.Num() > 0 ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf) | ImGuiTreeNodeFlags_DefaultOpen;
			ImGui::PushID((void*)Component);
			const BOOL8 bTreeNodeExpand = ImGui::TreeNodeEx("##node", TreeNodeFlag);
			ImGui::SameLine();
			const BOOL8 bSelected = ImGui::Selectable(*(POBJ_DEBUGNAME_GET(Component)), &bSelectedComponent);

			// Hierarchy editing: the drag carries this component, a drop re-parents it below
			// this node. The command helper refuses roots and cross-actor links (child-actor
			// roots render in this tree but are managed by actor attach), so no guard is needed
			// here beyond passing the pair along.
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				PSceneComponent* DraggedComponent = Component;
				ImGui::TextUnformatted(*(POBJ_DEBUGNAME_GET(Component)));
				ImGui::SetDragDropPayload("PSceneComponentDrag", &DraggedComponent, sizeof(PSceneComponent*));
				ImGui::EndDragDropSource();
			}
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("PSceneComponentDrag"))
				{
					PSceneComponent* DraggedComponent = ((Payload->Data) && (Payload->DataSize == sizeof(PSceneComponent*))) ? (*static_cast<PSceneComponent* const*>(Payload->Data)) : nullptr;
					if (DraggedComponent && EEditorWorldContext::GetEditorCommandStack(World))
					{
						PushReparentComponentCommand(*EEditorWorldContext::GetEditorCommandStack(World), *World, DraggedComponent, Component);
					}
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::PopID();

			if (bTreeNodeExpand)
			{
				for (PSceneComponent* Child : Children)
				{
					GenerateComponentNode(Child, World);
				}
				ImGui::TreePop();
			}

			if (bSelected)
			{
				EEditorWorldContext::SetSelectedComponent(World, Component);
			}
		}
	}

	namespace EWorldOutlinePanel
	{
		void GenerateOutline(PWorld* InWorld)
		{
			if (!InWorld)
			{
				return;
			}
			PActor* RootActor = InWorld->GetRootActor();
			if (!RootActor)
			{
				return;
			}
			FWorldEditorCommandStack* Commands = EEditorWorldContext::GetEditorCommandStack(InWorld);

			if (Commands)
			{
				if (ImGui::Button("Create Actor"))
				{
					ImGui::OpenPopup("CreateActorPopup");
				}
				if (ImGui::BeginPopup("CreateActorPopup"))
				{
					std::vector<ERTTIManager::ESerializableClassInfo> ClassInfos;
					ERTTIManager::GetManagerSingleton()->GetSerializableClassInfos(ClassInfos);
					const size_t ActorBaseHash = GetClassHashCode<PActor>();
					const size_t LevelActorHash = GetClassHashCode<PLevelActor>();
					const size_t ControllerBaseHash = GetClassHashCode<PController>();
					const size_t CharacterBaseHash = GetClassHashCode<PCharacter>();
					for (UINT32 Index = 0u, Num = static_cast<UINT32>(ClassInfos.size()); Index < Num; Index++)
					{
						const ERTTIManager::ESerializableClassInfo& ClassInfo = ClassInfos[Index];
						if ((!ClassInfo.bHasFactory) ||
							(!IsClassDerivedFrom(ClassInfo.ClassHash, ActorBaseHash)) ||
							(ClassInfo.ClassHash == LevelActorHash) ||
							IsClassDerivedFrom(ClassInfo.ClassHash, ControllerBaseHash) ||
							IsClassDerivedFrom(ClassInfo.ClassHash, CharacterBaseHash))
						{
							continue;
						}
						if (ImGui::MenuItem(ClassInfo.StableName.c_str()))
						{
							PActor* NewActor = CreateClassObjectByStableName<PActor>(ClassInfo.StableName.c_str());
							if (NewActor)
							{
								POBJ_DEBUGNAME_SET(NewActor, EString(ClassInfo.StableName.c_str()));
								InWorld->AddActor(NewActor);
								EEditorWorldContext::SetSelectedActor(InWorld, NewActor);
								PushSpawnActorCommand(*Commands, *InWorld, NewActor);
							}
						}
					}
					ImGui::EndPopup();
				}
				ImGui::Separator();
			}

			for (PActor* Actor : RootActor->GetAllActorsAttached())
			{
				GenerateActorNode(Actor, InWorld);
			}

			// Detach drop zone: dropping a dragged actor here re-homes it under the world
			// root, i.e. back to the top level. Nothing enters the history when the drop
			// is refused.
			if (Commands)
			{
				ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight()));
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("PWorldActorDrag"))
					{
						PActor* DraggedActor = ((Payload->Data) && (Payload->DataSize == sizeof(PActor*))) ? (*static_cast<PActor* const*>(Payload->Data)) : nullptr;
						if (DraggedActor && (DraggedActor != RootActor))
						{
							PushReparentActorCommand(*Commands, *InWorld, DraggedActor, RootActor);
						}
					}
					ImGui::EndDragDropTarget();
				}
			}
		}

		void GenerateComponentTree(PSceneComponent* InRoot, PWorld* InWorld)
		{
			if (InRoot && InWorld)
			{
				GenerateComponentNode(InRoot, InWorld);
			}
		}
	}
}

#endif
