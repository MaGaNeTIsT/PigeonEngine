#include "PropertyEditor.h"

#if ENGINE_EDITOR_ONLY

#include <imgui.h>

namespace PigeonEngine
{
	namespace
	{
		EPropertyEditor::FOnPropertyCommitted PropertyCommittedCallback;
		// Values at the start of the current drag/input, keyed by property name. Only one
		// property is interacted with at a time, so a name key is enough and avoids storing
		// a per-object map.
		TMap<EString, FPropertyValue> EditStartValues;

		void Commit(const FProperty& InProperty, void* InObject, const size_t& InClassHash, const FPropertyValue& InBefore, const FPropertyValue& InAfter)
		{
			if (PropertyCommittedCallback)
			{
				PropertyCommittedCallback(InObject, InClassHash, InProperty.Name, InBefore, InAfter);
			}
		}

		void StoreEditStart(const FProperty& InProperty, const FPropertyValue& InValue)
		{
			EditStartValues.Add(InProperty.Name, InValue);
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

		BOOL32 DrawElementWidget(FPropertyElement& InOutElement, const EPropertyValueType InType, BOOL32& bOutActivated, BOOL32& bOutDeactivated)
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
			case EPropertyValueType::ENUM:
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
			default:
				return FALSE;
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
				const BOOL32 bElementChanged = DrawElementWidget(InOutValue.ArrayValues[Index], InProperty.ElementType, bActivated, bDeactivated);
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
				NewElement.Type = InProperty.ElementType;
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
				const BOOL32 bKeyChanged = DrawElementWidget(Entry.Key, InProperty.KeyType, bKeyActivated, bKeyDeactivated);
				ImGui::SameLine();
				BOOL32 bValueActivated = FALSE;
				BOOL32 bValueDeactivated = FALSE;
				const BOOL32 bValueChanged = DrawElementWidget(Entry.Value, InProperty.MapValueType, bValueActivated, bValueDeactivated);
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
				Entry.Key.Type = InProperty.KeyType;
				if (InProperty.KeyType == EPropertyValueType::STRING)
				{
					Entry.Key.StringValue = EString(PE_TEXT("Key")) + EString::FromInt(InOutValue.MapEntries.Num<UINT32>());
				}
				else
				{
					Entry.Key.IntValue = static_cast<INT64>(InOutValue.MapEntries.Num<UINT32>());
				}
				Entry.Value.Type = InProperty.MapValueType;
				InOutValue.MapEntries.Add(Entry);
				InProperty.SetValue(InObject, InOutValue);
				Commit(InProperty, InObject, InClassHash, BeforeMap, InOutValue);
				bChanged = TRUE;
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
			const FPropertyValue Before = Value;

			ImGui::PushID(&InProperty);
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
			(void)Before;
			return bChanged;
		}
	}

	void EPropertyEditor::SetPropertyCommittedCallback(const FOnPropertyCommitted& InCallback)
	{
		PropertyCommittedCallback = InCallback;
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
