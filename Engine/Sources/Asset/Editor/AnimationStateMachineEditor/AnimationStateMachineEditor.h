#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/Text/String.h>
#include <imgui.h>

namespace PigeonEngine
{

	class EAnimStateMachine;
	class EAnimStateMachineAsset;

	/*
	* First-pass state machine editor: the graph is authored as JSON, validated on Apply,
	* and saved back into the same PAsset. A structured graph view can replace this window
	* later without changing the asset or runtime.
	*/
	class EAnimStateMachineEditor
	{
	public:
		void EditorInit();
		void EditorUpdate();
		void OpenStateMachineAsset(const EString& InAssetFullPath);

	private:
		void LoadJsonBufferFromAsset();
		void DrawGraph(const EAnimStateMachine& InMachine);
		void DrawStructureControls(const EAnimStateMachine& InMachine);
		void DrawTransitionConditions(const EAnimStateMachine& InMachine);
		void LoadNodePositionsFromJson(const EAnimStateMachine& InMachine);
		void AutoLayoutGraph(const EAnimStateMachine& InMachine);
		// Serializes the current node positions into nodeX/nodeY fields before Apply/Save.
		void WriteNodePositionsToJson();

		// Structural edits rewrite the JSON and rebuild the live graph. They only commit
		// when the manager accepts the new graph, so a failed edit leaves the buffer alone.
		BOOL32 AddState(const EAnimStateMachine& InMachine);
		BOOL32 DeleteState(const EString& InStateName);
		BOOL32 AddTransition(const EString& InFromStateName, const EString& InToStateName);
		BOOL32 DeleteTransition(INT32 InTransitionIndex);
		BOOL32 SetEntryState(const EString& InStateName);
		BOOL32 AddTransitionCondition(INT32 InTransitionIndex, INT32 InParameterIndex);
		BOOL32 UpdateTransitionCondition(INT32 InTransitionIndex, INT32 InConditionIndex
			, INT32 InParameterIndex, const EString& InOp, FLOAT InValue);
		BOOL32 RemoveTransitionCondition(INT32 InTransitionIndex, INT32 InConditionIndex);
		// Validates and commits serialized JSON through the asset manager, then replaces the
		// editor buffer. A rejected graph leaves the live machine and buffer untouched.
		BOOL32 CommitJsonText(const EString& InJsonText);
		void ReplaceJsonBuffer(const EString& InJsonText);

	private:
		BOOL8							bShowEditor = FALSE;
		const EAnimStateMachineAsset*	Asset = nullptr;
		EString							OpenedAssetFullPath;
		EString							LastError;
		TArray<CHAR>					JsonBuffer;
		TMap<EString, ImVec2>			NodePositions;
		INT32							SelectedStateIndex = 0;
		INT32							SelectedTransitionIndex = -1;
		// -1 means the "Any" pseudo state.
		INT32							TransitionFromIndex = -1;
		INT32							TransitionToIndex = 0;
	};

}

#endif
