#pragma once
#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Array.h>
#include "PhysicsUtility.h"
#include "JoltIncludes.h"

PIGEONENGINE_NAMESPACE_BEGIN
// All Jolt symbols are in the JPH namespace
using JPH::BroadPhaseLayer;
using JPH::BroadPhaseLayerFilter;
using JPH::BroadPhaseLayerInterface;
using JPH::ObjectLayer;
using JPH::ObjectLayerFilter;
using JPH::ObjectLayerPairFilter;
using JPH::ObjectVsBroadPhaseLayerFilter;
using JPH::uint;

class FPhysicsObjectLayer
{
public:
	using Type = UINT16;

	constexpr FPhysicsObjectLayer() : Value(0u) {}
	explicit constexpr FPhysicsObjectLayer(Type InValue) : Value(InValue) {}

	constexpr operator Type() const
	{
		return Value;
	}

	constexpr Type GetValue() const
	{
		return Value;
	}

	constexpr BOOL32 operator == (const FPhysicsObjectLayer& InRHS) const
	{
		return Value == InRHS.Value;
	}

	constexpr BOOL32 operator != (const FPhysicsObjectLayer& InRHS) const
	{
		return Value != InRHS.Value;
	}

	inline ObjectLayer ToJolt() const
	{
		return static_cast<ObjectLayer>(Value);
	}

	static inline FPhysicsObjectLayer FromJolt(ObjectLayer InLayer)
	{
		return FPhysicsObjectLayer(static_cast<Type>(InLayer));
	}

private:
	Type Value;
};

class FPhysicsBroadPhaseLayer
{
public:
	using Type = UINT8;

	constexpr FPhysicsBroadPhaseLayer() : Value(0u) {}
	explicit constexpr FPhysicsBroadPhaseLayer(Type InValue) : Value(InValue) {}

	constexpr operator Type() const
	{
		return Value;
	}

	constexpr Type GetValue() const
	{
		return Value;
	}

	constexpr BOOL32 operator == (const FPhysicsBroadPhaseLayer& InRHS) const
	{
		return Value == InRHS.Value;
	}

	constexpr BOOL32 operator != (const FPhysicsBroadPhaseLayer& InRHS) const
	{
		return Value != InRHS.Value;
	}

	inline BroadPhaseLayer ToJolt() const
	{
		return BroadPhaseLayer(static_cast<JPH::BroadPhaseLayer::Type>(Value));
	}

	static inline FPhysicsBroadPhaseLayer FromJolt(BroadPhaseLayer InLayer)
	{
		return FPhysicsBroadPhaseLayer(static_cast<Type>(InLayer.GetValue()));
	}

private:
	Type Value;
};

namespace Layers
{
	static constexpr FPhysicsObjectLayer NON_MOVING{ 0 };
	static constexpr FPhysicsObjectLayer MOVING{ 1 };
	static constexpr FPhysicsObjectLayer SENSOR{ 2 };
	static constexpr FPhysicsObjectLayer QUERY_ONLY{ 3 };
	static constexpr UINT8 NUM_LAYERS = 4;
}

namespace BroadPhaseLayers
{
	static constexpr FPhysicsBroadPhaseLayer NON_MOVING{ 0 };
	static constexpr FPhysicsBroadPhaseLayer MOVING{ 1 };
	static constexpr FPhysicsBroadPhaseLayer SENSOR{ 2 };
	static constexpr FPhysicsBroadPhaseLayer QUERY_ONLY{ 3 };
	static constexpr UINT8 NUM_LAYERS(4);
}

class FPhysicsLayerConfig
{
public:
	virtual ~FPhysicsLayerConfig() = default;

	virtual uint GetNumObjectLayers() const
	{
		return Layers::NUM_LAYERS;
	}

