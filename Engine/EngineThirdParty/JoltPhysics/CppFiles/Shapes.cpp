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
		m_Shape(nullptr),
		m_ShapeSettings(nullptr)
	{
	}

	FShape::~FShape()
	{
		//if (m_Shape)
		//{
		//	delete m_Shape;
		//	m_Shape = nullptr;
		//}
		//if (m_ShapeSettings)
		//{
		//	delete m_ShapeSettings;
		//	m_ShapeSettings = nullptr;
		//}
	}

	Shape* FShape::CreateShape(BOOL32 bCreateNew)
	{
		if (m_Shape && !bCreateNew)
			return m_Shape;
		else if (m_Shape)
			delete m_Shape;
		m_Shape = CreateShapeInternal();
		return m_Shape;
	}

	ShapeSettings* FShape::CreateShapeSettings(BOOL32 bCreateNew)
	{
		if (m_ShapeSettings && !bCreateNew)
			return m_ShapeSettings;
		else if (m_ShapeSettings)
			delete m_ShapeSettings;
		m_ShapeSettings = CreateShapeSettingsInternal();
		return m_ShapeSettings;
	}

	FCastRay::FCastRay()
	{
	}

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
		Manager->DrawCylinder(Bottom, Top, Raidus, Color4::Green());
		Manager->DrawSphere(Bottom, Raidus, Color4::Green());
		Manager->DrawSphere(Top, Raidus, Color4::Green());
	}

	void FRotatedTranslatedShape::DrawPrimitive(RDebugWireframePrimitiveManager* Manager, const Vector3& inPosition, const Quaternion& inRotation) const
	{
		return HostedShape->DrawPrimitive(Manager, inPosition + Position, inRotation * Rotation);
	}
}