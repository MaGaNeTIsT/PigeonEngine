#pragma once

#include <CoreMinimal.h>
#include "MeshSceneProxy.h"
#include <ShaderAsset/ShaderAsset.h>
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>

namespace PigeonEngine
{

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
		void			SetupProxy(const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices, const EStaticMeshAsset* InMeshAsset);		
	public:
		void			UpdateMeshAsset(const EStaticMeshAsset* InMeshAsset);
		virtual void	UpdateRenderResource()override;
		virtual void	BindRenderResource()override;
	protected:
		void			SetupShaders();
		void			BindMaterialParameter(const UINT32 InSlot);
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