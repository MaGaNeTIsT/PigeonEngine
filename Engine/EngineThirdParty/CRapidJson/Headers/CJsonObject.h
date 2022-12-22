#pragma once
#include "../../../../Entry/EngineMain.h"
#define RAPIDJSON_NOMEMBERITERATORCLASS 
#include "../../../../ThirdParty/rapidjson/document.h"
#include "../../../../ThirdParty/rapidjson/writer.h"
#include "../../../../ThirdParty/rapidjson/stringbuffer.h"

using namespace rapidjson;

class CJsonObject
{

public:
	CJsonObject();

	CJsonObject(const std::string& JsonStr);

public:
	BOOL GetIntegerField(const std::string& FieldName);
	BOOL GetNumberField(const std::string& FieldName);
	BOOL GetArrayField(const std::string& FieldName);
private:

	Document Doc;
};

