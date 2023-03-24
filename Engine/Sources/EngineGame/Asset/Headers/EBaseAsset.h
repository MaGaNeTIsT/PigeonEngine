#pragma once

#include "../../../EngineCore/Core/Headers/Main.h"
#include "../../../EngineCore/Base/Headers/ObjectBase.h"
#include "../../../EngineCore/Base/Headers/RTTI.h"

template<typename TCPUResourceType, typename TGPUResourceType>
class EBaseAsset : public EObjectBase
{
	EClass(EBaseAsset, EObjectBase);
public:
	EBaseAsset();
	virtual ~EBaseAsset();
};