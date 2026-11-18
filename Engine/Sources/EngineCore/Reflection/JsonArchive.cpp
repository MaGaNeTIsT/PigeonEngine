#include "JsonArchive.h"
#include <RapidJson/Headers/JsonObject.h>

namespace PigeonEngine
{
	EJsonArchiveWriter::EJsonArchiveWriter(CJsonObject& InRoot)
		: Root(&InRoot)
	{
	}

	CJsonObject* EJsonArchiveWriter::CurrentScope()
	{
		if (ObjectStack.Num<UINT32>() > 0u)
		{
			return (&ObjectStack.LastRef());
		}
		return Root;
	}

	const CJsonObject* EJsonArchiveWriter::CurrentScope() const
	{
		if (ObjectStack.Num<UINT32>() > 0u)
		{
			return (&ObjectStack[ObjectStack.Num<UINT32>() - 1u]);
		}
		return Root;
	}

	void EJsonArchiveWriter::BeginObject(const CHAR* InName)
	{
		// The child stays a standalone object while it is being filled; EndObject copies
		// it into its parent as one completed value.
		ObjectStack.Add(CJsonObject());
		ScopeNames.Add(EString(InName));
	}

	void EJsonArchiveWriter::EndObject()
	{
		if ((ObjectStack.Num<UINT32>() == 0u) || (ScopeNames.Num<UINT32>() == 0u))
		{
			return;
		}
		const UINT32 ChildIndex = ObjectStack.Num<UINT32>() - 1u;
		const UINT32 NameIndex = ScopeNames.Num<UINT32>() - 1u;
		CJsonObject Child = ObjectStack[ChildIndex];
		const EString Name = ScopeNames[NameIndex];
		ObjectStack.RemoveAt(ChildIndex);
		ScopeNames.RemoveAt(NameIndex);
		CurrentScope()->SetObjectField(Name, &Child);
	}

	void EJsonArchiveWriter::BeginArray(const CHAR* InName, UINT32& InOutCount)
	{
		// The JSON wrapper has no incremental array append, so an array is an object with
		// a Count field plus index keys. The interface stays array-shaped for a binary
		// backend that stores real arrays.
		BeginObject(InName);
		CurrentScope()->SetUIntField(EString(PE_TEXT("Count")), InOutCount);
	}

	void EJsonArchiveWriter::EndArray()
	{
		EndObject();
	}

	void EJsonArchiveWriter::SerializeBool(const CHAR* InName, BOOL32& InOutValue)
	{
		CurrentScope()->SetBoolField(EString(InName), InOutValue != 0);
	}

	void EJsonArchiveWriter::SerializeInt32(const CHAR* InName, INT32& InOutValue)
	{
		CurrentScope()->SetIntField(EString(InName), InOutValue);
	}

	void EJsonArchiveWriter::SerializeUInt32(const CHAR* InName, UINT32& InOutValue)
	{
		CurrentScope()->SetUIntField(EString(InName), InOutValue);
	}

	void EJsonArchiveWriter::SerializeInt64(const CHAR* InName, INT64& InOutValue)
	{
		CurrentScope()->SetInt64Field(EString(InName), InOutValue);
	}

	void EJsonArchiveWriter::SerializeUInt64(const CHAR* InName, UINT64& InOutValue)
	{
		CurrentScope()->SetUInt64Field(EString(InName), InOutValue);
	}

	void EJsonArchiveWriter::SerializeFloat(const CHAR* InName, FLOAT& InOutValue)
	{
		CurrentScope()->SetFloatField(EString(InName), InOutValue);
	}

	void EJsonArchiveWriter::SerializeDouble(const CHAR* InName, DOUBLE& InOutValue)
	{
		CurrentScope()->SetDoubleField(EString(InName), InOutValue);
	}

	void EJsonArchiveWriter::SerializeString(const CHAR* InName, EString& InOutValue)
	{
		CurrentScope()->SetStringField(EString(InName), InOutValue);
	}

	BOOL32 EJsonArchiveWriter::HasField(const CHAR* InName) const
	{
		(void)InName;
		return TRUE;
	}

