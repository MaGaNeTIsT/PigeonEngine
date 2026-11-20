#pragma once

#include <CoreMinimal.h>
#include <Base/DataStructure/Transform.h>
#include <type_traits>
#include "Archive.h"
#include "EnumRegistry.h"

namespace PigeonEngine
{

	// Property behaviour flags. Serialization skips TRANSIENT and EDITOR_ONLY; the edit
	// flags are consumed by the reflection-driven Details panel.
	enum class EPropertyFlagsType : UINT32
	{
		NONE = 0u,
		EDIT_ANYWHERE = (1u << 0),
		VISIBLE_ANYWHERE = (1u << 1),
		TRANSIENT = (1u << 2),
		// Editable in the Details panel but absent from the file. Used for a view onto data
		// whose serialized form is a custom property (for example a mesh/skeleton pair).
		EDITOR_ONLY = (1u << 3),
		// Apply an edit when the widget releases it instead of once per frame. Used for
		// values whose setter is expensive or has side effects, such as an animation rate.
		COMMIT_ONLY = (1u << 4),
	};

	// What the value looks like to an editor. CUSTOM means the component's hand-written
	// detail panel owns it; everything else can be generated from the property table.
	enum class EPropertyValueType : UINT8
	{
		NONE = 0u,
		BOOL,
		INT32,
		UINT32,
		INT64,
		UINT64,
		FLOAT,
		DOUBLE,
		STRING,
		ENUM,
		VECTOR2,
		VECTOR2INT,
		VECTOR3,
		QUATERNION,
		COLOR3,
		TRANSFORM,
		ARRAY,
		MAP,
		STRUCT,
		CUSTOM,
	};

	// Asset families the Details panel can present as a picker. This is intentionally
	// editor-facing metadata rather than EAssetType: Property.h is compiled into non-editor
	// builds, where the asset headers are excluded.
	enum class EPropertyAssetType : UINT8
	{
		NONE = 0u,
		STATIC_MESH,
		SKELETAL_MESH,
		SKELETON,
		ANIMATION,
		CUBE_MAP,
		NAV_MESH,
		MATERIAL,
	};

	/*
	* One scalar editor value. Container elements use this type rather than FPropertyValue
	* itself: TArray<T> requires sizeof(T) when it is instantiated, so a recursive container
	* of FPropertyValue would need FPropertyValue to be complete inside its own definition.
	* A nested container is represented by a shared FPropertyValue subtree instead.
	*/
	struct FPropertyValue;
	struct FPropertyElement
	{
		EPropertyValueType Type = EPropertyValueType::NONE;
		BOOL32				BoolValue = FALSE;
		INT64				IntValue = 0;
		DOUBLE				DoubleValue = 0.0;
		EString				StringValue;
		Vector2				Vector2Value;
		Vector2Int			Vector2IntValue;
		Vector3				Vector3Value;
		Quaternion			QuaternionValue;
		Color3				ColorValue;
		ETransform			TransformValue;
		// Enum registry key for an enum-shaped value; container properties also carry their
		// registry keys separately for compatibility with existing callers.
		size_t				EnumHash = 0u;
		// Struct registry key for a STRUCT element.
		size_t				StructHash = 0u;
		// Set only for ARRAY / MAP elements. Shared because FPropertyElement is copied by
		// value through TArray and undo values, while the nested tree is read-only payload.
		TSharedPtr<FPropertyValue> ContainerValue;
	};

	struct FPropertyMapEntry
	{
		FPropertyElement Key;
		FPropertyElement Value;
	};

	/*
	* Editor-side value container. One struct for every supported type keeps the Details
	* panel and the undo commands type erased without depending on ImGui or on any single
	* component. GetValue/SetValue on FProperty fill and consume it.
	*/
	struct FPropertyValue : public FPropertyElement
	{
		// Container shape, mirrored from FProperty so a caller that only has a value (the
		// editor's undo commands) can still present or compare it.
		EPropertyValueType ElementType = EPropertyValueType::NONE;
		EPropertyValueType KeyType = EPropertyValueType::NONE;
		EPropertyValueType MapValueType = EPropertyValueType::NONE;
		// Enum registry keys for enum-typed container elements.
		size_t ElementEnumHash = 0u;
		size_t KeyEnumHash = 0u;
		size_t MapValueEnumHash = 0u;
		// Struct registry keys for struct-typed container elements.
		size_t ElementStructHash = 0u;
		size_t KeyStructHash = 0u;
		size_t MapValueStructHash = 0u;
		// Field values for a STRUCT value. Each field is itself a full FPropertyValue so
		// nested structs and containers recurse through the same representation.
		TArray<TSharedPtr<FPropertyValue>> StructFields;
		// Container payloads; empty for scalar types.
		TArray<FPropertyElement> ArrayValues;
		TArray<FPropertyMapEntry> MapEntries;
		// Shape metadata for an empty or newly added nested container. ElementShape describes
		// one array element; KeyShape/ValueShape describe map entries. A scalar shape has only
		// Type set.
		TSharedPtr<FPropertyValue> ElementShape;
		TSharedPtr<FPropertyValue> KeyShape;
		TSharedPtr<FPropertyValue> ValueShape;
	};

	/*
	* One reflected property of a class. Name is the stable key used in files; Serialize
	* is type-erased over the owning object so the registry never needs the concrete
	* class. This is the single description that the serializer, the editor Details
	* panel and (later) the archive system all consume - a property is registered once.
	*/
	class FProperty
	{
	public:
		EString Name;
		UINT32 Flags = static_cast<UINT32>(EPropertyFlagsType::NONE);
		// How the Details panel should present the value. CUSTOM falls back to the
		// component's own GenerateComponentDetail.
		EPropertyValueType ValueType = EPropertyValueType::NONE;
		// ARRAY element / MAP key and value types. Nested containers are CUSTOM.
		EPropertyValueType ElementType = EPropertyValueType::NONE;
		EPropertyValueType KeyType = EPropertyValueType::NONE;
		EPropertyValueType MapValueType = EPropertyValueType::NONE;
		// Enum registry keys for enum-typed container elements.
		size_t ElementEnumHash = 0u;
		size_t KeyEnumHash = 0u;
		size_t MapValueEnumHash = 0u;
		// Struct registry keys for struct-typed values and container elements.
		size_t StructHash = 0u;
		size_t ElementStructHash = 0u;
		size_t KeyStructHash = 0u;
		size_t MapValueStructHash = 0u;
		// For STRING properties that name an asset: which asset family the picker lists.
		// NONE means the regular text field is used.
		EPropertyAssetType AssetType = EPropertyAssetType::NONE;
		// Only meaningful for ENUM; the EEnumRegistry key.
		size_t EnumHash = 0u;
		// Serializes the member into/from the archive; InContainer is the owning object.
		TFunction<void(EArchive&, void*)> Serialize;
		// Editor-side access. Null for CUSTOM properties; read-only when SetValue is null.
		TFunction<void(const void*, FPropertyValue&)> GetValue;
		TFunction<void(void*, const FPropertyValue&)> SetValue;
	};

	// Editor metadata for one reflected struct. Fields reuse FProperty so nested structs,
	// containers, enum names and asset pickers all use the same metadata shape.
	struct FStructDescriptor
	{
		EString Name;
		size_t StructHash = 0u;
		TArray<FProperty> Fields;
	};

	class EStructPropertyRegistry final
	{
	public:
		static void AddField(const size_t& InStructHash, const EString& InStructName, const FProperty& InField);
		PE_NODISCARD static const FStructDescriptor* Find(const size_t& InStructHash);
	};

	// Copies a property value tree, including nested containers, struct fields and shapes.
	void ClonePropertyValue(const FPropertyValue& InValue, FPropertyValue& OutValue);

