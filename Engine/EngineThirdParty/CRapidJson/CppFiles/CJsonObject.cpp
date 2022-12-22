#include "../Headers/CJsonObject.h"
#include "../../../../ThirdParty/rapidjson/writer.h"
#include "../../../../ThirdParty/rapidjson/stringbuffer.h"
using namespace rapidjson;
CJsonObject::CJsonObject()
{
	Doc = new Document();
	Doc->SetObject();
}

CJsonObject::CJsonObject(const std::string& JsonStr)
{
	Doc = new Document();
	Doc->Parse(JsonStr.c_str());
	assert(Doc->IsObject());
}

CJsonObject::CJsonObject(const char* JsonStr)
{
	Doc = new Document();
	Doc->Parse(JsonStr);
}

CJsonObject::CJsonObject(const CJsonObject& Other)
{
	Doc = new Document();
	Doc->CopyFrom(*Other.GetDocument(), Other.GetDocument()->GetAllocator());
}

CJsonObject::CJsonObject(rapidjson::Document* const Other)
{
	Doc = Other;
}

CJsonObject::~CJsonObject()
{
	delete Doc;
}

CJsonObject& CJsonObject::operator=(const CJsonObject& Other)
{
	new (this)CJsonObject(Other);    
	return(*this);
}

rapidjson::Document* CJsonObject::GetDocument() const
{
	return Doc;
}

void CJsonObject::SetDocument(rapidjson::Document* const InDoc)
{
	delete Doc;
	Doc = InDoc;
}

BOOL CJsonObject::HasField(const std::string& FieldName)
{
	return Doc->HasMember(FieldName.c_str());
}

BOOL CJsonObject::GetIntField(const std::string& FieldName, INT& Out)
{
	Value::ConstMemberIterator itr = Doc->FindMember(FieldName.c_str());
	if (itr == Doc->MemberEnd() || !itr->value.IsInt())
		return false;
	Out = itr->value.GetInt();
	return true;
}

BOOL CJsonObject::GetInt64Field(const std::string& FieldName, INT64& Out)
{
	Value::ConstMemberIterator itr = Doc->FindMember(FieldName.c_str());
	if (itr == Doc->MemberEnd() || !itr->value.IsInt64())
		return false;
	Out = itr->value.GetInt64();
	return true;
}

BOOL CJsonObject::GetUIntField(const std::string& FieldName, UINT& Out)
{
	Value::ConstMemberIterator itr = Doc->FindMember(FieldName.c_str());
	if (itr == Doc->MemberEnd() || !itr->value.IsUint())
		return false;
	Out = itr->value.GetUint();
	return true;
}

BOOL CJsonObject::GetUInt64Field(const std::string& FieldName, UINT64& Out)
{
	Value::ConstMemberIterator itr = Doc->FindMember(FieldName.c_str());
	if (itr == Doc->MemberEnd() || !itr->value.IsUint64())
		return false;
	Out = itr->value.GetUint64();
	return true;
}

BOOL CJsonObject::GetFloatField(const std::string& FieldName, FLOAT& Out)
{
	Value::ConstMemberIterator itr = Doc->FindMember(FieldName.c_str());
	if (itr == Doc->MemberEnd() || !itr->value.IsFloat())
		return false;
	Out = itr->value.GetFloat();
	return true;
}

BOOL CJsonObject::GetDoubleField(const std::string& FieldName, DOUBLE& Out)
{
	Value::ConstMemberIterator itr = Doc->FindMember(FieldName.c_str());
	if (itr == Doc->MemberEnd() || !itr->value.IsDouble())
		return false;
	Out = itr->value.GetDouble();
	return true;
}

BOOL CJsonObject::GetStringField(const std::string& FieldName, std::string& Out)
{
	Value::ConstMemberIterator itr = Doc->FindMember(FieldName.c_str());
	if (itr == Doc->MemberEnd() || !itr->value.IsString())
		return false;
	Out = itr->value.GetString();
	return true;
}

BOOL CJsonObject::GetBoolField(const std::string& FieldName, BOOL& Out)
{
	Value::ConstMemberIterator itr = Doc->FindMember(FieldName.c_str());
	if (itr == Doc->MemberEnd() || !itr->value.IsBool())
		return false;
	Out = itr->value.GetBool();
	return true;
}

BOOL CJsonObject::GetObjectField(const std::string& FieldName, CJsonObject& Out)
{
	Value::ConstMemberIterator itr = Doc->FindMember(FieldName.c_str());
	if (itr == Doc->MemberEnd() || !itr->value.IsObject())
		return false;
	Document* OutDoc = new Document;
	OutDoc->CopyFrom(itr->value, Doc->GetAllocator());
	Out.SetDocument(OutDoc);
	return true;
}

