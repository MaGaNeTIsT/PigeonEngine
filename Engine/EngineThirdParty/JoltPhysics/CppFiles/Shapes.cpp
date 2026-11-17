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

	FShape::FShape() :
		ShapeInstance(nullptr),
		ShapeSettingsInstance(nullptr)
	{
	}

	FShape::~FShape()
	{
		//if (ShapeInstance)
		//{
		//	delete ShapeInstance;
		//	ShapeInstance = nullptr;
		//}
		//if (ShapeSettingsInstance)
		//{
		//	delete ShapeSettingsInstance;
		//	ShapeSettingsInstance = nullptr;
		//}
	}

	Shape* FShape::CreateShape(BOOL32 bCreateNew)
	{
		if (ShapeInstance && !bCreateNew)
			return ShapeInstance;
		else if (ShapeInstance)
			delete ShapeInstance;
		ShapeInstance = CreateShapeInternal();
		return ShapeInstance;
	}

	ShapeSettings* FShape::CreateShapeSettings(BOOL32 bCreateNew)
	{
		if (ShapeSettingsInstance && !bCreateNew)
			return ShapeSettingsInstance;
		else if (ShapeSettingsInstance)
			delete ShapeSettingsInstance;
		ShapeSettingsInstance = CreateShapeSettingsInternal();
		return ShapeSettingsInstance;
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

	void FRotatedTranslatedShape::DrawPrimitive(RDebugWireframePrimitiveManager* Manager, const Vector3& inPosition, const Quaternion& inRotation) const
	{
		return HostedShape->DrawPrimitive(Manager, inPosition + Position, inRotation * Rotation);
	}
#endif
}