	/*
	* Per-class property tables, keyed by the runtime class hash (typeid hash - never
	* persisted; files use the stable class name). SerializeObject walks the class chain
	* root-first so base properties are written before derived ones and reading a file
	* produced by an older class layout leaves new fields at their constructor values.
	*/
	class EClassPropertyRegistry final
	{
	public:
		static void AddProperty(const size_t& InClassHash, const FProperty& InProperty);
		PE_NODISCARD static const TArray<FProperty>* FindProperties(const size_t& InClassHash);
		static void SerializeObject(EArchive& InArchive, const size_t& InClassHash, void* InContainer);
		// Walks the single-inheritance chain root first, including TRANSIENT properties; the
		// Details panel filters them out. Serialization and editing share the same order.
		static void ForEachProperty(const size_t& InClassHash, const TFunction<void(const FProperty&)>& InVisitor);
		// Applies one editor value by property name. Returns FALSE when the class has no such
		// property or the property is not editable.
		static BOOL32 SetPropertyValue(const size_t& InClassHash, void* InContainer, const EString& InName, const FPropertyValue& InValue);

		// Class version: bumped when a class's serialized layout changes. Files written by
		// an older version are read with the current property code (unknown fields are
		// skipped) and then the registered migrator patches the loaded values.
		static void SetClassVersion(const size_t& InClassHash, const UINT32& InVersion);
		PE_NODISCARD static UINT32 GetClassVersion(const size_t& InClassHash);
		static void SetClassMigrator(const size_t& InClassHash, const TFunction<void(void*, UINT32)>& InMigrator);
		PE_NODISCARD static const TFunction<void(void*, UINT32)>* FindClassMigrator(const size_t& InClassHash);
	};

	template <typename TClass>
	void SetClassMigration(const UINT32& InCurrentVersion, const TFunction<void(TClass&, UINT32)>& InMigrate)
	{
		const size_t ClassHash = GetClassHashCode<TClass>();
		EClassPropertyRegistry::SetClassVersion(ClassHash, InCurrentVersion);
		EClassPropertyRegistry::SetClassMigrator(ClassHash, [InMigrate](void* InContainer, UINT32 InFromVersion) -> void
		{
			InMigrate(*static_cast<TClass*>(InContainer), InFromVersion);
		});
	}

	// Value serialization. These are the one place that knows how a math type is stored,
	// so both the JSON and a future binary backend stay in sync through EArchive.
	BOOL32 SerializeVector2(EArchive& InArchive, const CHAR* InName, Vector2& InOutValue);
	BOOL32 SerializeVector2Int(EArchive& InArchive, const CHAR* InName, Vector2Int& InOutValue);
	BOOL32 SerializeVector3(EArchive& InArchive, const CHAR* InName, Vector3& InOutValue);
	BOOL32 SerializeQuaternion(EArchive& InArchive, const CHAR* InName, Quaternion& InOutValue);
	BOOL32 SerializeColor3(EArchive& InArchive, const CHAR* InName, Color3& InOutValue);
	BOOL32 SerializeTransform(EArchive& InArchive, const CHAR* InName, ETransform& InOutValue);

	/*
	* Enum values are stored by name. A value without a registered name falls back to the
	* integer, and a reader that finds an integer treats it as the legacy encoding - so the
	* integer-valued files written before the enum registry existed still load.
	*/
	void SerializeEnumValue(EArchive& InArchive, const CHAR* InName, INT32& InOutValue, const size_t& InEnumHash);

	// Overloads used by the property templates below.
	void SerializeValue(EArchive& InArchive, const CHAR* InName, BOOL32& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, INT32& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, UINT32& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, INT64& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, UINT64& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, FLOAT& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, DOUBLE& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, EString& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, Vector2& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, Vector2Int& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, Vector3& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, Quaternion& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, Color3& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, ETransform& InOutValue);

	// Containers. Element/key/value types must themselves support SerializeValue.
	template <typename TEnum, typename std::enable_if_t<std::is_enum_v<TEnum>, INT32> = 0>
	void SerializeValue(EArchive& InArchive, const CHAR* InName, TEnum& InOutValue)
	{
		INT32 Value = static_cast<INT32>(InOutValue);
		SerializeEnumValue(InArchive, InName, Value, GetClassHashCode<TEnum>());
		InOutValue = static_cast<TEnum>(Value);
	}

	template <typename TValue>
	void SerializeValue(EArchive& InArchive, const CHAR* InName, TArray<TValue>& InOutValues)
	{
		UINT32 Count = InOutValues.Num<UINT32>();
		InArchive.BeginArray(InName, Count);
		if (!InArchive.IsSaving())
		{
			InOutValues.Empty();
		}
		for (UINT32 Index = 0u; Index < Count; Index++)
		{
			const EString IndexKey = EString::FromInt(Index);
			TValue Value = InArchive.IsSaving() ? InOutValues[Index] : TValue();
			SerializeValue(InArchive, *IndexKey, Value);
			if (!InArchive.IsSaving())
			{
				InOutValues.Add(Value);
			}
		}
		InArchive.EndArray();
	}

	template <typename TKey, typename TValue>
	void SerializeValue(EArchive& InArchive, const CHAR* InName, TMap<TKey, TValue>& InOutValues)
	{
		UINT32 Count = InOutValues.Num<UINT32>();
		InArchive.BeginArray(InName, Count);
		if (InArchive.IsSaving())
		{
			UINT32 Index = 0u;
			for (typename TMap<TKey, TValue>::TConstIterator It = InOutValues.Begin(); It != InOutValues.End(); ++It, ++Index)
			{
				const EString IndexKey = EString::FromInt(Index);
				InArchive.BeginObject(*IndexKey);
				TKey Key = It->first;
				SerializeValue(InArchive, PE_TEXT("Key"), Key);
				TValue Value = It->second;
				SerializeValue(InArchive, PE_TEXT("Value"), Value);
				InArchive.EndObject();
			}
		}
		else
		{
			InOutValues.Empty();
			for (UINT32 Index = 0u; Index < Count; Index++)
			{
				const EString IndexKey = EString::FromInt(Index);
				InArchive.BeginObject(*IndexKey);
				TKey Key = TKey();
				TValue Value = TValue();
				SerializeValue(InArchive, PE_TEXT("Key"), Key);
				SerializeValue(InArchive, PE_TEXT("Value"), Value);
				InOutValues.Add(Key, Value);
				InArchive.EndObject();
			}
		}
		InArchive.EndArray();
	}

	template <typename TValue>
	struct TPropertyIsArray : public std::false_type
	{
	};
	template <typename TElement, typename TSizeType, typename TAllocator, TSizeType EnableValue>
	struct TPropertyIsArray<TArray<TElement, TSizeType, TAllocator, EnableValue>> : public std::true_type
	{
		using ElementType = TElement;
	};
	template <typename TValue>
	struct TPropertyIsMap : public std::false_type
	{
	};
	template <typename TKey, typename TValue>
	struct TPropertyIsMap<TMap<TKey, TValue>> : public std::true_type
	{
		using KeyType = TKey;
		using ValueType = TValue;
	};

