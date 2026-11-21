#include <Editor/Panels/WorldDetailsPanel.h>

#if ENGINE_EDITOR_ONLY

#include <imgui.h>

#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/ActorComponent.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/Component/Navigation/NavigationComponent.h>
#include <PigeonBase/Object/Component/Navigation/NavMeshComponent.h>
#include <PigeonBase/Object/World/World.h>
#include <Navigation/NavigationManager.h>
#include <Editor/Customization/EditorCustomization.h>
#include <Editor/Panels/PropertyEditor.h>
#include <Editor/Commands/WorldEditorCommand.h>
#include <Editor/Manager/EditorWorldContext.h>
#include <Editor/Panels/WorldOutlinePanel.h>

namespace PigeonEngine
{
	namespace
	{
		// The bake panel's asset-name input. One buffer shared across components, like the
		// outline's rename buffer.
		CHAR g_NavMeshAssetNameBuffer[128] = {};

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

		void DrawNavigationComponentExtras(PActorComponent* InComponent, PWorld* /*InWorld*/)
		{
			PNavigationComponent* Component = dynamic_cast<PNavigationComponent*>(InComponent);
			if (!Component)
			{
				return;
			}

			if (!ImGui::TreeNodeEx("Navigation", ImGuiTreeNodeFlags_DefaultOpen))
			{
				return;
			}

			ImGui::Text("Backend: %s", *Component->GetProviderName().ToString());
			bool bShowNavigation = (Component->IsDebugDrawEnabled() != FALSE);
			if (ImGui::Checkbox("Include In Overlay", &bShowNavigation))
			{
				// Through the setter: it forwards the flag to the manager, so the overlay drops
				// this provider's lines in the same frame instead of waiting for the next build.
				Component->SetDebugDrawEnabled((bShowNavigation ? TRUE : FALSE));
			}

			const ENavigationBuildStats& LastBuildStats = Component->GetLastBuildStats();
			if (LastBuildStats.PolygonCount > 0u)
			{
				ImGui::Text("Polygons: %u  Tiles: %u", LastBuildStats.PolygonCount, LastBuildStats.TileCount);
				if ((LastBuildStats.TriangleCount > 0u) || (LastBuildStats.Milliseconds > 0.0f))
				{
					ImGui::Text("Triangles: %u (%u walkable)", LastBuildStats.TriangleCount, LastBuildStats.WalkableTriangleCount);
					if (LastBuildStats.LayerCount > 0u)
					{
						ImGui::SameLine();
						ImGui::TextDisabled("Layers: %u", LastBuildStats.LayerCount);
					}
					ImGui::Text("Bake: %.2f ms", LastBuildStats.Milliseconds);
				}
				else
				{
					ImGui::TextDisabled("Imported from navmesh asset.");
				}
				if (LastBuildStats.NavMeshBytes > 0u)
				{
					// Tile bytes for a bake, the asset blob for an import.
					ImGui::Text("Navmesh: %.1f KB", static_cast<FLOAT>(LastBuildStats.NavMeshBytes) / 1024.0f);
				}
			}
			else
			{
				ImGui::TextDisabled("Not built yet.");
			}
			if (LastBuildStats.SkippedMeshCount > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Skipped meshes: %u", LastBuildStats.SkippedMeshCount);
			}

			ImGui::TreePop();
		}

		void DrawNavMeshComponentExtras(PActorComponent* InComponent, PWorld* InWorld)
		{
			PNavMeshComponent* Component = dynamic_cast<PNavMeshComponent*>(InComponent);
			if (!Component)
			{
				return;
			}

			if (!ImGui::TreeNodeEx("NavMesh Profile", ImGuiTreeNodeFlags_DefaultOpen))
			{
				return;
			}

			const EString& ProfileName = Component->GetNavMeshName();
			ImGui::Text("Profile: %s%s", (ProfileName.Length() > 0u) ? (*ProfileName) : "Default"
				, (ProfileName.Length() > 0u) ? "" : " (all agents without a profile name)");
			ImGui::TextDisabled("Build settings are editable in the Details grid above.");
			ImGui::TextDisabled("Coverage comes from PNavMeshBoundsComponent volumes that support this profile.");

			const ENavMeshAsset* LoadedAsset = Component->GetNavMeshAsset();
			const EString& AssetName = Component->GetNavMeshAssetName();
			if (LoadedAsset && (LoadedAsset->GetAssetName() == AssetName))
			{
				ImGui::TextDisabled("Asset loaded: %s", *AssetName);
			}
			else if (Component->HasNavMeshAssetFailedToLoad())
			{
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Asset '%s' failed to load; the level build bakes this profile from the world.", *AssetName);
			}
			else if (AssetName.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "Loading asset '%s'...", *AssetName);
			}
			else
			{
				ImGui::TextDisabled("No navmesh asset assigned; the level build bakes this profile from the world.");
			}
			ImGui::Separator();

