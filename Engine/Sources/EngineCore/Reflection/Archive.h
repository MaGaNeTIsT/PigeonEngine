#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	/*
	* Direction-agnostic serializer interface used by property reflection and, later, by
	* level and asset formats.
	*
	* One archive wraps one write pass or one read pass. The JSON implementation
	* (EJsonArchiveWriter/EJsonArchiveReader) backs editing, diffs and debugging now; a
	* binary implementation can be added later - the archive system - without touching
	* any property or class code, because every serializer only talks to this interface.
	*
	* Naming: values live under a field name inside the current object scope. Object
	* scopes nest through BeginObject/EndObject. Arrays are deliberately absent for now;
	* they arrive with the level format that needs them (Phase 3C), and adding them here
	* then only touches the two backends.
	*/
	class EArchive
	{
	public:
		virtual ~EArchive() = default;

		PE_NODISCARD virtual BOOL32 IsSaving() const = 0;

		virtual void BeginObject(const CHAR* InName) = 0;
		virtual void EndObject() = 0;

		virtual void SerializeBool(const CHAR* InName, BOOL32& InOutValue) = 0;
		virtual void SerializeInt32(const CHAR* InName, INT32& InOutValue) = 0;
		virtual void SerializeUInt32(const CHAR* InName, UINT32& InOutValue) = 0;
		virtual void SerializeInt64(const CHAR* InName, INT64& InOutValue) = 0;
		virtual void SerializeUInt64(const CHAR* InName, UINT64& InOutValue) = 0;
		virtual void SerializeFloat(const CHAR* InName, FLOAT& InOutValue) = 0;
		virtual void SerializeDouble(const CHAR* InName, DOUBLE& InOutValue) = 0;
		virtual void SerializeString(const CHAR* InName, EString& InOutValue) = 0;

		// When reading, TRUE when the current scope has this field. Lets an older file
		// omit fields the current class gained; when writing, always TRUE.
		PE_NODISCARD virtual BOOL32 HasField(const CHAR* InName) const = 0;
	};

}
