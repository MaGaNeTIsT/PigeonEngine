#pragma once

#include <CoreMinimal.h>
#include "PhysicsUtility.h"
#include "JoltIncludes.h"

#if _EDITOR_ONLY
#include <RenderProxy/RenderSingletonObject.h>
#endif

PIGEONENGINE_NAMESPACE_BEGIN
using namespace JPH;
struct AABBBox
{
	Vector3 Min;
	Vector3 Max;
};

class FShape : public ERTTIObject
{
public:
	FShape();
	virtual ~FShape();
#if _EDITOR_ONLY
	virtual void DrawPrimitive(RDebugWireframePrimitiveManager* Manager,const Vector3& inPosition, const Quaternion& inRotation) const = 0;
#endif
	virtual AABBBox GetWorldSpaceBounds(Quaternion Rotation, Vector3 Scale) { return AABBBox(); }
protected:
	Shape* m_Shape;
	ShapeSettings* m_ShapeSettings;
protected:
	virtual Shape* CreateShapeInternal() = 0;
	virtual ShapeSettings* CreateShapeSettingsInternal() = 0;

public:
	/// <summary>
	/// Create a physics shape,Use to Jolt Physics System
	/// </summary>
	/// <param name="bCreateNew">true to create a new shape</param>
	/// <returns>if has created,return the old shape.</returns>
	Shape* CreateShape(BOOL32 bCreateNew = FALSE);
	/// <summary>
	/// Create a physics shape setting,Use to Jolt Physics System
	/// </summary>
	/// <param name="bCreateNew">true to create a new shape setting</param>
	/// <returns>if has created,return the old shape setting.</returns>
	ShapeSettings* CreateShapeSettings(BOOL32 bCreateNew = FALSE);
};

class FCastRay : public ERTTIObject
{
public:
	FCastRay();
	virtual ~FCastRay() = default;
};


class FBoxShape : public FShape
{
public:
	Vector3 HalfExtent;
	FLOAT ConvexRadius;
	PhysicsMaterial* Material = nullptr;
public:
	FBoxShape(Vector3 inHalfExtent, FLOAT inConvexRadius, PhysicsMaterial* inMaterial = nullptr) :
		HalfExtent(inHalfExtent),
		ConvexRadius(inConvexRadius),
		Material(inMaterial)
	{
	}

	~FBoxShape()
	{
		//TODO: Did this necessary?
		//if (Material)
		//{
		//	delete Material;
		//	Material = nullptr;
		//}
	}
#if _EDITOR_ONLY
	virtual void DrawPrimitive(RDebugWireframePrimitiveManager* Manager, const Vector3& inPosition, const Quaternion& inRotation) const override;
#endif
protected:
	virtual Shape* CreateShapeInternal() override
	{
		return new BoxShape(PhysicsUtility::Convert2Meter(HalfExtent), PhysicsUtility::Convert2Meter(ConvexRadius), Material);
	}

	virtual ShapeSettings* CreateShapeSettingsInternal() override
	{
		return new BoxShapeSettings(PhysicsUtility::Convert2Meter(HalfExtent), PhysicsUtility::Convert2Meter(ConvexRadius), Material);
	}
};

class FSphereShape : public FShape
{
public:
	FLOAT Raidus;
	PhysicsMaterial* Material = nullptr;
public:
	FSphereShape(FLOAT inRaidus, PhysicsMaterial* inMaterial = nullptr) :
		Raidus(inRaidus),
		Material(inMaterial)
	{
	}

	~FSphereShape()
	{
	}
#if _EDITOR_ONLY
	virtual void DrawPrimitive(RDebugWireframePrimitiveManager* Manager, const Vector3& inPosition, const Quaternion& inRotation) const override;
#endif
protected:
	virtual Shape* CreateShapeInternal() override
	{
		return new SphereShape(PhysicsUtility::Convert2Meter(Raidus), Material);
	}

