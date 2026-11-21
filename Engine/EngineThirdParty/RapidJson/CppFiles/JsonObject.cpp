#include "../Headers/JsonObject.h"
#include "../../../ThirdParty/rapidjson/writer.h"
#include "../../../ThirdParty/rapidjson/stringbuffer.h"
#include <cstring>
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
		// The copy must own its storage. Using the source allocator leaves the copied
		// values tied to the source document; adding a member later reallocates them
		// through this document's still-empty allocator and dereferences a null chunk.
		Doc->CopyFrom(*Other.GetDocument(), Doc->GetAllocator());
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
		if (this != &Other)
		{
			rapidjson::Document* NewDoc = new rapidjson::Document();
			NewDoc->CopyFrom(*Other.GetDocument(), NewDoc->GetAllocator());
			delete Doc;
			Doc = NewDoc;
		}
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

	BOOL8 CJsonObject::IsValid()const
	{
		return (((Doc != nullptr) && (!Doc->HasParseError()) && (Doc->IsObject())) ? TRUE : FALSE);
	}

	EString CJsonObject::ToString()const
	{
		rapidjson::StringBuffer Buffer;
		rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);
		Doc->Accept(Writer);
		return EString(Buffer.GetString());
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
		// RapidJSON parses an ordinary JSON number as int/double; IsFloat() only matches a
		// value that was explicitly constructed with the float flag. Accept any number and
		// convert, otherwise authored values such as 0.8 silently fall back to defaults.
		if (itr == Doc->MemberEnd() || !itr->value.IsNumber())
			return FALSE;
		Out = static_cast<FLOAT>(itr->value.GetDouble());
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
		// Allocate the extracted object's storage from its own document so callers can
		// mutate it (add/remove fields) after the source document goes away.
		OutDoc->CopyFrom(itr->value, OutDoc->GetAllocator());
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
			// Each extracted element owns its own allocator so it can be edited safely.
			TempDoc->CopyFrom(v, TempDoc->GetAllocator());
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
		rapidjson::Value FieldNameValue(*FieldName, Doc->GetAllocator());
		Doc->AddMember(FieldNameValue, In, Doc->GetAllocator());
	}

	void CJsonObject::SetInt64Field(const EString& FieldName, const INT64& In)
	{
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = In;
			return;
		}

		rapidjson::Value FieldNameValue(*FieldName, Doc->GetAllocator());
		Doc->AddMember(FieldNameValue, In, Doc->GetAllocator());
	}

	void CJsonObject::SetUIntField(const EString& FieldName, const UINT32& In)
	{
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = In;
			return;
		}

		rapidjson::Value FieldNameValue(*FieldName, Doc->GetAllocator());
		Doc->AddMember(FieldNameValue, In, Doc->GetAllocator());
	}

	void CJsonObject::SetUInt64Field(const EString& FieldName, const UINT64& In)
	{
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = In;
			return;
		}

		rapidjson::Value FieldNameValue(*FieldName, Doc->GetAllocator());
		Doc->AddMember(FieldNameValue, In, Doc->GetAllocator());
	}

	void CJsonObject::SetFloatField(const EString& FieldName, const FLOAT& In)
	{
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = In;
			return;
		}

		rapidjson::Value FieldNameValue(*FieldName, Doc->GetAllocator());
		Doc->AddMember(FieldNameValue, In, Doc->GetAllocator());
	}

	void CJsonObject::SetDoubleField(const EString& FieldName, const DOUBLE& In)
	{
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = In;
			return;
		}

		rapidjson::Value FieldNameValue(*FieldName, Doc->GetAllocator());
		Doc->AddMember(FieldNameValue, In, Doc->GetAllocator());
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
		rapidjson::Value FieldNameValue(*FieldName, Doc->GetAllocator());
		Doc->AddMember(FieldNameValue, Val, Doc->GetAllocator());
	}

	void CJsonObject::SetBoolField(const EString& FieldName, const BOOL8& In)
	{
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = In;
			return;
		}

		rapidjson::Value FieldNameValue(*FieldName, Doc->GetAllocator());
		Doc->AddMember(FieldNameValue, In, Doc->GetAllocator());
	}

	void CJsonObject::SetObjectField(const EString& FieldName, const CJsonObject* In)
	{

		rapidjson::Value Val(rapidjson::Type::kObjectType);
		for (Value::ConstMemberIterator it = In->GetDocument()->MemberBegin(); it != In->GetDocument()->MemberEnd(); ++it)
		{
			rapidjson::Value Temp;
			Temp.CopyFrom(it->value, Doc->GetAllocator());
			rapidjson::Value MemberNameValue(it->name.GetString(), Doc->GetAllocator());
			Val.AddMember(MemberNameValue, Temp, Doc->GetAllocator());
		}
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = Val;
			return;
		}
		rapidjson::Value FieldNameValue(*FieldName, Doc->GetAllocator());
		Doc->AddMember(FieldNameValue, Val, Doc->GetAllocator());
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
		rapidjson::Value FieldNameValue(*FieldName, Doc->GetAllocator());
		Doc->AddMember(FieldNameValue, Val, Doc->GetAllocator());
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
		rapidjson::Value FieldNameValue(*FieldName, Doc->GetAllocator());
		Doc->AddMember(FieldNameValue, Val, Doc->GetAllocator());
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
		rapidjson::Value FieldNameValue(*FieldName, Doc->GetAllocator());
		Doc->AddMember(FieldNameValue, Val, Doc->GetAllocator());
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
		rapidjson::Value FieldNameValue(*FieldName, Doc->GetAllocator());
		Doc->AddMember(FieldNameValue, Val, Doc->GetAllocator());
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
		rapidjson::Value FieldNameValue(*FieldName, Doc->GetAllocator());
		Doc->AddMember(FieldNameValue, Val, Doc->GetAllocator());
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
		rapidjson::Value FieldNameValue(*FieldName, Doc->GetAllocator());
		Doc->AddMember(FieldNameValue, Val, Doc->GetAllocator());
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
		rapidjson::Value FieldNameValue(*FieldName, Doc->GetAllocator());
		Doc->AddMember(FieldNameValue, Val, Doc->GetAllocator());
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
		rapidjson::Value FieldNameValue(*FieldName, Doc->GetAllocator());
		Doc->AddMember(FieldNameValue, Val, Doc->GetAllocator());
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
				rapidjson::Value MemberNameValue(it->name.GetString(), Doc->GetAllocator());
				TempObj.AddMember(MemberNameValue, Temp, Doc->GetAllocator());
			}
			Val.PushBack(TempObj, Doc->GetAllocator());
		}
		if (HasField(FieldName))
		{
			(*Doc)[*FieldName] = Val;
			return;
		}
		rapidjson::Value FieldNameValue(*FieldName, Doc->GetAllocator());
		Doc->AddMember(FieldNameValue, Val, Doc->GetAllocator());
	}

	namespace
	{
		void SplitPath(const EString& InPath, TArray<EString>& OutSegments)
		{
			UINT32 Start = 0u;
			for (UINT32 Index = 0u; Index <= InPath.Length(); Index++)
			{
				if ((Index == InPath.Length()) || (InPath[Index] == '.'))
				{
					OutSegments.Add(InPath.Substring(Start, Index - Start));
					Start = Index + 1u;
				}
			}
		}

		BOOL8 JsonValueEquals(const Value& InLeft, const Value& InRight)
		{
			if (InLeft.GetType() != InRight.GetType())
			{
				return FALSE;
			}
			switch (InLeft.GetType())
			{
			case kNullType:
				return TRUE;
			case kFalseType:
			case kTrueType:
				return TRUE;
			case kObjectType:
			{
				if (InLeft.MemberCount() != InRight.MemberCount())
				{
					return FALSE;
				}
				for (Value::ConstMemberIterator It = InLeft.MemberBegin(); It != InLeft.MemberEnd(); ++It)
				{
					Value::ConstMemberIterator Other = InRight.FindMember(It->name.GetString());
					if ((Other == InRight.MemberEnd()) || (!JsonValueEquals(It->value, Other->value)))
					{
						return FALSE;
					}
				}
				return TRUE;
			}
			case kArrayType:
			{
				if (InLeft.Size() != InRight.Size())
				{
					return FALSE;
				}
				for (SizeType Index = 0u; Index < InLeft.Size(); Index++)
				{
					if (!JsonValueEquals(InLeft[Index], InRight[Index]))
					{
						return FALSE;
					}
				}
				return TRUE;
			}
			case kStringType:
				return ((InLeft.GetStringLength() == InRight.GetStringLength())
					&& (memcmp(InLeft.GetString(), InRight.GetString(), InLeft.GetStringLength()) == 0)) ? TRUE : FALSE;
			case kNumberType:
				return (InLeft.GetDouble() == InRight.GetDouble()) ? TRUE : FALSE;
			default:
				return FALSE;
			}
		}
	}

	rapidjson::Value* CJsonObject::FindValueByPath(const EString& InPath)
	{
		TArray<EString> Segments;
		SplitPath(InPath, Segments);
		rapidjson::Value* Current = Doc;
		for (UINT32 Index = 0u; Index < Segments.Num<UINT32>(); Index++)
		{
			if ((!Current) || (!Current->IsObject()))
			{
				return nullptr;
			}
			rapidjson::Value::MemberIterator It = Current->FindMember(*Segments[Index]);
			if (It == Current->MemberEnd())
			{
				return nullptr;
			}
			Current = &(It->value);
		}
		return Current;
	}

	const rapidjson::Value* CJsonObject::FindValueByPath(const EString& InPath) const
	{
		TArray<EString> Segments;
		SplitPath(InPath, Segments);
		const rapidjson::Value* Current = Doc;
		for (UINT32 Index = 0u; Index < Segments.Num<UINT32>(); Index++)
		{
			if ((!Current) || (!Current->IsObject()))
			{
				return nullptr;
			}
			rapidjson::Value::ConstMemberIterator It = Current->FindMember(*Segments[Index]);
			if (It == Current->MemberEnd())
			{
				return nullptr;
			}
			Current = &(It->value);
		}
		return Current;
	}

	rapidjson::Value* CJsonObject::EnsureObjectParent(const EString& InPath, EString& OutLeaf)
	{
		TArray<EString> Segments;
		SplitPath(InPath, Segments);
		if (Segments.Num<UINT32>() == 0u)
		{
			return nullptr;
		}
		rapidjson::Value* Current = Doc;
		for (UINT32 Index = 0u; (Index + 1u) < Segments.Num<UINT32>(); Index++)
		{
			if (!Current->IsObject())
			{
				Current->SetObject();
			}
			rapidjson::Value::MemberIterator It = Current->FindMember(*Segments[Index]);
			if (It == Current->MemberEnd())
			{
				rapidjson::Value NewObject(rapidjson::kObjectType);
				rapidjson::Value Name(*Segments[Index], Doc->GetAllocator());
				Current->AddMember(Name, NewObject, Doc->GetAllocator());
				It = Current->FindMember(*Segments[Index]);
			}
			if (!It->value.IsObject())
			{
				It->value.SetObject();
			}
			Current = &(It->value);
		}
		OutLeaf = Segments[Segments.Num<UINT32>() - 1u];
		return Current;
	}

	void CJsonObject::SetValueByPath(const EString& InPath, rapidjson::Value& InValue)
	{
		EString Leaf;
		rapidjson::Value* Parent = EnsureObjectParent(InPath, Leaf);
		if (!Parent)
		{
			return;
		}
		rapidjson::Value::MemberIterator It = Parent->FindMember(*Leaf);
		if (It != Parent->MemberEnd())
		{
			It->value.CopyFrom(InValue, Doc->GetAllocator());
			return;
		}
		rapidjson::Value Name(*Leaf, Doc->GetAllocator());
		Parent->AddMember(Name, InValue, Doc->GetAllocator());
	}

	BOOL8 CJsonObject::GetBoolFieldByPath(const EString& InPath, BOOL8& Out)
	{
		rapidjson::Value* Value = FindValueByPath(InPath);
		if ((!Value) || (!Value->IsBool()))
		{
			return FALSE;
		}
		Out = Value->GetBool() ? TRUE : FALSE;
		return TRUE;
	}

	BOOL8 CJsonObject::GetIntFieldByPath(const EString& InPath, INT32& Out)
	{
		rapidjson::Value* Value = FindValueByPath(InPath);
		if ((!Value) || (!Value->IsInt()))
		{
			return FALSE;
		}
		Out = Value->GetInt();
		return TRUE;
	}

	BOOL8 CJsonObject::GetFloatFieldByPath(const EString& InPath, FLOAT& Out)
	{
		rapidjson::Value* Value = FindValueByPath(InPath);
		if ((!Value) || (!Value->IsNumber()))
		{
			return FALSE;
		}
		Out = static_cast<FLOAT>(Value->GetDouble());
		return TRUE;
	}

	BOOL8 CJsonObject::GetStringFieldByPath(const EString& InPath, EString& Out)
	{
		rapidjson::Value* Value = FindValueByPath(InPath);
		if ((!Value) || (!Value->IsString()))
		{
			return FALSE;
		}
		Out = EString(Value->GetString());
		return TRUE;
	}

	void CJsonObject::SetBoolFieldByPath(const EString& InPath, const BOOL8& In)
	{
		rapidjson::Value Value(In != 0);
		SetValueByPath(InPath, Value);
	}

	void CJsonObject::SetIntFieldByPath(const EString& InPath, const INT32& In)
	{
		rapidjson::Value Value(In);
		SetValueByPath(InPath, Value);
	}

	void CJsonObject::SetFloatFieldByPath(const EString& InPath, const FLOAT& In)
	{
		rapidjson::Value Value(static_cast<DOUBLE>(In));
		SetValueByPath(InPath, Value);
	}

	void CJsonObject::SetStringFieldByPath(const EString& InPath, const EString& In)
	{
		rapidjson::Value Value;
		Value.SetString(*In, Doc->GetAllocator());
		SetValueByPath(InPath, Value);
	}

	BOOL8 CJsonObject::CopyFieldByPath(const EString& InPath, const CJsonObject& InSource)
	{
		const rapidjson::Value* Source = InSource.FindValueByPath(InPath);
		if (!Source)
		{
			return FALSE;
		}
		EString Leaf;
		rapidjson::Value* Parent = EnsureObjectParent(InPath, Leaf);
		if (!Parent)
		{
			return FALSE;
		}
		rapidjson::Value::MemberIterator It = Parent->FindMember(*Leaf);
		if (It != Parent->MemberEnd())
		{
			It->value.CopyFrom(*Source, Doc->GetAllocator());
			return TRUE;
		}
		rapidjson::Value Copy;
		Copy.CopyFrom(*Source, Doc->GetAllocator());
		rapidjson::Value Name(*Leaf, Doc->GetAllocator());
		Parent->AddMember(Name, Copy, Doc->GetAllocator());
		return TRUE;
	}

	BOOL8 CJsonObject::IsFieldEqualByPath(const EString& InPath, const CJsonObject& InOther) const
	{
		const rapidjson::Value* Left = FindValueByPath(InPath);
		const rapidjson::Value* Right = InOther.FindValueByPath(InPath);
		if ((!Left) && (!Right))
		{
			return TRUE;
		}
		if ((!Left) || (!Right))
		{
			return FALSE;
		}
		return JsonValueEquals(*Left, *Right);
	}

	BOOL8 CJsonObject::IsEmpty() const
	{
		return (Doc->MemberCount() == 0u) ? TRUE : FALSE;
	}

};
