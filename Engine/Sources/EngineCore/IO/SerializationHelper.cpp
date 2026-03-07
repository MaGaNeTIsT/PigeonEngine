#include "SerializationHelper.h"

#include "../../../EngineThirdParty/RapidJson/Headers/JsonManager.h"

namespace PigeonEngine
{
    TSharedPtr<CJsonObject> ESerializationHelper::SerializeTransform(const ETransform& Trans)
    {
        TSharedPtr<CJsonObject> TransJsonObj = EMemory::MakeShared<CJsonObject>();
        
        TSharedPtr<CJsonObject> LocationJsonObj = SerializeVector3   (Trans.GetLocation_Local());
        TSharedPtr<CJsonObject> RotationJsonObj = SerializeQuaternion(Trans.GetRotation_Local());
        TSharedPtr<CJsonObject> ScaleJsonObj    = SerializeVector3   (Trans.GetScaling_Local());
        
        TransJsonObj->SetObjectField("Location",   EMemory::GetPtr(TransJsonObj));
        TransJsonObj->SetObjectField("Quaternion", EMemory::GetPtr(RotationJsonObj));
        TransJsonObj->SetObjectField("Scale",      EMemory::GetPtr(ScaleJsonObj));

        return TransJsonObj;
    }

    TSharedPtr<CJsonObject> ESerializationHelper::SerializeVector3(const Vector3& InVector3)
    {
        TSharedPtr<CJsonObject> Vec3JsonObj = EMemory::MakeShared<CJsonObject>();
        Vec3JsonObj->SetDoubleField("X", InVector3.x);
        Vec3JsonObj->SetDoubleField("Y", InVector3.y);
        Vec3JsonObj->SetDoubleField("Z", InVector3.z);
        return Vec3JsonObj;
    }

    TSharedPtr<CJsonObject> ESerializationHelper::SerializeQuaternion(const Quaternion& InQuat)
    {
        TSharedPtr<CJsonObject> QuatJsonObj = EMemory::MakeShared<CJsonObject>();
        QuatJsonObj->SetDoubleField("X", InQuat.x);
        QuatJsonObj->SetDoubleField("Y", InQuat.y);
        QuatJsonObj->SetDoubleField("Z", InQuat.z);
        QuatJsonObj->SetDoubleField("W", InQuat.w);
        return QuatJsonObj;
    }
}
