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
