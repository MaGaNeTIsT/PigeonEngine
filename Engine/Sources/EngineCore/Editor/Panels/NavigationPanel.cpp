#include <Editor/Panels/NavigationPanel.h>

#if ENGINE_EDITOR_ONLY

#include <imgui.h>

#include <PigeonBase/Object/Actor/NavMeshActor.h>
#include <PigeonBase/Object/World/World.h>
#include <Navigation/NavigationManager.h>
#include <Editor/Commands/WorldEditorCommand.h>
#include <Editor/Manager/EditorWorldContext.h>

namespace PigeonEngine
{
	namespace
	{
		EString MakeAgentLabel(const EString& InAgentName)
		{
			return (InAgentName.Length() > 0u) ? InAgentName : EString(PE_TEXT("Default"));
		}

		const ENavigationManager::FNavigationProfileBuildResult* FindAgentResult(
			const ENavigationManager::FNavigationWorldBuildReport& InReport, const EString& InAgentName)
		{
			for (UINT32 Index = 0u, Num = InReport.Profiles.Num<UINT32>(); Index < Num; Index++)
			{
				if (InReport.Profiles[Index].ProfileName == InAgentName)
				{
					return &(InReport.Profiles[Index]);
				}
			}
			return nullptr;
		}

		// The build pass publishes the earliest actor per agent name; the panel marks the others
		// so a duplicate is visible where it is authored instead of only in the log.
		UINT32 CountAgentActors(const TArray<PNavMeshActor*>& InActors, const EString& InAgentName)
		{
			UINT32 Count = 0u;
			for (UINT32 Index = 0u, Num = InActors.Num<UINT32>(); Index < Num; Index++)
			{
				if (InActors[Index]->GetAgentName() == InAgentName)
				{
					Count++;
				}
			}
			return Count;
		}

		BOOL32 IsPrimaryActor(const TArray<PNavMeshActor*>& InActors, const PNavMeshActor& InActor)
		{
			const ObjectIdentityType ActorId = InActor.GetUniqueID();
			for (UINT32 Index = 0u, Num = InActors.Num<UINT32>(); Index < Num; Index++)
			{
				const PNavMeshActor* Other = InActors[Index];
				if ((Other != (&InActor)) && (Other->GetAgentName() == InActor.GetAgentName())
					&& (Other->GetUniqueID() < ActorId))
				{
					return FALSE;
				}
			}
			return TRUE;
		}
	}

	namespace EWorldNavigationPanel
	{
		void GenerateNavigation(PWorld* InWorld)
		{
			if (!InWorld)
			{
				ImGui::TextDisabled("No level is open.");
				return;
			}

			ENavigationManager* Manager = ENavigationManager::GetManagerSingleton();
			FWorldEditorCommandStack* Commands = EEditorWorldContext::GetEditorCommandStack(InWorld);
			const BOOL32 bBuilding = Manager->IsWorldBuildRunning(*InWorld);

			TArray<PNavMeshActor*> Actors;
			const TArray<PActor*> AllActors = InWorld->GetAllActors();
			for (UINT32 Index = 0u, Num = AllActors.Num<UINT32>(); Index < Num; Index++)
			{
				if (PNavMeshActor* Actor = dynamic_cast<PNavMeshActor*>(AllActors[Index]))
				{
					Actors.Add(Actor);
				}
			}

			// ---- Level actions ----
			const BOOL32 bCanBuild = ((Actors.Num<UINT32>() > 0u) && (!bBuilding)) ? TRUE : FALSE;
			if (!bCanBuild)
			{
				ImGui::BeginDisabled();
			}
			if (ImGui::Button("Build Navigation"))
			{
				Manager->RequestWorldBuild(*InWorld, ENavigationManager::ENavigationWorldBuildReason::EDITOR);
			}
			if (!bCanBuild)
			{
				ImGui::EndDisabled();
			}
			ImGui::SameLine();
			if (!Commands)
			{
				ImGui::BeginDisabled();
			}
			if (ImGui::Button("Create NavMesh Actor"))
			{
				PNavMeshActor* NewActor = new PNavMeshActor();
				POBJ_DEBUGNAME_SET(NewActor, "NavMeshActor");
				InWorld->AddActor(NewActor);
				EEditorWorldContext::SetSelectedActor(InWorld, NewActor);
				PushSpawnActorCommand(*Commands, *InWorld, NewActor);
			}
			if (!Commands)
			{
				ImGui::EndDisabled();
			}

			if (bBuilding)
			{
				ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "Building every agent of the level...");
			}
			else
			{
				const ENavigationManager::FNavigationWorldBuildReport& Report = Manager->GetWorldBuildReport(*InWorld);
				if (Report.bHasRun)
				{
					UINT32 Succeeded = 0u;
					for (UINT32 Index = 0u, Num = Report.Profiles.Num<UINT32>(); Index < Num; Index++)
					{
						if (Report.Profiles[Index].bSucceeded)
						{
							Succeeded++;
						}
					}
					const EString Label = EString(PE_TEXT("Last build: ")) + EString::FromInt(Succeeded)
						+ EString(PE_TEXT("/")) + EString::FromInt(Report.Profiles.Num<UINT32>()) + EString(PE_TEXT(" agents"));
					ImGui::TextColored(((Report.bAllSucceeded != FALSE) ? ImVec4(0.4f, 0.8f, 0.4f, 1.0f) : ImVec4(0.9f, 0.6f, 0.2f, 1.0f)), "%s", *Label);
				}
				else
				{
					ImGui::TextDisabled("The level has not been built yet.");
				}
			}

