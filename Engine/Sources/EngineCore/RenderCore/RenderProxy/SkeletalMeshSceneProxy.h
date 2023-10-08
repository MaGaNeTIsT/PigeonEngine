#pragma once

#include <CoreMinimal.h>
#include "MeshSceneProxy.h"
#include <SkeletonAsset/SkeletonAsset.h>
#include <PigeonBase/Object/Component/Primitive/SkeletalMeshComponent.h>

namespace PigeonEngine
{

	class ESkinnedMeshAsset;
	class ESkeletonAsset;
	class EVertexShaderAsset;
	class EPixelShaderAsset;

	class RSkeletalMeshMaterialParameter : public RMeshMaterialParameter
	{
	public:
		virtual void	SetupParameters()override;

		CLASS_MATERIAL_PARAMETER(RSkeletalMeshMaterialParameter)
	};

	class RSkeletalMeshSceneProxy : public RMeshSceneProxy
	{
	public:
		RSkeletalMeshSceneProxy(PSkeletalMeshComponent* InComponent);
		void			SetupProxy(const BOOL32 InIsHidden, const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices, const ESkinnedMeshAsset* InMeshAsset, const ESkeletonAsset* InSkeletonAsset);
	public:
		virtual BOOL32	IsRenderValid()const override;
		void			UpdateMeshAsset(const ESkinnedMeshAsset* InMeshAsset);
		void			UpdateSkeletonAsset(const ESkeletonAsset* InSkeletonAsset);
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
		const ESkinnedMeshAsset*		MeshAsset;
		const ESkeletonAsset*			SkeletonAsset;
		ESkeletonRenderResource			SkeletonRenderResource;
		RSkeletalMeshMaterialParameter	MaterialParameter;
		TArray<Matrix4x4>				BoneValues;
	protected:
		const PSkeletalMeshComponent*	Component;

		RENDER_PROXY_CLASS_BODY(RSkeletalMeshSceneProxy)

	};

};