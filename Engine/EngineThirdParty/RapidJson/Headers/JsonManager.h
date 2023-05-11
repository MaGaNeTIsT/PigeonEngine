#pragma once

#include "JsonObject.h"

namespace PigeonEngine
{

	class CJsonManager
	{
	public:
		// Construct a empty json object
		static CJsonObject* ConstructJsonObject();

		static void Deserialize(const std::string& JsonStr, std::vector<CJsonObject*>& Out);

		static std::string Serialize(const CJsonObject* Obj);
		static std::string Serialize(const std::vector<CJsonObject*>& Obj);
	};

};