	// What the editor can present for this C++ type. BOOL32 and INT32 are the same type in
	// this engine, so bool registrations pass the value type explicitly through
	// AddClassBoolPropertyAccessors; int registrations use the inferred INT32.
	template <typename TValue>
	constexpr EPropertyValueType GetPropertyValueType()
	{
		using T = std::remove_cv_t<std::remove_reference_t<TValue>>;
		if constexpr (TPropertyIsArray<T>::value)		{ return EPropertyValueType::ARRAY; }
		else if constexpr (TPropertyIsMap<T>::value)	{ return EPropertyValueType::MAP; }
		else if constexpr (std::is_same_v<T, BOOL32>)	{ return EPropertyValueType::BOOL; }
		else if constexpr (std::is_same_v<T, INT32>)	{ return EPropertyValueType::INT32; }
		else if constexpr (std::is_same_v<T, UINT32>)	{ return EPropertyValueType::UINT32; }
		else if constexpr (std::is_same_v<T, INT64>)	{ return EPropertyValueType::INT64; }
		else if constexpr (std::is_same_v<T, UINT64>)	{ return EPropertyValueType::UINT64; }
		else if constexpr (std::is_same_v<T, FLOAT>)	{ return EPropertyValueType::FLOAT; }
		else if constexpr (std::is_same_v<T, DOUBLE>)	{ return EPropertyValueType::DOUBLE; }
		else if constexpr (std::is_same_v<T, EString>)	{ return EPropertyValueType::STRING; }
		else if constexpr (std::is_same_v<T, Vector2>)	{ return EPropertyValueType::VECTOR2; }
		else if constexpr (std::is_same_v<T, Vector2Int>) { return EPropertyValueType::VECTOR2INT; }
		else if constexpr (std::is_same_v<T, Vector3>)	{ return EPropertyValueType::VECTOR3; }
		else if constexpr (std::is_same_v<T, Quaternion>) { return EPropertyValueType::QUATERNION; }
		else if constexpr (std::is_same_v<T, Color3>)	{ return EPropertyValueType::COLOR3; }
		else if constexpr (std::is_same_v<T, ETransform>) { return EPropertyValueType::TRANSFORM; }
		else if constexpr (std::is_enum_v<T>)			{ return EPropertyValueType::ENUM; }
		else { return EPropertyValueType::CUSTOM; }
	}

	template <typename TValue>
	constexpr size_t GetPropertyEnumHash()
	{
		using T = std::remove_cv_t<std::remove_reference_t<TValue>>;
		if constexpr (std::is_enum_v<T>)
		{
			return GetClassHashCode<T>();
		}
		else
		{
			return 0u;
		}
	}

	template <typename TValue>
	size_t GetPropertyStructHash()
	{
		using T = std::remove_cv_t<std::remove_reference_t<TValue>>;
		if constexpr (TPropertyIsArray<T>::value || TPropertyIsMap<T>::value)
		{
			return 0u;
		}
		else
		{
			const size_t StructHash = GetClassHashCode<T>();
			return (EStructPropertyRegistry::Find(StructHash) != nullptr) ? StructHash : 0u;
		}
	}

	template <typename TValue>
	EPropertyValueType ResolvePropertyValueType()
	{
		using T = std::remove_cv_t<std::remove_reference_t<TValue>>;
		const EPropertyValueType Type = GetPropertyValueType<T>();
		if ((Type == EPropertyValueType::CUSTOM) && (GetPropertyStructHash<T>() != 0u))
		{
			return EPropertyValueType::STRUCT;
		}
		return Type;
	}

	template <typename TValue>
	BOOL32 MakeStructFields(const TValue& InValue, const size_t InStructHash, FPropertyValue& OutValue)
	{
		const FStructDescriptor* Descriptor = EStructPropertyRegistry::Find(InStructHash);
		if (!Descriptor)
		{
			return FALSE;
		}
		OutValue = FPropertyValue();
		OutValue.Type = EPropertyValueType::STRUCT;
		OutValue.StructHash = InStructHash;
		for (UINT32 Index = 0u, Num = Descriptor->Fields.Num<UINT32>(); Index < Num; Index++)
		{
			const FProperty& Field = Descriptor->Fields[Index];
			if (!Field.GetValue)
			{
				OutValue.StructFields.Add(nullptr);
				continue;
			}
			TSharedPtr<FPropertyValue> FieldValue(new FPropertyValue());
			Field.GetValue(&InValue, *FieldValue);
			OutValue.StructFields.Add(FieldValue);
		}
		return TRUE;
	}

	template <typename TValue>
	BOOL32 ReadStructFields(const FPropertyValue& InValue, TValue& OutValue)
	{
		const FStructDescriptor* Descriptor = EStructPropertyRegistry::Find(InValue.StructHash);
		if (!Descriptor)
		{
			return FALSE;
		}
		for (UINT32 Index = 0u, Num = Descriptor->Fields.Num<UINT32>(); Index < Num; Index++)
		{
			const FProperty& Field = Descriptor->Fields[Index];
			if ((Field.SetValue) && (Index < InValue.StructFields.Num<UINT32>()) && InValue.StructFields[Index])
			{
				Field.SetValue(&OutValue, *InValue.StructFields[Index]);
			}
		}
		return TRUE;
	}

	// Builds the type-only shape of one value. The editor uses this to initialize a nested
	// container that has no element to copy metadata from yet.
	template <typename TValue>
	void MakePropertyShape(FPropertyValue& OutShape)
	{
		using T = std::remove_cv_t<std::remove_reference_t<TValue>>;
		OutShape = FPropertyValue();
		OutShape.Type = ResolvePropertyValueType<T>();
		OutShape.EnumHash = GetPropertyEnumHash<T>();
		OutShape.StructHash = GetPropertyStructHash<T>();
		if constexpr (TPropertyIsArray<T>::value)
		{
			using ElementType = typename TPropertyIsArray<T>::ElementType;
			OutShape.ElementType = ResolvePropertyValueType<ElementType>();
			OutShape.ElementEnumHash = GetPropertyEnumHash<ElementType>();
			OutShape.ElementStructHash = GetPropertyStructHash<ElementType>();
			OutShape.ElementShape = TSharedPtr<FPropertyValue>(new FPropertyValue());
			MakePropertyShape<ElementType>(*OutShape.ElementShape);
		}
		else if constexpr (TPropertyIsMap<T>::value)
		{
			using KeyType = typename TPropertyIsMap<T>::KeyType;
			using MapValueType = typename TPropertyIsMap<T>::ValueType;
			OutShape.KeyType = ResolvePropertyValueType<KeyType>();
			OutShape.MapValueType = ResolvePropertyValueType<MapValueType>();
			OutShape.KeyEnumHash = GetPropertyEnumHash<KeyType>();
			OutShape.MapValueEnumHash = GetPropertyEnumHash<MapValueType>();
			OutShape.KeyStructHash = GetPropertyStructHash<KeyType>();
			OutShape.MapValueStructHash = GetPropertyStructHash<MapValueType>();
			OutShape.KeyShape = TSharedPtr<FPropertyValue>(new FPropertyValue());
			MakePropertyShape<KeyType>(*OutShape.KeyShape);
			OutShape.ValueShape = TSharedPtr<FPropertyValue>(new FPropertyValue());
			MakePropertyShape<MapValueType>(*OutShape.ValueShape);
		}
		else if constexpr (std::is_class_v<T>)
		{
			if (OutShape.Type == EPropertyValueType::STRUCT)
			{
				T DefaultValue{};
				MakeStructFields(DefaultValue, OutShape.StructHash, OutShape);
			}
		}
	}

