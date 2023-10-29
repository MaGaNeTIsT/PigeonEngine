#include "../Headers/JsonObject.h"
using namespace rapidjson;

namespace PigeonEngine
{

	CJsonObject::CJsonObject()
	{
		Doc = new rapidjson::Document();
		Doc->SetObject();
	}

	CJsonObject::CJsonObject(const EString& JsonStr)
	{
		Doc = new rapidjson::Document();
		Doc->Parse(*JsonStr);
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

	BOOL8 CJsonObject::HasField(const EString& FieldName)
	{
		return Doc->HasMember(*FieldName);
	}

	void CJsonObject::RemoveField(const EString& FieldName)
	{
		if (!HasField(FieldName))
			return;
		Doc->RemoveMember(*FieldName);
	}

	BOOL8 CJsonObject::GetIntField(const EString& FieldName, INT32& Out)
	{
		Value::ConstMemberIterator itr = Doc->FindMember(*FieldName);
		if (itr == Doc->MemberEnd() || !itr->value.IsInt())
			return FALSE;
		Out = itr->value.GetInt();
		return TRUE;
	}

	BOOL8 CJsonObject::GetInt64Field(const EString& FieldName, INT64& Out)
	{
		Value::ConstMemberIterator itr = Doc->FindMember(*FieldName);
		if (itr == Doc->MemberEnd() || !itr->value.IsInt64())
			return FALSE;
		Out = itr->value.GetInt64();
		return TRUE;
	}

	BOOL8 CJsonObject::GetUIntField(const EString& FieldName, UINT32& Out)
	{
		Value::ConstMemberIterator itr = Doc->FindMember(*FieldName);
		if (itr == Doc->MemberEnd() || !itr->value.IsUint())
			return FALSE;
		Out = itr->value.GetUint();
		return TRUE;
	}

	BOOL8 CJsonObject::GetUInt64Field(const EString& FieldName, UINT64& Out)
	{
		Value::ConstMemberIterator itr = Doc->FindMember(*FieldName);
		if (itr == Doc->MemberEnd() || !itr->value.IsUint64())
			return FALSE;
		Out = itr->value.GetUint64();
		return TRUE;
	}

	BOOL8 CJsonObject::GetFloatField(const EString& FieldName, FLOAT& Out)
	{
		Value::ConstMemberIterator itr = Doc->FindMember(*FieldName);
		if (itr == Doc->MemberEnd() || !itr->value.IsFloat())
			return FALSE;
		Out = itr->value.GetFloat();
		return TRUE;
	}

	BOOL8 CJsonObject::GetDoubleField(const EString& FieldName, DOUBLE& Out)
	{
		Value::ConstMemberIterator itr = Doc->FindMember(*FieldName);
		if (itr == Doc->MemberEnd() || !itr->value.IsDouble())
			return FALSE;
		Out = itr->value.GetDouble();
		return TRUE;
	}

	BOOL8 CJsonObject::GetStringField(const EString& FieldName, EString& Out)
	{
		Value::ConstMemberIterator itr = Doc->FindMember(*FieldName);
		if (itr == Doc->MemberEnd() || !itr->value.IsString())
			return FALSE;
		Out = itr->value.GetString();
		return TRUE;
	}

	BOOL8 CJsonObject::GetBoolField(const EString& FieldName, BOOL8& Out)
	{
		Value::ConstMemberIterator itr = Doc->FindMember(*FieldName);
		if (itr == Doc->MemberEnd() || !itr->value.IsBool())
			return FALSE;
		Out = itr->value.GetBool();
		return TRUE;
	}

	BOOL8 CJsonObject::GetObjectField(const EString& FieldName, CJsonObject& Out)
	{
		Value::ConstMemberIterator itr = Doc->FindMember(*FieldName);
		if (itr == Doc->MemberEnd() || !itr->value.IsObject())
			return FALSE;
		rapidjson::Document* OutDoc = new rapidjson::Document;
		OutDoc->CopyFrom(itr->value, Doc->GetAllocator());
		Out.SetDocument(OutDoc);
		return TRUE;
	}

	BOOL8 CJsonObject::GetIntArrayField(const EString& FieldName, TArray<INT32>& Out)
	{
		Value::ConstMemberIterator itr = Doc->FindMember(*FieldName);
		if (itr == Doc->MemberEnd() || !itr->value.IsArray())
			return FALSE;
		for (auto& v : itr->value.GetArray())
		{
			if (!v.IsInt())
				return FALSE;
			Out.Add(v.GetInt());
		}
		return TRUE;
	}

	BOOL8 CJsonObject::GetInt64ArrayField(const EString& FieldName, TArray<INT64>& Out)
	{
		Value::ConstMemberIterator itr = Doc->FindMember(*FieldName);
		if (itr == Doc->MemberEnd() || !itr->value.IsArray())
			return FALSE;
		for (auto& v : itr->value.GetArray())
		{
			if (!v.IsInt64())
				return FALSE;
			Out.Add(v.GetInt64());
		}
		return TRUE;
	}

	BOOL8 CJsonObject::GetUIntArrayField(const EString& FieldName, TArray<UINT32>& Out)
	{
		Value::ConstMemberIterator itr = Doc->FindMember(*FieldName);
		if (itr == Doc->MemberEnd() || !itr->value.IsArray())
			return FALSE;
		for (auto& v : itr->value.GetArray())
		{
			if (!v.IsUint())
				return FALSE;
			Out.Add(v.GetUint());
		}
		return TRUE;
	}

	BOOL8 CJsonObject::GetUInt64ArrayField(const EString& FieldName, TArray<UINT64>& Out)
	{
		Value::ConstMemberIterator itr = Doc->FindMember(*FieldName);
		if (itr == Doc->MemberEnd() || !itr->value.IsArray())
			return FALSE;
		for (auto& v : itr->value.GetArray())
		{
			if (!v.IsUint64())
				return FALSE;
			Out.Add(v.GetUint64());
		}
		return TRUE;
	}

	BOOL8 CJsonObject::GetFloatArrayField(const EString& FieldName, TArray<FLOAT>& Out)
	{
		Value::ConstMemberIterator itr = Doc->FindMember(*FieldName);
		if (itr == Doc->MemberEnd() || !itr->value.IsArray())
			return FALSE;
		for (auto& v : itr->value.GetArray())
		{
			if (!v.IsFloat())
				return FALSE;
			Out.Add(v.GetFloat());
		}
		return TRUE;
	}

	BOOL8 CJsonObject::GetDoubleArrayField(const EString& FieldName, TArray<DOUBLE>& Out)
	{
		Value::ConstMemberIterator itr = Doc->FindMember(*FieldName);
		if (itr == Doc->MemberEnd() || !itr->value.IsArray())
			return FALSE;
		for (auto& v : itr->value.GetArray())
		{
			if (!v.IsDouble())
				return FALSE;
			Out.Add(v.GetDouble());
		}
		return TRUE;
	}

	BOOL8 CJsonObject::GetStringArrayField(const EString& FieldName, TArray<EString>& Out)
	{
		Value::ConstMemberIterator itr = Doc->FindMember(*FieldName);
		if (itr == Doc->MemberEnd() || !itr->value.IsArray())
			return FALSE;
		for (auto& v : itr->value.GetArray())
		{
			if (!v.IsString())
				return FALSE;
			Out.Add(v.GetString());
		}
		return TRUE;
	}

	BOOL8 CJsonObject::GetBoolArrayField(const EString& FieldName, TArray<BOOL8>& Out)
	{
		Value::ConstMemberIterator itr = Doc->FindMember(*FieldName);
		if (itr == Doc->MemberEnd() || !itr->value.IsArray())
			return FALSE;
		for (auto& v : itr->value.GetArray())
		{
			if (!v.IsBool())
				return FALSE;
			Out.Add(v.GetBool());
		}
		return TRUE;
	}

	BOOL8 CJsonObject::GetObjectArrayField(const EString& FieldName, TArray<CJsonObject*>& Out)
	{
		Value::ConstMemberIterator itr = Doc->FindMember(*FieldName);
		if (itr == Doc->MemberEnd() || !itr->value.IsArray())
			return FALSE;
		for (auto& v : itr->value.GetArray())
		{
			if (!v.IsObject())
				return FALSE;
			rapidjson::Document* TempDoc = new rapidjson::Document;
			TempDoc->CopyFrom(v, Doc->GetAllocator());
			CJsonObject* Obj = new CJsonObject(TempDoc);
			Out.Add(Obj);
		}
		return TRUE;
	}

	void CJsonObject::SetIntField(const EString& FieldName, const INT32& In)
	{
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = In;
			return;
		}
		Doc->AddMember(StringRef(*FieldName), In, Doc->GetAllocator());
	}

