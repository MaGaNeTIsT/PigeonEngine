#include "PropertyEditor.h"

#if ENGINE_EDITOR_ONLY

#include <imgui.h>
#include <Config/EngineConfig.h>
#include <IO/FileHelper.h>
#include <Base/DataStructure/Text/Path.h>

namespace PigeonEngine
{
	namespace
	{
		EPropertyEditor::FOnPropertyCommitted PropertyCommittedCallback;
		// Values at the start of the current drag/input, keyed by property name. Only one
		// property is interacted with at a time, so a name key is enough and avoids storing
		// a per-object map.
		TMap<EString, FPropertyValue> EditStartValues;

		struct FAssetNameCache
		{
			EPropertyAssetType Type = EPropertyAssetType::NONE;
			TArray<EString> Names;
			DOUBLE RefreshedAt = -1.0;
		};
		TArray<FAssetNameCache> AssetNameCaches;

		const CHAR* GetAssetDirectory(const EPropertyAssetType InAssetType)
		{
			switch (InAssetType)
			{
			case EPropertyAssetType::STATIC_MESH:
			case EPropertyAssetType::SKELETAL_MESH:
				return EBaseSettings::ENGINE_MESH_PATH;
			case EPropertyAssetType::SKELETON:
				return EBaseSettings::ENGINE_SKELETON_PATH;
			case EPropertyAssetType::ANIMATION:
				return EBaseSettings::ENGINE_ANIMATION_PATH;
			case EPropertyAssetType::CUBE_MAP:
				return EBaseSettings::ENGINE_TEXTURE_PATH;
			case EPropertyAssetType::NAV_MESH:
				return EBaseSettings::ENGINE_NAV_MESH_PATH;
			case EPropertyAssetType::MATERIAL:
				return EBaseSettings::ENGINE_SHADER_SOURCE_DIRECTORY;
			default:
				return nullptr;
			}
		}

		EString GetLeafName(const EString& InPath)
		{
			const INT32 Separator = InPath.RightFind(EString(PE_TEXT("/")));
			return (Separator >= 0)
				? InPath.Right(InPath.Length() - static_cast<UINT32>(Separator) - 1u)
				: InPath;
		}

		EString GetParentPath(const EString& InPath)
		{
			const INT32 Separator = InPath.RightFind(EString(PE_TEXT("/")));
			return (Separator > 0) ? InPath.Left(static_cast<UINT32>(Separator)) : EString();
		}

		void CollectMaterialSourceNamesFromDirectory(const EString& InDirectory, const EString& InSourceRoot, TArray<EString>& OutNames)
		{
			TArray<EString> Folders;
			TArray<EString> Files;
			if (!EFileHelper::ScanDirectory(InDirectory, Folders, Files, TRUE))
			{
				return;
			}
			for (UINT32 Index = 0u, Num = Files.Num<UINT32>(); Index < Num; Index++)
			{
				const EString& File = Files[Index];
				if (EPath::GetExtension(File) != EString(PE_TEXT("json")))
				{
					continue;
				}
				EString FileText;
				const BOOL32 bRead = EFileHelper::ReadFileAsString(File, FileText);
				const BOOL32 bHasMarker = (bRead && FileText.Contains(EString(PE_TEXT("PEMaterial")))) ? TRUE : FALSE;
				if (InDirectory.Contains(EString(PE_TEXT("Materials"))) && (EPath::GetExtension(File) == EString(PE_TEXT("json"))))
				{
				}
				if (!bHasMarker)
				{
					continue;
				}
				const EString NormalizedFile = File.Replace(EString(PE_TEXT("\\")), EString(PE_TEXT("/")));
				if (NormalizedFile.Left(InSourceRoot.Length()) != InSourceRoot)
				{
					continue;
				}
				const EString RelativePath = NormalizedFile.Right(NormalizedFile.Length() - InSourceRoot.Length());
				if (RelativePath.Length() <= 5u)
				{
					continue;
				}
				const EString RelativeStem = RelativePath.Left(RelativePath.Length() - 5u);
				EString MaterialName;
				const EString TemplatesPrefix(PE_TEXT("Materials/Templates/"));
				const EString MaterialsPrefix(PE_TEXT("Materials/"));
				if (RelativeStem.Left(TemplatesPrefix.Length()) == TemplatesPrefix)
				{
					MaterialName = GetLeafName(RelativeStem);
				}
				else if (RelativeStem.Left(MaterialsPrefix.Length()) == MaterialsPrefix)
				{
					MaterialName = GetLeafName(RelativeStem);
				}
				else
				{
					// Project materials live in their own folder and are named by that folder.
					MaterialName = GetParentPath(RelativeStem);
				}
				if ((MaterialName.Length() > 0u) && (!OutNames.Contains(MaterialName)))
				{
					OutNames.Add(MaterialName);
				}
			}
			for (UINT32 Index = 0u, Num = Folders.Num<UINT32>(); Index < Num; Index++)
			{
				CollectMaterialSourceNamesFromDirectory(Folders[Index], InSourceRoot, OutNames);
			}
		}

		void CollectAssetNamesFromDirectory(const EString& InDirectory, TArray<EString>& OutNames)
		{
			TArray<EString> Folders;
			TArray<EString> Files;
			if (!EFileHelper::ScanDirectory(InDirectory, Folders, Files, TRUE))
			{
				return;
			}
			for (UINT32 Index = 0u, Num = Files.Num<UINT32>(); Index < Num; Index++)
			{
				const EString& File = Files[Index];
				if (EPath::GetExtension(File) != EString(PE_TEXT("PAsset")))
				{
					continue;
				}
				const EString AssetName = EPath::GetFileNameWithoutExtension(File);
				if ((AssetName.Length() > 0u) && (!OutNames.Contains(AssetName)))
				{
					OutNames.Add(AssetName);
				}
			}
			// Shader assets live under target / shader model folders; the other families are
			// flat today, but scanning recursively keeps this independent of that layout.
			for (UINT32 Index = 0u, Num = Folders.Num<UINT32>(); Index < Num; Index++)
			{
				CollectAssetNamesFromDirectory(Folders[Index], OutNames);
			}
		}

		void RefreshAssetNames(FAssetNameCache& InOutCache)
		{
			const DOUBLE Now = ImGui::GetTime();
			if ((InOutCache.RefreshedAt >= 0.0) && ((Now - InOutCache.RefreshedAt) < 1.0))
			{
				return;
			}
			InOutCache.RefreshedAt = Now;
			InOutCache.Names.Empty();

			const CHAR* AssetDirectory = GetAssetDirectory(InOutCache.Type);
			if (!AssetDirectory)
			{
				return;
			}
			if (InOutCache.Type == EPropertyAssetType::MATERIAL)
			{
				EString SourceRoot(AssetDirectory);
				SourceRoot = SourceRoot.Replace(EString(PE_TEXT("\\")), EString(PE_TEXT("/")));
				if ((SourceRoot.Length() > 0u) && (SourceRoot.Right(1u) != EString(PE_TEXT("/"))))
				{
					SourceRoot += EString(PE_TEXT("/"));
				}
				CollectMaterialSourceNamesFromDirectory(SourceRoot, SourceRoot, InOutCache.Names);
			}
			else
			{
				CollectAssetNamesFromDirectory(EString(AssetDirectory), InOutCache.Names);
			}
			InOutCache.Names.Sort([](const EString& InLeft, const EString& InRight) -> BOOL32
			{
				return ((InLeft < InRight) != FALSE) ? TRUE : FALSE;
			});
		}

