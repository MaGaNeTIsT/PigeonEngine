#include "AnimationWorkspace.h"

#if ENGINE_EDITOR_ONLY

#include <imgui.h>
#include "ClipDocument.h"
#include "MontageDocument.h"
#include "NotifyDocument.h"
#include "SlotGroupDocument.h"
#include "StateMachineDocument.h"

namespace PigeonEngine
{

	void EAnimationWorkspace::EditorInit()
	{
		m_bOpen = FALSE;
		// Respect the layout stored in ImGuiEngine.ini. Reset Layout is an explicit user
		// action and is the only path that overrides the saved position/size.
		m_bApplyWindowLayout = FALSE;
		m_LastError = EString();
	}

	void EAnimationWorkspace::ShutDown()
	{
		m_Documents.CloseAll();
		m_bOpen = FALSE;
	}

	BOOL32 EAnimationWorkspace::IsOpen()const
	{
		return m_bOpen;
	}

	BOOL32 EAnimationWorkspace::IsAssetOpen(const EString& InAssetFullPath)const
	{
		return ((m_Documents.FindByPath(InAssetFullPath) != nullptr) ? TRUE : FALSE);
	}

	void EAnimationWorkspace::OpenStateMachineAsset(const EString& InAssetFullPath)
	{
		m_bOpen = TRUE;
		EString Error;
		if (!m_Documents.OpenStateMachine(InAssetFullPath, Error))
		{
			m_LastError = Error;
			return;
		}
		m_LastError = EString();
	}

	void EAnimationWorkspace::OpenAnimationAsset(const EString& InAssetFullPath)
	{
		m_bOpen = TRUE;
		EString Error;
		if (!m_Documents.OpenClip(InAssetFullPath, Error))
		{
			m_LastError = Error;
			return;
		}
		m_LastError = EString();
	}

	void EAnimationWorkspace::OpenMontageAsset(const EString& InAssetFullPath)
	{
		m_bOpen = TRUE;
		EString Error;
		if (!m_Documents.OpenMontage(InAssetFullPath, Error))
		{
			m_LastError = Error;
			return;
		}
		m_LastError = EString();
	}

	void EAnimationWorkspace::OpenNotifyAsset(const EString& InAssetFullPath)
	{
		m_bOpen = TRUE;
		EString Error;
		if (!m_Documents.OpenNotify(InAssetFullPath, Error))
		{
			m_LastError = Error;
			return;
		}
		m_LastError = EString();
	}

	void EAnimationWorkspace::OpenSlotGroupAsset(const EString& InAssetFullPath)
	{
		m_bOpen = TRUE;
		EString Error;
		if (!m_Documents.OpenSlotGroups(InAssetFullPath, Error))
		{
			m_LastError = Error;
			return;
		}
		m_LastError = EString();
	}

	void EAnimationWorkspace::EditorUpdate()
	{
		if (m_bOpen == FALSE)
		{
			return;
		}
		DrawWorkspace();
	}

