#pragma once
#define RAPIDJSON_NOMEMBERITERATORCLASS 
#include "../../../ThirdParty/rapidjson/document.h"
#include "../../../Sources/EngineCore/Core/Headers/Main.h"
using namespace rapidjson;

namespace PigeonEngine
{

	class CJsonObject
	{
		friend class CJsonManager;
	public:

		CJsonObject();

		CJsonObject(const std::string& JsonStr);
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
		BOOL HasField(const std::string& FieldName);
		void RemoveField(const std::string& FieldName);

	public:
		/*Field Getter*/
		BOOL GetIntField(const std::string& FieldName, INT& Out);
		BOOL GetInt64Field(const std::string& FieldName, INT64& Out);
		BOOL GetUIntField(const std::string& FieldName, UINT& Out);
		BOOL GetUInt64Field(const std::string& FieldName, UINT64& Out);
		BOOL GetFloatField(const std::string& FieldName, FLOAT& Out);
		BOOL GetDoubleField(const std::string& FieldName, DOUBLE& Out);
		BOOL GetStringField(const std::string& FieldName, std::string& Out);
		BOOL GetBoolField(const std::string& FieldName, BOOL& Out);

		BOOL GetObjectField(const std::string& FieldName, CJsonObject& Out);

		BOOL GetIntArrayField(const std::string& FieldName, std::vector<INT>& Out);
		BOOL GetInt64ArrayField(const std::string& FieldName, std::vector<INT64>& Out);
		BOOL GetUIntArrayField(const std::string& FieldName, std::vector<UINT>& Out);
		BOOL GetUInt64ArrayField(const std::string& FieldName, std::vector<UINT64>& Out);
		BOOL GetFloatArrayField(const std::string& FieldName, std::vector<FLOAT>& Out);
		BOOL GetDoubleArrayField(const std::string& FieldName, std::vector<DOUBLE>& Out);
		BOOL GetStringArrayField(const std::string& FieldName, std::vector<std::string>& Out);
		BOOL GetBoolArrayField(const std::string& FieldName, std::vector<BOOL>& Out);

		BOOL GetObjectArrayField(const std::string& FieldName, std::vector<CJsonObject*>& Out);

	public:
		/*Field Setter*/
		void SetIntField(const std::string& FieldName, const INT& In);
		void SetInt64Field(const std::string& FieldName, const INT64& In);
		void SetUIntField(const std::string& FieldName, const UINT& In);
		void SetUInt64Field(const std::string& FieldName, const UINT64& In);
		void SetFloatField(const std::string& FieldName, const FLOAT& In);
		void SetDoubleField(const std::string& FieldName, const DOUBLE& In);
		void SetStringField(const std::string& FieldName, const std::string& In);
		void SetBoolField(const std::string& FieldName, const BOOL& In);

		void SetObjectField(const std::string& FieldName, const CJsonObject* In);

		void SetIntArrayField(const std::string& FieldName, const std::vector<INT>& In);
		void SetInt64ArrayField(const std::string& FieldName, const std::vector<INT64>& In);
		void SetUIntArrayField(const std::string& FieldName, const std::vector<UINT>& In);
		void SetUInt64ArrayField(const std::string& FieldName, const std::vector<UINT64>& In);
		void SetFloatArrayField(const std::string& FieldName, const std::vector<FLOAT>& In);
		void SetDoubleArrayField(const std::string& FieldName, const std::vector<DOUBLE>& In);
		void SetStringArrayField(const std::string& FieldName, const std::vector<std::string>& In);
		void SetBoolArrayField(const std::string& FieldName, const std::vector<BOOL>& In);

		void SetObjectArrayField(const std::string& FieldName, const std::vector<CJsonObject*>& In);

	private:

		rapidjson::Document* Doc;

	};

};