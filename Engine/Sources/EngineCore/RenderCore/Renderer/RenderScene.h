#pragma once

#include <CoreMinimal.h>
#include <RenderOctree.h>

namespace PigeonEngine
{

	class RPrimitiveProxy;

	class RScene : public EObjectBase
	{
	public:
		BOOL32	AddStaticPrimitive(RPrimitiveProxy* InPrimitiveProxy);
		BOOL32	RemoveStaticPrimitive(RPrimitiveProxy* InPrimitiveProxy);
		BOOL32	AddDynamicPrimitive(RPrimitiveProxy* InPrimitiveProxy);
		BOOL32	RemoveDynamicPrimitive(RPrimitiveProxy* InPrimitiveProxy);
	protected:
		ROctree		ScenePrimitiveOctree;
	protected:
		TMap<ObjectIdentityType, UINT32>	StaticPrimitiveMapping;
		TArray<RPrimitiveProxy*>			StaticPrimitives;
		TMap<ObjectIdentityType, UINT32>	DynamicPrimitiveMapping;
		TArray<RPrimitiveProxy*>			DynamicPrimitives;
	public:
		RScene();
		virtual ~RScene();
		RScene(const RScene&) = delete;
		RScene& operator=(const RScene&) = delete;
	};

};