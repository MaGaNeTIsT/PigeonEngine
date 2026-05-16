#pragma once

#include <CoreMinimal.h>
#include "MeshSceneProxy.h"
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>
#include <RenderMaterials/MaterialBinding.h>

namespace PigeonEngine
{

	class EStaticMeshAsset;
	class EVertexShaderAsset;
	class EPixelShaderAsset;
	class EMaterialAsset;

	class RStaticMeshMaterialParameter : public RMeshMaterialParameter
	{
	public:
		virtual void	SetupParameters()override;

		CLASS_MATERIAL_PARAMETER(RStaticMeshMaterialParameter)
	};

	class RStaticMeshSceneProxy : public RMeshSceneProxy
	{
	public:
		RStaticMeshSceneProxy(PStaticMeshComponent* InComponent);
		void			SetupProxy(const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices, const EStaticMeshAsset* InMeshAsset, const EMaterialAsset* InMaterialAsset);
	public:
		virtual BOOL32	IsRenderValid()const override;
		void			UpdateMeshAsset(const EStaticMeshAsset* InMeshAsset);
		void			UpdateMaterialAsset(const EMaterialAsset* InMaterialAsset);
		void			UpdateMaterialCBData(const TArray<TArray<BYTE>>& InCBDatas);
		void			UpdateMaterialTextures(const TArray<RMaterialTextureSRV>& InSRVs);
		void			UpdateRenderResource();
		void			BindRenderResource()const;
		void			Draw()const;
	protected:
		void			SetupShaders();
		void			SetupMaterialResources();
		void			BindVertexShader()const;
		void			BindPixelShader()const;
		void			BindMeshResource()const;
		void			BindMaterialParameter(const UINT32 InSlot)const;
		void			BindMaterialCBs()const;
		void			BindMaterialTextures()const;
	protected:
		const EVertexShaderAsset*		VertexShader;
		const EPixelShaderAsset*		PixelShader;
		const EStaticMeshAsset*			MeshAsset;
		const EMaterialAsset*			MaterialAsset;
		RStaticMeshMaterialParameter	MaterialParameter;
		TArray<RBufferResource>			MaterialCBBuffers;
		TArray<RMaterialTextureSRV>		MaterialSRVs;
	protected:
		const PStaticMeshComponent*		Component;

		RENDER_PROXY_CLASS_BODY(RStaticMeshSceneProxy)

	};

};