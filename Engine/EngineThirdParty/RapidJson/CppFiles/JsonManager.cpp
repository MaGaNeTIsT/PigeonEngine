#include "../Headers/JsonManager.h"
#include "../../../ThirdParty/rapidjson/writer.h"
#include "../../../ThirdParty/rapidjson/stringbuffer.h"
#include "../../../ThirdParty/rapidjson/prettywriter.h"
#include "../../../ThirdParty/rapidjson/writer.h"

namespace PigeonEngine
{

	CJsonObject* CJsonManager::ConstructJsonObject()
	{
		return new CJsonObject();
	}

	void CJsonManager::Deserialize(const std::string& JsonStr, std::vector<CJsonObject*>& Out)
	{
		rapidjson::Document* Doc = new rapidjson::Document;
		Doc->Parse(JsonStr.c_str());
		assert(!Doc->HasParseError());

		if (Doc->IsObject())
		{
			Out.push_back(new CJsonObject(Doc));
		}
		if (Doc->IsArray())
		{
			for (auto& v : Doc->GetArray())
			{
				assert(v.IsObject());
				rapidjson::Document* TempDoc = new rapidjson::Document;
				TempDoc->CopyFrom(v, Doc->GetAllocator());
				CJsonObject* Obj = new CJsonObject(TempDoc);
				Out.push_back(Obj);
			}
		}
	}

	std::string CJsonManager::Serialize(const CJsonObject* Obj)
	{
		rapidjson::StringBuffer buffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		Obj->GetDocument()->Accept(writer);
		std::string temp(buffer.GetString());
		return temp;
	}

	std::string CJsonManager::Serialize(const std::vector<CJsonObject*>& Obj)
	{
		return std::string();
	}

};