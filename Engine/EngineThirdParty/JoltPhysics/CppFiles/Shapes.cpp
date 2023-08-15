#include "../Headers/Shapes.h"
namespace PigeonEngine
{
	FShape::FShape() :
		m_Shape(nullptr),
		m_ShapeSettings(nullptr)
	{
	}

	FShape::~FShape()
	{
		if (m_Shape)
		{
			delete m_Shape;
			m_Shape = nullptr;
		}
		if (m_ShapeSettings)
		{
			delete m_ShapeSettings;
			m_ShapeSettings = nullptr;
		}
	}

	Shape* FShape::CreateShape(BOOL bCreateNew)
	{
		if (m_Shape && !bCreateNew)
			return m_Shape;
		else if (m_Shape)
			delete m_Shape;
		m_Shape = CreateShapeInternal();
		return m_Shape;
	}

	ShapeSettings* FShape::CreateShapeSettings(BOOL bCreateNew)
	{
		if (m_ShapeSettings && !bCreateNew)
			return m_ShapeSettings;
		else if (m_ShapeSettings)
			delete m_ShapeSettings;
		m_ShapeSettings = CreateShapeSettingsInternal();
		return m_ShapeSettings;
	}
}