	void EAnimationWorkspace::DrawWorkspace()
	{
		const ImGuiViewport* Viewport = ImGui::GetMainViewport();
		// WorkPos/WorkSize are already in ImGui logical units; DisplaySize is logical too.
		// Dividing by DisplayFramebufferScale again made the workspace half-size on 200% DPI.
		const ImVec2 WorkPosition = Viewport->WorkPos;
		const ImVec2 WorkSize = Viewport->WorkSize;
		const ImVec2 WindowPosition(WorkPosition.x + 10.0f, WorkPosition.y + 10.0f);
		const ImVec2 WindowSize(EMath::Max(480.0f, EMath::Min(900.0f, WorkSize.x - 20.0f))
			, EMath::Max(320.0f, EMath::Min(480.0f, WorkSize.y - 20.0f)));
		ImGui::SetNextWindowSizeConstraints(ImVec2(480.0f, 320.0f), ImVec2(FLT_MAX, FLT_MAX));
		const BOOL32 bApplyLayout = m_bApplyWindowLayout;
		if (bApplyLayout != FALSE)
		{
			// Explicit Reset Layout only. Normal startup uses the saved ImGui window state,
			// and a first-ever window gets this suggestion through ImGuiCond_FirstUseEver.
			ImGui::SetNextWindowSize(WindowSize, ImGuiCond_Always);
			ImGui::SetNextWindowPos(WindowPosition, ImGuiCond_Always);
		}
		else
		{
			ImGui::SetNextWindowSize(WindowSize, ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowPos(WindowPosition, ImGuiCond_FirstUseEver);
		}
		BOOL32 bWindowOpen = TRUE;
		ImGui::Begin("Animation Workspace", &bWindowOpen
			, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking);
		if (bWindowOpen == FALSE)
		{
			m_bOpen = FALSE;
			ImGui::End();
			return;
		}
		if (bApplyLayout != FALSE)
		{
			// Some multi-viewport backends report the window at the platform size on the
			// first Begin even when SetNextWindowSize was requested. Applying it again here
			// makes Reset Layout deterministic; normal startup still uses the saved layout.
			ImGui::SetWindowSize(WindowSize);
			ImGui::SetWindowPos(WindowPosition);
			m_bApplyWindowLayout = FALSE;
		}

		// A saved .ini layout can be larger than the current work area (for example after a
		// DPI change). An over-sized window pushes the Clip editor's bottom Source pane
		// below the screen, which looks like a fixed/missing height. Clamp the outer window
		// to the viewport it currently lives on; the user's smaller position/size is kept.
		const ImGuiViewport* WindowViewport = ImGui::GetWindowViewport();
		const ImVec2 ViewportPosition = (WindowViewport ? WindowViewport->WorkPos : WorkPosition);
		const ImVec2 ViewportSize = (WindowViewport ? WindowViewport->WorkSize : WorkSize);
		const ImVec2 CurrentSize = ImGui::GetWindowSize();
		const ImVec2 ClampedSize(
			EMath::Min(CurrentSize.x, ViewportSize.x),
			EMath::Min(CurrentSize.y, ViewportSize.y));
		if ((ClampedSize.x != CurrentSize.x) || (ClampedSize.y != CurrentSize.y))
		{
			ImGui::SetWindowSize(ClampedSize);
		}
		const ImVec2 CurrentPosition = ImGui::GetWindowPos();
		const ImVec2 MaxPosition(
			EMath::Max(ViewportPosition.x, ViewportPosition.x + ViewportSize.x - ClampedSize.x),
			EMath::Max(ViewportPosition.y, ViewportPosition.y + ViewportSize.y - ClampedSize.y));
		const ImVec2 ClampedPosition(
			EMath::Clamp(CurrentPosition.x, ViewportPosition.x, MaxPosition.x),
			EMath::Clamp(CurrentPosition.y, ViewportPosition.y, MaxPosition.y));
		if ((ClampedPosition.x != CurrentPosition.x) || (ClampedPosition.y != CurrentPosition.y))
		{
			ImGui::SetWindowPos(ClampedPosition);
		}

		FAnimationDocument* ActiveDocument = m_Documents.GetActive();
		auto ApplyOrSaveActive = [this, &ActiveDocument](const BOOL32 bApply)
		{
			if (!ActiveDocument)
			{
				return;
			}
			EString Error;
			const BOOL32 bSucceeded = ((bApply != FALSE)
				? ActiveDocument->Apply(Error) : ActiveDocument->Save(Error));
			m_LastError = ((bSucceeded != FALSE) ? EString() : Error);
		};

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				FAnimationDocument* Active = m_Documents.GetActive();
				if (ImGui::MenuItem("Close Tab", nullptr, false, (Active != nullptr)))
				{
					Active->bClosePending = TRUE;
				}
				if (ImGui::MenuItem("Save", "Ctrl+S", false, (ActiveDocument != nullptr)))
				{
					ApplyOrSaveActive(FALSE);
				}
				if (ImGui::MenuItem("Apply", "Ctrl+Shift+S", false, (ActiveDocument != nullptr)))
				{
					ApplyOrSaveActive(TRUE);
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Reset Layout"))
				{
					m_bApplyWindowLayout = TRUE;
				}
				if (ImGui::MenuItem("Close Workspace"))
				{
					m_bOpen = FALSE;
				}
				ImGui::EndMenu();
			}
			ImGui::TextDisabled("Animation Authoring");
			ImGui::EndMenuBar();
		}

		const ImGuiIO& IO = ImGui::GetIO();
		if ((IO.WantTextInput == false) && (IO.KeyCtrl != false)
			&& (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) != false)
			&& (ImGui::IsKeyPressed(ImGuiKey_S, false) != false))
		{
			ApplyOrSaveActive((IO.KeyShift != false) ? TRUE : FALSE);
		}
		if (m_LastError.Length() > 0u)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *m_LastError);
		}
		DrawDocumentTabs();
		DrawCloseDocumentPopup();
		ImGui::End();
	}

	void EAnimationWorkspace::DrawDocumentTabs()
	{
		const TArray<FAnimationDocument*>& Documents = m_Documents.GetDocuments();
		if (Documents.Num() <= 0)
		{
			DrawEmptyState();
			return;
		}

		if (ImGui::BeginTabBar("##animation_documents", ImGuiTabBarFlags_Reorderable))
		{
			for (INT32 DocumentIndex = 0; DocumentIndex < Documents.Num(); DocumentIndex++)
			{
				FAnimationDocument* Document = Documents[DocumentIndex];
				if (!Document)
				{
					continue;
				}
				EString Label = Document->DisplayName;
				if (Document->IsDirty())
				{
					Label += EString(" *");
				}
				Label += EString("###");
				Label += Document->AssetPath;
				bool bTabOpen = true;
				if (ImGui::BeginTabItem(*Label, &bTabOpen, ImGuiTabItemFlags_None))
				{
					m_Documents.SetActive(Document);
					if (Document->Kind == EAnimationDocumentKind::STATE_MACHINE)
					{
						m_StateMachinePanel.Draw(*static_cast<FStateMachineDocument*>(Document));
					}
					else if (Document->Kind == EAnimationDocumentKind::CLIP)
					{
						m_ClipPanel.Draw(*static_cast<FClipDocument*>(Document));
					}
					else if (Document->Kind == EAnimationDocumentKind::MONTAGE)
					{
						m_MontagePanel.Draw(*static_cast<FMontageDocument*>(Document));
					}
					else if (Document->Kind == EAnimationDocumentKind::NOTIFY)
					{
						m_NotifyPanel.Draw(*static_cast<FNotifyDocument*>(Document));
					}
					else if (Document->Kind == EAnimationDocumentKind::SLOT_GROUP)
					{
						m_SlotGroupPanel.Draw(*static_cast<FSlotGroupDocument*>(Document));
					}
					else
					{
						ImGui::TextDisabled("This document type is not migrated yet.");
					}
					ImGui::EndTabItem();
				}
				if (!bTabOpen)
				{
					Document->bClosePending = TRUE;
				}
			}
			ImGui::EndTabBar();
		}
	}

	void EAnimationWorkspace::DrawCloseDocumentPopup()
	{
		FAnimationDocument* PendingDocument = nullptr;
		const TArray<FAnimationDocument*>& Documents = m_Documents.GetDocuments();
		for (INT32 DocumentIndex = 0; DocumentIndex < Documents.Num(); DocumentIndex++)
		{
			if (Documents[DocumentIndex] && (Documents[DocumentIndex]->bClosePending != FALSE))
			{
				PendingDocument = Documents[DocumentIndex];
				break;
			}
		}
		if (!PendingDocument)
		{
			return;
		}

		if (!ImGui::IsPopupOpen("##close_animation_document"))
		{
			ImGui::OpenPopup("##close_animation_document");
		}
		if (ImGui::BeginPopupModal("##close_animation_document", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Close \"%s\"?", *PendingDocument->DisplayName);
			if (PendingDocument->IsDirty())
			{
				ImGui::TextDisabled("This document has unsaved changes.");
			}
			else if (PendingDocument->IsApplied() == FALSE)
			{
				ImGui::TextDisabled("The draft was not applied to the live asset.");
			}

			if (ImGui::Button("Save and Close"))
			{
				EString Error;
				if (PendingDocument->Save(Error))
				{
					m_Documents.Close(PendingDocument);
					PendingDocument = nullptr;
					ImGui::CloseCurrentPopup();
				}
				else
				{
					m_LastError = Error;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Discard and Close"))
			{
				EString Error;
				if (PendingDocument->Discard(Error))
				{
					m_Documents.Close(PendingDocument);
					PendingDocument = nullptr;
					ImGui::CloseCurrentPopup();
				}
				else
				{
					m_LastError = Error;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				PendingDocument->bClosePending = FALSE;
				PendingDocument = nullptr;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	void EAnimationWorkspace::DrawEmptyState()
	{
		ImGui::TextDisabled("No animation document is open.");
		ImGui::TextDisabled("Double-click an Animation, Montage, or State Machine PAsset in the Content Browser to open it here.");
	}

}

#endif
