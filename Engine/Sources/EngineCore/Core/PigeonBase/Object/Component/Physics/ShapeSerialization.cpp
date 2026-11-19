#include "ShapeSerialization.h"
#include "../../../../../../../EngineThirdParty/JoltPhysics/Headers/Shapes.h"
#include <Reflection/Property.h>

namespace PigeonEngine
{
	namespace
	{
		// On-disk shape tags. Never renumber them.
		enum class ESerializedShapeType : UINT32
		{
			NONE = 0u,
			BOX = 1u,
			SPHERE = 2u,
			CAPSULE = 3u,
			CYLINDER = 4u,
			ROTATED_TRANSLATED = 5u,
		};

		void WriteShapeBody(EArchive& InArchive, FShape* InShape)
		{
			UINT32 ShapeType = static_cast<UINT32>(ESerializedShapeType::NONE);
			if (dynamic_cast<FBoxShape*>(InShape))
			{
				ShapeType = static_cast<UINT32>(ESerializedShapeType::BOX);
			}
			else if (dynamic_cast<FSphereShape*>(InShape))
			{
				ShapeType = static_cast<UINT32>(ESerializedShapeType::SPHERE);
			}
			else if (dynamic_cast<FCapsuleShape*>(InShape))
			{
				ShapeType = static_cast<UINT32>(ESerializedShapeType::CAPSULE);
			}
			else if (dynamic_cast<FCylinerShape*>(InShape))
			{
				ShapeType = static_cast<UINT32>(ESerializedShapeType::CYLINDER);
			}
			else if (dynamic_cast<FRotatedTranslatedShape*>(InShape))
			{
				ShapeType = static_cast<UINT32>(ESerializedShapeType::ROTATED_TRANSLATED);
			}
			InArchive.SerializeUInt32(PE_TEXT("ShapeType"), ShapeType);

			if (FBoxShape* Box = dynamic_cast<FBoxShape*>(InShape))
			{
				SerializeVector3(InArchive, PE_TEXT("HalfExtent"), Box->HalfExtent);
				InArchive.SerializeFloat(PE_TEXT("ConvexRadius"), Box->ConvexRadius);
			}
			else if (FSphereShape* Sphere = dynamic_cast<FSphereShape*>(InShape))
			{
				InArchive.SerializeFloat(PE_TEXT("Radius"), Sphere->Raidus);
			}
			else if (FCapsuleShape* Capsule = dynamic_cast<FCapsuleShape*>(InShape))
			{
				InArchive.SerializeFloat(PE_TEXT("HalfHeight"), Capsule->HalfHeightOfCylinder);
				InArchive.SerializeFloat(PE_TEXT("Radius"), Capsule->Raidus);
			}
			else if (FCylinerShape* Cylinder = dynamic_cast<FCylinerShape*>(InShape))
			{
				InArchive.SerializeFloat(PE_TEXT("HalfHeight"), Cylinder->HalfHeight);
				InArchive.SerializeFloat(PE_TEXT("Radius"), Cylinder->Raidus);
				InArchive.SerializeFloat(PE_TEXT("ConvexRadius"), Cylinder->ConvexRaidus);
			}
			else if (FRotatedTranslatedShape* Rotated = dynamic_cast<FRotatedTranslatedShape*>(InShape))
			{
				SerializeVector3(InArchive, PE_TEXT("Position"), Rotated->Position);
				SerializeQuaternion(InArchive, PE_TEXT("Rotation"), Rotated->Rotation);
				// HostedShape is owned by the rotated shape; nested tags keep the file
				// self-describing without a registry.
				InArchive.BeginObject(PE_TEXT("HostedShape"));
				WriteShapeBody(InArchive, Rotated->HostedShape);
				InArchive.EndObject();
			}
		}

		FShape* ReadShapeBody(EArchive& InArchive)
		{
			UINT32 ShapeType = static_cast<UINT32>(ESerializedShapeType::NONE);
			InArchive.SerializeUInt32(PE_TEXT("ShapeType"), ShapeType);
			switch (static_cast<ESerializedShapeType>(ShapeType))
			{
			case ESerializedShapeType::BOX:
			{
				Vector3 HalfExtent = Vector3::Zero();
				FLOAT ConvexRadius = 0.f;
				SerializeVector3(InArchive, PE_TEXT("HalfExtent"), HalfExtent);
				InArchive.SerializeFloat(PE_TEXT("ConvexRadius"), ConvexRadius);
				return new FBoxShape(HalfExtent, ConvexRadius);
			}
			case ESerializedShapeType::SPHERE:
			{
				FLOAT Radius = 0.f;
				InArchive.SerializeFloat(PE_TEXT("Radius"), Radius);
				return new FSphereShape(Radius);
			}
			case ESerializedShapeType::CAPSULE:
			{
				FLOAT HalfHeight = 0.f;
				FLOAT Radius = 0.f;
				InArchive.SerializeFloat(PE_TEXT("HalfHeight"), HalfHeight);
				InArchive.SerializeFloat(PE_TEXT("Radius"), Radius);
				return new FCapsuleShape(HalfHeight, Radius);
			}
			case ESerializedShapeType::CYLINDER:
			{
				FLOAT HalfHeight = 0.f;
				FLOAT Radius = 0.f;
				FLOAT ConvexRadius = 0.05f;
				InArchive.SerializeFloat(PE_TEXT("HalfHeight"), HalfHeight);
				InArchive.SerializeFloat(PE_TEXT("Radius"), Radius);
				InArchive.SerializeFloat(PE_TEXT("ConvexRadius"), ConvexRadius);
				return new FCylinerShape(HalfHeight, Radius, ConvexRadius);
			}
			case ESerializedShapeType::ROTATED_TRANSLATED:
			{
				Vector3 Position = Vector3::Zero();
				Quaternion Rotation = Quaternion::Identity();
				SerializeVector3(InArchive, PE_TEXT("Position"), Position);
				SerializeQuaternion(InArchive, PE_TEXT("Rotation"), Rotation);
				InArchive.BeginObject(PE_TEXT("HostedShape"));
				FShape* Hosted = ReadShapeBody(InArchive);
				InArchive.EndObject();
				return new FRotatedTranslatedShape(Position, Rotation, Hosted);
			}
			default:
				return nullptr;
			}
		}
	}

	BOOL32 SerializePhysicsShape(EArchive& InArchive, const CHAR* InName, FShape*& InOutShape)
	{
		if (InArchive.IsSaving())
		{
			InArchive.BeginObject(InName);
			WriteShapeBody(InArchive, InOutShape);
			InArchive.EndObject();
			return TRUE;
		}
		if (!InArchive.HasField(InName))
		{
			return FALSE;
		}
		InArchive.BeginObject(InName);
		FShape* LoadedShape = ReadShapeBody(InArchive);
		InArchive.EndObject();
		InOutShape = LoadedShape;
		return TRUE;
	}
}