	EJsonArchiveReader::EJsonArchiveReader(const CJsonObject& InRoot)
	{
		ObjectStack.Add(InRoot);
	}

	CJsonObject* EJsonArchiveReader::CurrentScope()
	{
		if (ObjectStack.Num<UINT32>() > 0u)
		{
			return (&ObjectStack.LastRef());
		}
		return nullptr;
	}

	void EJsonArchiveReader::BeginObject(const CHAR* InName)
	{
		// A missing scope pushes an empty object: every read inside it misses through
		// HasField and the object keeps its current state.
		CJsonObject Child;
		CJsonObject* Parent = CurrentScope();
		if ((!Parent) || (!Parent->GetObjectField(EString(InName), Child)))
		{
			Child = CJsonObject();
		}
		ObjectStack.Add(Child);
	}

	void EJsonArchiveReader::EndObject()
	{
		if (ObjectStack.Num<UINT32>() > 1u)
		{
			ObjectStack.RemoveAt(ObjectStack.Num<UINT32>() - 1u);
		}
	}

	void EJsonArchiveReader::BeginArray(const CHAR* InName, UINT32& InOutCount)
	{
		InOutCount = 0u;
		BeginObject(InName);
		CJsonObject* Scope = CurrentScope();
		if (Scope)
		{
			Scope->GetUIntField(EString(PE_TEXT("Count")), InOutCount);
		}
	}

	void EJsonArchiveReader::EndArray()
	{
		EndObject();
	}

	void EJsonArchiveReader::SerializeBool(const CHAR* InName, BOOL32& InOutValue)
	{
		BOOL8 Stored = 0;
		CJsonObject* Scope = CurrentScope();
		if (Scope && Scope->GetBoolField(EString(InName), Stored))
		{
			InOutValue = (Stored != 0) ? TRUE : FALSE;
		}
	}

	void EJsonArchiveReader::SerializeInt32(const CHAR* InName, INT32& InOutValue)
	{
		CJsonObject* Scope = CurrentScope();
		if (Scope && Scope->GetIntField(EString(InName), InOutValue))
		{
			return;
		}
	}

	void EJsonArchiveReader::SerializeUInt32(const CHAR* InName, UINT32& InOutValue)
	{
		CJsonObject* Scope = CurrentScope();
		if (Scope && Scope->GetUIntField(EString(InName), InOutValue))
		{
			return;
		}
	}

	void EJsonArchiveReader::SerializeInt64(const CHAR* InName, INT64& InOutValue)
	{
		CJsonObject* Scope = CurrentScope();
		if (Scope && Scope->GetInt64Field(EString(InName), InOutValue))
		{
			return;
		}
	}

	void EJsonArchiveReader::SerializeUInt64(const CHAR* InName, UINT64& InOutValue)
	{
		CJsonObject* Scope = CurrentScope();
		if (Scope && Scope->GetUInt64Field(EString(InName), InOutValue))
		{
			return;
		}
	}

	void EJsonArchiveReader::SerializeFloat(const CHAR* InName, FLOAT& InOutValue)
	{
		CJsonObject* Scope = CurrentScope();
		if (Scope && Scope->GetFloatField(EString(InName), InOutValue))
		{
			return;
		}
	}

	void EJsonArchiveReader::SerializeDouble(const CHAR* InName, DOUBLE& InOutValue)
	{
		CJsonObject* Scope = CurrentScope();
		if (Scope && Scope->GetDoubleField(EString(InName), InOutValue))
		{
			return;
		}
	}

	void EJsonArchiveReader::SerializeString(const CHAR* InName, EString& InOutValue)
	{
		CJsonObject* Scope = CurrentScope();
		if (Scope && Scope->GetStringField(EString(InName), InOutValue))
		{
			return;
		}
	}

	BOOL32 EJsonArchiveReader::HasField(const CHAR* InName) const
	{
		CJsonObject* Scope = const_cast<EJsonArchiveReader*>(this)->CurrentScope();
		return (Scope && Scope->HasField(EString(InName))) ? TRUE : FALSE;
	}

}