BOOL CJsonObject::GetIntArrayField(const std::string& FieldName, std::vector<INT>& Out)
{
	Value::ConstMemberIterator itr = Doc->FindMember(FieldName.c_str());
	if (itr == Doc->MemberEnd() || !itr->value.IsArray())
		return false;
	for (auto& v : itr->value.GetArray())
	{
		if (!v.IsInt())
			return false;
		Out.push_back(v.GetInt());
	}
	return true;
}

BOOL CJsonObject::GetInt64ArrayField(const std::string& FieldName, std::vector<INT64>& Out)
{
	Value::ConstMemberIterator itr = Doc->FindMember(FieldName.c_str());
	if (itr == Doc->MemberEnd() || !itr->value.IsArray())
		return false;
	for (auto& v : itr->value.GetArray())
	{
		if (!v.IsInt64())
			return false;
		Out.push_back(v.GetInt64());
	}
	return true;
}

BOOL CJsonObject::GetUIntArrayField(const std::string& FieldName, std::vector<UINT>& Out)
{
	Value::ConstMemberIterator itr = Doc->FindMember(FieldName.c_str());
	if (itr == Doc->MemberEnd() || !itr->value.IsArray())
		return false;
	for (auto& v : itr->value.GetArray())
	{
		if (!v.IsUint())
			return false;
		Out.push_back(v.GetUint());
	}
	return true;
}

BOOL CJsonObject::GetUInt64ArrayField(const std::string& FieldName, std::vector<UINT64>& Out)
{
	Value::ConstMemberIterator itr = Doc->FindMember(FieldName.c_str());
	if (itr == Doc->MemberEnd() || !itr->value.IsArray())
		return false;
	for (auto& v : itr->value.GetArray())
	{
		if (!v.IsUint64())
			return false;
		Out.push_back(v.GetUint64());
	}
	return true;
}

BOOL CJsonObject::GetFloatArrayField(const std::string& FieldName, std::vector<FLOAT>& Out)
{
	Value::ConstMemberIterator itr = Doc->FindMember(FieldName.c_str());
	if (itr == Doc->MemberEnd() || !itr->value.IsArray())
		return false;
	for (auto& v : itr->value.GetArray())
	{
		if (!v.IsFloat())
			return false;
		Out.push_back(v.GetFloat());
	}
	return true;
}

BOOL CJsonObject::GetDoubleArrayField(const std::string& FieldName, std::vector<DOUBLE>& Out)
{
	Value::ConstMemberIterator itr = Doc->FindMember(FieldName.c_str());
	if (itr == Doc->MemberEnd() || !itr->value.IsArray())
		return false;
	for (auto& v : itr->value.GetArray())
	{
		if (!v.IsDouble())
			return false;
		Out.push_back(v.GetDouble());
	}
	return true;
}

BOOL CJsonObject::GetStringArrayField(const std::string& FieldName, std::vector<std::string>& Out)
{
	Value::ConstMemberIterator itr = Doc->FindMember(FieldName.c_str());
	if (itr == Doc->MemberEnd() || !itr->value.IsArray())
		return false;
	for (auto& v : itr->value.GetArray())
	{
		if (!v.IsString())
			return false;
		Out.push_back(v.GetString());
	}
	return true;
}

BOOL CJsonObject::GetBoolArrayField(const std::string& FieldName, std::vector<BOOL>& Out)
{
	Value::ConstMemberIterator itr = Doc->FindMember(FieldName.c_str());
	if (itr == Doc->MemberEnd() || !itr->value.IsArray())
		return false;
	for (auto& v : itr->value.GetArray())
	{
		if (!v.IsBool())
			return false;
		Out.push_back(v.GetBool());
	}
	return true;
}

BOOL CJsonObject::GetObjectArrayField(const std::string& FieldName, std::vector<CJsonObject*>& Out)
{
	Value::ConstMemberIterator itr = Doc->FindMember(FieldName.c_str());
	if (itr == Doc->MemberEnd() || !itr->value.IsArray())
		return false;
	for (auto& v : itr->value.GetArray())
	{
		if (!v.IsObject())
			return false;
		Document* TempDoc = new Document;
		TempDoc->CopyFrom(v, Doc->GetAllocator());
		CJsonObject* Obj = new CJsonObject(TempDoc);
		Out.push_back(Obj);
	}
	return true;
}


