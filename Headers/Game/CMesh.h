#pragma once

#include "../../Headers/Base/CRenderDevice.h"

class CMesh
{

private:
	shared_ptr<ID3D11Buffer>			m_VertexBuffer	= NULL;
	shared_ptr<ID3D11Buffer>			m_IndexBuffer	= NULL;
	shared_ptr<CustomStruct::CVertex3D>	m_Data			= NULL;
};