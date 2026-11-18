#include "Property.h"

namespace PigeonEngine
{
	namespace
	{
		TMap<size_t, TArray<FProperty>>& GetPropertyMap()
		{
			static TMap<size_t, TArray<FProperty>> Properties;
			return Properties;
		}

		TMap<size_t, UINT32>& GetClassVersions()
		{
			static TMap<size_t, UINT32> Versions;
			return Versions;
		}

		TMap<size_t, TFunction<void(void*, UINT32)>>& GetClassMigrators()
		{
			static TMap<size_t, TFunction<void(void*, UINT32)>> Migrators;
			return Migrators;
		}
	}

	void EClassPropertyRegistry::SetClassVersion(const size_t& InClassHash, const UINT32& InVersion)
	{
		GetClassVersions().Add(InClassHash, InVersion);
	}

	UINT32 EClassPropertyRegistry::GetClassVersion(const size_t& InClassHash)
	{
		UINT32 Version = 1u;
		GetClassVersions().FindValue(InClassHash, Version);
		return Version;
	}

	void EClassPropertyRegistry::SetClassMigrator(const size_t& InClassHash, const TFunction<void(void*, UINT32)>& InMigrator)
	{
		GetClassMigrators().Add(InClassHash, InMigrator);
	}

	const TFunction<void(void*, UINT32)>* EClassPropertyRegistry::FindClassMigrator(const size_t& InClassHash)
	{
		return GetClassMigrators().FindValueAsPtr(InClassHash);
	}

	void EClassPropertyRegistry::AddProperty(const size_t& InClassHash, const FProperty& InProperty)
	{
		if (!InProperty.Serialize)
		{
			return;
		}
		TArray<FProperty>& List = GetPropertyMap()[InClassHash];
		List.Add(InProperty);
	}

	const TArray<FProperty>* EClassPropertyRegistry::FindProperties(const size_t& InClassHash)
	{
		return GetPropertyMap().FindValueAsPtr(InClassHash);
	}

	void EClassPropertyRegistry::SerializeObject(EArchive& InArchive, const size_t& InClassHash, void* InContainer)
	{
		if (!InContainer)
		{
			return;
		}
		// Collect the single-inheritance chain and walk it root first: base properties are
		// written before derived ones, and when reading a file that lacks a newer field the
		// field keeps the value its constructor gave it.
		TArray<size_t> Chain;
		for (size_t ClassHash = InClassHash; ClassHash != static_cast<size_t>(-1); ClassHash = GetClassParentHash(ClassHash))
		{
			Chain.Add(ClassHash);
		}
		for (INT32 Index = Chain.Num<INT32>() - 1; Index >= 0; Index--)
		{
			const TArray<FProperty>* Properties = FindProperties(Chain[static_cast<UINT32>(Index)]);
			if (!Properties)
			{
				continue;
			}
			for (UINT32 PropertyIndex = 0u, PropertyNum = Properties->Num<UINT32>(); PropertyIndex < PropertyNum; PropertyIndex++)
			{
				const FProperty& Property = (*Properties)[PropertyIndex];
				if ((Property.Flags & static_cast<UINT32>(EPropertyFlagsType::TRANSIENT)) != 0u)
				{
					continue;
				}
				if (Property.Serialize)
				{
					Property.Serialize(InArchive, InContainer);
				}
			}
		}
	}

	BOOL32 SerializeVector3(EArchive& InArchive, const CHAR* InName, Vector3& InOutValue)
	{
		InArchive.BeginObject(InName);
		InArchive.SerializeFloat(PE_TEXT("X"), InOutValue.x);
		InArchive.SerializeFloat(PE_TEXT("Y"), InOutValue.y);
		InArchive.SerializeFloat(PE_TEXT("Z"), InOutValue.z);
		InArchive.EndObject();
		return TRUE;
	}

	BOOL32 SerializeQuaternion(EArchive& InArchive, const CHAR* InName, Quaternion& InOutValue)
	{
		InArchive.BeginObject(InName);
		InArchive.SerializeFloat(PE_TEXT("X"), InOutValue.x);
		InArchive.SerializeFloat(PE_TEXT("Y"), InOutValue.y);
		InArchive.SerializeFloat(PE_TEXT("Z"), InOutValue.z);
		InArchive.SerializeFloat(PE_TEXT("W"), InOutValue.w);
		InArchive.EndObject();
		return TRUE;
	}

	BOOL32 SerializeTransform(EArchive& InArchive, const CHAR* InName, ETransform& InOutValue)
	{
		Vector3 Location = InOutValue.GetLocation_Local();
		Quaternion Rotation = InOutValue.GetRotation_Local();
		Vector3 Scaling = InOutValue.GetScaling_Local();
		InArchive.BeginObject(InName);
		SerializeVector3(InArchive, PE_TEXT("Location"), Location);
		SerializeQuaternion(InArchive, PE_TEXT("Rotation"), Rotation);
		SerializeVector3(InArchive, PE_TEXT("Scaling"), Scaling);
		InArchive.EndObject();
		if (!InArchive.IsSaving())
		{
			InOutValue = ETransform(Location, Rotation, Scaling);
		}
		return TRUE;
	}

	void SerializeValue(EArchive& InArchive, const CHAR* InName, BOOL32& InOutValue)
	{
		InArchive.SerializeBool(InName, InOutValue);
	}

	void SerializeValue(EArchive& InArchive, const CHAR* InName, INT32& InOutValue)
	{
		InArchive.SerializeInt32(InName, InOutValue);
	}

	void SerializeValue(EArchive& InArchive, const CHAR* InName, UINT32& InOutValue)
	{
		InArchive.SerializeUInt32(InName, InOutValue);
	}

	void SerializeValue(EArchive& InArchive, const CHAR* InName, INT64& InOutValue)
	{
		InArchive.SerializeInt64(InName, InOutValue);
	}

	void SerializeValue(EArchive& InArchive, const CHAR* InName, UINT64& InOutValue)
	{
		InArchive.SerializeUInt64(InName, InOutValue);
	}

	void SerializeValue(EArchive& InArchive, const CHAR* InName, FLOAT& InOutValue)
	{
		InArchive.SerializeFloat(InName, InOutValue);
	}

	void SerializeValue(EArchive& InArchive, const CHAR* InName, DOUBLE& InOutValue)
	{
		InArchive.SerializeDouble(InName, InOutValue);
	}

	void SerializeValue(EArchive& InArchive, const CHAR* InName, EString& InOutValue)
	{
		InArchive.SerializeString(InName, InOutValue);
	}

	void SerializeValue(EArchive& InArchive, const CHAR* InName, Vector3& InOutValue)
	{
		SerializeVector3(InArchive, InName, InOutValue);
	}

	void SerializeValue(EArchive& InArchive, const CHAR* InName, Quaternion& InOutValue)
	{
		SerializeQuaternion(InArchive, InName, InOutValue);
	}

	void SerializeValue(EArchive& InArchive, const CHAR* InName, ETransform& InOutValue)
	{
		SerializeTransform(InArchive, InName, InOutValue);
	}

}
