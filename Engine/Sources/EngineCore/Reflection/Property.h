#pragma once

#include <CoreMinimal.h>
#include <Base/DataStructure/Transform.h>
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
		// Serializes the member into/from the archive; InContainer is the owning object.
		TFunction<void(EArchive&, void*)> Serialize;
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

	// Register a property backed by accessors. Used when the member is protected (most
	// OCA components) - the accessor lambdas are written where the class allows it.
	template <typename TClass, typename TValue>
	void AddClassPropertyAccessors(const size_t& InClassHash, const CHAR* InName,
		const TFunction<TValue(const TClass*)>& InGetter,
		const TFunction<void(TClass*, const TValue&)>& InSetter,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::NONE))
	{
		FProperty Property;
		Property.Name = InName;
		Property.Flags = InFlags;
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
		EClassPropertyRegistry::AddProperty(InClassHash, Property);
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
