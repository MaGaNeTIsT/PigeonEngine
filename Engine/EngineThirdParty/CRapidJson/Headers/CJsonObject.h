#pragma once
#define RAPIDJSON_NOMEMBERITERATORCLASS 
#include "../../../../ThirdParty/rapidjson/document.h"
#include "../../../../Entry/EngineMain.h"


class CJsonObject
{
public:

	CJsonObject();

	CJsonObject(const std::string& JsonStr);
	CJsonObject(const char* JsonStr);
	CJsonObject(const CJsonObject& Other);
	CJsonObject(rapidjson::Document* const Other);
	~CJsonObject();

public:

	CJsonObject& operator=(const CJsonObject& Other);

public:

	rapidjson::Document* GetDocument()const;
	void SetDocument(rapidjson::Document* const InDoc);

public:

	/*Field query*/
	BOOL HasField(const std::string& FieldName);

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
	/*Field set*/
private:

	rapidjson::Document* Doc;
	
};

