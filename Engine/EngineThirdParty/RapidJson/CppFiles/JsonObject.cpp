
#include "../Headers/JsonObject.h"

using namespace rapidjson;
CJsonObject::CJsonObject()
{
	Doc = new rapidjson::Document();
	Doc->SetObject();
}

CJsonObject::CJsonObject(const std::string& JsonStr)
{
	Doc = new rapidjson::Document();
	Doc->Parse(JsonStr.c_str());
	assert(Doc->IsObject());
}

CJsonObject::CJsonObject(const char* JsonStr)
{
	Doc = new rapidjson::Document();
	Doc->Parse(JsonStr);
}

CJsonObject::CJsonObject(const CJsonObject& Other)
{
	Doc = new rapidjson::Document();
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

void CJsonObject::RemoveField(const std::string& FieldName)
{
	if (!HasField(FieldName))
		return;
	Doc->RemoveMember(FieldName.c_str());
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
	rapidjson::Document* OutDoc = new rapidjson::Document;
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
		rapidjson::Document* TempDoc = new rapidjson::Document;
		TempDoc->CopyFrom(v, Doc->GetAllocator());
		CJsonObject* Obj = new CJsonObject(TempDoc);
		Out.push_back(Obj);
	}
	return true;
}

void CJsonObject::SetIntField(const std::string& FieldName, const INT& In)
{
	if (HasField(FieldName))
	{
		(*Doc)[FieldName.c_str()] = In;
		return;
	}
	std::string* newK = new std::string(FieldName);
	Doc->AddMember(StringRef(newK->c_str()), In, Doc->GetAllocator());
}

void CJsonObject::SetInt64Field(const std::string& FieldName, const INT64& In)
{
	if (HasField(FieldName))
	{
		(*Doc)[FieldName.c_str()] = In;
		return;
	}
	std::string* newK = new std::string(FieldName);
	Doc->AddMember(StringRef(newK->c_str()), In, Doc->GetAllocator());
}

void CJsonObject::SetUIntField(const std::string& FieldName, const UINT& In)
{
	if (HasField(FieldName))
	{
		(*Doc)[FieldName.c_str()] = In;
		return;
	}
	std::string* newK = new std::string(FieldName);
	Doc->AddMember(StringRef(newK->c_str()), In, Doc->GetAllocator());
}

void CJsonObject::SetUInt64Field(const std::string& FieldName, const UINT64& In)
{
	if (HasField(FieldName))
	{
		(*Doc)[FieldName.c_str()] = In;
		return;
	}
	std::string* newK = new std::string(FieldName);
	Doc->AddMember(StringRef(newK->c_str()), In, Doc->GetAllocator());
}

void CJsonObject::SetFloatField(const std::string& FieldName, const FLOAT& In)
{
	if (HasField(FieldName))
	{
		(*Doc)[FieldName.c_str()] = In;
		return;
	}
	std::string* newK = new std::string(FieldName);
	Doc->AddMember(StringRef(newK->c_str()), In, Doc->GetAllocator());
}

void CJsonObject::SetDoubleField(const std::string& FieldName, const DOUBLE& In)
{
	if (HasField(FieldName))
	{
		(*Doc)[FieldName.c_str()] = In;
		return;
	}
	std::string* newK = new std::string(FieldName);
	Doc->AddMember(StringRef(newK->c_str()), In, Doc->GetAllocator());
}

void CJsonObject::SetStringField(const std::string& FieldName, const std::string& In)
{
	rapidjson::Value Val(kStringType);
	if (HasField(FieldName))
	{
		(*Doc)[FieldName.c_str()] = Val.SetString(In.c_str(), Doc->GetAllocator());
		return;
	}
	std::string* newK = new std::string(FieldName);
	Val.SetString(In.c_str(), Doc->GetAllocator());
	Doc->AddMember(StringRef(newK->c_str()), Val, Doc->GetAllocator());
}

void CJsonObject::SetBoolField(const std::string& FieldName, const BOOL& In)
{
	if (HasField(FieldName))
	{
		(*Doc)[FieldName.c_str()] = In;
		return;
	}
	std::string* newK = new std::string(FieldName);
	Doc->AddMember(StringRef(newK->c_str()), In, Doc->GetAllocator());
}

void CJsonObject::SetObjectField(const std::string& FieldName, const CJsonObject* In)
{
	std::string* newK = new std::string(FieldName);
	rapidjson::Value Val(rapidjson::Type::kObjectType);
	for (Value::ConstMemberIterator it = In->GetDocument()->MemberBegin(); it != In->GetDocument()->MemberEnd(); ++it)
	{
		rapidjson::Value Temp;
		Temp.CopyFrom(it->value, Doc->GetAllocator());
		Val.AddMember(StringRef(it->name.GetString()), Temp, Doc->GetAllocator());
	}
	if (HasField(FieldName))
	{
		(*Doc)[FieldName.c_str()] = Val;
		return;
	}
	Doc->AddMember(StringRef(newK->c_str()), Val, Doc->GetAllocator());
}

void CJsonObject::SetIntArrayField(const std::string& FieldName, const std::vector<INT>& In)
{
	std::string* newK = new std::string(FieldName);
	rapidjson::Value Val(rapidjson::Type::kArrayType);
	for (auto& elem : In)
	{
		Value Temp(kNumberType);
		Temp = elem;
		Val.PushBack(Temp, Doc->GetAllocator());
	}
	if (HasField(FieldName))
	{
		(*Doc)[FieldName.c_str()] = Val;
		return;
	}
	Doc->AddMember(StringRef(newK->c_str()), Val, Doc->GetAllocator());
}

