#pragma once

#include <CoreMinimal.h>
#include <Base/DataStructure/Transform.h>
#include <type_traits>
#include "Archive.h"
#include "EnumRegistry.h"

namespace PigeonEngine
{

	// Property behaviour flags. Serialization only skips TRANSIENT; the edit flags are
	// consumed by the reflection-driven Details panel (Phase 3D).
	enum class EPropertyFlagsType : UINT32
	{
		NONE = 0u,
		EDIT_ANYWHERE = (1u << 0),
		VISIBLE_ANYWHERE = (1u << 1),
		TRANSIENT = (1u << 2),
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
		CUSTOM,
	};

	/*
	* Editor-side value container. One struct for every supported type keeps the Details
	* panel and the undo commands type erased without depending on ImGui or on any single
	* component. GetValue/SetValue on FProperty fill and consume it.
	*/
	struct FPropertyValue
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
		// Only meaningful for ENUM; the EEnumRegistry key.
		size_t EnumHash = 0u;
		// Serializes the member into/from the archive; InContainer is the owning object.
		TFunction<void(EArchive&, void*)> Serialize;
		// Editor-side access. Null for CUSTOM properties; read-only when SetValue is null.
		TFunction<void(const void*, FPropertyValue&)> GetValue;
		TFunction<void(void*, const FPropertyValue&)> SetValue;
	};

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

	// What the editor can present for this C++ type. BOOL32 and INT32 are the same type in
	// this engine, so bool registrations pass the value type explicitly through
	// AddClassBoolPropertyAccessors; int registrations use the inferred INT32.
	template <typename TValue>
	constexpr EPropertyValueType GetPropertyValueType()
	{
		using T = std::remove_cv_t<std::remove_reference_t<TValue>>;
		if constexpr (std::is_same_v<T, BOOL32>)		{ return EPropertyValueType::INT32; }
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
		else { return EPropertyValueType::CUSTOM; }
	}

	template <typename TClass, typename TValue>
	void AddClassPropertyAccessorsTyped(const size_t& InClassHash, const CHAR* InName,
		const TFunction<TValue(const TClass*)>& InGetter,
		const TFunction<void(TClass*, const TValue&)>& InSetter,
		const UINT32& InFlags,
		const EPropertyValueType& InValueType)
	{
		using T = std::remove_cv_t<std::remove_reference_t<TValue>>;
		EPropertyValueType ValueType = InValueType;
		if (ValueType == EPropertyValueType::NONE)
		{
			ValueType = GetPropertyValueType<TValue>();
		}

		FProperty Property;
		Property.Name = InName;
		Property.Flags = InFlags;
		Property.ValueType = ValueType;
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

		if constexpr (std::is_same_v<T, BOOL32>)
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
