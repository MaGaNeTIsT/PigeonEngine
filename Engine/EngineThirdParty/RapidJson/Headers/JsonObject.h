#pragma once
#define RAPIDJSON_NOMEMBERITERATORCLASS 
#include "../../../ThirdParty/rapidjson/document.h"
#include <CoreMinimal.h>
using namespace rapidjson;

namespace PigeonEngine
{

	class CJsonObject
	{
		friend class CJsonManager;
	public:

		CJsonObject();

		CJsonObject(const EString& JsonStr);
		CJsonObject(const char* JsonStr);
		CJsonObject(const CJsonObject& Other);
		CJsonObject(rapidjson::Document* const Other);
		~CJsonObject();

	public:

		CJsonObject& operator=(const CJsonObject& Other);

	protected:

		rapidjson::Document* GetDocument()const;
		void SetDocument(rapidjson::Document* const InDoc);

	public:
		
		/*Field query*/
		BOOL8 HasField(const EString& FieldName);
		void RemoveField(const EString& FieldName);

	public:
		/*Field Getter*/
		BOOL8 GetIntField   (const EString& FieldName, INT32& Out);
		BOOL8 GetInt64Field (const EString& FieldName, INT64& Out);
		BOOL8 GetUIntField  (const EString& FieldName, UINT32& Out);
		BOOL8 GetUInt64Field(const EString& FieldName, UINT64& Out);
		BOOL8 GetFloatField (const EString& FieldName, FLOAT& Out);
		BOOL8 GetDoubleField(const EString& FieldName, DOUBLE& Out);
		BOOL8 GetStringField(const EString& FieldName, EString& Out);
		BOOL8 GetBoolField  (const EString& FieldName, BOOL8& Out);

		BOOL8 GetObjectField(const EString& FieldName, CJsonObject& Out);

		BOOL8 GetIntArrayField   (const EString& FieldName, TArray<INT32>& Out);
		BOOL8 GetInt64ArrayField (const EString& FieldName, TArray<INT64>& Out);
		BOOL8 GetUIntArrayField  (const EString& FieldName, TArray<UINT32>& Out);
		BOOL8 GetUInt64ArrayField(const EString& FieldName, TArray<UINT64>& Out);
		BOOL8 GetFloatArrayField (const EString& FieldName, TArray<FLOAT>& Out);
		BOOL8 GetDoubleArrayField(const EString& FieldName, TArray<DOUBLE>& Out);
		BOOL8 GetStringArrayField(const EString& FieldName, TArray<EString>& Out);
		BOOL8 GetBoolArrayField  (const EString& FieldName, TArray<BOOL8>& Out);

		BOOL8 GetObjectArrayField(const EString& FieldName, TArray<CJsonObject*>& Out);

	public:
		/*Field Setter*/
		void SetIntField   (const EString& FieldName, const INT32& In);
		void SetInt64Field (const EString& FieldName, const INT64& In);
		void SetUIntField  (const EString& FieldName, const UINT32& In);
		void SetUInt64Field(const EString& FieldName, const UINT64& In);
		void SetFloatField (const EString& FieldName, const FLOAT& In);
		void SetDoubleField(const EString& FieldName, const DOUBLE& In);
		void SetStringField(const EString& FieldName, const EString& In);
		void SetBoolField  (const EString& FieldName, const BOOL8& In);

		void SetObjectField(const EString& FieldName, const CJsonObject* In);

		void SetIntArrayField   (const EString& FieldName, const TArray<INT32>& In);
		void SetInt64ArrayField (const EString& FieldName, const TArray<INT64>& In);
		void SetUIntArrayField  (const EString& FieldName, const TArray<UINT32>& In);
		void SetUInt64ArrayField(const EString& FieldName, const TArray<UINT64>& In);
		void SetFloatArrayField (const EString& FieldName, const TArray<FLOAT>& In);
		void SetDoubleArrayField(const EString& FieldName, const TArray<DOUBLE>& In);
		void SetStringArrayField(const EString& FieldName, const TArray<EString>& In);
		void SetBoolArrayField  (const EString& FieldName, const TArray<BOOL8>& In);

		void SetObjectArrayField(const EString& FieldName, const TArray<CJsonObject*>& In);

	private:

		rapidjson::Document* Doc;

	};

};