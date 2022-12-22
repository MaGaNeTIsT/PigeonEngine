#include "../Headers/CJsonObject.h"

CJsonObject::CJsonObject()
{
}

CJsonObject::CJsonObject(const std::string& JsonStr)
{
	Doc.Parse(JsonStr.c_str());
}
