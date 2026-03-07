#pragma once
#include "../../../EngineThirdParty/RapidJson/Headers/JsonObject.h"
#include "Base/DataStructure/Pointer/SharedPtr.h"

namespace PigeonEngine
{
    class ESerializationHelper
    {
    public:
        static TSharedPtr<CJsonObject> SerializeTransform(const ETransform& Trans);
        static TSharedPtr<CJsonObject> SerializeVector3(const Vector3& InVector3);
        static TSharedPtr<CJsonObject> SerializeQuaternion(const Quaternion& InQuat);

    };

}
