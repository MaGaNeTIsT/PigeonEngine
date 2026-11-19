#include "../Headers/Shapes.h"
namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<FShape, ERTTIObject>();
		RegisterClassType<FCastRay, ERTTIObject>();
		RegisterClassType<FBoxShape, FShape>();
		RegisterClassType<FSphereShape, FShape>();
		RegisterClassType<FCapsuleShape, FShape>();
		RegisterClassType<FCylinerShape, FShape>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	FShape::FShape()
	{
	}

	FShape::~FShape()
	{
		// ShapeInstance / ShapeSettingsInstance are JPH::Ref members: their
		// destructors release the references this FShape owns. Bodies, characters
		// and compound shapes hold their own references, so nothing is freed while
		// they still use it.
	}

	Shape* FShape::CreateShape(BOOL32 bCreateNew)
	{
		if (ShapeInstance.GetPtr() != nullptr && !bCreateNew)
		{
			return ShapeInstance.GetPtr();
		}
		// Assigning nullptr releases the previous instance; assigning the new raw
		// pointer lets Ref take its reference (CreateShapeInternal returns refcount 0).
		ShapeInstance = nullptr;
		ShapeInstance = CreateShapeInternal();
		return ShapeInstance.GetPtr();
	}

	ShapeSettings* FShape::CreateShapeSettings(BOOL32 bCreateNew)
	{
		if (ShapeSettingsInstance.GetPtr() != nullptr && !bCreateNew)
		{
			return ShapeSettingsInstance.GetPtr();
		}
		ShapeSettingsInstance = nullptr;
		ShapeSettingsInstance = CreateShapeSettingsInternal();
		return ShapeSettingsInstance.GetPtr();
	}

	FCastRay::FCastRay()
	{
	}
#if ENGINE_EDITOR_ONLY
	void FBoxShape::DrawPrimitive(RDebugWireframePrimitiveManager* Manager, const Vector3& inPosition, const Quaternion& inRotation) const
	{
		Manager->DrawCuboid(inPosition, inRotation, HalfExtent.x * 2, HalfExtent.y * 2, HalfExtent.z * 2, Color4::Green());
	}

	void FSphereShape::DrawPrimitive(RDebugWireframePrimitiveManager* Manager, const Vector3& inPosition, const Quaternion& inRotation) const
	{
		Manager->DrawSphere(inPosition, Raidus, Color4::Green());
	}

	void FCapsuleShape::DrawPrimitive(RDebugWireframePrimitiveManager* Manager, const Vector3& inPosition, const Quaternion& inRotation) const
	{
		Vector3 Direction = QuaternionTransformVector(inRotation, Vector3(0.f, HalfHeightOfCylinder, 0.f));
		Vector3 Top = inPosition + Direction;
		Vector3 Bottom = inPosition - Direction;
		Manager->DrawCapsule(Bottom, Top, Raidus, Raidus, Color4::Green());
	}

	void FCylinerShape::DrawPrimitive(RDebugWireframePrimitiveManager* Manager, const Vector3& inPosition, const Quaternion& inRotation) const
	{
		// The debug manager has no cylinder primitive; the bounding box is enough to place
		// the shape while editing.
		Manager->DrawCuboid(inPosition, inRotation, Raidus * 2.f, HalfHeight * 2.f, Raidus * 2.f, Color4::Green());
	}

	void FRotatedTranslatedShape::DrawPrimitive(RDebugWireframePrimitiveManager* Manager, const Vector3& inPosition, const Quaternion& inRotation) const
	{
		return HostedShape->DrawPrimitive(Manager, inPosition + Position, inRotation * Rotation);
	}
#endif
}