	template <typename TValue>
	BOOL32 MakePropertyElement(const TValue& InValue, FPropertyElement& OutElement)
	{
		using T = std::remove_cv_t<std::remove_reference_t<TValue>>;
		OutElement = FPropertyElement();
		if constexpr (TPropertyIsArray<T>::value)
		{
			using ElementType = typename TPropertyIsArray<T>::ElementType;
			OutElement.Type = EPropertyValueType::ARRAY;
			OutElement.ContainerValue = TSharedPtr<FPropertyValue>(new FPropertyValue());
			OutElement.ContainerValue->ElementType = ResolvePropertyValueType<ElementType>();
			OutElement.ContainerValue->ElementEnumHash = GetPropertyEnumHash<ElementType>();
			OutElement.ContainerValue->ElementStructHash = GetPropertyStructHash<ElementType>();
			OutElement.ContainerValue->ElementShape = TSharedPtr<FPropertyValue>(new FPropertyValue());
			MakePropertyShape<ElementType>(*OutElement.ContainerValue->ElementShape);
			for (UINT32 Index = 0u, Num = InValue.Num<UINT32>(); Index < Num; Index++)
			{
				FPropertyElement Element;
				if (MakePropertyElement(InValue[Index], Element))
				{
					OutElement.ContainerValue->ArrayValues.Add(Element);
				}
			}
			return TRUE;
		}
		else if constexpr (TPropertyIsMap<T>::value)
		{
			using KeyType = typename TPropertyIsMap<T>::KeyType;
			using MapValueType = typename TPropertyIsMap<T>::ValueType;
			OutElement.Type = EPropertyValueType::MAP;
			OutElement.ContainerValue = TSharedPtr<FPropertyValue>(new FPropertyValue());
			OutElement.ContainerValue->KeyType = ResolvePropertyValueType<KeyType>();
			OutElement.ContainerValue->MapValueType = ResolvePropertyValueType<MapValueType>();
			OutElement.ContainerValue->KeyEnumHash = GetPropertyEnumHash<KeyType>();
			OutElement.ContainerValue->MapValueEnumHash = GetPropertyEnumHash<MapValueType>();
			OutElement.ContainerValue->KeyStructHash = GetPropertyStructHash<KeyType>();
			OutElement.ContainerValue->MapValueStructHash = GetPropertyStructHash<MapValueType>();
			OutElement.ContainerValue->KeyShape = TSharedPtr<FPropertyValue>(new FPropertyValue());
			MakePropertyShape<KeyType>(*OutElement.ContainerValue->KeyShape);
			OutElement.ContainerValue->ValueShape = TSharedPtr<FPropertyValue>(new FPropertyValue());
			MakePropertyShape<MapValueType>(*OutElement.ContainerValue->ValueShape);
			for (typename T::TConstIterator It = InValue.Begin(); It != InValue.End(); ++It)
			{
				FPropertyMapEntry Entry;
				if (MakePropertyElement(It->first, Entry.Key) && MakePropertyElement(It->second, Entry.Value))
				{
					OutElement.ContainerValue->MapEntries.Add(Entry);
				}
			}
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, BOOL32>)
		{
			OutElement.Type = EPropertyValueType::BOOL;
			OutElement.BoolValue = InValue;
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, INT32>)
		{
			OutElement.Type = EPropertyValueType::INT32;
			OutElement.IntValue = static_cast<INT64>(InValue);
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, UINT32>)
		{
			OutElement.Type = EPropertyValueType::UINT32;
			OutElement.IntValue = static_cast<INT64>(InValue);
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, INT64> || std::is_same_v<T, UINT64>)
		{
			OutElement.Type = std::is_same_v<T, INT64> ? EPropertyValueType::INT64 : EPropertyValueType::UINT64;
			OutElement.IntValue = static_cast<INT64>(InValue);
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, FLOAT> || std::is_same_v<T, DOUBLE>)
		{
			OutElement.Type = std::is_same_v<T, FLOAT> ? EPropertyValueType::FLOAT : EPropertyValueType::DOUBLE;
			OutElement.DoubleValue = static_cast<DOUBLE>(InValue);
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, EString>)
		{
			OutElement.Type = EPropertyValueType::STRING;
			OutElement.StringValue = InValue;
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, Vector2>)
		{
			OutElement.Type = EPropertyValueType::VECTOR2;
			OutElement.Vector2Value = InValue;
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, Vector2Int>)
		{
			OutElement.Type = EPropertyValueType::VECTOR2INT;
			OutElement.Vector2IntValue = InValue;
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, Vector3>)
		{
			OutElement.Type = EPropertyValueType::VECTOR3;
			OutElement.Vector3Value = InValue;
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, Quaternion>)
		{
			OutElement.Type = EPropertyValueType::QUATERNION;
			OutElement.QuaternionValue = InValue;
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, Color3>)
		{
			OutElement.Type = EPropertyValueType::COLOR3;
			OutElement.ColorValue = InValue;
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, ETransform>)
		{
			OutElement.Type = EPropertyValueType::TRANSFORM;
			OutElement.TransformValue = InValue;
			return TRUE;
		}
		else if constexpr (std::is_enum_v<T>)
		{
			OutElement.Type = EPropertyValueType::ENUM;
			OutElement.EnumHash = GetClassHashCode<T>();
			OutElement.IntValue = static_cast<INT64>(InValue);
			return TRUE;
		}
		else
		{
			const size_t StructHash = GetPropertyStructHash<T>();
			if ((StructHash != 0u) && (EStructPropertyRegistry::Find(StructHash) != nullptr))
			{
				OutElement.Type = EPropertyValueType::STRUCT;
				OutElement.StructHash = StructHash;
				OutElement.ContainerValue = TSharedPtr<FPropertyValue>(new FPropertyValue());
				MakeStructFields(InValue, StructHash, *OutElement.ContainerValue);
				return TRUE;
			}
			OutElement.Type = EPropertyValueType::CUSTOM;
			return FALSE;
		}
	}

	template <typename TValue>
	BOOL32 ReadPropertyElement(const FPropertyElement& InElement, TValue& OutValue)
	{
		using T = std::remove_cv_t<std::remove_reference_t<TValue>>;
		if constexpr (TPropertyIsArray<T>::value)
		{
			using ElementType = typename TPropertyIsArray<T>::ElementType;
			if (!InElement.ContainerValue)
			{
				return FALSE;
			}
			OutValue.Empty();
			for (UINT32 Index = 0u, Num = InElement.ContainerValue->ArrayValues.Num<UINT32>(); Index < Num; Index++)
			{
				ElementType Element = ElementType();
				if (ReadPropertyElement(InElement.ContainerValue->ArrayValues[Index], Element))
				{
					OutValue.Add(Element);
				}
			}
			return TRUE;
		}
		else if constexpr (TPropertyIsMap<T>::value)
		{
			using KeyType = typename TPropertyIsMap<T>::KeyType;
			using MapValueType = typename TPropertyIsMap<T>::ValueType;
			if (!InElement.ContainerValue)
			{
				return FALSE;
			}
			OutValue.Empty();
			for (UINT32 Index = 0u, Num = InElement.ContainerValue->MapEntries.Num<UINT32>(); Index < Num; Index++)
			{
				KeyType Key = KeyType();
				MapValueType Value = MapValueType();
				if (ReadPropertyElement(InElement.ContainerValue->MapEntries[Index].Key, Key) &&
					ReadPropertyElement(InElement.ContainerValue->MapEntries[Index].Value, Value))
				{
					OutValue.Add(Key, Value);
				}
			}
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, BOOL32>)
		{
			OutValue = (InElement.Type == EPropertyValueType::BOOL) ? InElement.BoolValue : static_cast<BOOL32>(InElement.IntValue);
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, INT32>)
		{
			OutValue = static_cast<INT32>(InElement.IntValue);
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, UINT32>)
		{
			OutValue = static_cast<UINT32>(InElement.IntValue);
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, INT64> || std::is_same_v<T, UINT64>)
		{
			OutValue = static_cast<T>(InElement.IntValue);
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, FLOAT> || std::is_same_v<T, DOUBLE>)
		{
			OutValue = static_cast<T>(InElement.DoubleValue);
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, EString>)
		{
			OutValue = InElement.StringValue;
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, Vector2>)
		{
			OutValue = InElement.Vector2Value;
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, Vector2Int>)
		{
			OutValue = InElement.Vector2IntValue;
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, Vector3>)
		{
			OutValue = InElement.Vector3Value;
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, Quaternion>)
		{
			OutValue = InElement.QuaternionValue;
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, Color3>)
		{
			OutValue = InElement.ColorValue;
			return TRUE;
		}
		else if constexpr (std::is_same_v<T, ETransform>)
		{
			OutValue = InElement.TransformValue;
			return TRUE;
		}
		else if constexpr (std::is_enum_v<T>)
		{
			OutValue = static_cast<T>(InElement.IntValue);
			return TRUE;
		}
		else
		{
			const size_t StructHash = GetPropertyStructHash<T>();
			if ((StructHash != 0u) && (InElement.ContainerValue))
			{
				return ReadStructFields(*InElement.ContainerValue, OutValue);
			}
			return FALSE;
		}
	}