			ImGui::Separator();
			if (Actors.Num<UINT32>() == 0u)
			{
				ImGui::TextDisabled("This level has no navigation.");
				ImGui::TextDisabled("Create a NavMesh Actor for each agent size, then build.");
				return;
			}

			const ENavigationManager::FNavigationWorldBuildReport& Report = Manager->GetWorldBuildReport(*InWorld);
			if (ImGui::BeginTable("NavMeshAgents", 4,
				ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
			{
				ImGui::TableSetupColumn("Agent");
				ImGui::TableSetupColumn("Radius / Height");
				ImGui::TableSetupColumn("Asset");
				ImGui::TableSetupColumn("Last build");
				ImGui::TableHeadersRow();

				for (UINT32 Index = 0u, Num = Actors.Num<UINT32>(); Index < Num; Index++)
				{
					PNavMeshActor* Actor = Actors[Index];
					const FNavMeshProfile& Profile = Actor->GetProfile();
					const BOOL32 bDuplicate = (CountAgentActors(Actors, Profile.Name) > 1u) ? TRUE : FALSE;
					const BOOL32 bPrimary = IsPrimaryActor(Actors, *Actor);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					if (bDuplicate && (!bPrimary))
					{
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
					}
					const EString Label = MakeAgentLabel(Profile.Name);
					if (ImGui::Selectable(*Label, false, ImGuiSelectableFlags_SpanAllColumns))
					{
						EEditorWorldContext::SetSelectedActor(InWorld, Actor);
					}
					if (bDuplicate && (!bPrimary))
					{
						ImGui::PopStyleColor();
					}

					ImGui::TableNextColumn();
					ImGui::Text("%.0f / %.0f", Profile.BuildSettings.AgentRadius, Profile.BuildSettings.AgentHeight);

					ImGui::TableNextColumn();
					if (Profile.NavMeshAsset.Length() > 0u)
					{
						ImGui::TextUnformatted(*Profile.NavMeshAsset);
					}
					else
					{
						ImGui::TextDisabled("(baked on save)");
					}

					ImGui::TableNextColumn();
					const ENavigationManager::FNavigationProfileBuildResult* Result = FindAgentResult(Report, Profile.Name);
					if (bDuplicate && (!bPrimary))
					{
						ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Duplicate: earliest actor is used");
					}
					else if (!Report.bHasRun)
					{
						ImGui::TextDisabled("Not built");
					}
					else if (!Result)
					{
						ImGui::TextDisabled("Not in the last build");
					}
					else if (Result->bSucceeded)
					{
						ImGui::Text("OK: %u polygons", Result->Stats.PolygonCount);
					}
					else
					{
						ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", *Result->Error);
					}
				}
				ImGui::EndTable();
			}
		}
	}
}

#endif