	void CJsonObject::SetInt64Field(const EString& FieldName, const INT64& In)
	{
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = In;
			return;
		}
		
		Doc->AddMember(StringRef(*FieldName), In, Doc->GetAllocator());
	}

	void CJsonObject::SetUIntField(const EString& FieldName, const UINT32& In)
	{
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = In;
			return;
		}
		
		Doc->AddMember(StringRef(*FieldName), In, Doc->GetAllocator());
	}

	void CJsonObject::SetUInt64Field(const EString& FieldName, const UINT64& In)
	{
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = In;
			return;
		}
		
		Doc->AddMember(StringRef(*FieldName), In, Doc->GetAllocator());
	}

	void CJsonObject::SetFloatField(const EString& FieldName, const FLOAT& In)
	{
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = In;
			return;
		}
		
		Doc->AddMember(StringRef(*FieldName), In, Doc->GetAllocator());
	}

	void CJsonObject::SetDoubleField(const EString& FieldName, const DOUBLE& In)
	{
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = In;
			return;
		}
		
		Doc->AddMember(StringRef(*FieldName), In, Doc->GetAllocator());
	}

	void CJsonObject::SetStringField(const EString& FieldName, const EString& In)
	{
		rapidjson::Value Val(kStringType);
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = Val.SetString(*In, Doc->GetAllocator());
			return;
		}
		
		Val.SetString(*In, Doc->GetAllocator());
		Doc->AddMember(StringRef(*FieldName), Val, Doc->GetAllocator());
	}

	void CJsonObject::SetBoolField(const EString& FieldName, const BOOL8& In)
	{
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = In;
			return;
		}
		
		Doc->AddMember(StringRef(*FieldName), In, Doc->GetAllocator());
	}

	void CJsonObject::SetObjectField(const EString& FieldName, const CJsonObject* In)
	{
		
		rapidjson::Value Val(rapidjson::Type::kObjectType);
		for (Value::ConstMemberIterator it = In->GetDocument()->MemberBegin(); it != In->GetDocument()->MemberEnd(); ++it)
		{
			rapidjson::Value Temp;
			Temp.CopyFrom(it->value, Doc->GetAllocator());
			Val.AddMember(StringRef(it->name.GetString()), Temp, Doc->GetAllocator());
		}
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = Val;
			return;
		}
		Doc->AddMember(StringRef(*FieldName), Val, Doc->GetAllocator());
	}

	void CJsonObject::SetIntArrayField(const EString& FieldName, const TArray<INT32>& In)
	{
		
		rapidjson::Value Val(rapidjson::Type::kArrayType);
		for (auto& elem : In)
		{
			Value Temp(kNumberType);
			Temp = elem;
			Val.PushBack(Temp, Doc->GetAllocator());
		}
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = Val;
			return;
		}
		Doc->AddMember(StringRef(*FieldName), Val, Doc->GetAllocator());
	}

	void CJsonObject::SetInt64ArrayField(const EString& FieldName, const TArray<INT64>& In)
	{
		
		rapidjson::Value Val(rapidjson::Type::kArrayType);
		for (auto& elem : In)
		{
			Value Temp(kNumberType);
			Temp = elem;
			Val.PushBack(Temp, Doc->GetAllocator());
		}
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = Val;
			return;
		}
		Doc->AddMember(StringRef(*FieldName), Val, Doc->GetAllocator());
	}

	void CJsonObject::SetUIntArrayField(const EString& FieldName, const TArray<UINT32>& In)
	{
		
		rapidjson::Value Val(rapidjson::Type::kArrayType);
		for (auto& elem : In)
		{
			Value Temp(kNumberType);
			Temp = elem;
			Val.PushBack(Temp, Doc->GetAllocator());
		}
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = Val;
			return;
		}
		Doc->AddMember(StringRef(*FieldName), Val, Doc->GetAllocator());
	}

	void CJsonObject::SetUInt64ArrayField(const EString& FieldName, const TArray<UINT64>& In)
	{
		
		rapidjson::Value Val(rapidjson::Type::kArrayType);
		for (auto& elem : In)
		{
			Value Temp(kNumberType);
			Temp = elem;
			Val.PushBack(Temp, Doc->GetAllocator());
		}
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = Val;
			return;
		}
		Doc->AddMember(StringRef(*FieldName), Val, Doc->GetAllocator());
	}

	void CJsonObject::SetFloatArrayField(const EString& FieldName, const TArray<FLOAT>& In)
	{
		
		rapidjson::Value Val(rapidjson::Type::kArrayType);
		for (auto& elem : In)
		{
			Value Temp(kNumberType);
			Temp = elem;
			Val.PushBack(Temp, Doc->GetAllocator());
		}
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = Val;
			return;
		}
		Doc->AddMember(StringRef(*FieldName), Val, Doc->GetAllocator());
	}

	void CJsonObject::SetDoubleArrayField(const EString& FieldName, const TArray<DOUBLE>& In)
	{
		
		rapidjson::Value Val(rapidjson::Type::kArrayType);
		for (auto& elem : In)
		{
			Value Temp(kNumberType);
			Temp = elem;
			Val.PushBack(Temp, Doc->GetAllocator());
		}
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = Val;
			return;
		}
		Doc->AddMember(StringRef(*FieldName), Val, Doc->GetAllocator());
	}

	void CJsonObject::SetStringArrayField(const EString& FieldName, const TArray<EString>& In)
	{
		
		rapidjson::Value Val(rapidjson::Type::kArrayType);
		for (auto& elem : In)
		{
			Value Temp(kStringType);
			Temp.SetString(*elem, Doc->GetAllocator());
			Val.PushBack(Temp, Doc->GetAllocator());
		}
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = Val;
			return;
		}
		Doc->AddMember(StringRef(*FieldName), Val, Doc->GetAllocator());
	}

	void CJsonObject::SetBoolArrayField(const EString& FieldName, const TArray<BOOL8>& In)
	{
		
		rapidjson::Value Val(rapidjson::Type::kArrayType);
		for (const auto& elem : In)
		{
			Value Temp(elem);
			Val.PushBack(Temp, Doc->GetAllocator());
		}
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = Val;
			return;
		}
		Doc->AddMember(StringRef(*FieldName), Val, Doc->GetAllocator());
	}

	void CJsonObject::SetObjectArrayField(const EString& FieldName, const TArray<CJsonObject*>& In)
	{
		
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
			(*Doc)[*FieldName] = Val;
			return;
		}
		Doc->AddMember(StringRef(*FieldName), Val, Doc->GetAllocator());
	}

};