	/*
	* Root-value forms. Array/Map GetValue lambdas historically store their payload directly
	* on FPropertyValue; struct fields use their own tree. These helpers keep both root and
	* nested representations consistent for struct-aware property accessors.
	*/
	template <typename TValue>
	BOOL32 MakePropertyValue(const TValue& InValue, FPropertyValue& OutValue)
	{
		using T = std::remove_cv_t<std::remove_reference_t<TValue>>;
		OutValue = FPropertyValue();
		if constexpr (TPropertyIsArray<T>::value)
		{
			using ElementType = typename TPropertyIsArray<T>::ElementType;
			OutValue.Type = EPropertyValueType::ARRAY;
			OutValue.ElementType = ResolvePropertyValueType<ElementType>();
			OutValue.ElementEnumHash = GetPropertyEnumHash<ElementType>();
			OutValue.ElementStructHash = GetPropertyStructHash<ElementType>();
			OutValue.ElementShape = TSharedPtr<FPropertyValue>(new FPropertyValue());
			MakePropertyShape<ElementType>(*OutValue.ElementShape);
			for (UINT32 Index = 0u, Num = InValue.Num<UINT32>(); Index < Num; Index++)
			{
				FPropertyElement Element;
				if (MakePropertyElement(InValue[Index], Element))
				{
					OutValue.ArrayValues.Add(Element);
				}
			}
			return TRUE;
		}
		else if constexpr (TPropertyIsMap<T>::value)
		{
			using KeyType = typename TPropertyIsMap<T>::KeyType;
			using MapValueType = typename TPropertyIsMap<T>::ValueType;
			OutValue.Type = EPropertyValueType::MAP;
			OutValue.KeyType = ResolvePropertyValueType<KeyType>();
			OutValue.MapValueType = ResolvePropertyValueType<MapValueType>();
			OutValue.KeyEnumHash = GetPropertyEnumHash<KeyType>();
			OutValue.MapValueEnumHash = GetPropertyEnumHash<MapValueType>();
			OutValue.KeyStructHash = GetPropertyStructHash<KeyType>();
			OutValue.MapValueStructHash = GetPropertyStructHash<MapValueType>();
			OutValue.KeyShape = TSharedPtr<FPropertyValue>(new FPropertyValue());
			MakePropertyShape<KeyType>(*OutValue.KeyShape);
			OutValue.ValueShape = TSharedPtr<FPropertyValue>(new FPropertyValue());
			MakePropertyShape<MapValueType>(*OutValue.ValueShape);
			for (typename T::TConstIterator It = InValue.Begin(); It != InValue.End(); ++It)
			{
				FPropertyMapEntry Entry;
				if (MakePropertyElement(It->first, Entry.Key) && MakePropertyElement(It->second, Entry.Value))
				{
					OutValue.MapEntries.Add(Entry);
				}
			}
			return TRUE;
		}
		else if constexpr (std::is_class_v<T>)
		{
			const size_t StructHash = GetPropertyStructHash<T>();
			if (StructHash != 0u)
			{
				return MakeStructFields(InValue, StructHash, OutValue);
			}
			FPropertyElement Element;
			if (!MakePropertyElement(InValue, Element))
			{
				return FALSE;
			}
			static_cast<FPropertyElement&>(OutValue) = Element;
			return TRUE;
		}
		else
		{
			FPropertyElement Element;
			if (!MakePropertyElement(InValue, Element))
			{
				return FALSE;
			}
			static_cast<FPropertyElement&>(OutValue) = Element;
			return TRUE;
		}
	}

	template <typename TValue>
	BOOL32 ReadPropertyValue(const FPropertyValue& InValue, TValue& OutValue)
	{
		using T = std::remove_cv_t<std::remove_reference_t<TValue>>;
		if constexpr (TPropertyIsArray<T>::value)
		{
			using ElementType = typename TPropertyIsArray<T>::ElementType;
			OutValue.Empty();
			for (UINT32 Index = 0u, Num = InValue.ArrayValues.Num<UINT32>(); Index < Num; Index++)
			{
				ElementType Element = ElementType();
				if (ReadPropertyElement(InValue.ArrayValues[Index], Element))
				{
					OutValue.Add(Element);
				}
			}
			return TRUE;
		}
		else if constexpr (TPropertyIsMap<T>::value)
		{
			using KeyType = typename TPropertyIsMap<T>::KeyType;
			using MapValueType = typename TPropertyIsMap<T>::ValueType;
			OutValue.Empty();
			for (UINT32 Index = 0u, Num = InValue.MapEntries.Num<UINT32>(); Index < Num; Index++)
			{
				KeyType Key = KeyType();
				MapValueType Value = MapValueType();
				if (ReadPropertyElement(InValue.MapEntries[Index].Key, Key) &&
					ReadPropertyElement(InValue.MapEntries[Index].Value, Value))
				{
					OutValue.Add(Key, Value);
				}
			}
			return TRUE;
		}
		else if constexpr (std::is_class_v<T>)
		{
			const size_t StructHash = GetPropertyStructHash<T>();
			if (StructHash != 0u)
			{
				return ReadStructFields(InValue, OutValue);
			}
			return ReadPropertyElement(InValue, OutValue);
		}
		else
		{
			return ReadPropertyElement(InValue, OutValue);
		}
	}

