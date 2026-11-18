#pragma once

#include <CoreMinimal.h>
#include <RapidJson/Headers/JsonObject.h>
#include "Archive.h"

namespace PigeonEngine
{
	class CJsonObject;

	/*
	* JSON backend of EArchive. The writer builds nested CJsonObject values bottom-up
	* (each closed scope is copied into its parent); the reader keeps a stack of scope
	* copies and treats a missing field as "leave the value alone", which is what lets an
	* older file load into a newer class.
	*
	* Reading is a whole-document copy per nested scope. That is fine at level-file sizes
	* and keeps the wrapper's ownership rules simple; the binary backend will stream
	* instead.
	*/
	class EJsonArchiveWriter final : public EArchive
	{
	public:
		explicit EJsonArchiveWriter(CJsonObject& InRoot);

	public:
		PE_NODISCARD virtual BOOL32 IsSaving() const override { return TRUE; }
		virtual void BeginObject(const CHAR* InName) override;
		virtual void EndObject() override;
		virtual void BeginArray(const CHAR* InName, UINT32& InOutCount) override;
		virtual void EndArray() override;
		virtual void SerializeBool(const CHAR* InName, BOOL32& InOutValue) override;
		virtual void SerializeInt32(const CHAR* InName, INT32& InOutValue) override;
		virtual void SerializeUInt32(const CHAR* InName, UINT32& InOutValue) override;
		virtual void SerializeInt64(const CHAR* InName, INT64& InOutValue) override;
		virtual void SerializeUInt64(const CHAR* InName, UINT64& InOutValue) override;
		virtual void SerializeFloat(const CHAR* InName, FLOAT& InOutValue) override;
		virtual void SerializeDouble(const CHAR* InName, DOUBLE& InOutValue) override;
		virtual void SerializeString(const CHAR* InName, EString& InOutValue) override;
		PE_NODISCARD virtual BOOL32 HasField(const CHAR* InName) const override;

	private:
		PE_NODISCARD CJsonObject* CurrentScope();
		PE_NODISCARD const CJsonObject* CurrentScope() const;

	private:
		CJsonObject* Root = nullptr;
		TArray<CJsonObject> ObjectStack;
		TArray<EString> ScopeNames;
	};

	class EJsonArchiveReader final : public EArchive
	{
	public:
		explicit EJsonArchiveReader(const CJsonObject& InRoot);

	public:
		PE_NODISCARD virtual BOOL32 IsSaving() const override { return FALSE; }
		virtual void BeginObject(const CHAR* InName) override;
		virtual void EndObject() override;
		virtual void BeginArray(const CHAR* InName, UINT32& InOutCount) override;
		virtual void EndArray() override;
		virtual void SerializeBool(const CHAR* InName, BOOL32& InOutValue) override;
		virtual void SerializeInt32(const CHAR* InName, INT32& InOutValue) override;
		virtual void SerializeUInt32(const CHAR* InName, UINT32& InOutValue) override;
		virtual void SerializeInt64(const CHAR* InName, INT64& InOutValue) override;
		virtual void SerializeUInt64(const CHAR* InName, UINT64& InOutValue) override;
		virtual void SerializeFloat(const CHAR* InName, FLOAT& InOutValue) override;
		virtual void SerializeDouble(const CHAR* InName, DOUBLE& InOutValue) override;
		virtual void SerializeString(const CHAR* InName, EString& InOutValue) override;
		PE_NODISCARD virtual BOOL32 HasField(const CHAR* InName) const override;

	private:
		PE_NODISCARD CJsonObject* CurrentScope();

	private:
		TArray<CJsonObject> ObjectStack;
	};

}
