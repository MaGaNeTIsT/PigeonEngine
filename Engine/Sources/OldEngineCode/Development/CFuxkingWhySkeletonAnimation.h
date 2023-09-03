/*
#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class CFuxkingWhySkeletonAnimation
{
public:
	struct CFuxkingWhyMeshVertex
	{
		FLOAT	Pos[4];
		FLOAT	Nml[4];
		USHORT	BnI[4];
		FLOAT	BnW[4];
	};
	struct CFuxkingWhyMesh
	{
		UINT32									IndexCount;
		Microsoft::WRL::ComPtr<ID3D11Buffer>	VertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>	IndexBuffer;
	};
	struct CFuxkingWhyBone
	{
		UINT32					Index;
		std::string				Name;
		DirectX::XMFLOAT4X4		BindPoseMatrix;
		DirectX::XMFLOAT4X4		BoneMatrix;
	};
	struct CFuxkingWhyPerDrawCBuffer
	{
		DirectX::XMFLOAT4X4		WorldMatrix;
		DirectX::XMFLOAT4X4		WorldInvMatrix;
		DirectX::XMFLOAT4X4		WorldInvTransposeMatrix;
	};
	struct CFuxkingWhyBoneCBuffer
	{
		DirectX::XMFLOAT4		SkeletonBoneNum;
		DirectX::XMFLOAT4X4		SkeletonMatrix[CustomStruct::CRenderBaseSetting::RenderBoneMaxNum * 2u];
	};
public:
	void Init();
	void Update();
	void Draw();
	void Uninit();
public:
	Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_VertexShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_InputLayout;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_PixelShader;
public:
	CFuxkingWhyPerDrawCBuffer					m_PerDrawCBufferData;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_PerDrawCBuffer;
	CFuxkingWhyBoneCBuffer						m_BoneCBufferData;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_BoneCBuffer;
public:
	std::map<std::string, UINT32>					m_BoneMap;
	std::vector<CFuxkingWhyBone>				m_BoneList;
	std::vector<CFuxkingWhyMesh>				m_Meshes;
	FLOAT										m_AnimationT;
public:
	Assimp::Importer							m_AssimpImpoter;
	const aiScene*								m_AssimpScene;
	Assimp::Importer							m_AssimpAnimImpoter;
	const aiScene*								m_AssimpAnimScene;
	std::vector<const aiMesh*>					m_AssimpMeshes;
	std::vector<const aiAnimation*>				m_AssimpAnims;
public:
	CFuxkingWhySkeletonAnimation();
	~CFuxkingWhySkeletonAnimation();
};
*/