	template <typename TClass, typename TValue>
	void AddClassPropertyAccessorsTyped(const size_t& InClassHash, const CHAR* InName,
		const TFunction<TValue(const TClass*)>& InGetter,
		const TFunction<void(TClass*, const TValue&)>& InSetter,
		const UINT32& InFlags,
		const EPropertyValueType& InValueType,
		const EPropertyAssetType& InAssetType = EPropertyAssetType::NONE)
	{
		using T = std::remove_cv_t<std::remove_reference_t<TValue>>;
		EPropertyValueType ValueType = InValueType;
		if (ValueType == EPropertyValueType::NONE)
		{
			ValueType = ResolvePropertyValueType<TValue>();
		}

		FProperty Property;
		Property.Name = InName;
		Property.Flags = InFlags;
		Property.ValueType = ValueType;
		Property.AssetType = InAssetType;
		Property.StructHash = GetPropertyStructHash<T>();
		if constexpr (TPropertyIsArray<T>::value)
		{
			using ElementType = typename TPropertyIsArray<T>::ElementType;
			Property.ElementType = ResolvePropertyValueType<ElementType>();
			Property.ElementEnumHash = GetPropertyEnumHash<ElementType>();
			Property.ElementStructHash = GetPropertyStructHash<ElementType>();
			if (Property.ElementType == EPropertyValueType::CUSTOM)
			{
				Property.ValueType = EPropertyValueType::CUSTOM;
			}
		}
		else if constexpr (TPropertyIsMap<T>::value)
		{
			using KeyType = typename TPropertyIsMap<T>::KeyType;
			using MapValueType = typename TPropertyIsMap<T>::ValueType;
			Property.KeyType = ResolvePropertyValueType<KeyType>();
			Property.MapValueType = ResolvePropertyValueType<MapValueType>();
			Property.KeyEnumHash = GetPropertyEnumHash<KeyType>();
			Property.MapValueEnumHash = GetPropertyEnumHash<MapValueType>();
			Property.KeyStructHash = GetPropertyStructHash<KeyType>();
			Property.MapValueStructHash = GetPropertyStructHash<MapValueType>();
			if ((Property.KeyType == EPropertyValueType::CUSTOM) || (Property.MapValueType == EPropertyValueType::CUSTOM))
			{
				Property.ValueType = EPropertyValueType::CUSTOM;
			}
		}
		Property.Serialize = [InName = EString(InName), InGetter, InSetter](EArchive& InArchive, void* InContainer) -> void
		{
			TClass* Object = static_cast<TClass*>(InContainer);
			TValue Value = InGetter(Object);
			SerializeValue(InArchive, *InName, Value);
			if (!InArchive.IsSaving())
			{
				InSetter(Object, Value);
			}
		};

		if constexpr (TPropertyIsArray<T>::value)
		{
			if (Property.ValueType == EPropertyValueType::ARRAY)
			{
				Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
				{
					MakePropertyValue(InGetter(static_cast<const TClass*>(InContainer)), OutValue);
				};
				Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
				{
					T Values;
					if (ReadPropertyValue(InValue, Values))
					{
						InSetter(static_cast<TClass*>(InContainer), Values);
					}
				};
			}
		}
		else if constexpr (TPropertyIsMap<T>::value)
		{
			if (Property.ValueType == EPropertyValueType::MAP)
			{
				Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
				{
					MakePropertyValue(InGetter(static_cast<const TClass*>(InContainer)), OutValue);
				};
				Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
				{
					T Values;
					if (ReadPropertyValue(InValue, Values))
					{
						InSetter(static_cast<TClass*>(InContainer), Values);
					}
				};
			}
		}
		else if constexpr (std::is_same_v<T, BOOL32>)
		{
			if (ValueType == EPropertyValueType::BOOL)
			{
				Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
				{
					OutValue.Type = EPropertyValueType::BOOL;
					OutValue.BoolValue = InGetter(static_cast<const TClass*>(InContainer));
				};
				Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
				{
					InSetter(static_cast<TClass*>(InContainer), InValue.BoolValue);
				};
			}
			else
			{
				Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
				{
					OutValue.Type = EPropertyValueType::INT32;
					OutValue.IntValue = static_cast<INT64>(InGetter(static_cast<const TClass*>(InContainer)));
				};
				Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
				{
					InSetter(static_cast<TClass*>(InContainer), static_cast<TValue>(InValue.IntValue));
				};
			}
		}
		else if constexpr (std::is_same_v<T, INT32>)
		{
			Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
			{
				OutValue.Type = EPropertyValueType::INT32;
				OutValue.IntValue = static_cast<INT64>(InGetter(static_cast<const TClass*>(InContainer)));
			};
			Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
			{
				InSetter(static_cast<TClass*>(InContainer), static_cast<INT32>(InValue.IntValue));
			};
		}
		else if constexpr (std::is_same_v<T, UINT32>)
		{
			Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
			{
				OutValue.Type = EPropertyValueType::UINT32;
				OutValue.IntValue = static_cast<INT64>(InGetter(static_cast<const TClass*>(InContainer)));
			};
			Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
			{
				InSetter(static_cast<TClass*>(InContainer), static_cast<UINT32>(InValue.IntValue));
			};
		}
		else if constexpr (std::is_same_v<T, INT64>)
		{
			Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
			{
				OutValue.Type = EPropertyValueType::INT64;
				OutValue.IntValue = InGetter(static_cast<const TClass*>(InContainer));
			};
			Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
			{
				InSetter(static_cast<TClass*>(InContainer), InValue.IntValue);
			};
		}
		else if constexpr (std::is_same_v<T, UINT64>)
		{
			Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
			{
				OutValue.Type = EPropertyValueType::UINT64;
				OutValue.IntValue = static_cast<INT64>(InGetter(static_cast<const TClass*>(InContainer)));
			};
			Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
			{
				InSetter(static_cast<TClass*>(InContainer), static_cast<UINT64>(InValue.IntValue));
			};
		}
		else if constexpr (std::is_same_v<T, FLOAT> || std::is_same_v<T, DOUBLE>)
		{
			Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
			{
				OutValue.Type = GetPropertyValueType<T>();
				OutValue.DoubleValue = static_cast<DOUBLE>(InGetter(static_cast<const TClass*>(InContainer)));
			};
			Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
			{
				InSetter(static_cast<TClass*>(InContainer), static_cast<T>(InValue.DoubleValue));
			};
		}
		else if constexpr (std::is_same_v<T, EString>)
		{
			Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
			{
				OutValue.Type = EPropertyValueType::STRING;
				OutValue.StringValue = InGetter(static_cast<const TClass*>(InContainer));
			};
			Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
			{
				InSetter(static_cast<TClass*>(InContainer), InValue.StringValue);
			};
		}
		else if constexpr (std::is_same_v<T, Vector2>)
		{
			Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
			{
				OutValue.Type = EPropertyValueType::VECTOR2;
				OutValue.Vector2Value = InGetter(static_cast<const TClass*>(InContainer));
			};
			Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
			{
				InSetter(static_cast<TClass*>(InContainer), InValue.Vector2Value);
			};
		}
		else if constexpr (std::is_same_v<T, Vector2Int>)
		{
			Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
			{
				OutValue.Type = EPropertyValueType::VECTOR2INT;
				OutValue.Vector2IntValue = InGetter(static_cast<const TClass*>(InContainer));
			};
			Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
			{
				InSetter(static_cast<TClass*>(InContainer), InValue.Vector2IntValue);
			};
		}
		else if constexpr (std::is_same_v<T, Vector3>)
		{
			Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
			{
				OutValue.Type = EPropertyValueType::VECTOR3;
				OutValue.Vector3Value = InGetter(static_cast<const TClass*>(InContainer));
			};
			Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
			{
				InSetter(static_cast<TClass*>(InContainer), InValue.Vector3Value);
			};
		}
		else if constexpr (std::is_same_v<T, Quaternion>)
		{
			Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
			{
				OutValue.Type = EPropertyValueType::QUATERNION;
				OutValue.QuaternionValue = InGetter(static_cast<const TClass*>(InContainer));
			};
			Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
			{
				InSetter(static_cast<TClass*>(InContainer), InValue.QuaternionValue);
			};
		}
		else if constexpr (std::is_same_v<T, Color3>)
		{
			Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
			{
				OutValue.Type = EPropertyValueType::COLOR3;
				OutValue.ColorValue = InGetter(static_cast<const TClass*>(InContainer));
			};
			Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
			{
				InSetter(static_cast<TClass*>(InContainer), InValue.ColorValue);
			};
		}
		else if constexpr (std::is_same_v<T, ETransform>)
		{
			Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
			{
				OutValue.Type = EPropertyValueType::TRANSFORM;
				OutValue.TransformValue = InGetter(static_cast<const TClass*>(InContainer));
			};
			Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
			{
				InSetter(static_cast<TClass*>(InContainer), InValue.TransformValue);
			};
		}
		else if constexpr (std::is_class_v<T>)
		{
			if ((Property.ValueType == EPropertyValueType::STRUCT) && (Property.StructHash != 0u))
			{
				Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
				{
					MakePropertyValue(InGetter(static_cast<const TClass*>(InContainer)), OutValue);
				};
				Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
				{
					T Value;
					if (ReadPropertyValue(InValue, Value))
					{
						InSetter(static_cast<TClass*>(InContainer), Value);
					}
				};
			}
			else
			{
				Property.ValueType = EPropertyValueType::CUSTOM;
			}
		}
		else
		{
			Property.ValueType = EPropertyValueType::CUSTOM;
		}

		EClassPropertyRegistry::AddProperty(InClassHash, Property);
	}

