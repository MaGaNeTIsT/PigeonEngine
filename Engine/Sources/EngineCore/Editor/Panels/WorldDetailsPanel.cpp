#include <Editor/Panels/WorldDetailsPanel.h>

#if ENGINE_EDITOR_ONLY

#include <imgui.h>

#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/ActorComponent.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/World/World.h>
#include <Editor/Customization/EditorCustomization.h>
#include <Editor/Panels/PropertyEditor.h>
#include <Editor/Commands/WorldEditorCommand.h>
#include <Editor/Manager/EditorWorldContext.h>
#include <Editor/Panels/WorldOutlinePanel.h>

namespace PigeonEngine
{
	namespace
	{
		// PActor extras: the scene component tree and the non-scene component list. The name
		// row and the reflection grid are host-level and drawn before this.
		void DrawDefaultActorExtras(PActor* Actor, PWorld* World)
		{
			if (Actor->GetRootComponent())
			{
				BOOL8 bTreeNodeExpand = ImGui::TreeNodeEx("Components", ImGuiTreeNodeFlags_DefaultOpen);
				// Dropping a component on the header re-homes it directly under this actor's root.
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("PSceneComponentDrag"))
					{
						PSceneComponent* DraggedComponent = ((Payload->Data) && (Payload->DataSize == sizeof(PSceneComponent*))) ? (*static_cast<PSceneComponent* const*>(Payload->Data)) : nullptr;
						FWorldEditorCommandStack* Commands = World ? EEditorWorldContext::GetEditorCommandStack(World) : nullptr;
						if (DraggedComponent && Commands)
						{
							PushReparentComponentCommand(*Commands, *World, DraggedComponent, Actor->GetRootComponent());
						}
					}
					ImGui::EndDragDropTarget();
				}
				if (bTreeNodeExpand)
				{
					EWorldOutlinePanel::GenerateComponentTree(Actor->GetRootComponent(), World);
					ImGui::TreePop();
				}
			}

			// Non-scene components (physics, navigation, input, ...) are not part of the scene
			// component tree, so they get their own list. The insertion order is the actor's
			// component order.
			ImGui::Separator();
			ImGui::TextUnformatted("Other Components");
			const TArray<PActorComponent*> AllComponents = Actor->GetAllComponents();
			for (UINT32 Index = 0u, Num = AllComponents.Num<UINT32>(); Index < Num; Index++)
			{
				PActorComponent* Component = AllComponents[Index];
				if ((!Component) || (dynamic_cast<PSceneComponent*>(Component) != nullptr))
				{
					continue;
				}
				ImGui::PushID(Component);
				const BOOL32 bSelected = (World && (Component == EEditorWorldContext::GetSelectedComponent(World))) ? TRUE : FALSE;
				if (ImGui::Selectable(*POBJ_DEBUGNAME_GET(Component), bSelected))
				{
					if (World)
					{
						EEditorWorldContext::SetSelectedComponent(World, Component);
					}
				}
				ImGui::PopID();
			}
		}

		void DrawSceneComponentExtras(PActorComponent* InComponent, PWorld* /*InWorld*/)
		{
			PSceneComponent* Component = dynamic_cast<PSceneComponent*>(InComponent);
			if (!Component)
			{
				return;
			}
			if (ImGui::Button("Reset Transform"))
			{
				FPropertyValue Before;
				Before.Type = EPropertyValueType::TRANSFORM;
				Before.TransformValue = Component->GetTransform();
				Component->SetComponentTransform(ETransform());
				FPropertyValue After;
				After.Type = EPropertyValueType::TRANSFORM;
				After.TransformValue = Component->GetTransform();
				EPropertyEditor::NotifyPropertyChanged(Component, GetClassHashCode(Component), EString(PE_TEXT("Transform")), Before, After);
			}
		}

	}

	namespace EWorldDetailsPanel
	{
		void RegisterBuiltinCustomEditors()
		{
			EEditorCustomization::RegisterActorCustomEditor<PActor>(DrawDefaultActorExtras);
			EEditorCustomization::RegisterComponentCustomEditor<PSceneComponent>(DrawSceneComponentExtras);
		}

		void GenerateDetails(PWorld* InWorld)
		{
			if (!InWorld)
			{
				return;
			}
			PActor* SelectedActor = EEditorWorldContext::GetSelectedActor(InWorld);
			if (!SelectedActor)
			{
				return;
			}

			// Host frame: the actor's name row, then the grid plus every per-class editor on
			// the actor chain, then the selected component's details.
			CHAR NameBuffer[256] = {};
			{
				const EString& ActorDebugName = POBJ_DEBUGNAME_GET(SelectedActor);
				const UINT32 NameLength = EMath::Min(ActorDebugName.Length(), static_cast<UINT32>(sizeof(NameBuffer) - 1u));
				if (NameLength > 0u)
				{
					memcpy(NameBuffer, *ActorDebugName, NameLength);
				}
			}
			if (ImGui::InputText("##ActorName", NameBuffer, sizeof(NameBuffer)))
			{
				POBJ_DEBUGNAME_SET(SelectedActor, EString(NameBuffer));
			}
			EEditorCustomization::DrawActorDetails(SelectedActor, InWorld);

			PActorComponent* SelectedComponent = EEditorWorldContext::GetSelectedComponent(InWorld);
			if (!SelectedComponent)
			{
				return;
			}
			EEditorCustomization::DrawComponentDetails(SelectedComponent, InWorld);

			FWorldEditorCommandStack* Commands = EEditorWorldContext::GetEditorCommandStack(InWorld);
			if (Commands &&
				(SelectedComponent != SelectedActor->GetRootComponent()) &&
				(SelectedActor->GetWorld() == InWorld))
			{
				ImGui::Separator();
				if (ImGui::Button("Remove Component"))
				{
					PushDeleteComponentCommand(*Commands, *InWorld, SelectedComponent);
				}
			}
		}
	}
}

#endif