			// One pass builds every profile of the level; this panel only reports this profile's
			// slice of the last pass and offers the same entry point as the Build menu.
			ENavigationManager* Manager = ENavigationManager::GetManagerSingleton();
			if (InWorld && (Manager->IsWorldBuildRunning(*InWorld) != FALSE))
			{
				ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "Building all navmesh profiles of the level...");
			}
			else if (InWorld)
			{
				const ENavigationManager::FNavigationWorldBuildReport& Report = Manager->GetWorldBuildReport(*InWorld);
				const ENavigationManager::FNavigationProfileBuildResult* ProfileResult = nullptr;
				for (UINT32 Index = 0u, Num = Report.Profiles.Num<UINT32>(); Index < Num; Index++)
				{
					if (Report.Profiles[Index].NavMeshName == ProfileName)
					{
						ProfileResult = &Report.Profiles[Index];
						break;
					}
				}
				if (!Report.bHasRun)
				{
					ImGui::TextDisabled("The level has not been built yet.");
				}
				else if (!ProfileResult)
				{
					ImGui::TextDisabled("The last level build did not include this profile.");
				}
				else if (ProfileResult->bSucceeded)
				{
					ImGui::Text("Last build: %u polygons, %.2f ms", ProfileResult->Stats.PolygonCount, ProfileResult->Stats.Milliseconds);
				}
				else
				{
					ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Last build failed: %s", *(ProfileResult->Error));
				}
				if (Report.bHasRun && (Report.bAllSucceeded == FALSE))
				{
					ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.2f, 1.0f), "Some profiles of this level failed; see the log.");
				}
			}
			if (ImGui::Button("Build Navigation (All Profiles)"))
			{
				if (InWorld)
				{
					Manager->RequestWorldBuild(*InWorld, ENavigationManager::ENavigationWorldBuildReason::EDITOR);
				}
			}
			ImGui::SameLine();
			const BOOL32 bCanReload = (AssetName.Length() > 0u) ? TRUE : FALSE;
			if (!bCanReload)
			{
				ImGui::BeginDisabled();
			}
			if (ImGui::Button("Reload Asset"))
			{
				// The method records its own failure into the last-asset error state.
				EString ReloadError;
				Component->ReloadNavMeshAsset(ReloadError);
			}
			if (!bCanReload)
			{
				ImGui::EndDisabled();
			}

			ImGui::InputText("New Asset Name", g_NavMeshAssetNameBuffer, sizeof(g_NavMeshAssetNameBuffer));
			const BOOL32 bCanSave = (g_NavMeshAssetNameBuffer[0] != '\0') ? TRUE : FALSE;
			if (!bCanSave)
			{
				ImGui::BeginDisabled();
			}
			if (ImGui::Button("Bake & Save This Profile"))
			{
				// Manual authoring shortcut; the level-wide path is Build Navigation / Save.
				EString SaveError;
				Component->SaveBakeAsAsset(EString(g_NavMeshAssetNameBuffer), SaveError);
			}
			if (!bCanSave)
			{
				ImGui::EndDisabled();
			}
			if (Component->GetLastNavMeshAssetError().Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", *(Component->GetLastNavMeshAssetError()));
			}

			ImGui::TreePop();
		}
	}

	namespace EWorldDetailsPanel
	{
		void RegisterBuiltinCustomEditors()
		{
			EEditorCustomization::RegisterActorCustomEditor<PActor>(DrawDefaultActorExtras);
			EEditorCustomization::RegisterComponentCustomEditor<PSceneComponent>(DrawSceneComponentExtras);
			EEditorCustomization::RegisterComponentCustomEditor<PNavigationComponent>(DrawNavigationComponentExtras);
			EEditorCustomization::RegisterComponentCustomEditor<PNavMeshComponent>(DrawNavMeshComponentExtras);
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
