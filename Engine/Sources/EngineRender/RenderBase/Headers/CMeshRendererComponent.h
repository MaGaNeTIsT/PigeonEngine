#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "./CRenderStructCommon.h"
#include "./CMaterialBase.h"
#include "../../../EngineGame/Headers/CComponent.h"
#include "../../AssetsManager/Headers/CMeshComponent.h"

class CMeshRendererComponent : public CRenderComponent
{
public:
	struct RenderPerDrawInfo
	{
		CustomStruct::CShaderGlobalPerDraw		PerDrawData;
		Microsoft::WRL::ComPtr<ID3D11Buffer>	PerDrawBuffer;
	};
public:
	BOOL	HasMesh()const { return (this->m_MeshComponent != NULL); }
	BOOL	HasMaterial()const { return (this->m_Material != NULL); }
	BOOL	HasConstantBuffer()const { return (this->m_ConstantBuffer != nullptr); }
public:
	CMaterialBase::MaterialType		GetMaterialType()const;
public:
	template<class T>
	T* AddMaterial()
	{
		if (this->HasMaterial())
		{
			delete (this->m_Material);
			this->m_Material = NULL;
		}
		this->m_Material = new T();
		return (reinterpret_cast<T*>(this->m_Material));
	}
	void			SetMeshComponent(const CMeshComponent* meshComponent);
public:
	virtual void	Init()override;
	virtual void	Update()override;
	virtual void	Draw()const override;
	virtual void	DrawExtra()const override;
#if _DEVELOPMENT_EDITOR
public:
	virtual void	SelectedEditorUpdate()override;
#endif
protected:
	void			UpdatePerDrawInfo(CustomType::Matrix4x4& worldMatrix, CustomType::Matrix4x4& worldInvMatrix, const CustomType::Vector4& customParameter);
	BOOL			CreateConstantBuffer(const UINT& size);
	void			UploadConstantBuffer(const void* data)const;
	void			BindConstantBuffer(const UINT& startSlot)const;
	void			UploadPerDrawConstantBuffer()const;
	virtual void	Bind(const BOOL& needPixelShader = TRUE)const;
public:
	CMeshRendererComponent();
	virtual ~CMeshRendererComponent();
protected:
	mutable BOOL										m_CurrentFramePerDrawUpload;
	mutable BOOL										m_CurrentFrameMaterialUpload;
	const CMeshComponent*								m_MeshComponent;
	CMaterialBase*										m_Material;
	RenderPerDrawInfo									m_PerDrawInfo;
	CustomStruct::CRenderBufferDesc						m_ConstantBufferDesc;
	Microsoft::WRL::ComPtr<ID3D11Buffer>				m_ConstantBuffer;
};