#pragma once

#include "../../Entry/MyMain.h"

class CTexture;

namespace CustomStruct
{
	struct CVertex3D
	{
		CVertex3D() { ZeroMemory(this, sizeof(*this)); }
		XMFLOAT4	Position;
		XMFLOAT4	Normal;
		XMFLOAT4	Tangent;
		XMFLOAT4	Color;
		XMFLOAT2	UV0;
	};

	struct CColor
	{
		CColor() { r = 1.f; g = 1.f; b = 1.f; a = 1.f; }
		CColor(const FLOAT& _r, const FLOAT& _g, const FLOAT& _b, const FLOAT& _a)
		{
			r = _r;
			g = _g;
			b = _b;
			a = _a;
		}
		FLOAT	r;
		FLOAT	g;
		FLOAT	b;
		FLOAT	a;
	};

	struct CLightData
	{
		XMFLOAT4	Direction;
		XMFLOAT4	Color;
	};

	struct CMaterial
	{
		CColor		Ambient;
		CColor		Diffuse;
		CColor		Specular;
		CColor		Emission;
		FLOAT		Shininess;
		FLOAT		Dummy[3];
	};

	struct DX11_MODEL_MATERIAL
	{
		CMaterial		Material;
		class CTexture*	Texture;
	};

	struct DX11_SUBSET
	{
		USHORT				StartIndex;
		USHORT				IndexNum;
		DX11_MODEL_MATERIAL	Material;
	};
}