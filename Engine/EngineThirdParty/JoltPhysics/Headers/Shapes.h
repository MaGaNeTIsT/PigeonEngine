#pragma once

#include <CoreMinimal.h>
#include "PhysicsUtility.h"
#include "JoltIncludes.h"

namespace PigeonEngine
{
	using namespace JPH;
	struct AABBBox
	{
		Vector3 Min;
		Vector3 Max;
	};

	class FShape : public ERTTI
	{
		EClass(FShape, ERTTI)
		FShape();
		virtual ~FShape();
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
		Shape* CreateShape(BOOL bCreateNew = false);
		/// <summary>
		/// Create a physics shape setting,Use to Jolt Physics System
		/// </summary>
		/// <param name="bCreateNew">true to create a new shape setting</param>
		/// <returns>if has created,return the old shape setting.</returns>
		ShapeSettings* CreateShapeSettings(BOOL bCreateNew = false);
	};

	class FCastRay : public ERTTI
	{
		EClass(FCastRay, ERTTI)
		FCastRay();
		virtual ~FCastRay() = default;
	};


	class FBoxShape : public FShape
	{
		EClass(FBoxShape, FShape)
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
	protected:
		virtual Shape* CreateShapeInternal() override
		{
			return new BoxShape(PhysicsUtility::Convert2Meter(HalfExtent), ConvexRadius, Material);
		}

		virtual ShapeSettings* CreateShapeSettingsInternal() override
		{
			return new BoxShapeSettings(PhysicsUtility::Convert2Meter(HalfExtent), ConvexRadius, Material);
		}
	};

	class FSphereShape : public FShape
	{
		EClass(FSphereShape, FShape)
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

	protected:
		virtual Shape* CreateShapeInternal() override
		{
			return new SphereShape(Raidus, Material);
		}

		virtual ShapeSettings* CreateShapeSettingsInternal() override
		{
			return new SphereShapeSettings(Raidus, Material);
		}
	};

	class FCapsuleShape : public FShape
	{
		EClass(FCapsuleShape, FShape)
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
	protected:
		virtual Shape* CreateShapeInternal() override
		{
			return new CapsuleShape(HalfHeightOfCylinder, Raidus, Material);
		}

		virtual ShapeSettings* CreateShapeSettingsInternal() override
		{
			return new CapsuleShapeSettings(HalfHeightOfCylinder, Raidus, Material);
		}
	};

	class FCylinerShape : public FShape
	{
		EClass(FCylinerShape, FShape)
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
	protected:
		virtual Shape* CreateShapeInternal() override
		{
			return new CylinderShape(HalfHeight, Raidus, ConvexRaidus, Material);
		}

		virtual ShapeSettings* CreateShapeSettingsInternal() override
		{
			return new CylinderShapeSettings(HalfHeight, Raidus, ConvexRaidus, Material);
		}
	};


}