		FAssetNameCache& FindAssetNameCache(const EPropertyAssetType InAssetType)
		{
			for (UINT32 Index = 0u, Num = AssetNameCaches.Num<UINT32>(); Index < Num; Index++)
			{
				if (AssetNameCaches[Index].Type == InAssetType)
				{
					return AssetNameCaches[Index];
				}
			}
			FAssetNameCache NewCache;
			NewCache.Type = InAssetType;
			AssetNameCaches.Add(NewCache);
			return AssetNameCaches[AssetNameCaches.Num<UINT32>() - 1u];
		}

		void Commit(const FProperty& InProperty, void* InObject, const size_t& InClassHash, const FPropertyValue& InBefore, const FPropertyValue& InAfter)
		{
			if (PropertyCommittedCallback)
			{
				PropertyCommittedCallback(InObject, InClassHash, InProperty.Name, InBefore, InAfter);
			}
		}

		void StoreEditStart(const FProperty& InProperty, const FPropertyValue& InValue)
		{
			FPropertyValue Snapshot;
			ClonePropertyValue(InValue, Snapshot);
			EditStartValues.Add(InProperty.Name, Snapshot);
		}

		BOOL32 TakeEditStart(const FProperty& InProperty, FPropertyValue& OutValue)
		{
			return EditStartValues.FindValue(InProperty.Name, OutValue);
		}

		BOOL32 DrawBoolProperty(const FProperty& InProperty, void* InObject, const size_t& InClassHash, FPropertyValue& InOutValue)
		{
			BOOL32 bValue = (InOutValue.BoolValue != FALSE) ? TRUE : FALSE;
			if (ImGui::Checkbox("##value", &bValue))
			{
				const FPropertyValue Before = InOutValue;
				InOutValue.BoolValue = bValue;
				InProperty.SetValue(InObject, InOutValue);
				Commit(InProperty, InObject, InClassHash, Before, InOutValue);
				return TRUE;
			}
			return FALSE;
		}

		BOOL32 DrawIntProperty(const FProperty& InProperty, void* InObject, const size_t& InClassHash, FPropertyValue& InOutValue, const BOOL32 bUnsigned)
		{
			INT32 IntValue = static_cast<INT32>(InOutValue.IntValue);
			const BOOL32 bChanged = bUnsigned
				? ImGui::DragInt("##value", &IntValue, 1.0f, 0, 0x7FFFFFFF)
				: ImGui::DragInt("##value", &IntValue, 1.0f);
			if (ImGui::IsItemActivated())
			{
				StoreEditStart(InProperty, InOutValue);
			}
			if (bChanged)
			{
				InOutValue.IntValue = static_cast<INT64>(IntValue);
				InProperty.SetValue(InObject, InOutValue);
			}
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				FPropertyValue Before;
				if (TakeEditStart(InProperty, Before))
				{
					Commit(InProperty, InObject, InClassHash, Before, InOutValue);
				}
			}
			return bChanged;
		}

