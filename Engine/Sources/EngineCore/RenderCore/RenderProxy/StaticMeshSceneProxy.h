#pragma once

#include <CoreMinimal.h>
#include "MeshSceneProxy.h"
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>

namespace PigeonEngine
{

	class EStaticMeshAsset;
	class EVertexShaderAsset;
	class EPixelShaderAsset;

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
		void			SetupProxy(const BOOL32 InIsHidden, const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices, const EStaticMeshAsset* InMeshAsset);
	public:
		virtual BOOL32	IsRenderValid()const override;
		void			UpdateMeshAsset(const EStaticMeshAsset* InMeshAsset);
		void			UpdateRenderResource();
		void			BindRenderResource()const;
		void			Draw()const;
	protected:
		void			SetupShaders();
		void			BindVertexShader()const;
		void			BindPixelShader()const;
		void			BindMeshResource()const;
		void			BindMaterialParameter(const UINT32 InSlot)const;
	protected:
		const EVertexShaderAsset*		VertexShader;
		const EPixelShaderAsset*		PixelShader;
		const EStaticMeshAsset*			MeshAsset;
		RStaticMeshMaterialParameter	MaterialParameter;
	protected:
		const PStaticMeshComponent*		Component;

		RENDER_PROXY_CLASS_BODY(RStaticMeshSceneProxy)

	};

};