	virtual uint GetNumBroadPhaseLayers() const
	{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	virtual FPhysicsBroadPhaseLayer GetBroadPhaseLayer(FPhysicsObjectLayer inLayer) const
	{
		switch (inLayer)
		{
		case Layers::NON_MOVING:
			return BroadPhaseLayers::NON_MOVING;
		case Layers::MOVING:
			return BroadPhaseLayers::MOVING;
		case Layers::SENSOR:
			return BroadPhaseLayers::SENSOR;
		case Layers::QUERY_ONLY:
			return BroadPhaseLayers::QUERY_ONLY;
		default:
			JPH_ASSERT(FALSE);
			return BroadPhaseLayers::NON_MOVING;
		}
	}

	virtual bool ShouldObjectCollideWithBroadPhase(FPhysicsObjectLayer inLayer1, FPhysicsBroadPhaseLayer inLayer2) const
	{
		switch (inLayer1)
		{
		case Layers::NON_MOVING:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::MOVING:
			return inLayer2 != BroadPhaseLayers::QUERY_ONLY;
		case Layers::SENSOR:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::QUERY_ONLY:
			return FALSE;
		default:
			JPH_ASSERT(FALSE);
			return FALSE;
		}
	}

	virtual bool ShouldObjectCollide(FPhysicsObjectLayer inObject1, FPhysicsObjectLayer inObject2) const
	{
		switch (inObject1)
		{
		case Layers::NON_MOVING:
			return inObject2 == Layers::MOVING;
		case Layers::MOVING:
			return inObject2 != Layers::QUERY_ONLY;
		case Layers::SENSOR:
			return inObject2 == Layers::MOVING;
		case Layers::QUERY_ONLY:
			return FALSE;
		default:
			JPH_ASSERT(FALSE);
			return FALSE;
		}
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char* GetBroadPhaseLayerName(FPhysicsBroadPhaseLayer inLayer) const
	{
		switch (inLayer.GetValue())
		{
		case BroadPhaseLayers::NON_MOVING.GetValue():	return "NON_MOVING";
		case BroadPhaseLayers::MOVING.GetValue():		return "MOVING";
		case BroadPhaseLayers::SENSOR.GetValue():		return "SENSOR";
		case BroadPhaseLayers::QUERY_ONLY.GetValue():	return "QUERY_ONLY";
		default:											JPH_ASSERT(FALSE); return "INVALID";
		}
	}
#endif
};

class FPhysicsBroadPhaseLayerFilterBase : public BroadPhaseLayerFilter
{
public:
	virtual ~FPhysicsBroadPhaseLayerFilterBase() = default;

	virtual bool ShouldCollide(FPhysicsBroadPhaseLayer inLayer) const
	{
		return TRUE;
	}

	virtual bool ShouldCollide(BroadPhaseLayer inLayer) const override final
	{
		return ShouldCollide(FPhysicsBroadPhaseLayer::FromJolt(inLayer));
	}
};

class FPhysicsBroadPhaseLayerFilter : public FPhysicsBroadPhaseLayerFilterBase
{
public:
	FPhysicsBroadPhaseLayerFilter() = default;

	explicit FPhysicsBroadPhaseLayerFilter(FPhysicsBroadPhaseLayer InLayer)
	{
		AllowedLayers.Add(InLayer);
	}

	explicit FPhysicsBroadPhaseLayerFilter(TInitializerList<FPhysicsBroadPhaseLayer> InLayers)
		: AllowedLayers(InLayers)
	{
	}

	virtual bool ShouldCollide(FPhysicsBroadPhaseLayer inLayer) const override
	{
		if (AllowedLayers.IsEmpty())
			return TRUE;

		for (const FPhysicsBroadPhaseLayer Layer : AllowedLayers)
		{
			if (Layer == inLayer)
				return TRUE;
		}

		return FALSE;
	}

	void AddLayer(FPhysicsBroadPhaseLayer InLayer)
	{
		for (const FPhysicsBroadPhaseLayer Layer : AllowedLayers)
		{
			if (Layer == InLayer)
				return;
		}

		AllowedLayers.Add(InLayer);
	}

	void Reset()
	{
		AllowedLayers.Empty();
	}

private:
	TArray<FPhysicsBroadPhaseLayer> AllowedLayers;
};

class FPhysicsDefaultBroadPhaseLayerFilter final : public FPhysicsBroadPhaseLayerFilter
{
public:
    static const FPhysicsDefaultBroadPhaseLayerFilter& Get()
	{
        static FPhysicsDefaultBroadPhaseLayerFilter Instance;
		return Instance;
	}
};

class FPhysicsDefaultLayerConfig final : public FPhysicsLayerConfig
{
public:
    static const FPhysicsDefaultLayerConfig& Get()
	{
        static FPhysicsDefaultLayerConfig Instance;
		return Instance;
	}
};

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class FPhysicsBroadPhaseLayerMapper final : public BroadPhaseLayerInterface
{
public:
    explicit FPhysicsBroadPhaseLayerMapper(const FPhysicsLayerConfig& InLayerConfig)
		: mLayerConfig(InLayerConfig)
	{
		mObjectToBroadPhase.SetNum(mLayerConfig.GetNumObjectLayers());
		for (UINT32 LayerIndex = 0u; LayerIndex < mLayerConfig.GetNumObjectLayers(); ++LayerIndex)
		{
			mObjectToBroadPhase[LayerIndex] = mLayerConfig.GetBroadPhaseLayer(FPhysicsObjectLayer(static_cast<FPhysicsObjectLayer::Type>(LayerIndex))).ToJolt();
		}
	}

	virtual uint					GetNumBroadPhaseLayers() const override
	{
		return mLayerConfig.GetNumBroadPhaseLayers();
	}

	virtual BroadPhaseLayer			GetBroadPhaseLayer(ObjectLayer inLayer) const override
	{
		JPH_ASSERT(inLayer < mObjectToBroadPhase.Num<ObjectLayer>());
		return mObjectToBroadPhase[(size_t)inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const
	{
		return mLayerConfig.GetBroadPhaseLayerName(FPhysicsBroadPhaseLayer::FromJolt(inLayer));
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
	const FPhysicsLayerConfig& mLayerConfig;
	TArray<BroadPhaseLayer> mObjectToBroadPhase;
};

/// Class that determines if an object layer can collide with a broadphase layer
class FPhysicsObjectVsBroadPhaseLayerCollisionFilter final : public ObjectVsBroadPhaseLayerFilter
{
public:
    explicit FPhysicsObjectVsBroadPhaseLayerCollisionFilter(const FPhysicsLayerConfig& InLayerConfig)
		: mLayerConfig(InLayerConfig)
	{
	}

	virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
	{
		return mLayerConfig.ShouldObjectCollideWithBroadPhase(FPhysicsObjectLayer::FromJolt(inLayer1), FPhysicsBroadPhaseLayer::FromJolt(inLayer2));
	}

private:
	const FPhysicsLayerConfig& mLayerConfig;
};

/// Class that determines if two object layers can collide
class FPhysicsObjectLayerPairCollisionFilter final : public ObjectLayerPairFilter
{
public:
    explicit FPhysicsObjectLayerPairCollisionFilter(const FPhysicsLayerConfig& InLayerConfig)
		: mLayerConfig(InLayerConfig)
	{
	}

	virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
	{
		return mLayerConfig.ShouldObjectCollide(FPhysicsObjectLayer::FromJolt(inObject1), FPhysicsObjectLayer::FromJolt(inObject2));
	}

private:
	const FPhysicsLayerConfig& mLayerConfig;
};

class FPhysicsQueryLayerFilterBase : public ObjectLayerFilter
{
public:
	virtual ~FPhysicsQueryLayerFilterBase() = default;

	virtual bool ShouldCollide(FPhysicsObjectLayer inLayer) const
	{
		return TRUE;
	}

	virtual bool ShouldCollide(ObjectLayer inLayer) const override final
	{
		return ShouldCollide(FPhysicsObjectLayer::FromJolt(inLayer));
	}
};

class FPhysicsQueryLayerFilter : public FPhysicsQueryLayerFilterBase
{
public:
	FPhysicsQueryLayerFilter() = default;

	explicit FPhysicsQueryLayerFilter(FPhysicsObjectLayer InLayer)
	{
		AllowedLayers.Add(InLayer);
	}

	explicit FPhysicsQueryLayerFilter(TInitializerList<FPhysicsObjectLayer> InLayers)
		: AllowedLayers(InLayers)
	{
	}

	virtual bool ShouldCollide(FPhysicsObjectLayer inLayer) const override
	{
		if (AllowedLayers.IsEmpty())
			return TRUE;

		for (const FPhysicsObjectLayer Layer : AllowedLayers)
		{
			if (Layer == inLayer)
				return TRUE;
		}

		return FALSE;
	}

	void AddLayer(FPhysicsObjectLayer InLayer)
	{
		for (const FPhysicsObjectLayer Layer : AllowedLayers)
		{
			if (Layer == InLayer)
				return;
		}

		AllowedLayers.Add(InLayer);
	}

	void Reset()
	{
		AllowedLayers.Empty();
	}

private:
	TArray<FPhysicsObjectLayer> AllowedLayers;
};

class FPhysicsDefaultQueryLayerFilter final : public FPhysicsQueryLayerFilter
{
public:
	static const FPhysicsDefaultQueryLayerFilter& Get()
	{
        static FPhysicsDefaultQueryLayerFilter Instance;
		return Instance;
	}
};
PIGEONENGINE_NAMESPACE_END