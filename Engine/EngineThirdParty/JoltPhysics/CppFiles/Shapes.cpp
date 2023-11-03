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

	void FBoxShape::DrawPrimitive(RDebugWireframePrimitiveManager* Manager, Vector3 inPosition, Quaternion inRotation)
	{
		Manager->DrawCuboid(inPosition, inRotation, HalfExtent.x, HalfExtent.y, HalfExtent.z);
	}

	void FSphereShape::DrawPrimitive(RDebugWireframePrimitiveManager* Manager, Vector3 inPosition, Quaternion inRotation)
	{
		Manager->DrawSphere(inPosition, Raidus, Color4::Green());
	}

	void FCapsuleShape::DrawPrimitive(RDebugWireframePrimitiveManager* Manager, Vector3 inPosition, Quaternion inRotation)
	{

	}

	void FRotatedTranslatedShape::DrawPrimitive(RDebugWireframePrimitiveManager* Manager, Vector3 inPosition, Quaternion inRotation)
	{
		return HostedShape->DrawPrimitive(Manager, inPosition + Position, inRotation * Rotation);
	}
}