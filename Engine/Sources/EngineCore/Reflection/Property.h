#pragma once

#include <CoreMinimal.h>
#include <Base/DataStructure/Transform.h>
#include "Archive.h"

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
	BOOL32 SerializeVector3(EArchive& InArchive, const CHAR* InName, Vector3& InOutValue);
	BOOL32 SerializeQuaternion(EArchive& InArchive, const CHAR* InName, Quaternion& InOutValue);
	BOOL32 SerializeTransform(EArchive& InArchive, const CHAR* InName, ETransform& InOutValue);

	// Overloads used by the property templates below.
	void SerializeValue(EArchive& InArchive, const CHAR* InName, BOOL32& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, INT32& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, UINT32& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, INT64& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, UINT64& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, FLOAT& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, DOUBLE& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, EString& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, Vector3& InOutValue);
	void SerializeValue(EArchive& InArchive, const CHAR* InName, Quaternion& InOutValue);
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

}
