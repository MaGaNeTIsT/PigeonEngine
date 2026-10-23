#include "AnimationStateMachineAsset.h"

#include <IO/FileHelper.h>
#include <RapidJson/Headers/JsonObject.h>
#if _EDITOR_ONLY
#include <Editor/EditorLogManager.h>
#endif

namespace PigeonEngine
{

	namespace
	{
		// A state machine graph is small. The limit only protects the loader from a corrupt
		// length field turning into a huge allocation.
		constexpr UINT32 MaxStateMachineJsonLength = 16u * 1024u * 1024u;

		EAnimationLoopMode ParseLoopMode(const EString& InText)
		{
			if ((InText == EString("Once")) || (InText == EString("ONCE")) || (InText == EString("once")))
			{
				return ANIMATION_LOOP_ONCE;
			}
			if ((InText == EString("PingPong")) || (InText == EString("PINGPONG")) || (InText == EString("pingpong")))
			{
				return ANIMATION_LOOP_PINGPONG;
			}
			if ((InText == EString("Clamp")) || (InText == EString("CLAMP")) || (InText == EString("clamp")))
			{
				return ANIMATION_LOOP_CLAMP;
			}
			return ANIMATION_LOOP_REPEAT;
		}

		EAnimStateMachineParameterType ParseParameterType(const EString& InText)
		{
			if ((InText == EString("bool")) || (InText == EString("Bool")) || (InText == EString("BOOL")))
			{
				return ANIM_PARAM_BOOL;
			}
			if ((InText == EString("trigger")) || (InText == EString("Trigger")) || (InText == EString("TRIGGER")))
			{
				return ANIM_PARAM_TRIGGER;
			}
			return ANIM_PARAM_FLOAT;
		}

		EAnimStateMachineCompareType ParseCompareType(const EString& InText)
		{
			if (InText == EString(">=")) { return ANIM_COMPARE_GREATER_EQUAL; }
			if (InText == EString("<=")) { return ANIM_COMPARE_LESS_EQUAL; }
			if (InText == EString("==")) { return ANIM_COMPARE_EQUAL; }
			if (InText == EString("!=")) { return ANIM_COMPARE_NOT_EQUAL; }
			if (InText == EString("<")) { return ANIM_COMPARE_LESS; }
			if ((InText == EString("trigger")) || (InText == EString("Trigger"))) { return ANIM_COMPARE_TRIGGER; }
			return ANIM_COMPARE_GREATER;
		}

		struct FJsonObjectArrayGuard
		{
			explicit FJsonObjectArrayGuard(TArray<CJsonObject*>& InObjects) : Objects(InObjects) {}
			~FJsonObjectArrayGuard()
			{
				for (INT32 Index = 0; Index < Objects.Num(); Index++)
				{
					delete Objects[Index];
				}
				Objects.Empty();
			}
			TArray<CJsonObject*>& Objects;
		};

		// EAnimStateMachine is non-copyable as an object (it owns identity), but the editor
		// replaces its parsed contents in place. Copying the fields keeps the old graph alive
		// until the new JSON has parsed successfully.
		void CopyStateMachineData(EAnimStateMachine& OutMachine, const EAnimStateMachine& InMachine)
		{
			OutMachine.Name = InMachine.Name;
			OutMachine.SkeletonName = InMachine.SkeletonName;
			OutMachine.AnimationAssetName = InMachine.AnimationAssetName;
			OutMachine.SourceJson = InMachine.SourceJson;
			OutMachine.Parameters = InMachine.Parameters;
			OutMachine.States = InMachine.States;
			OutMachine.Transitions = InMachine.Transitions;
			OutMachine.EntryStateIndex = InMachine.EntryStateIndex;
			OutMachine.ParameterMapping = InMachine.ParameterMapping;
			OutMachine.StateMapping = InMachine.StateMapping;
		}
	}

	EAnimStateMachineManager::EAnimStateMachineManager()
	{
#if _EDITOR_ONLY
		DebugName = "AnimationStateMachineManager";
#endif
	}

	EAnimStateMachineManager::~EAnimStateMachineManager()
	{
	}

	EAnimStateMachineAsset::EAnimStateMachineAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
	) : TBaseAsset<EAnimStateMachine>(InAssetPath, InAssetName
#if _EDITOR_ONLY
		, InDebugName
#endif
	)
	{
	}

	EAnimStateMachineAsset::~EAnimStateMachineAsset()
	{
	}

	BOOL32 EAnimStateMachineAsset::InitResource()
	{
		if (IsInitialized())
		{
			return TRUE;
		}
		bIsInitialized = TRUE;
		return TRUE;
	}

	void EAnimStateMachineManager::Initialize()
	{
	}

	void EAnimStateMachineManager::ShutDown()
	{
		ReleaseAllAssets();
	}

	void EAnimStateMachineManager::Uninit()
	{
		ReleaseAllAssets();
	}

	void EAnimStateMachineManager::ReleaseAllAssets()
	{
		ClearStateMachines();
	}

	void EAnimStateMachineManager::ClearStateMachines()
	{
		StateMachineAssetManager.Clear();
	}

