#pragma once

//#include "./CRenderDevice.h"
//#include "../Game/CGameObject.h"
//
//struct MODEL_MATERIAL
//{
//	CHAR					Name[256];
//	CHAR					TextureName[256];
//	CustomStruct::CMaterial	Material;
//};
//
//struct SUBSET
//{
//	USHORT			StartIndex;
//	USHORT			IndexNum;
//	MODEL_MATERIAL	Material;
//};
//
//struct MODEL
//{
//	CustomStruct::CVertex3D*	VertexArray;
//	USHORT		VertexNum;
//	USHORT*		IndexArray;
//	USHORT		IndexNum;
//	SUBSET*		SubsetArray;
//	USHORT		SubsetNum;
//};
//
//class CShader;
//class CTexture;
//
//class CReadOBJ : public CGameObject
//{
//private:
//	ID3D11Buffer*				m_VertexBuffer	= NULL;
//	ID3D11Buffer*				m_IndexBuffer	= NULL;
//
//	CustomStruct::DX11_SUBSET*	m_SubsetArray	= NULL;
//	USHORT						m_SubsetNum;
//
//	CShader*					m_Shader		= NULL;
//	CTexture*					m_Texture		= NULL;
//
//	void	LoadObj(const CHAR* FileName, MODEL* Model);
//	void	LoadMaterial(const CHAR* FileName, MODEL_MATERIAL** MaterialArray, USHORT* MaterialNum);
//public:
//	void	Init();
//	void	Uninit();
//	void	Update();
//	void	Draw();
//	void	DrawShadow();
//
//	void	Load(const CHAR* FileName);
//	void	Unload();
//};