		BOOL32 DrawFloatProperty(const FProperty& InProperty, void* InObject, const size_t& InClassHash, FPropertyValue& InOutValue)
		{
			FLOAT FloatValue = static_cast<FLOAT>(InOutValue.DoubleValue);
			const BOOL32 bChanged = ImGui::DragFloat("##value", &FloatValue, 0.01f);
			if (ImGui::IsItemActivated())
			{
				StoreEditStart(InProperty, InOutValue);
			}
			if (bChanged)
			{
				InOutValue.DoubleValue = static_cast<DOUBLE>(FloatValue);
				InProperty.SetValue(InObject, InOutValue);
			}
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				FPropertyValue Before;
				if (TakeEditStart(InProperty, Before))
				{
					Commit(InProperty, InObject, InClassHash, Before, InOutValue);
				}
			}
			return bChanged;
		}

		BOOL32 DrawDoubleProperty(const FProperty& InProperty, void* InObject, const size_t& InClassHash, FPropertyValue& InOutValue)
		{
			const BOOL32 bChanged = ImGui::DragScalar("##value", ImGuiDataType_Double, &InOutValue.DoubleValue, 0.01f);
			if (ImGui::IsItemActivated())
			{
				StoreEditStart(InProperty, InOutValue);
			}
			if (bChanged)
			{
				InProperty.SetValue(InObject, InOutValue);
			}
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				FPropertyValue Before;
				if (TakeEditStart(InProperty, Before))
				{
					Commit(InProperty, InObject, InClassHash, Before, InOutValue);
				}
			}
			return bChanged;
		}

		BOOL32 DrawStringProperty(const FProperty& InProperty, void* InObject, const size_t& InClassHash, FPropertyValue& InOutValue)
		{
			if (InProperty.AssetType != EPropertyAssetType::NONE)
			{
				EString SelectedName = InOutValue.StringValue;
				if (!EPropertyEditor::DrawAssetCombo("##value", InProperty.AssetType, SelectedName))
				{
					return FALSE;
				}
				const FPropertyValue Before = InOutValue;
				InOutValue.StringValue = SelectedName;
				InProperty.SetValue(InObject, InOutValue);
				Commit(InProperty, InObject, InClassHash, Before, InOutValue);
				return TRUE;
			}

			CHAR Buffer[512] = {};
			const UINT32 Length = EMath::Min(InOutValue.StringValue.Length(), static_cast<UINT32>(sizeof(Buffer) - 1u));
			if (Length > 0u)
			{
				memcpy(Buffer, *InOutValue.StringValue, Length);
			}
			const BOOL32 bChanged = ImGui::InputText("##value", Buffer, sizeof(Buffer));
			if (ImGui::IsItemActivated())
			{
				StoreEditStart(InProperty, InOutValue);
			}
			if (bChanged)
			{
				InOutValue.StringValue = Buffer;
				InProperty.SetValue(InObject, InOutValue);
			}
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				FPropertyValue Before;
				if (TakeEditStart(InProperty, Before))
				{
					Commit(InProperty, InObject, InClassHash, Before, InOutValue);
				}
			}
			return bChanged;
		}

		BOOL32 DrawEnumProperty(const FProperty& InProperty, void* InObject, const size_t& InClassHash, FPropertyValue& InOutValue)
		{
			TArray<FEnumEntry> Entries;
			EEnumRegistry::GetValues(InProperty.EnumHash, Entries);
			EString CurrentName;
			if (!EEnumRegistry::TryValueToString(InProperty.EnumHash, static_cast<INT32>(InOutValue.IntValue), CurrentName))
			{
				CurrentName = EString::FromInt(static_cast<UINT32>(InOutValue.IntValue));
			}

			BOOL32 bChanged = FALSE;
			if (ImGui::BeginCombo("##value", *CurrentName))
			{
				for (UINT32 Index = 0u, Num = Entries.Num<UINT32>(); Index < Num; Index++)
				{
					const BOOL32 bSelected = (Entries[Index].Value == static_cast<INT32>(InOutValue.IntValue)) ? TRUE : FALSE;
					if (ImGui::Selectable(*Entries[Index].Name, bSelected))
					{
						const FPropertyValue Before = InOutValue;
						InOutValue.IntValue = static_cast<INT64>(Entries[Index].Value);
						InProperty.SetValue(InObject, InOutValue);
						Commit(InProperty, InObject, InClassHash, Before, InOutValue);
						bChanged = TRUE;
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			return bChanged;
		}

		BOOL32 DrawVector2Property(const FProperty& InProperty, void* InObject, const size_t& InClassHash, FPropertyValue& InOutValue)
		{
			FLOAT Values[2] = { InOutValue.Vector2Value.x, InOutValue.Vector2Value.y };
			const BOOL32 bChanged = ImGui::DragFloat2("##value", Values, 0.01f);
			if (ImGui::IsItemActivated())
			{
				StoreEditStart(InProperty, InOutValue);
			}
			if (bChanged)
			{
				InOutValue.Vector2Value = Vector2(Values[0], Values[1]);
				InProperty.SetValue(InObject, InOutValue);
			}
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				FPropertyValue Before;
				if (TakeEditStart(InProperty, Before))
				{
					Commit(InProperty, InObject, InClassHash, Before, InOutValue);
				}
			}
			return bChanged;
		}

		BOOL32 DrawVector2IntProperty(const FProperty& InProperty, void* InObject, const size_t& InClassHash, FPropertyValue& InOutValue)
		{
			INT32 Values[2] = { InOutValue.Vector2IntValue.x, InOutValue.Vector2IntValue.y };
			const BOOL32 bChanged = ImGui::DragInt2("##value", Values);
			if (ImGui::IsItemActivated())
			{
				StoreEditStart(InProperty, InOutValue);
			}
			if (bChanged)
			{
				InOutValue.Vector2IntValue = Vector2Int(Values[0], Values[1]);
				InProperty.SetValue(InObject, InOutValue);
			}
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				FPropertyValue Before;
				if (TakeEditStart(InProperty, Before))
				{
					Commit(InProperty, InObject, InClassHash, Before, InOutValue);
				}
			}
			return bChanged;
		}

		BOOL32 DrawVector3Property(const FProperty& InProperty, void* InObject, const size_t& InClassHash, FPropertyValue& InOutValue)
		{
			FLOAT Values[3] = { InOutValue.Vector3Value.x, InOutValue.Vector3Value.y, InOutValue.Vector3Value.z };
			const BOOL32 bChanged = ImGui::DragFloat3("##value", Values, 0.01f);
			if (ImGui::IsItemActivated())
			{
				StoreEditStart(InProperty, InOutValue);
			}
			if (bChanged)
			{
				InOutValue.Vector3Value = Vector3(Values[0], Values[1], Values[2]);
				InProperty.SetValue(InObject, InOutValue);
			}
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				FPropertyValue Before;
				if (TakeEditStart(InProperty, Before))
				{
					Commit(InProperty, InObject, InClassHash, Before, InOutValue);
				}
			}
			return bChanged;
		}

		BOOL32 DrawQuaternionProperty(const FProperty& InProperty, void* InObject, const size_t& InClassHash, FPropertyValue& InOutValue)
		{
			FLOAT Values[4] = { InOutValue.QuaternionValue.x, InOutValue.QuaternionValue.y, InOutValue.QuaternionValue.z, InOutValue.QuaternionValue.w };
			const BOOL32 bChanged = ImGui::DragFloat4("##value", Values, 0.005f);
			if (ImGui::IsItemActivated())
			{
				StoreEditStart(InProperty, InOutValue);
			}
			if (bChanged)
			{
				InOutValue.QuaternionValue = Quaternion(Values[0], Values[1], Values[2], Values[3]);
				InProperty.SetValue(InObject, InOutValue);
			}
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				FPropertyValue Before;
				if (TakeEditStart(InProperty, Before))
				{
					Commit(InProperty, InObject, InClassHash, Before, InOutValue);
				}
			}
			return bChanged;
		}

		BOOL32 DrawColor3Property(const FProperty& InProperty, void* InObject, const size_t& InClassHash, FPropertyValue& InOutValue)
		{
			FLOAT Values[3] = { InOutValue.ColorValue.x, InOutValue.ColorValue.y, InOutValue.ColorValue.z };
			const BOOL32 bChanged = ImGui::DragFloat3("##value", Values, 0.005f, 0.0f, 1.0f);
			if (ImGui::IsItemActivated())
			{
				StoreEditStart(InProperty, InOutValue);
			}
			if (bChanged)
			{
				InOutValue.ColorValue = Color3(Values[0], Values[1], Values[2]);
				InProperty.SetValue(InObject, InOutValue);
			}
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				FPropertyValue Before;
				if (TakeEditStart(InProperty, Before))
				{
					Commit(InProperty, InObject, InClassHash, Before, InOutValue);
				}
			}
			return bChanged;
		}

		BOOL32 DrawTransformProperty(const FProperty& InProperty, void* InObject, const size_t& InClassHash, FPropertyValue& InOutValue)
		{
			ETransform Transform = InOutValue.TransformValue;
			Vector3 Location = Transform.GetLocation_Local();
			Quaternion Rotation = Transform.GetRotation_Local();
			Vector3 Scaling = Transform.GetScaling_Local();
			FLOAT LocationValues[3] = { Location.x, Location.y, Location.z };
			FLOAT RotationValues[4] = { Rotation.x, Rotation.y, Rotation.z, Rotation.w };
			FLOAT ScalingValues[3] = { Scaling.x, Scaling.y, Scaling.z };

			BOOL32 bChanged = FALSE;
			auto HandleEdit = [&InProperty, InObject, InClassHash, &InOutValue](const BOOL32 bFieldChanged, const ETransform& NewTransform) -> void
			{
				if (ImGui::IsItemActivated())
				{
					StoreEditStart(InProperty, InOutValue);
				}
				if (bFieldChanged)
				{
					InOutValue.TransformValue = NewTransform;
					InProperty.SetValue(InObject, InOutValue);
				}
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					FPropertyValue Before;
					if (TakeEditStart(InProperty, Before))
					{
						Commit(InProperty, InObject, InClassHash, Before, InOutValue);
					}
				}
			};

			ImGui::TextUnformatted("Location");
			const BOOL32 bLocationChanged = ImGui::DragFloat3("##location", LocationValues, 0.01f);
			{
				ETransform NewTransform = Transform;
				if (bLocationChanged)
				{
					NewTransform.SetLocation_Local(Vector3(LocationValues[0], LocationValues[1], LocationValues[2]));
				}
				HandleEdit(bLocationChanged, NewTransform);
				if (bLocationChanged)
				{
					Transform = NewTransform;
					bChanged = TRUE;
				}
			}
			ImGui::TextUnformatted("Rotation");
			const BOOL32 bRotationChanged = ImGui::DragFloat4("##rotation", RotationValues, 0.005f);
			{
				ETransform NewTransform = Transform;
				if (bRotationChanged)
				{
					NewTransform.SetRotation_Local(Quaternion(RotationValues[0], RotationValues[1], RotationValues[2], RotationValues[3]));
				}
				HandleEdit(bRotationChanged, NewTransform);
				if (bRotationChanged)
				{
					Transform = NewTransform;
					bChanged = TRUE;
				}
			}
			ImGui::TextUnformatted("Scaling");
			const BOOL32 bScalingChanged = ImGui::DragFloat3("##scaling", ScalingValues, 0.01f);
			{
				ETransform NewTransform = Transform;
				if (bScalingChanged)
				{
					NewTransform.SetScaling_Local(Vector3(ScalingValues[0], ScalingValues[1], ScalingValues[2]));
				}
				HandleEdit(bScalingChanged, NewTransform);
				if (bScalingChanged)
				{
					Transform = NewTransform;
					bChanged = TRUE;
				}
			}

			return bChanged;
		}

		BOOL32 DrawElementWidget(FPropertyElement& InOutElement, const EPropertyValueType InType, const size_t InEnumHash,
			const size_t InStructHash, const EPropertyAssetType InAssetType, BOOL32& bOutActivated, BOOL32& bOutDeactivated);
		BOOL32 DrawNestedValue(FPropertyValue& InOutValue, const EPropertyAssetType InAssetType, BOOL32& bOutActivated, BOOL32& bOutDeactivated);
		BOOL32 DrawStructFields(FPropertyValue& InOutValue, const FStructDescriptor& InDescriptor, BOOL32& bOutActivated, BOOL32& bOutDeactivated);

		void InitializeDefaultElement(FPropertyElement& OutElement, const FPropertyValue* InShape);

		BOOL32 DrawNestedArray(FPropertyValue& InOutValue, const EPropertyAssetType InElementAssetType, BOOL32& bOutActivated, BOOL32& bOutDeactivated)
		{
			BOOL32 bChanged = FALSE;
			for (UINT32 Index = 0u; Index < InOutValue.ArrayValues.Num<UINT32>(); Index++)
			{
				ImGui::PushID(Index);
				BOOL32 bActivated = FALSE;
				BOOL32 bDeactivated = FALSE;
				if (DrawElementWidget(InOutValue.ArrayValues[Index], InOutValue.ElementType, InOutValue.ElementEnumHash,
					InOutValue.ElementStructHash, InElementAssetType, bActivated, bDeactivated))
				{
					bChanged = TRUE;
				}
				bOutActivated = (bOutActivated || bActivated) ? TRUE : FALSE;
				bOutDeactivated = (bOutDeactivated || bDeactivated) ? TRUE : FALSE;
				ImGui::SameLine();
				if (ImGui::SmallButton("X"))
				{
					InOutValue.ArrayValues.RemoveAt(Index);
					bChanged = TRUE;
					ImGui::PopID();
					break;
				}
				ImGui::PopID();
			}
			if (ImGui::SmallButton("Add Element"))
			{
				FPropertyElement NewElement;
				InitializeDefaultElement(NewElement, InOutValue.ElementShape.get());
				InOutValue.ArrayValues.Add(NewElement);
				bChanged = TRUE;
			}
			return bChanged;
		}

		BOOL32 DrawNestedMap(FPropertyValue& InOutValue, const EPropertyAssetType InAssetType, BOOL32& bOutActivated, BOOL32& bOutDeactivated)
		{
			BOOL32 bChanged = FALSE;
			for (UINT32 Index = 0u; Index < InOutValue.MapEntries.Num<UINT32>(); Index++)
			{
				ImGui::PushID(Index);
				FPropertyMapEntry& Entry = InOutValue.MapEntries[Index];
				BOOL32 bKeyActivated = FALSE;
				BOOL32 bKeyDeactivated = FALSE;
				ImGui::PushID("Key");
				if (DrawElementWidget(Entry.Key, InOutValue.KeyType, InOutValue.KeyEnumHash,
					InOutValue.KeyStructHash, InAssetType, bKeyActivated, bKeyDeactivated))
				{
					bChanged = TRUE;
				}
				ImGui::PopID();
				ImGui::SameLine();
				BOOL32 bValueActivated = FALSE;
				BOOL32 bValueDeactivated = FALSE;
				ImGui::PushID("Value");
				if (DrawElementWidget(Entry.Value, InOutValue.MapValueType, InOutValue.MapValueEnumHash,
					InOutValue.MapValueStructHash, InAssetType, bValueActivated, bValueDeactivated))
				{
					bChanged = TRUE;
				}
				ImGui::PopID();
				bOutActivated = (bOutActivated || bKeyActivated || bValueActivated) ? TRUE : FALSE;
				bOutDeactivated = (bOutDeactivated || bKeyDeactivated || bValueDeactivated) ? TRUE : FALSE;
				ImGui::SameLine();
				if (ImGui::SmallButton("X"))
				{
					InOutValue.MapEntries.RemoveAt(Index);
					bChanged = TRUE;
					ImGui::PopID();
					break;
				}
				ImGui::PopID();
			}
			if (ImGui::SmallButton("Add Entry"))
			{
				FPropertyMapEntry Entry;
				InitializeDefaultElement(Entry.Key, InOutValue.KeyShape.get());
				InitializeDefaultElement(Entry.Value, InOutValue.ValueShape.get());
				if (Entry.Key.Type == EPropertyValueType::STRING)
				{
					Entry.Key.StringValue = EString(PE_TEXT("Key")) + EString::FromInt(InOutValue.MapEntries.Num<UINT32>());
				}
				else if ((Entry.Key.Type != EPropertyValueType::ENUM) && (Entry.Key.Type != EPropertyValueType::ARRAY) && (Entry.Key.Type != EPropertyValueType::MAP))
				{
					Entry.Key.IntValue = static_cast<INT64>(InOutValue.MapEntries.Num<UINT32>());
				}
				InOutValue.MapEntries.Add(Entry);
				bChanged = TRUE;
			}
			return bChanged;
		}

		BOOL32 DrawNestedValue(FPropertyValue& InOutValue, const EPropertyAssetType InAssetType, BOOL32& bOutActivated, BOOL32& bOutDeactivated)
		{
			if (InOutValue.Type == EPropertyValueType::STRUCT)
			{
				const FStructDescriptor* Descriptor = EStructPropertyRegistry::Find(InOutValue.StructHash);
				if (!Descriptor)
				{
					return FALSE;
				}
				return DrawStructFields(InOutValue, *Descriptor, bOutActivated, bOutDeactivated);
			}
			if (InOutValue.Type == EPropertyValueType::ARRAY)
			{
				return DrawNestedArray(InOutValue, InAssetType, bOutActivated, bOutDeactivated);
			}
			if (InOutValue.Type == EPropertyValueType::MAP)
			{
				return DrawNestedMap(InOutValue, InAssetType, bOutActivated, bOutDeactivated);
			}
			return DrawElementWidget(static_cast<FPropertyElement&>(InOutValue), InOutValue.Type, InOutValue.EnumHash,
				InOutValue.StructHash, InAssetType, bOutActivated, bOutDeactivated);
		}

		BOOL32 DrawStructFields(FPropertyValue& InOutValue, const FStructDescriptor& InDescriptor, BOOL32& bOutActivated, BOOL32& bOutDeactivated)
		{
			BOOL32 bChanged = FALSE;
			for (UINT32 Index = 0u, Num = InDescriptor.Fields.Num<UINT32>(); Index < Num; Index++)
			{
				const FProperty& Field = InDescriptor.Fields[Index];
				const UINT32 VisibleFlags = static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE) |
					static_cast<UINT32>(EPropertyFlagsType::VISIBLE_ANYWHERE);
				if ((!Field.GetValue) || (Field.ValueType == EPropertyValueType::CUSTOM) ||
					((Field.Flags & static_cast<UINT32>(EPropertyFlagsType::TRANSIENT)) != 0u) ||
					((Field.Flags & VisibleFlags) == 0u))
				{
					continue;
				}
				if ((Index >= InOutValue.StructFields.Num<UINT32>()) || (!InOutValue.StructFields[Index]))
				{
					continue;
				}
				FPropertyValue& FieldValue = *InOutValue.StructFields[Index];
				ImGui::PushID(Index);
				if (!Field.SetValue)
				{
					ImGui::BeginDisabled();
				}
				BOOL32 bFieldChanged = FALSE;
				BOOL32 bFieldActivated = FALSE;
				BOOL32 bFieldDeactivated = FALSE;
				if ((Field.ValueType == EPropertyValueType::STRUCT) || (Field.ValueType == EPropertyValueType::ARRAY) ||
					(Field.ValueType == EPropertyValueType::MAP))
				{
					if (ImGui::TreeNodeEx(*Field.Name, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
					{
						bFieldChanged = DrawNestedValue(FieldValue, Field.AssetType, bFieldActivated, bFieldDeactivated);
						ImGui::TreePop();
					}
				}
				else
				{
					ImGui::TextUnformatted(*Field.Name);
					ImGui::SameLine();
					ImGui::SetNextItemWidth(-1.0f);
					bFieldChanged = DrawElementWidget(static_cast<FPropertyElement&>(FieldValue), Field.ValueType,
						Field.EnumHash, Field.StructHash, Field.AssetType, bFieldActivated, bFieldDeactivated);
				}
				const BOOL32 bFieldActive = ImGui::IsItemActive();
				if (!Field.SetValue)
				{
					ImGui::EndDisabled();
				}
				ImGui::PopID();
				bOutActivated = (bOutActivated || bFieldActivated) ? TRUE : FALSE;
				bOutDeactivated = (bOutDeactivated || bFieldDeactivated) ? TRUE : FALSE;
				// COMMIT_ONLY fields mutate their cached value while dragging but only ask
				// the owning property to apply it when the control releases.
				if ((Field.Flags & static_cast<UINT32>(EPropertyFlagsType::COMMIT_ONLY)) != 0u)
				{
					if (bFieldDeactivated)
					{
						bChanged = TRUE;
					}
					else if (bFieldChanged && (!bFieldActive) && (!bFieldActivated))
					{
						bChanged = TRUE;
					}
					continue;
				}
				if (bFieldChanged)
				{
					bChanged = TRUE;
				}
			}
			return bChanged;
		}

		BOOL32 DrawElementWidget(FPropertyElement& InOutElement, const EPropertyValueType InType, const size_t InEnumHash,
			const size_t InStructHash, const EPropertyAssetType InAssetType, BOOL32& bOutActivated, BOOL32& bOutDeactivated)
		{
			bOutActivated = FALSE;
			bOutDeactivated = FALSE;
			switch (InType)
			{
			case EPropertyValueType::BOOL:
			{
				BOOL32 bValue = (InOutElement.BoolValue != FALSE) ? TRUE : FALSE;
				const BOOL32 bChanged = ImGui::Checkbox("##element", &bValue);
				if (bChanged)
				{
					InOutElement.BoolValue = bValue;
				}
				return bChanged;
			}
			case EPropertyValueType::INT32:
			case EPropertyValueType::UINT32:
			{
				INT32 Value = static_cast<INT32>(InOutElement.IntValue);
				const BOOL32 bChanged = ImGui::DragInt("##element", &Value, 1.0f);
				bOutActivated = ImGui::IsItemActivated();
				bOutDeactivated = ImGui::IsItemDeactivatedAfterEdit();
				if (bChanged)
				{
					InOutElement.IntValue = static_cast<INT64>(Value);
				}
				return bChanged;
			}
			case EPropertyValueType::ENUM:
			{
				const size_t EffectiveEnumHash = (InEnumHash != 0u) ? InEnumHash : InOutElement.EnumHash;
				TArray<FEnumEntry> Entries;
				EEnumRegistry::GetValues(EffectiveEnumHash, Entries);
				if (Entries.Num<UINT32>() == 0u)
				{
					INT32 Value = static_cast<INT32>(InOutElement.IntValue);
					const BOOL32 bChanged = ImGui::DragInt("##element", &Value, 1.0f);
					bOutActivated = ImGui::IsItemActivated();
					bOutDeactivated = ImGui::IsItemDeactivatedAfterEdit();
					if (bChanged)
					{
						InOutElement.IntValue = static_cast<INT64>(Value);
					}
					return bChanged;
				}
				EString CurrentName;
				if (!EEnumRegistry::TryValueToString(EffectiveEnumHash, static_cast<INT32>(InOutElement.IntValue), CurrentName))
				{
					CurrentName = EString::FromInt(static_cast<UINT32>(InOutElement.IntValue));
				}
				BOOL32 bChanged = FALSE;
				if (ImGui::BeginCombo("##element", *CurrentName))
				{
					for (UINT32 Index = 0u, Num = Entries.Num<UINT32>(); Index < Num; Index++)
					{
						const BOOL32 bSelected = (Entries[Index].Value == static_cast<INT32>(InOutElement.IntValue)) ? TRUE : FALSE;
						if (ImGui::Selectable(*Entries[Index].Name, bSelected))
						{
							InOutElement.IntValue = static_cast<INT64>(Entries[Index].Value);
							bChanged = TRUE;
						}
						if (bSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				return bChanged;
			}
			case EPropertyValueType::INT64:
			case EPropertyValueType::UINT64:
			{
				const BOOL32 bChanged = ImGui::DragScalar("##element", ImGuiDataType_S64, &InOutElement.IntValue, 1.0);
				bOutActivated = ImGui::IsItemActivated();
				bOutDeactivated = ImGui::IsItemDeactivatedAfterEdit();
				return bChanged;
			}
			case EPropertyValueType::FLOAT:
			{
				FLOAT Value = static_cast<FLOAT>(InOutElement.DoubleValue);
				const BOOL32 bChanged = ImGui::DragFloat("##element", &Value, 0.01f);
				bOutActivated = ImGui::IsItemActivated();
				bOutDeactivated = ImGui::IsItemDeactivatedAfterEdit();
				if (bChanged)
				{
					InOutElement.DoubleValue = static_cast<DOUBLE>(Value);
				}
				return bChanged;
			}
			case EPropertyValueType::DOUBLE:
			{
				const BOOL32 bChanged = ImGui::DragScalar("##element", ImGuiDataType_Double, &InOutElement.DoubleValue, 0.01);
				bOutActivated = ImGui::IsItemActivated();
				bOutDeactivated = ImGui::IsItemDeactivatedAfterEdit();
				return bChanged;
			}
			case EPropertyValueType::STRING:
			{
				if (InAssetType != EPropertyAssetType::NONE)
				{
					EString SelectedName = InOutElement.StringValue;
					if (EPropertyEditor::DrawAssetCombo("##element", InAssetType, SelectedName))
					{
						InOutElement.StringValue = SelectedName;
						return TRUE;
					}
					return FALSE;
				}
				CHAR Buffer[512] = {};
				const UINT32 Length = EMath::Min(InOutElement.StringValue.Length(), static_cast<UINT32>(sizeof(Buffer) - 1u));
				if (Length > 0u)
				{
					memcpy(Buffer, *InOutElement.StringValue, Length);
				}
				const BOOL32 bChanged = ImGui::InputText("##element", Buffer, sizeof(Buffer));
				bOutActivated = ImGui::IsItemActivated();
				bOutDeactivated = ImGui::IsItemDeactivatedAfterEdit();
				if (bChanged)
				{
					InOutElement.StringValue = Buffer;
				}
				return bChanged;
			}
			case EPropertyValueType::VECTOR2:
			{
				FLOAT Values[2] = { InOutElement.Vector2Value.x, InOutElement.Vector2Value.y };
				const BOOL32 bChanged = ImGui::DragFloat2("##element", Values, 0.01f);
				bOutActivated = ImGui::IsItemActivated();
				bOutDeactivated = ImGui::IsItemDeactivatedAfterEdit();
				if (bChanged)
				{
					InOutElement.Vector2Value = Vector2(Values[0], Values[1]);
				}
				return bChanged;
			}
			case EPropertyValueType::VECTOR2INT:
			{
				INT32 Values[2] = { InOutElement.Vector2IntValue.x, InOutElement.Vector2IntValue.y };
				const BOOL32 bChanged = ImGui::DragInt2("##element", Values);
				bOutActivated = ImGui::IsItemActivated();
				bOutDeactivated = ImGui::IsItemDeactivatedAfterEdit();
				if (bChanged)
				{
					InOutElement.Vector2IntValue = Vector2Int(Values[0], Values[1]);
				}
				return bChanged;
			}
			case EPropertyValueType::VECTOR3:
			{
				FLOAT Values[3] = { InOutElement.Vector3Value.x, InOutElement.Vector3Value.y, InOutElement.Vector3Value.z };
				const BOOL32 bChanged = ImGui::DragFloat3("##element", Values, 0.01f);
				bOutActivated = ImGui::IsItemActivated();
				bOutDeactivated = ImGui::IsItemDeactivatedAfterEdit();
				if (bChanged)
				{
					InOutElement.Vector3Value = Vector3(Values[0], Values[1], Values[2]);
				}
				return bChanged;
			}
			case EPropertyValueType::QUATERNION:
			{
				FLOAT Values[4] = { InOutElement.QuaternionValue.x, InOutElement.QuaternionValue.y, InOutElement.QuaternionValue.z, InOutElement.QuaternionValue.w };
				const BOOL32 bChanged = ImGui::DragFloat4("##element", Values, 0.005f);
				bOutActivated = ImGui::IsItemActivated();
				bOutDeactivated = ImGui::IsItemDeactivatedAfterEdit();
				if (bChanged)
				{
					InOutElement.QuaternionValue = Quaternion(Values[0], Values[1], Values[2], Values[3]);
				}
				return bChanged;
			}
			case EPropertyValueType::COLOR3:
			{
				FLOAT Values[3] = { InOutElement.ColorValue.x, InOutElement.ColorValue.y, InOutElement.ColorValue.z };
				const BOOL32 bChanged = ImGui::DragFloat3("##element", Values, 0.005f, 0.0f, 1.0f);
				bOutActivated = ImGui::IsItemActivated();
				bOutDeactivated = ImGui::IsItemDeactivatedAfterEdit();
				if (bChanged)
				{
					InOutElement.ColorValue = Color3(Values[0], Values[1], Values[2]);
				}
				return bChanged;
			}
			case EPropertyValueType::TRANSFORM:
			{
				ETransform& Transform = InOutElement.TransformValue;
				Vector3 Location = Transform.GetLocation_Local();
				Quaternion Rotation = Transform.GetRotation_Local();
				Vector3 Scaling = Transform.GetScaling_Local();
				FLOAT LocationValues[3] = { Location.x, Location.y, Location.z };
				FLOAT RotationValues[4] = { Rotation.x, Rotation.y, Rotation.z, Rotation.w };
				FLOAT ScalingValues[3] = { Scaling.x, Scaling.y, Scaling.z };
				BOOL32 bChanged = FALSE;
				if (ImGui::DragFloat3("##elementLoc", LocationValues, 0.01f))
				{
					Transform.SetLocation_Local(Vector3(LocationValues[0], LocationValues[1], LocationValues[2]));
					bChanged = TRUE;
				}
				bOutActivated = (bOutActivated || (ImGui::IsItemActivated() != FALSE)) ? TRUE : FALSE;
				bOutDeactivated = (bOutDeactivated || (ImGui::IsItemDeactivatedAfterEdit() != FALSE)) ? TRUE : FALSE;
				if (ImGui::DragFloat4("##elementRot", RotationValues, 0.005f))
				{
					Transform.SetRotation_Local(Quaternion(RotationValues[0], RotationValues[1], RotationValues[2], RotationValues[3]));
					bChanged = TRUE;
				}
				bOutActivated = (bOutActivated || (ImGui::IsItemActivated() != FALSE)) ? TRUE : FALSE;
				bOutDeactivated = (bOutDeactivated || (ImGui::IsItemDeactivatedAfterEdit() != FALSE)) ? TRUE : FALSE;
				if (ImGui::DragFloat3("##elementScale", ScalingValues, 0.01f))
				{
					Transform.SetScaling_Local(Vector3(ScalingValues[0], ScalingValues[1], ScalingValues[2]));
					bChanged = TRUE;
				}
				bOutActivated = (bOutActivated || (ImGui::IsItemActivated() != FALSE)) ? TRUE : FALSE;
				bOutDeactivated = (bOutDeactivated || (ImGui::IsItemDeactivatedAfterEdit() != FALSE)) ? TRUE : FALSE;
				return bChanged;
			}
			case EPropertyValueType::STRUCT:
			{
				const size_t EffectiveStructHash = (InStructHash != 0u) ? InStructHash : InOutElement.StructHash;
				const FStructDescriptor* Descriptor = EStructPropertyRegistry::Find(EffectiveStructHash);
				if ((!Descriptor) || (!InOutElement.ContainerValue))
				{
					return FALSE;
				}
				return DrawStructFields(*InOutElement.ContainerValue, *Descriptor, bOutActivated, bOutDeactivated);
			}
			case EPropertyValueType::ARRAY:
			{
				if (!InOutElement.ContainerValue)
				{
					return FALSE;
				}
				return DrawNestedArray(*InOutElement.ContainerValue, InAssetType, bOutActivated, bOutDeactivated);
			}
			case EPropertyValueType::MAP:
			{
				if (!InOutElement.ContainerValue)
				{
					return FALSE;
				}
				return DrawNestedMap(*InOutElement.ContainerValue, InAssetType, bOutActivated, bOutDeactivated);
			}
			default:
				return FALSE;
			}
		}

		void InitializeDefaultElement(FPropertyElement& OutElement, const FPropertyValue* InShape)
		{
			OutElement = FPropertyElement();
			if (!InShape)
			{
				return;
			}
			OutElement.Type = InShape->Type;
			OutElement.EnumHash = InShape->EnumHash;
			OutElement.StructHash = InShape->StructHash;
			if (InShape->Type == EPropertyValueType::STRUCT)
			{
				// Clone the complete shape: every array element needs its own field tree,
				// otherwise editing one element would mutate all of them.
				OutElement.ContainerValue = TSharedPtr<FPropertyValue>(new FPropertyValue());
				ClonePropertyValue(*InShape, *OutElement.ContainerValue);
			}
			else if ((InShape->Type == EPropertyValueType::ARRAY) || (InShape->Type == EPropertyValueType::MAP))
			{
				OutElement.ContainerValue = TSharedPtr<FPropertyValue>(new FPropertyValue());
				ClonePropertyValue(*InShape, *OutElement.ContainerValue);
				OutElement.ContainerValue->ArrayValues.Empty();
				OutElement.ContainerValue->MapEntries.Empty();
			}
			else if (InShape->Type == EPropertyValueType::ENUM)
			{
				TArray<FEnumEntry> Entries;
				EEnumRegistry::GetValues(OutElement.EnumHash, Entries);
				if (Entries.Num<UINT32>() > 0u)
				{
					OutElement.IntValue = static_cast<INT64>(Entries[0].Value);
				}
			}
		}

		BOOL32 DrawArrayProperty(const FProperty& InProperty, void* InObject, const size_t& InClassHash, FPropertyValue& InOutValue)
		{
			BOOL32 bChanged = FALSE;
			for (UINT32 Index = 0u; Index < InOutValue.ArrayValues.Num<UINT32>(); Index++)
			{
				ImGui::PushID(Index);
				FPropertyValue BeforeElement;
				InProperty.GetValue(InObject, BeforeElement);
				BOOL32 bActivated = FALSE;
				BOOL32 bDeactivated = FALSE;
				const BOOL32 bElementChanged = DrawElementWidget(InOutValue.ArrayValues[Index], InProperty.ElementType,
					InProperty.ElementEnumHash, InProperty.ElementStructHash, InProperty.AssetType, bActivated, bDeactivated);
				if (bActivated)
				{
					StoreEditStart(InProperty, BeforeElement);
				}
				if (bElementChanged)
				{
					InProperty.SetValue(InObject, InOutValue);
					bChanged = TRUE;
				}
				if (bDeactivated)
				{
					FPropertyValue Start;
					if (TakeEditStart(InProperty, Start))
					{
						Commit(InProperty, InObject, InClassHash, Start, InOutValue);
					}
				}
				else if (bElementChanged && (!bActivated))
				{
					// Checkbox/string-style controls commit immediately.
					Commit(InProperty, InObject, InClassHash, BeforeElement, InOutValue);
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("X"))
				{
					FPropertyValue BeforeArray;
					InProperty.GetValue(InObject, BeforeArray);
					InOutValue.ArrayValues.RemoveAt(Index);
					InProperty.SetValue(InObject, InOutValue);
					Commit(InProperty, InObject, InClassHash, BeforeArray, InOutValue);
					bChanged = TRUE;
					ImGui::PopID();
					break;
				}
				ImGui::PopID();
			}
			if (ImGui::SmallButton("Add Element"))
			{
				FPropertyValue BeforeArray;
				InProperty.GetValue(InObject, BeforeArray);
				FPropertyElement NewElement;
				InitializeDefaultElement(NewElement, InOutValue.ElementShape.get());
				InOutValue.ArrayValues.Add(NewElement);
				InProperty.SetValue(InObject, InOutValue);
				Commit(InProperty, InObject, InClassHash, BeforeArray, InOutValue);
				bChanged = TRUE;
			}
			return bChanged;
		}

		BOOL32 DrawMapProperty(const FProperty& InProperty, void* InObject, const size_t& InClassHash, FPropertyValue& InOutValue)
		{
			BOOL32 bChanged = FALSE;
			for (UINT32 Index = 0u; Index < InOutValue.MapEntries.Num<UINT32>(); Index++)
			{
				ImGui::PushID(Index);
				FPropertyMapEntry& Entry = InOutValue.MapEntries[Index];
				FPropertyValue BeforeEntry;
				InProperty.GetValue(InObject, BeforeEntry);
				BOOL32 bKeyActivated = FALSE;
				BOOL32 bKeyDeactivated = FALSE;
				ImGui::PushID("Key");
				const BOOL32 bKeyChanged = DrawElementWidget(Entry.Key, InProperty.KeyType, InProperty.KeyEnumHash,
					InProperty.KeyStructHash, InProperty.AssetType, bKeyActivated, bKeyDeactivated);
				ImGui::PopID();
				ImGui::SameLine();
				BOOL32 bValueActivated = FALSE;
				BOOL32 bValueDeactivated = FALSE;
				ImGui::PushID("Value");
				const BOOL32 bValueChanged = DrawElementWidget(Entry.Value, InProperty.MapValueType, InProperty.MapValueEnumHash,
					InProperty.MapValueStructHash, InProperty.AssetType, bValueActivated, bValueDeactivated);
				ImGui::PopID();
				if (bKeyActivated || bValueActivated)
				{
					StoreEditStart(InProperty, BeforeEntry);
				}
				if (bKeyChanged || bValueChanged)
				{
					InProperty.SetValue(InObject, InOutValue);
					bChanged = TRUE;
				}
				if (bKeyDeactivated || bValueDeactivated)
				{
					FPropertyValue Start;
					if (TakeEditStart(InProperty, Start))
					{
						Commit(InProperty, InObject, InClassHash, Start, InOutValue);
					}
				}
				else if ((bKeyChanged || bValueChanged) && (!bKeyActivated) && (!bValueActivated))
				{
					Commit(InProperty, InObject, InClassHash, BeforeEntry, InOutValue);
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("X"))
				{
					FPropertyValue BeforeMap;
					InProperty.GetValue(InObject, BeforeMap);
					InOutValue.MapEntries.RemoveAt(Index);
					InProperty.SetValue(InObject, InOutValue);
					Commit(InProperty, InObject, InClassHash, BeforeMap, InOutValue);
					bChanged = TRUE;
					ImGui::PopID();
					break;
				}
				ImGui::PopID();
			}
			if (ImGui::SmallButton("Add Entry"))
			{
				FPropertyValue BeforeMap;
				InProperty.GetValue(InObject, BeforeMap);
				FPropertyMapEntry Entry;
				InitializeDefaultElement(Entry.Key, InOutValue.KeyShape.get());
				if (Entry.Key.Type == EPropertyValueType::STRING)
				{
					Entry.Key.StringValue = EString(PE_TEXT("Key")) + EString::FromInt(InOutValue.MapEntries.Num<UINT32>());
				}
				else if ((Entry.Key.Type != EPropertyValueType::ENUM) && (Entry.Key.Type != EPropertyValueType::ARRAY) && (Entry.Key.Type != EPropertyValueType::MAP))
				{
					Entry.Key.IntValue = static_cast<INT64>(InOutValue.MapEntries.Num<UINT32>());
				}
				InitializeDefaultElement(Entry.Value, InOutValue.ValueShape.get());
				InOutValue.MapEntries.Add(Entry);
				InProperty.SetValue(InObject, InOutValue);
				Commit(InProperty, InObject, InClassHash, BeforeMap, InOutValue);
				bChanged = TRUE;
			}
			return bChanged;
		}

		BOOL32 DrawStructProperty(const FProperty& InProperty, void* InObject, const size_t& InClassHash, FPropertyValue& InOutValue)
		{
			const FStructDescriptor* Descriptor = EStructPropertyRegistry::Find(InProperty.StructHash);
			if (!Descriptor)
			{
				return FALSE;
			}
			FPropertyValue Before;
			InProperty.GetValue(InObject, Before);
			BOOL32 bActivated = FALSE;
			BOOL32 bDeactivated = FALSE;
			BOOL32 bChanged = FALSE;
			if (ImGui::TreeNodeEx(*InProperty.Name, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
			{
				bChanged = DrawStructFields(InOutValue, *Descriptor, bActivated, bDeactivated);
				ImGui::TreePop();
			}
			if (bActivated)
			{
				StoreEditStart(InProperty, Before);
			}
			if (bChanged)
			{
				InProperty.SetValue(InObject, InOutValue);
			}
			if (bDeactivated)
			{
				FPropertyValue Start;
				if (TakeEditStart(InProperty, Start))
				{
					Commit(InProperty, InObject, InClassHash, Start, InOutValue);
				}
			}
			else if (bChanged && (!bActivated))
			{
				// Checkbox/asset/enum controls apply on selection, without a drag lifetime.
				Commit(InProperty, InObject, InClassHash, Before, InOutValue);
			}
			return bChanged;
		}

		BOOL32 DrawProperty(const FProperty& InProperty, void* InObject, const size_t& InClassHash)
		{
			if ((InProperty.ValueType == EPropertyValueType::CUSTOM) ||
				(!InProperty.GetValue) || (!InProperty.SetValue) ||
				((InProperty.Flags & static_cast<UINT32>(EPropertyFlagsType::TRANSIENT)) != 0u) ||
				((InProperty.Flags & static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE)) == 0u))
			{
				return FALSE;
			}

			FPropertyValue Value;
			InProperty.GetValue(InObject, Value);

			ImGui::PushID(&InProperty);
			if (InProperty.ValueType == EPropertyValueType::STRUCT)
			{
				const BOOL32 bChanged = DrawStructProperty(InProperty, InObject, InClassHash, Value);
				ImGui::PopID();
				return bChanged;
			}
			ImGui::TextUnformatted(*InProperty.Name);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-1.0f);

			BOOL32 bChanged = FALSE;
			switch (InProperty.ValueType)
			{
			case EPropertyValueType::BOOL:
				bChanged = DrawBoolProperty(InProperty, InObject, InClassHash, Value);
				break;
			case EPropertyValueType::INT32:
				bChanged = DrawIntProperty(InProperty, InObject, InClassHash, Value, FALSE);
				break;
			case EPropertyValueType::UINT32:
				bChanged = DrawIntProperty(InProperty, InObject, InClassHash, Value, TRUE);
				break;
			case EPropertyValueType::INT64:
			case EPropertyValueType::UINT64:
				bChanged = DrawIntProperty(InProperty, InObject, InClassHash, Value, InProperty.ValueType == EPropertyValueType::UINT64);
				break;
			case EPropertyValueType::FLOAT:
				bChanged = DrawFloatProperty(InProperty, InObject, InClassHash, Value);
				break;
			case EPropertyValueType::DOUBLE:
				bChanged = DrawDoubleProperty(InProperty, InObject, InClassHash, Value);
				break;
			case EPropertyValueType::STRING:
				bChanged = DrawStringProperty(InProperty, InObject, InClassHash, Value);
				break;
			case EPropertyValueType::ENUM:
				bChanged = DrawEnumProperty(InProperty, InObject, InClassHash, Value);
				break;
			case EPropertyValueType::VECTOR2:
				bChanged = DrawVector2Property(InProperty, InObject, InClassHash, Value);
				break;
			case EPropertyValueType::VECTOR2INT:
				bChanged = DrawVector2IntProperty(InProperty, InObject, InClassHash, Value);
				break;
			case EPropertyValueType::VECTOR3:
				bChanged = DrawVector3Property(InProperty, InObject, InClassHash, Value);
				break;
			case EPropertyValueType::QUATERNION:
				bChanged = DrawQuaternionProperty(InProperty, InObject, InClassHash, Value);
				break;
			case EPropertyValueType::COLOR3:
				bChanged = DrawColor3Property(InProperty, InObject, InClassHash, Value);
				break;
			case EPropertyValueType::TRANSFORM:
				bChanged = DrawTransformProperty(InProperty, InObject, InClassHash, Value);
				break;
			case EPropertyValueType::ARRAY:
				bChanged = DrawArrayProperty(InProperty, InObject, InClassHash, Value);
				break;
			case EPropertyValueType::MAP:
				bChanged = DrawMapProperty(InProperty, InObject, InClassHash, Value);
				break;
			default:
				break;
			}

			ImGui::PopID();
			return bChanged;
		}
	}

	void EPropertyEditor::SetPropertyCommittedCallback(const FOnPropertyCommitted& InCallback)
	{
		PropertyCommittedCallback = InCallback;
	}

	void EPropertyEditor::MakeDefaultElement(const FPropertyValue* InShape, FPropertyElement& OutElement)
	{
		InitializeDefaultElement(OutElement, InShape);
	}

	void EPropertyEditor::CollectAssetNames(const EPropertyAssetType InAssetType, TArray<EString>& OutNames)
	{
		OutNames.Empty();
		if (InAssetType == EPropertyAssetType::NONE)
		{
			return;
		}
		FAssetNameCache& Cache = FindAssetNameCache(InAssetType);
		RefreshAssetNames(Cache);
		OutNames = Cache.Names;
	}

	BOOL32 EPropertyEditor::DrawAssetCombo(const CHAR* InLabel, const EPropertyAssetType InAssetType, EString& InOutAssetName)
	{
		if (InAssetType == EPropertyAssetType::NONE)
		{
			return FALSE;
		}
		FAssetNameCache& Cache = FindAssetNameCache(InAssetType);
		RefreshAssetNames(Cache);

		const CHAR* Preview = (InOutAssetName.Length() > 0u) ? *InOutAssetName : "None";
		if (!ImGui::BeginCombo(InLabel, Preview))
		{
			return FALSE;
		}

		BOOL32 bChanged = FALSE;
		const BOOL32 bNoneSelected = (InOutAssetName.Length() == 0u) ? TRUE : FALSE;
		if (ImGui::Selectable("None", bNoneSelected))
		{
			InOutAssetName = EString();
			bChanged = TRUE;
		}
		if (bNoneSelected)
		{
			ImGui::SetItemDefaultFocus();
		}
		for (UINT32 Index = 0u, Num = Cache.Names.Num<UINT32>(); Index < Num; Index++)
		{
			const EString& AssetName = Cache.Names[Index];
			const BOOL32 bSelected = (AssetName == InOutAssetName) ? TRUE : FALSE;
			if (ImGui::Selectable(*AssetName, bSelected))
			{
				InOutAssetName = AssetName;
				bChanged = TRUE;
			}
			if (bSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
		return bChanged;
	}

	void EPropertyEditor::NotifyPropertyChanged(void* InObject, const size_t& InClassHash, const EString& InPropertyName,
		const FPropertyValue& InBefore, const FPropertyValue& InAfter)
	{
		if ((InObject) && (PropertyCommittedCallback))
		{
			PropertyCommittedCallback(InObject, InClassHash, InPropertyName, InBefore, InAfter);
		}
	}

	BOOL32 EPropertyEditor::DrawProperties(void* InObject, const size_t& InClassHash)
	{
		BOOL32 bAnyChanged = FALSE;
		EClassPropertyRegistry::ForEachProperty(InClassHash, [&InObject, &InClassHash, &bAnyChanged](const FProperty& Property) -> void
		{
			if (DrawProperty(Property, InObject, InClassHash))
			{
				bAnyChanged = TRUE;
			}
		});
		return bAnyChanged;
	}
}

#endif