	virtual ShapeSettings* CreateShapeSettingsInternal() override
	{
		return new SphereShapeSettings(PhysicsUtility::Convert2Meter(Raidus), Material);
	}
};

class FCapsuleShape : public FShape
{
public:
	FLOAT HalfHeightOfCylinder;
	FLOAT Raidus;
	PhysicsMaterial* Material = nullptr;
public:
	FCapsuleShape(FLOAT inHalfHeightOfCylinder, FLOAT inRaidus, PhysicsMaterial* inMaterial = nullptr) :
		HalfHeightOfCylinder(inHalfHeightOfCylinder),
		Raidus(inRaidus),
		Material(inMaterial)
	{
	}

	~FCapsuleShape()
	{
	}
#if _EDITOR_ONLY
	virtual void DrawPrimitive(RDebugWireframePrimitiveManager* Manager, const Vector3& inPosition, const Quaternion& inRotation) const override;
#endif
protected:
	virtual Shape* CreateShapeInternal() override
	{
		return new CapsuleShape(PhysicsUtility::Convert2Meter(HalfHeightOfCylinder), PhysicsUtility::Convert2Meter(Raidus), Material);
	}

	virtual ShapeSettings* CreateShapeSettingsInternal() override
	{
		return new CapsuleShapeSettings(PhysicsUtility::Convert2Meter(HalfHeightOfCylinder), PhysicsUtility::Convert2Meter(Raidus), Material);
	}
};

class FCylinerShape : public FShape
{
public:
	FLOAT HalfHeight;
	FLOAT Raidus;
	FLOAT ConvexRaidus;
	PhysicsMaterial* Material = nullptr;
public:
	FCylinerShape(FLOAT inHalfHeight, FLOAT inRaidus, FLOAT inConvexRaidus = 0.05f, PhysicsMaterial* inMaterial = nullptr) :
		HalfHeight(inHalfHeight),
		Raidus(inRaidus),
		ConvexRaidus(inConvexRaidus),
		Material(inMaterial)
	{
	}

	~FCylinerShape()
	{
	}
#if _EDITOR_ONLY
	virtual void DrawPrimitive(RDebugWireframePrimitiveManager* Manager, const Vector3& inPosition, const Quaternion& inRotation) const override;
#endif
protected:
	virtual Shape* CreateShapeInternal() override
	{
		return new CylinderShape(PhysicsUtility::Convert2Meter(HalfHeight), PhysicsUtility::Convert2Meter(Raidus), ConvexRaidus, Material);
	}

	virtual ShapeSettings* CreateShapeSettingsInternal() override
	{
		return new CylinderShapeSettings(PhysicsUtility::Convert2Meter(HalfHeight), PhysicsUtility::Convert2Meter(Raidus), ConvexRaidus, Material);
	}
};

class FRotatedTranslatedShape : public FShape
{

public:
	Vector3 Position;
	Quaternion Rotation;
	FShape* HostedShape;
public:
	FRotatedTranslatedShape(Vector3 InPosition, Quaternion InRotation, FShape* InShape) :
		Position(InPosition),
		Rotation(InRotation),
		HostedShape(InShape)
	{
	}

	~FRotatedTranslatedShape()
	{
		delete HostedShape;
	}
#if _EDITOR_ONLY
	virtual void DrawPrimitive(RDebugWireframePrimitiveManager* Manager, const Vector3& inPosition, const Quaternion& inRotation) const override;
#endif
protected:
	virtual Shape* CreateShapeInternal() override
	{
		return new RotatedTranslatedShape(PhysicsUtility::Convert2Meter(Position), PhysicsUtility::Convert(Rotation), HostedShape->CreateShape());
	}

	virtual ShapeSettings* CreateShapeSettingsInternal() override
	{
		return new RotatedTranslatedShapeSettings(PhysicsUtility::Convert2Meter(Position), PhysicsUtility::Convert(Rotation), HostedShape->CreateShape());
	}
};
PIGEONENGINE_NAMESPACE_END