#if _EDITOR_ONLY
	BOOL32 EAnimStateMachineManager::ImportStateMachine(const EString& InAssetName, const EString& InJsonFullPathName, const EString& InSavePath)
	{
		EString JsonText;
		if (!(EFileHelper::ReadFileAsString(InJsonFullPathName, JsonText)))
		{
			PE_LOG_WARN(EString("State machine importer could not read '") + InJsonFullPathName + EString("'."));
			return FALSE;
		}

		EAnimStateMachine TempMachine;
		EString Error;
		if (!BuildStateMachine(TempMachine, JsonText, Error))
		{
			PE_LOG_WARN(EString("State machine importer rejected '") + InJsonFullPathName + EString("': ") + Error);
			return FALSE;
		}
		if (TempMachine.Name.Length() <= 0u)
		{
			TempMachine.Name = InAssetName;
		}
		return SaveStateMachineResource(InSavePath, InAssetName, JsonText);
	}
#endif

	BOOL32 EAnimStateMachineManager::LoadStateMachineAsset(const EString& InLoadPath, const EString& InLoadName
		, const EAnimStateMachineAsset*& OutAsset)
	{
		OutAsset = nullptr;
		const EString FullPathName = MakeAssetFullPathName(InLoadPath, InLoadName);
		if (EAnimStateMachineAsset* CachedAsset = StateMachineAssetManager.Find(FullPathName))
		{
			OutAsset = CachedAsset;
			return TRUE;
		}

		EString Error;
		EAnimStateMachine* LoadedResource = LoadStateMachineResource(InLoadPath, InLoadName, Error);
		if ((!LoadedResource) || (!LoadedResource->IsResourceValid()))
		{
			delete LoadedResource;
			return FALSE;
		}

		EAnimStateMachineAsset* NewAsset = new EAnimStateMachineAsset(InLoadPath, InLoadName
#if _EDITOR_ONLY
			, InLoadName
#endif
		);
		if (!(NewAsset->StorageResourceInternal([LoadedResource]() -> EAnimStateMachine*
			{
				return LoadedResource;
			})))
		{
			delete NewAsset;
			return FALSE;
		}
		if (!(NewAsset->InitResource()))
		{
			delete NewAsset;
			return FALSE;
		}
		if (StateMachineAssetManager.Add(FullPathName, NewAsset, TRUE) == 0u)
		{
			delete NewAsset;
			return FALSE;
		}
		OutAsset = NewAsset;
		return TRUE;
	}

	BOOL32 EAnimStateMachineManager::RequireLoadStateMachineAsset(const EString& InLoadPath, const EString& InLoadName
		, const EAnimStateMachineAsset*& OutAsset, EAssetLoadCompleteType InOnLoadComplete)
	{
		const EString FullPathName = MakeAssetFullPathName(InLoadPath, InLoadName);
		if (EAnimStateMachineAsset* CachedAsset = StateMachineAssetManager.Find(FullPathName))
		{
			OutAsset = CachedAsset;
			return TRUE;
		}
		OutAsset = nullptr;
		EAssetLoadManager::GetManagerSingleton()->RequestLoad(
			MakeAssetLoadRequestKey(PE_TEXT("AnimStateMachine"), FullPathName),
			[InLoadPath, InLoadName]() -> BOOL32
			{
				const EAnimStateMachineAsset* LoadedAsset = nullptr;
				return EAnimStateMachineManager::GetManagerSingleton()->LoadStateMachineAsset(InLoadPath, InLoadName, LoadedAsset);
			},
			static_cast<EAssetLoadCompleteType&&>(InOnLoadComplete));
		return FALSE;
	}

	BOOL32 EAnimStateMachineManager::RebuildStateMachineAsset(const EAnimStateMachineAsset* InAsset
		, const EString& InJsonText, EString& OutError)
	{
		OutError = EString();
		if (!InAsset)
		{
			OutError = EString("Null state machine asset.");
			return FALSE;
		}
		EAnimStateMachine TempMachine;
		if (!BuildStateMachine(TempMachine, InJsonText, OutError))
		{
			return FALSE;
		}
		if (TempMachine.Name.Length() <= 0u)
		{
			TempMachine.Name = InAsset->GetAssetName();
		}

		EAnimStateMachine* Machine = const_cast<EAnimStateMachine*>(InAsset->GetStoragedResource());
		if (!Machine)
		{
			OutError = EString("State machine asset has no resource.");
			return FALSE;
		}
		CopyStateMachineData(*Machine, TempMachine);
		return TRUE;
	}

	BOOL32 EAnimStateMachineManager::SaveStateMachineAsset(const EAnimStateMachineAsset* InAsset
		, const EString& InJsonText, EString& OutError)
	{
		if (!RebuildStateMachineAsset(InAsset, InJsonText, OutError))
		{
			return FALSE;
		}
		return SaveStateMachineResource(InAsset->GetAssetPath(), InAsset->GetAssetName(), InJsonText);
	}

	EAnimStateMachineAsset* EAnimStateMachineManager::LoadStateMachineAsset(const EString& InLoadPath, const EString& InLoadName)
	{
		const EAnimStateMachineAsset* LoadedAsset = nullptr;
		if (!LoadStateMachineAsset(InLoadPath, InLoadName, LoadedAsset))
		{
			return nullptr;
		}
		return const_cast<EAnimStateMachineAsset*>(LoadedAsset);
	}

	EAnimStateMachine* EAnimStateMachineManager::LoadStateMachineResource(const EString& InLoadPath, const EString& InLoadName
		, EString& OutError)
	{
		OutError = EString();
		const EString FullPathName = MakeAssetFullPathName(InLoadPath, InLoadName);
		void* ReadFileMem = nullptr;
		ULONG ReadFileSize = 0u;
		if (!(EFileHelper::ReadFileAsBinary(FullPathName, ReadFileMem, ReadFileSize)) || (!ReadFileMem))
		{
			OutError = EString("Could not read state machine PAsset '") + FullPathName + EString("'.");
			return nullptr;
		}

		const BYTE* Cursor = static_cast<const BYTE*>(ReadFileMem);
		LONGLONG Remaining = static_cast<LONGLONG>(ReadFileSize);
		UINT32 AssetType = 0u;
		UINT32 JsonLength = 0u;
		if (Remaining < static_cast<LONGLONG>(sizeof(UINT32) * 2u))
		{
			OutError = EString("State machine PAsset header is truncated.");
			delete[] static_cast<CHAR*>(ReadFileMem);
			return nullptr;
		}
		AssetType = *reinterpret_cast<const UINT32*>(Cursor);
		Cursor += sizeof(UINT32);
		Remaining -= sizeof(UINT32);
		if (AssetType != static_cast<UINT32>(EAssetType::ASSET_TYPE_ANIM_STATE_MACHINE))
		{
			OutError = EString("Asset is not an animation state machine.");
			delete[] static_cast<CHAR*>(ReadFileMem);
			return nullptr;
		}
		JsonLength = *reinterpret_cast<const UINT32*>(Cursor);
		Cursor += sizeof(UINT32);
		Remaining -= sizeof(UINT32);
		if ((JsonLength == 0u) || (JsonLength > MaxStateMachineJsonLength) || (Remaining < static_cast<LONGLONG>(JsonLength)))
		{
			OutError = EString("State machine JSON length is invalid.");
			delete[] static_cast<CHAR*>(ReadFileMem);
			return nullptr;
		}

		TArray<CHAR> JsonBuffer;
		JsonBuffer.SetNum(static_cast<INT32>(JsonLength) + 1);
		::memcpy_s(JsonBuffer.GetData(), JsonLength, Cursor, JsonLength);
		JsonBuffer[static_cast<INT32>(JsonLength)] = '\0';
		const EString JsonText(JsonBuffer.GetData());
		delete[] static_cast<CHAR*>(ReadFileMem);

		EAnimStateMachine* LoadedResource = new EAnimStateMachine();
		if (!BuildStateMachine(*LoadedResource, JsonText, OutError))
		{
			delete LoadedResource;
			return nullptr;
		}
		if (LoadedResource->Name.Length() <= 0u)
		{
			LoadedResource->Name = InLoadName;
		}
		return LoadedResource;
	}

	BOOL32 EAnimStateMachineManager::SaveStateMachineResource(const EString& InSavePath, const EString& InSaveName, const EString& InJsonText)
	{
		const EString FullPathName = MakeAssetFullPathName(InSavePath, InSaveName);
		const UINT32 JsonLength = static_cast<UINT32>(InJsonText.Length() + 1u);
		const ULONG OutputSize = sizeof(UINT32) + sizeof(UINT32) + JsonLength;
		BYTE* OutputMem = new BYTE[OutputSize];

		BYTE* Cursor = OutputMem;
		*reinterpret_cast<UINT32*>(Cursor) = static_cast<UINT32>(EAssetType::ASSET_TYPE_ANIM_STATE_MACHINE);
		Cursor += sizeof(UINT32);
		*reinterpret_cast<UINT32*>(Cursor) = JsonLength;
		Cursor += sizeof(UINT32);
		::memcpy_s(Cursor, JsonLength, *InJsonText, JsonLength);

		const BOOL32 Result = EFileHelper::SaveBytesToFile(FullPathName, OutputMem, OutputSize);
		delete[] OutputMem;
		return Result;
	}

	BOOL32 EAnimStateMachineManager::BuildStateMachine(EAnimStateMachine& OutMachine, const EString& InJsonText, EString& OutError)
	{
		OutMachine.Reset();
		OutError = EString();
		if (InJsonText.Length() <= 0u)
		{
			OutError = EString("State machine JSON is empty.");
			return FALSE;
		}

		// The const char* constructor does not assert on a malformed document; IsValid()
		// reports both a parse error and a non-object root.
		CJsonObject Root(*InJsonText);
		if (!Root.IsValid())
		{
			OutError = EString("State machine JSON is not a valid object.");
			return FALSE;
		}

		Root.GetStringField(PE_TEXT("name"), OutMachine.Name);
		Root.GetStringField(PE_TEXT("skeleton"), OutMachine.SkeletonName);
		Root.GetStringField(PE_TEXT("animation"), OutMachine.AnimationAssetName);

		{
			TArray<CJsonObject*> ParameterObjects;
			Root.GetObjectArrayField(PE_TEXT("parameters"), ParameterObjects);
			FJsonObjectArrayGuard ParameterGuard(ParameterObjects);
			for (INT32 ParameterIndex = 0; ParameterIndex < ParameterObjects.Num(); ParameterIndex++)
			{
				CJsonObject* ParameterObject = ParameterObjects[ParameterIndex];
				if (!ParameterObject)
				{
					continue;
				}
				EAnimStateMachineParameter Parameter;
				ParameterObject->GetStringField(PE_TEXT("name"), Parameter.Name);
				EString TypeText;
				ParameterObject->GetStringField(PE_TEXT("type"), TypeText);
				Parameter.Type = ParseParameterType(TypeText);
				if (Parameter.Type == ANIM_PARAM_FLOAT)
				{
					FLOAT DefaultValue = 0.0f;
					if (ParameterObject->GetFloatField(PE_TEXT("default"), DefaultValue))
					{
						Parameter.DefaultFloat = DefaultValue;
					}
				}
				else if (Parameter.Type == ANIM_PARAM_BOOL)
				{
					BOOL8 DefaultValue = FALSE;
					if (ParameterObject->GetBoolField(PE_TEXT("default"), DefaultValue))
					{
						Parameter.DefaultBool = DefaultValue;
					}
				}
				OutMachine.Parameters.Add(Parameter);
			}
		}

		{
			TArray<CJsonObject*> StateObjects;
			if (!(Root.GetObjectArrayField(PE_TEXT("states"), StateObjects)))
			{
				for (INT32 StateIndex = 0; StateIndex < StateObjects.Num(); StateIndex++)
				{
					delete StateObjects[StateIndex];
				}
				OutError = EString("State machine JSON has no states array.");
				return FALSE;
			}
			FJsonObjectArrayGuard StateGuard(StateObjects);
			for (INT32 StateIndex = 0; StateIndex < StateObjects.Num(); StateIndex++)
			{
				CJsonObject* StateObject = StateObjects[StateIndex];
				if (!StateObject)
				{
					continue;
				}
				EAnimStateMachineState State;
				StateObject->GetStringField(PE_TEXT("name"), State.Name);
				StateObject->GetStringField(PE_TEXT("clip"), State.ClipName);
				EString LoopText;
				StateObject->GetStringField(PE_TEXT("loop"), LoopText);
				State.LoopMode = ParseLoopMode(LoopText);
				FLOAT PlayRate = 1.0f;
				if (StateObject->GetFloatField(PE_TEXT("rate"), PlayRate))
				{
					State.PlayRate = PlayRate;
				}
				FLOAT BlendSeconds = 0.2f;
				if (StateObject->GetFloatField(PE_TEXT("blend"), BlendSeconds))
				{
					State.BlendSeconds = BlendSeconds;
				}
				if ((State.Name.Length() <= 0u) || (State.ClipName.Length() <= 0u))
				{
					OutError = EString("Every state needs a name and a clip.");
					return FALSE;
				}
				OutMachine.States.Add(State);
			}
		}

		if (!OutMachine.RebuildMappings())
		{
			OutError = EString("State or parameter names are empty or duplicated.");
			return FALSE;
		}

		{
			TArray<CJsonObject*> TransitionObjects;
			Root.GetObjectArrayField(PE_TEXT("transitions"), TransitionObjects);
			FJsonObjectArrayGuard TransitionGuard(TransitionObjects);
			for (INT32 TransitionIndex = 0; TransitionIndex < TransitionObjects.Num(); TransitionIndex++)
			{
				CJsonObject* TransitionObject = TransitionObjects[TransitionIndex];
				if (!TransitionObject)
				{
					continue;
				}
				EAnimStateMachineTransition Transition;
				EString FromText;
				EString ToText;
				TransitionObject->GetStringField(PE_TEXT("from"), FromText);
				TransitionObject->GetStringField(PE_TEXT("to"), ToText);
				const BOOL32 bAnyState = ((FromText.Length() <= 0u) || (FromText == EString("Any")));
				Transition.FromStateIndex = (bAnyState ? -1 : OutMachine.FindState(FromText));
				Transition.ToStateIndex = OutMachine.FindState(ToText);
				if ((Transition.ToStateIndex < 0) || ((!bAnyState) && (Transition.FromStateIndex < 0)))
				{
					OutError = EString("Transition references an unknown state (from '") + FromText + EString("', to '") + ToText + EString("').");
					return FALSE;
				}

				INT32 Priority = 0;
				if (TransitionObject->GetIntField(PE_TEXT("priority"), Priority))
				{
					Transition.Priority = Priority;
				}
				FLOAT BlendSeconds = 0.2f;
				if (TransitionObject->GetFloatField(PE_TEXT("blend"), BlendSeconds))
				{
					Transition.BlendSeconds = BlendSeconds;
				}
				FLOAT MinTimeInState = 0.0f;
				if (TransitionObject->GetFloatField(PE_TEXT("minTime"), MinTimeInState))
				{
					Transition.MinTimeInState = MinTimeInState;
				}
				BOOL8 bCanTransitionToSelf = FALSE;
				if (TransitionObject->GetBoolField(PE_TEXT("canSelf"), bCanTransitionToSelf))
				{
					Transition.bCanTransitionToSelf = bCanTransitionToSelf;
				}

				{
					TArray<CJsonObject*> ConditionObjects;
					TransitionObject->GetObjectArrayField(PE_TEXT("conditions"), ConditionObjects);
					FJsonObjectArrayGuard ConditionGuard(ConditionObjects);
					for (INT32 ConditionIndex = 0; ConditionIndex < ConditionObjects.Num(); ConditionIndex++)
					{
						CJsonObject* ConditionObject = ConditionObjects[ConditionIndex];
						if (!ConditionObject)
						{
							continue;
						}
						EAnimStateMachineCondition Condition;
						EString ParameterName;
						EString CompareText;
						ConditionObject->GetStringField(PE_TEXT("param"), ParameterName);
						ConditionObject->GetStringField(PE_TEXT("op"), CompareText);
						Condition.ParameterIndex = OutMachine.FindParameter(ParameterName);
						if (Condition.ParameterIndex < 0)
						{
							OutError = EString("Transition condition references unknown parameter '") + ParameterName + EString("'.");
							return FALSE;
						}
						Condition.Compare = ParseCompareType(CompareText);
						FLOAT CompareValue = 0.0f;
						if (ConditionObject->GetFloatField(PE_TEXT("value"), CompareValue))
						{
							Condition.Value = CompareValue;
						}
						Transition.Conditions.Add(Condition);
					}
				}

				OutMachine.Transitions.Add(Transition);
			}
		}

		EString EntryName;
		Root.GetStringField(PE_TEXT("entry"), EntryName);
		OutMachine.EntryStateIndex = ((EntryName.Length() > 0u) ? OutMachine.FindState(EntryName) : 0);
		if (OutMachine.EntryStateIndex < 0)
		{
			OutError = EString("State machine entry state '") + EntryName + EString("' does not exist.");
			return FALSE;
		}

		OutMachine.SourceJson = InJsonText;
		if (!OutMachine.IsValid())
		{
			OutError = EString("State machine graph is invalid.");
			return FALSE;
		}
		return TRUE;
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<EAnimStateMachine, EObjectBase, EResourceInterface>();
		RegisterBaseAssetClassTypes<EAnimStateMachine>();
		RegisterClassType<EAnimStateMachineAsset, TBaseAsset<EAnimStateMachine>>();
		RegisterClassType<EAnimStateMachineManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

}
