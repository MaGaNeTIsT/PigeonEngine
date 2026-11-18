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
	* scopes nest through BeginObject/EndObject, arrays through BeginArray/EndArray.
	*/
	class EArchive
	{
	public:
		virtual ~EArchive() = default;

		PE_NODISCARD virtual BOOL32 IsSaving() const = 0;

		virtual void BeginObject(const CHAR* InName) = 0;
		virtual void EndObject() = 0;

		// Indexed array scope. InOutCount is the element count: written on save, read on
		// load (0 when the array is absent). While the scope is open, element values are
		// serialized under their index as a string key ("0", "1", ...), so the same
		// interface works for the object-based JSON backend and a real array in a binary
		// backend.
		virtual void BeginArray(const CHAR* InName, UINT32& InOutCount) = 0;
		virtual void EndArray() = 0;

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