	// Register a property backed by accessors. Used when the member is protected (most
	// OCA components) - the accessor lambdas are written where the class allows it.
	template <typename TClass, typename TValue>
	void AddClassPropertyAccessors(const size_t& InClassHash, const CHAR* InName,
		const TFunction<TValue(const TClass*)>& InGetter,
		const TFunction<void(TClass*, const TValue&)>& InSetter,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::NONE))
	{
		AddClassPropertyAccessorsTyped<TClass, TValue>(InClassHash, InName, InGetter, InSetter, InFlags, EPropertyValueType::NONE);
	}

	// BOOL32 is the same C++ type as INT32, so the type cannot be inferred: bool properties
	// register through this overload and the Details panel renders a checkbox.
	template <typename TClass>
	void AddClassBoolPropertyAccessors(const size_t& InClassHash, const CHAR* InName,
		const TFunction<BOOL32(const TClass*)>& InGetter,
		const TFunction<void(TClass*, const BOOL32&)>& InSetter,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::NONE))
	{
		AddClassPropertyAccessorsTyped<TClass, BOOL32>(InClassHash, InName, InGetter, InSetter, InFlags, EPropertyValueType::BOOL);
	}

	template <typename TClass>
	void AddClassAssetPropertyAccessors(const size_t& InClassHash, const CHAR* InName,
		const TFunction<EString(const TClass*)>& InGetter,
		const TFunction<void(TClass*, const EString&)>& InSetter,
		const EPropertyAssetType& InAssetType,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::NONE))
	{
		AddClassPropertyAccessorsTyped<TClass, EString>(InClassHash, InName, InGetter, InSetter,
			InFlags, EPropertyValueType::STRING, InAssetType);
	}

	// Register a property whose value has its own serializer (a struct, an array, a shape).
	// The getter/setter form is kept so the property stays one registration shared by
	// serialization and the editor.
	template <typename TClass, typename TValue>
	void AddClassPropertyCustom(const size_t& InClassHash, const CHAR* InName,
		const TFunction<TValue(const TClass*)>& InGetter,
		const TFunction<void(TClass*, const TValue&)>& InSetter,
		const TFunction<void(EArchive&, const CHAR*, TValue&)>& InSerializeValue,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::NONE))
	{
		FProperty Property;
		Property.Name = InName;
		Property.Flags = InFlags;
		Property.ValueType = EPropertyValueType::CUSTOM;
		Property.Serialize = [InName = EString(InName), InGetter, InSetter, InSerializeValue](EArchive& InArchive, void* InContainer) -> void
		{
			TClass* Object = static_cast<TClass*>(InContainer);
			TValue Value = InGetter(Object);
			InSerializeValue(InArchive, *InName, Value);
			if (!InArchive.IsSaving())
			{
				InSetter(Object, Value);
			}
		};
		EClassPropertyRegistry::AddProperty(InClassHash, Property);
	}

	template <typename TValue>
	void FillStructFieldMetadata(FProperty& OutProperty)
	{
		using T = std::remove_cv_t<std::remove_reference_t<TValue>>;
		OutProperty.ValueType = ResolvePropertyValueType<T>();
		OutProperty.EnumHash = GetPropertyEnumHash<T>();
		OutProperty.StructHash = GetPropertyStructHash<T>();
		if constexpr (TPropertyIsArray<T>::value)
		{
			using ElementType = typename TPropertyIsArray<T>::ElementType;
			OutProperty.ElementType = ResolvePropertyValueType<ElementType>();
			OutProperty.ElementEnumHash = GetPropertyEnumHash<ElementType>();
			OutProperty.ElementStructHash = GetPropertyStructHash<ElementType>();
		}
		else if constexpr (TPropertyIsMap<T>::value)
		{
			using KeyType = typename TPropertyIsMap<T>::KeyType;
			using MapValueType = typename TPropertyIsMap<T>::ValueType;
			OutProperty.KeyType = ResolvePropertyValueType<KeyType>();
			OutProperty.MapValueType = ResolvePropertyValueType<MapValueType>();
			OutProperty.KeyEnumHash = GetPropertyEnumHash<KeyType>();
			OutProperty.MapValueEnumHash = GetPropertyEnumHash<MapValueType>();
			OutProperty.KeyStructHash = GetPropertyStructHash<KeyType>();
			OutProperty.MapValueStructHash = GetPropertyStructHash<MapValueType>();
		}
	}

	template <typename TStruct, typename TValue>
	void AddStructProperty(const size_t& InStructHash, const EString& InStructName, const CHAR* InName,
		const TFunction<TValue(const TStruct*)>& InGetter,
		const TFunction<void(TStruct*, const TValue&)>& InSetter,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE))
	{
		FProperty Field;
		Field.Name = InName;
		Field.Flags = InFlags;
		FillStructFieldMetadata<TValue>(Field);
		if (Field.ValueType != EPropertyValueType::CUSTOM)
		{
			Field.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
			{
				MakePropertyValue(InGetter(static_cast<const TStruct*>(InContainer)), OutValue);
			};
			Field.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
			{
				TValue Value;
				if (ReadPropertyValue(InValue, Value))
				{
					InSetter(static_cast<TStruct*>(InContainer), Value);
				}
			};
		}
		EStructPropertyRegistry::AddField(InStructHash, InStructName, Field);
	}

	template <typename TStruct, typename TValue>
	void AddStructProperty(const size_t& InStructHash, const EString& InStructName, const CHAR* InName,
		TValue TStruct::* InMember,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE))
	{
		AddStructProperty<TStruct, TValue>(InStructHash, InStructName, InName,
			[InMember](const TStruct* InObject) -> TValue { return InObject->*InMember; },
			[InMember](TStruct* InObject, const TValue& InValue) -> void { InObject->*InMember = InValue; },
			InFlags);
	}

	// BOOL32 is an alias of INT32 at the C++ level, so bool fields register through this
	// overload and the editor knows to draw a checkbox.
	template <typename TStruct>
	void AddStructBoolProperty(const size_t& InStructHash, const EString& InStructName, const CHAR* InName,
		const TFunction<BOOL32(const TStruct*)>& InGetter,
		const TFunction<void(TStruct*, const BOOL32&)>& InSetter,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE))
	{
		FProperty Field;
		Field.Name = InName;
		Field.Flags = InFlags;
		FillStructFieldMetadata<BOOL32>(Field);
		Field.ValueType = EPropertyValueType::BOOL;
		Field.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
		{
			MakePropertyValue(InGetter(static_cast<const TStruct*>(InContainer)), OutValue);
		};
		Field.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
		{
			BOOL32 Value = FALSE;
			if (ReadPropertyValue(InValue, Value))
			{
				InSetter(static_cast<TStruct*>(InContainer), Value);
			}
		};
		EStructPropertyRegistry::AddField(InStructHash, InStructName, Field);
	}

	template <typename TStruct>
	void AddStructBoolProperty(const size_t& InStructHash, const EString& InStructName, const CHAR* InName,
		BOOL32 TStruct::* InMember,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE))
	{
		AddStructBoolProperty<TStruct>(InStructHash, InStructName, InName,
			[InMember](const TStruct* InObject) -> BOOL32 { return InObject->*InMember; },
			[InMember](TStruct* InObject, const BOOL32& InValue) -> void { InObject->*InMember = InValue; },
			InFlags);
	}

	template <typename TStruct, typename TEnum>
	void AddStructEnumProperty(const size_t& InStructHash, const EString& InStructName, const CHAR* InName,
		const TFunction<TEnum(const TStruct*)>& InGetter,
		const TFunction<void(TStruct*, const TEnum&)>& InSetter,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE))
	{
		static_assert(std::is_enum_v<TEnum>, "AddStructEnumProperty requires an enum value.");
		AddStructProperty<TStruct, TEnum>(InStructHash, InStructName, InName, InGetter, InSetter, InFlags);
	}

	template <typename TStruct, typename TEnum>
	void AddStructEnumProperty(const size_t& InStructHash, const EString& InStructName, const CHAR* InName,
		TEnum TStruct::* InMember,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE))
	{
		AddStructEnumProperty<TStruct, TEnum>(InStructHash, InStructName, InName,
			[InMember](const TStruct* InObject) -> TEnum { return InObject->*InMember; },
			[InMember](TStruct* InObject, const TEnum& InValue) -> void { InObject->*InMember = InValue; },
			InFlags);
	}

	template <typename TStruct>
	void AddStructAssetProperty(const size_t& InStructHash, const EString& InStructName, const CHAR* InName,
		const TFunction<EString(const TStruct*)>& InGetter,
		const TFunction<void(TStruct*, const EString&)>& InSetter,
		const EPropertyAssetType& InAssetType,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE))
	{
		FProperty Field;
		Field.Name = InName;
		Field.Flags = InFlags;
		Field.ValueType = EPropertyValueType::STRING;
		Field.AssetType = InAssetType;
		Field.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
		{
			MakePropertyValue(InGetter(static_cast<const TStruct*>(InContainer)), OutValue);
		};
		Field.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
		{
			EString Value;
			if (ReadPropertyValue(InValue, Value))
			{
				InSetter(static_cast<TStruct*>(InContainer), Value);
			}
		};
		EStructPropertyRegistry::AddField(InStructHash, InStructName, Field);
	}

	template <typename TStruct>
	void AddStructAssetProperty(const size_t& InStructHash, const EString& InStructName, const CHAR* InName,
		EString TStruct::* InMember,
		const EPropertyAssetType& InAssetType,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE))
	{
		AddStructAssetProperty<TStruct>(InStructHash, InStructName, InName,
			[InMember](const TStruct* InObject) -> EString { return InObject->*InMember; },
			[InMember](TStruct* InObject, const EString& InValue) -> void { InObject->*InMember = InValue; },
			InAssetType, InFlags);
	}

	template <typename TStruct, typename TValue>
	void AddStructReadOnlyProperty(const size_t& InStructHash, const EString& InStructName, const CHAR* InName,
		const TFunction<TValue(const TStruct*)>& InGetter,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::VISIBLE_ANYWHERE))
	{
		FProperty Field;
		Field.Name = InName;
		Field.Flags = InFlags;
		FillStructFieldMetadata<TValue>(Field);
		if (Field.ValueType != EPropertyValueType::CUSTOM)
		{
			Field.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
			{
				MakePropertyValue(InGetter(static_cast<const TStruct*>(InContainer)), OutValue);
			};
		}
		EStructPropertyRegistry::AddField(InStructHash, InStructName, Field);
	}

	template <typename TStruct, typename TValue>
	void AddStructReadOnlyProperty(const size_t& InStructHash, const EString& InStructName, const CHAR* InName,
		TValue TStruct::* InMember,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::VISIBLE_ANYWHERE))
	{
		AddStructReadOnlyProperty<TStruct, TValue>(InStructHash, InStructName, InName,
			[InMember](const TStruct* InObject) -> TValue { return InObject->*InMember; },
			InFlags);
	}

	// Register a property with a hand-written serializer while still exposing a reflected
	// value tree to the editor. The serializer stays authoritative for the file layout;
	// fields are read and written through the same accessors as the Details panel.
	template <typename TClass, typename TValue>
	void AddClassPropertyStructCustom(const size_t& InClassHash, const CHAR* InName,
		const TFunction<TValue(const TClass*)>& InGetter,
		const TFunction<void(TClass*, const TValue&)>& InSetter,
		const TFunction<void(EArchive&, const CHAR*, TValue&)>& InSerializeValue,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::NONE))
	{
		FProperty Property;
		Property.Name = InName;
		Property.Flags = InFlags;
		FillStructFieldMetadata<TValue>(Property);
		Property.Serialize = [InName = EString(InName), InGetter, InSetter, InSerializeValue](EArchive& InArchive, void* InContainer) -> void
		{
			TClass* Object = static_cast<TClass*>(InContainer);
			TValue Value = InGetter(Object);
			InSerializeValue(InArchive, *InName, Value);
			if (!InArchive.IsSaving())
			{
				InSetter(Object, Value);
			}
		};
		if (Property.ValueType != EPropertyValueType::CUSTOM)
		{
			Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
			{
				MakePropertyValue(InGetter(static_cast<const TClass*>(InContainer)), OutValue);
			};
			Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
			{
				TValue Value;
				if (ReadPropertyValue(InValue, Value))
				{
					InSetter(static_cast<TClass*>(InContainer), Value);
				}
			};
		}
		EClassPropertyRegistry::AddProperty(InClassHash, Property);
	}

	// Enum-like values that are not plain C++ enums (FPhysicsObjectLayer wraps a UINT16):
	// the getter/setter work in INT32 and the registry supplies the on-disk names.
	template <typename TClass>
	void AddClassEnumValuePropertyAccessors(const size_t& InClassHash, const CHAR* InName, const size_t& InEnumHash,
		const TFunction<INT32(const TClass*)>& InGetter,
		const TFunction<void(TClass*, const INT32&)>& InSetter,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::NONE))
	{
		FProperty Property;
		Property.Name = InName;
		Property.Flags = InFlags;
		Property.ValueType = EPropertyValueType::ENUM;
		Property.EnumHash = InEnumHash;
		Property.Serialize = [InName = EString(InName), InEnumHash, InGetter, InSetter](EArchive& InArchive, void* InContainer) -> void
		{
			TClass* Object = static_cast<TClass*>(InContainer);
			INT32 Value = InGetter(Object);
			SerializeEnumValue(InArchive, *InName, Value, InEnumHash);
			if (!InArchive.IsSaving())
			{
				InSetter(Object, Value);
			}
		};
		Property.GetValue = [InGetter](const void* InContainer, FPropertyValue& OutValue) -> void
		{
			OutValue.Type = EPropertyValueType::ENUM;
			OutValue.IntValue = static_cast<INT64>(InGetter(static_cast<const TClass*>(InContainer)));
		};
		Property.SetValue = [InSetter](void* InContainer, const FPropertyValue& InValue) -> void
		{
			InSetter(static_cast<TClass*>(InContainer), static_cast<INT32>(InValue.IntValue));
		};
		EClassPropertyRegistry::AddProperty(InClassHash, Property);
	}

	template <typename TClass, typename TEnum>
	void AddClassEnumPropertyAccessors(const size_t& InClassHash, const CHAR* InName,
		const TFunction<TEnum(const TClass*)>& InGetter,
		const TFunction<void(TClass*, const TEnum&)>& InSetter,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::NONE))
	{
		const size_t EnumHash = GetClassHashCode<TEnum>();
		AddClassEnumValuePropertyAccessors<TClass>(InClassHash, InName, EnumHash,
			[InGetter](const TClass* Object) -> INT32 { return static_cast<INT32>(InGetter(Object)); },
			[InSetter](TClass* Object, const INT32& Value) -> void { InSetter(Object, static_cast<TEnum>(Value)); },
			InFlags);
	}

}