void CJsonObject::SetInt64ArrayField(const std::string& FieldName, const std::vector<INT64>& In)
{
	std::string* newK = new std::string(FieldName);
	rapidjson::Value Val(rapidjson::Type::kArrayType);
	for (auto& elem : In)
	{
		Value Temp(kNumberType);
		Temp = elem;
		Val.PushBack(Temp, Doc->GetAllocator());
	}
	if (HasField(FieldName))
	{
		(*Doc)[FieldName.c_str()] = Val;
		return;
	}
	Doc->AddMember(StringRef(newK->c_str()), Val, Doc->GetAllocator());
}

void CJsonObject::SetUIntArrayField(const std::string& FieldName, const std::vector<UINT>& In)
{
	std::string* newK = new std::string(FieldName);
	rapidjson::Value Val(rapidjson::Type::kArrayType);
	for (auto& elem : In)
	{
		Value Temp(kNumberType);
		Temp = elem;
		Val.PushBack(Temp, Doc->GetAllocator());
	}
	if (HasField(FieldName))
	{
		(*Doc)[FieldName.c_str()] = Val;
		return;
	}
	Doc->AddMember(StringRef(newK->c_str()), Val, Doc->GetAllocator());
}

void CJsonObject::SetUInt64ArrayField(const std::string& FieldName, const std::vector<UINT64>& In)
{
	std::string* newK = new std::string(FieldName);
	rapidjson::Value Val(rapidjson::Type::kArrayType);
	for (auto& elem : In)
	{
		Value Temp(kNumberType);
		Temp = elem;
		Val.PushBack(Temp, Doc->GetAllocator());
	}
	if (HasField(FieldName))
	{
		(*Doc)[FieldName.c_str()] = Val;
		return;
	}
	Doc->AddMember(StringRef(newK->c_str()), Val, Doc->GetAllocator());
}

void CJsonObject::SetFloatArrayField(const std::string& FieldName, const std::vector<FLOAT>& In)
{
	std::string* newK = new std::string(FieldName);
	rapidjson::Value Val(rapidjson::Type::kArrayType);
	for (auto& elem : In)
	{
		Value Temp(kNumberType);
		Temp = elem;
		Val.PushBack(Temp, Doc->GetAllocator());
	}
	if (HasField(FieldName))
	{
		(*Doc)[FieldName.c_str()] = Val;
		return;
	}
	Doc->AddMember(StringRef(newK->c_str()), Val, Doc->GetAllocator());
}

void CJsonObject::SetDoubleArrayField(const std::string& FieldName, const std::vector<DOUBLE>& In)
{
	std::string* newK = new std::string(FieldName);
	rapidjson::Value Val(rapidjson::Type::kArrayType);
	for (auto& elem : In)
	{
		Value Temp(kNumberType);
		Temp = elem;
		Val.PushBack(Temp, Doc->GetAllocator());
	}
	if (HasField(FieldName))
	{
		(*Doc)[FieldName.c_str()] = Val;
		return;
	}
	Doc->AddMember(StringRef(newK->c_str()), Val, Doc->GetAllocator());
}

void CJsonObject::SetStringArrayField(const std::string& FieldName, const std::vector<std::string>& In)
{
	std::string* newK = new std::string(FieldName);
	rapidjson::Value Val(rapidjson::Type::kArrayType);
	for (auto& elem : In)
	{
		Value Temp(kStringType);
		Temp.SetString(elem.c_str(), Doc->GetAllocator());
		Val.PushBack(Temp, Doc->GetAllocator());
	}
	if (HasField(FieldName))
	{
		(*Doc)[FieldName.c_str()] = Val;
		return;
	}
	Doc->AddMember(StringRef(newK->c_str()), Val, Doc->GetAllocator());
}

void CJsonObject::SetBoolArrayField(const std::string& FieldName, const std::vector<BOOL>& In)
{
	std::string* newK = new std::string(FieldName);
	rapidjson::Value Val(rapidjson::Type::kArrayType);
	for (auto& elem : In)
	{
		Value Temp(elem);
		Val.PushBack(Temp, Doc->GetAllocator());
	}
	if (HasField(FieldName))
	{
		(*Doc)[FieldName.c_str()] = Val;
		return;
	}
	Doc->AddMember(StringRef(newK->c_str()), Val, Doc->GetAllocator());
}

void CJsonObject::SetObjectArrayField(const std::string& FieldName, const std::vector<CJsonObject*>& In)
{
	std::string* newK = new std::string(FieldName);
	rapidjson::Value Val(rapidjson::Type::kArrayType);
	for (auto& elem : In)
	{
		Value TempObj(kObjectType);
		for (Value::ConstMemberIterator it = elem->GetDocument()->MemberBegin(); it != elem->GetDocument()->MemberEnd(); ++it)
		{
			rapidjson::Value Temp;
			Temp.CopyFrom(it->value, Doc->GetAllocator());
			TempObj.AddMember(StringRef(it->name.GetString()), Temp, Doc->GetAllocator());
		}
		Val.PushBack(TempObj, Doc->GetAllocator());
	}
	if (HasField(FieldName))
	{
		(*Doc)[FieldName.c_str()] = Val;
		return;
	}
	Doc->AddMember(StringRef(newK->c_str()), Val, Doc->GetAllocator());
}


