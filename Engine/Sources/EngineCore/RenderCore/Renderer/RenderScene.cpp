#include "RenderScene.h"
#include <MeshAsset/MeshAsset.h>
#include <TextureAsset/TextureAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <RenderProxy/ViewProxy.h>
#include <RenderProxy/SkyLightProxy.h>
#include <RenderProxy/LightSceneProxy.h>
#include <RenderProxy/PrimitiveSceneProxy.h>
#include <RenderProxy/MeshSceneProxy.h>
#include <RenderProxy/StaticMeshSceneProxy.h>
#include <RenderProxy/SkeletalMeshSceneProxy.h>
#include <PigeonBase/Object/Component/CameraAndLight/CameraComponent.h>
#include <PigeonBase/Object/Component/Primitive/SkyLightComponent.h>
#include <PigeonBase/Object/Component/CameraAndLight/DirectionalLightComponent.h>
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>
#include <PigeonBase/Object/Component/Primitive/SkeletalMeshComponent.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RScene, EObjectBase, RSceneInterface>();
		RegisterClassType<RSceneNull, EObjectBase, RSceneInterface>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RScene::RScene()
	{
	}
	RScene::~RScene()
	{
		UnbindErrorCheck();
	}
	void RScene::Init()
	{

	}
	void RScene::Uninit()
	{
		ClearAll();
	}
	void RScene::ClearAll()
	{
		RenderSceneOctree.ClearPrimitives();

		RenderAddCommands.DoCommands();
		RenderAddCommands.EmptyQueue();
		RenderUpdateCommands.DoCommands();
		RenderUpdateCommands.EmptyQueue();
		RenderRemoveCommands.DoCommands();
		RenderRemoveCommands.EmptyQueue();

		UnbindErrorCheck();
	}
	void RScene::UnbindErrorCheck()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all views' mapping failed"), (ViewProxies.SceneProxyMapping.Length() == 0u));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all views failed"), (ViewProxies.SceneProxies.Length() == 0u));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all sky lights' failed"), (SkyLightProxies.SceneProxyMapping.Length() == 0u));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all sky lights failed"), (SkyLightProxies.SceneProxies.Length() == 0u));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all directional lights' mapping failed"), (DirectionalLightSceneProxies.SceneProxyMapping.Length() == 0u));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all directional lights failed"), (DirectionalLightSceneProxies.SceneProxies.Length() == 0u));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all static meshes' mapping failed"), (StaticMeshSceneProxies.SceneProxyMapping.Length() == 0u));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all static meshes failed"), (StaticMeshSceneProxies.SceneProxies.Length() == 0u));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all skeletal meshes' mapping failed"), (SkeletalMeshSceneProxies.SceneProxyMapping.Length() == 0u));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all skeletal meshes failed"), (SkeletalMeshSceneProxies.SceneProxies.Length() == 0u));
	}
	void RScene::AddCamera(PCameraComponent* InComponent)
	{
		RScene* Scene = this;
		RViewProxy* SceneProxy = InComponent->CreateSceneProxy();

		const BOOL32 TempIsMainCamera = InComponent->IsMainCamera();
		ERenderViewMatrices* TempMatrices = new ERenderViewMatrices(
			InComponent->GetComponentWorldLocation(),
			InComponent->GetComponentWorldRotation(),
			InComponent->GetComponentWorldScale(),
			InComponent->GetCameraMatrix());
		ERenderViewParams* TempParams = new ERenderViewParams(InComponent->GetCameraFrustum(), InComponent->GetCameraViewInfo());

		RenderAddCommands.EnqueueCommand(
			[Scene, SceneProxy, TempIsMainCamera, TempMatrices, TempParams]()->void
			{
				SceneProxy->SetupProxy(TempIsMainCamera, *TempMatrices, *TempParams);
				delete TempMatrices;
				delete TempParams;
				Scene->AddOrRemoveCamera_RenderThread(SceneProxy, TRUE);
			});
	}
	void RScene::RemoveCamera(PCameraComponent* InComponent)
	{
		RScene* Scene = this;
		RViewProxy* SceneProxy = InComponent->ViewProxy;
		InComponent->ViewProxy = nullptr;
		RenderRemoveCommands.EnqueueCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->AddOrRemoveCamera_RenderThread(SceneProxy, FALSE);
				delete SceneProxy;
			});
	}
	void RScene::UpdateCamera(PCameraComponent* InComponent)
	{
		RScene* Scene = this;
		RViewProxy* SceneProxy = InComponent->ViewProxy;

		UINT8 UpdateState = InComponent->GetUpdateRenderState();

		const BOOL32 TempIsMainCamera = InComponent->IsMainCamera();
		ERenderViewMatrices* TempMatrices = nullptr;
		if ((UpdateState & PCameraComponent::PCameraUpdateState::CAMERA_UPDATE_STATE_MATRIX) != 0u)
		{
			TempMatrices = new ERenderViewMatrices(
				InComponent->GetComponentWorldLocation(),
				InComponent->GetComponentWorldRotation(),
				InComponent->GetComponentWorldScale(),
				InComponent->GetCameraMatrix());
		}
		ERenderViewParams* TempParams = nullptr;
		if ((UpdateState & PCameraComponent::PCameraUpdateState::CAMERA_UPDATE_STATE_VIEW) != 0u)
		{
			TempParams = new ERenderViewParams(InComponent->GetCameraFrustum(), InComponent->GetCameraViewInfo());
		}
#if _EDITOR_ONLY
		if ((!!TempParams) && (!TempMatrices))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Check update camera render state failed"));
		}
#endif

		RenderUpdateCommands.EnqueueCommand(
			[Scene, SceneProxy, TempIsMainCamera, TempMatrices, TempParams]()->void
			{
				SceneProxy->UpdateViewSettings(TempIsMainCamera);
				BOOL32 NeedUpdateRenderResource = FALSE;
				// View params MUST be updating before matrices.
				if (TempParams)
				{
					SceneProxy->UpdateViewParams(*TempParams);
					delete TempParams;
					NeedUpdateRenderResource = TRUE;
				}
				if (TempMatrices)
				{
					SceneProxy->UpdateMatrices(*TempMatrices);
					delete TempMatrices;
					NeedUpdateRenderResource = TRUE;
				}
				if (NeedUpdateRenderResource)
				{
					SceneProxy->UpdateRenderResource();
				}
			});
	}
	void RScene::AddSkyLight(PSkyLightComponent* InComponent)
	{
		RScene* Scene = this;
		RSkyLightSceneProxy* SceneProxy = InComponent->CreateSceneProxy();

		ERenderSkyLightMatrices* TempMatrices = new ERenderSkyLightMatrices(
			InComponent->GetComponentWorldLocation(),
			InComponent->GetComponentWorldRotation(),
			InComponent->GetComponentWorldScale());
		ERenderSkyLightParams* TempParams = new ERenderSkyLightParams(InComponent->GetLightAdjust(), InComponent->GetIntensity());
		const ETextureCubeAsset* TempCubeMapAsset = InComponent->GetCubeMapAsset();

		RenderAddCommands.EnqueueCommand(
			[Scene, SceneProxy, TempMatrices, TempParams, TempCubeMapAsset]()->void
			{
				SceneProxy->SetupProxy(*TempMatrices, *TempParams, TempCubeMapAsset);
				delete TempMatrices;
				delete TempParams;
				Scene->AddOrRemoveSkyLight_RenderThread(SceneProxy, TRUE);
			});
	}
	void RScene::RemoveSkyLight(PSkyLightComponent* InComponent)
	{
		RScene* Scene = this;
		RSkyLightSceneProxy* SceneProxy = InComponent->SceneProxy;
		InComponent->SceneProxy = nullptr;
		RenderRemoveCommands.EnqueueCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->AddOrRemoveSkyLight_RenderThread(SceneProxy, FALSE);
				delete SceneProxy;
			});
	}
	void RScene::UpdateSkyLight(PSkyLightComponent* InComponent)
	{
		RScene* Scene = this;
		RSkyLightSceneProxy* SceneProxy = InComponent->SceneProxy;

		UINT8 UpdateState = InComponent->GetUpdateRenderState();

		ERenderSkyLightMatrices* TempMatrices = nullptr;
		if ((UpdateState & PSkyLightComponent::PSkyLightUpdateState::SKY_LIGHT_UPDATE_STATE_MATRIX) != 0u)
		{
			TempMatrices = new ERenderSkyLightMatrices(
				InComponent->GetComponentWorldLocation(),
				InComponent->GetComponentWorldRotation(),
				InComponent->GetComponentWorldScale());
		}
		ERenderSkyLightParams* TempParams = nullptr;
		if ((UpdateState & PSkyLightComponent::PSkyLightUpdateState::SKY_LIGHT_UPDATE_STATE_PARAMS) != 0u)
		{
			TempParams = new ERenderSkyLightParams(InComponent->GetLightAdjust(), InComponent->GetIntensity());
		}
		const ETextureCubeAsset* TempCubeMapAsset = nullptr;
		if ((UpdateState & PSkyLightComponent::PSkyLightUpdateState::SKY_LIGHT_UPDATE_STATE_TEXTURE) != 0u)
		{
			TempCubeMapAsset = InComponent->GetCubeMapAsset();
		}

		RenderUpdateCommands.EnqueueCommand(
			[Scene, SceneProxy, TempMatrices, TempParams, TempCubeMapAsset]()->void
			{
				BOOL32 NeedUpdateRenderResource = FALSE;
				if (TempMatrices)
				{
					SceneProxy->UpdateMatrices(*TempMatrices);
					delete TempMatrices;
					NeedUpdateRenderResource = TRUE;
				}
				if (TempParams)
				{
					SceneProxy->UpdateSkyLightParams(*TempParams);
					delete TempParams;
					NeedUpdateRenderResource = TRUE;
				}
				if (TempCubeMapAsset)
				{
					SceneProxy->UpdateTextureCubeAsset(TempCubeMapAsset);
				}
				if (NeedUpdateRenderResource)
				{
					SceneProxy->UpdateRenderResource();
				}
			});
	}
	void RScene::AddDirectionalLight(PDirectionalLightComponent* InComponent)
	{
		RScene* Scene = this;
		RDirectionalLightSceneProxy* SceneProxy = InComponent->CreateSceneProxy();

		ERenderDirectionalLightMatrices* TempMatrices = new ERenderDirectionalLightMatrices(
			InComponent->GetComponentWorldLocation(),
			InComponent->GetComponentWorldRotation(),
			InComponent->GetComponentWorldScale());
		ERenderLightParams* TempParams = new ERenderLightParams(
			InComponent->GetLightColor(),
			InComponent->GetLightIntensity(),
			InComponent->IsLightCastShadow(),
			InComponent->GetShadowMapSize());
		ECascadeShadowData* TempUsedCascadeShadowData = nullptr;
		if (const ECascadeShadowData* TempCascadeShadowData = InComponent->GetCascadeShadowData(); !!TempCascadeShadowData)
		{
			TempUsedCascadeShadowData = new ECascadeShadowData(*TempCascadeShadowData);
		}

		RenderAddCommands.EnqueueCommand(
			[Scene, SceneProxy, TempMatrices, TempParams, TempUsedCascadeShadowData]()->void
			{
				SceneProxy->SetupProxy(*TempMatrices, *TempParams, TempUsedCascadeShadowData);
				delete TempMatrices;
				delete TempParams;
				if (TempUsedCascadeShadowData) { delete TempUsedCascadeShadowData; }
				Scene->AddOrRemoveDirectionalLight_RenderThread(SceneProxy, TRUE);
			});
	}
	void RScene::RemoveDirectionalLight(PDirectionalLightComponent* InComponent)
	{
		RScene* Scene = this;
		RDirectionalLightSceneProxy* SceneProxy = InComponent->SceneProxy;
		InComponent->SceneProxy = nullptr;
		RenderRemoveCommands.EnqueueCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->AddOrRemoveDirectionalLight_RenderThread(SceneProxy, FALSE);
				delete SceneProxy;
			});
	}
	void RScene::UpdateDirectionalLight(PDirectionalLightComponent* InComponent)
	{
		RScene* Scene = this;
		RDirectionalLightSceneProxy* SceneProxy = InComponent->SceneProxy;

		UINT8 UpdateState = InComponent->GetUpdateRenderState();

		ERenderDirectionalLightMatrices* TempMatrices = nullptr;
		if ((UpdateState & PCameraComponent::PCameraUpdateState::CAMERA_UPDATE_STATE_MATRIX) != 0u)
		{
			TempMatrices = new ERenderDirectionalLightMatrices(
				InComponent->GetComponentWorldLocation(),
				InComponent->GetComponentWorldRotation(),
				InComponent->GetComponentWorldScale());
		}
		ERenderLightParams* TempParams = nullptr;
		if ((UpdateState & PDirectionalLightComponent::PLightUpdateState::LIGHT_UPDATE_STATE_DATA) != 0u)
		{
			TempParams = new ERenderLightParams(
				InComponent->GetLightColor(),
				InComponent->GetLightIntensity(),
				InComponent->IsLightCastShadow(),
				InComponent->GetShadowMapSize());
		}
		ECascadeShadowData* TempUsedCascadeShadowData = nullptr;
		if ((UpdateState & PDirectionalLightComponent::PLightUpdateState::LIGHT_UPDATE_STATE_CASCADE) != 0u)
		{
			if (const ECascadeShadowData* TempCascadeShadowData = InComponent->GetCascadeShadowData(); !!TempCascadeShadowData)
			{
				TempUsedCascadeShadowData = new ECascadeShadowData(*TempCascadeShadowData);
			}
		}

		RenderUpdateCommands.EnqueueCommand(
			[Scene, SceneProxy, TempMatrices, TempParams, TempUsedCascadeShadowData]()->void
			{
				BOOL32 NeedUpdateParams = FALSE;
				if (TempMatrices)
				{
					SceneProxy->UpdateMatrices(*TempMatrices);
					delete TempMatrices;
					NeedUpdateParams = TRUE;
				}
				if (TempParams)
				{
					SceneProxy->UpdateLightParams(*TempParams);
					delete TempParams;
					NeedUpdateParams = TRUE;
				}
				if (TempUsedCascadeShadowData)
				{
					SceneProxy->UpdateCascadeData(TempUsedCascadeShadowData);
					delete TempUsedCascadeShadowData;
					NeedUpdateParams = TRUE;
				}
				if (NeedUpdateParams)
				{
					SceneProxy->MarkNeedUpdateParams();
				}
			});
	}
	void RScene::AddStaticMesh(PStaticMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RStaticMeshSceneProxy* SceneProxy = InComponent->CreateSceneProxy();

		const BOOL32 InIsRenderHidden = InComponent->IsPrimitiveRenderHidden(),
			InIsMovable = InComponent->GetMobility() != EMobilityType::EMT_STATIC,
			InIsCastShadow = InComponent->IsPrimitiveCastShadow(),
			InIsReceiveShadow = InComponent->IsPrimitiveReceiveShadow();
		ERenderPrimitiveMatrices* TempMatrices = new ERenderPrimitiveMatrices(
			InComponent->GetComponentWorldLocation(),
			InComponent->GetComponentWorldRotation(),
			InComponent->GetComponentWorldScale());
		const EStaticMeshAsset* TempMeshAsset = InComponent->GetMeshAsset();

		RenderAddCommands.EnqueueCommand(
			[Scene, SceneProxy, InIsRenderHidden, InIsMovable, InIsCastShadow, InIsReceiveShadow, TempMatrices, TempMeshAsset]()->void
			{
				SceneProxy->SetupProxy(InIsRenderHidden, InIsMovable, InIsCastShadow, InIsReceiveShadow, *TempMatrices, TempMeshAsset);
				delete TempMatrices;
				Scene->AddOrRemoveStaticMesh_RenderThread(SceneProxy, TRUE);
			});
	}
	void RScene::RemoveStaticMesh(PStaticMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RStaticMeshSceneProxy* SceneProxy = InComponent->SceneProxy;
		InComponent->SceneProxy = nullptr;
		RenderRemoveCommands.EnqueueCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->AddOrRemoveStaticMesh_RenderThread(SceneProxy, FALSE);
				delete SceneProxy;
			});
	}
	void RScene::UpdateStaticMesh(PStaticMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RStaticMeshSceneProxy* SceneProxy = InComponent->SceneProxy;

		UINT8 UpdateState = InComponent->GetUpdateRenderState();

		const BOOL32 InIsRenderHidden = InComponent->IsPrimitiveRenderHidden(),
			InIsMovable = InComponent->GetMobility() != EMobilityType::EMT_STATIC,
			InIsCastShadow = InComponent->IsPrimitiveCastShadow(),
			InIsReceiveShadow = InComponent->IsPrimitiveReceiveShadow();
		ERenderPrimitiveMatrices* TempMatrices = nullptr;
		if ((UpdateState & PStaticMeshComponent::PStaticMeshUpdateState::STATIC_MESH_UPDATE_STATE_MATRIX) != 0u)
		{
			TempMatrices = new ERenderPrimitiveMatrices(
				InComponent->GetComponentWorldLocation(),
				InComponent->GetComponentWorldRotation(),
				InComponent->GetComponentWorldScale());
		}
		const EStaticMeshAsset* TempMeshAsset = nullptr;
		if ((UpdateState & PStaticMeshComponent::PStaticMeshUpdateState::STATIC_MESH_UPDATE_STATE_ASSET) != 0u)
		{
			TempMeshAsset = InComponent->GetMeshAsset();
		}

		RenderUpdateCommands.EnqueueCommand(
			[Scene, SceneProxy, InIsRenderHidden, InIsMovable, InIsCastShadow, InIsReceiveShadow, TempMatrices, TempMeshAsset]()->void
			{
				BOOL32 NeedUpdateRenderResource = FALSE;
				SceneProxy->SetPrimitiveSettings(InIsRenderHidden, InIsMovable, InIsCastShadow, InIsReceiveShadow);
				if (TempMatrices)
				{
					SceneProxy->UpdatePrimitiveMatrices(*TempMatrices);
					delete TempMatrices;
					NeedUpdateRenderResource = TRUE;
				}
				if (TempMeshAsset)
				{
					SceneProxy->UpdateMeshAsset(TempMeshAsset);
				}
				if (NeedUpdateRenderResource)
				{
					SceneProxy->UpdateRenderResource();
				}
			});
	}
	void RScene::AddSkeletalMesh(PSkeletalMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RSkeletalMeshSceneProxy* SceneProxy = InComponent->CreateSceneProxy();

		Check((InComponent->GetMobility() == EMobilityType::EMT_DYNAMIC));
		const BOOL32 InIsRenderHidden = InComponent->IsPrimitiveRenderHidden(),
			InIsCastShadow = InComponent->IsPrimitiveCastShadow(),
			InIsReceiveShadow = InComponent->IsPrimitiveReceiveShadow();
		ERenderPrimitiveMatrices* TempMatrices = new ERenderPrimitiveMatrices(
			InComponent->GetComponentWorldLocation(),
			InComponent->GetComponentWorldRotation(),
			InComponent->GetComponentWorldScale());
		const ESkinnedMeshAsset* TempMeshAsset = InComponent->GetMeshAsset();
		const ESkeletonAsset* TempSkeletonAsset = InComponent->GetSkeletonAsset();
		const ESkeletonBoneMemoryPool& TempBoneMemoryPool = InComponent->GetBoneMemoryPool();

		RenderAddCommands.EnqueueCommand(
			[Scene, SceneProxy, InIsRenderHidden, InIsCastShadow, InIsReceiveShadow,
			TempMatrices, TempMeshAsset, TempSkeletonAsset,
			TempBoneToRootTransforms = TempBoneMemoryPool.GetBoneToRootTransforms()
			]()->void
			{
				TArray<Matrix4x4> TempBoneToRootMatrices;
				if (const UINT32 MatrixNum = TempBoneToRootTransforms.Length(); MatrixNum > 0u)
				{
					TempBoneToRootMatrices.Resize(MatrixNum);
					for (UINT32 MatrixIndex = 0u; MatrixIndex < MatrixNum; MatrixIndex++)
					{
#if (_USE_MATRIX_FOR_BONE_TO_ROOT)
						TempBoneToRootMatrices[MatrixIndex] = TempBoneToRootTransforms[MatrixIndex];
#else
						TempBoneToRootMatrices[MatrixIndex] = TempBoneToRootTransforms[MatrixIndex].ToMatrix4x4();
#endif
					}
				}
				SceneProxy->SetupProxy(InIsRenderHidden, TRUE, InIsCastShadow, InIsReceiveShadow,
					*TempMatrices, TempMeshAsset, TempSkeletonAsset, TempBoneToRootMatrices);
				delete TempMatrices;
				Scene->AddOrRemoveSkeletalMesh_RenderThread(SceneProxy, TRUE);
			});
	}
	void RScene::RemoveSkeletalMesh(PSkeletalMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RSkeletalMeshSceneProxy* SceneProxy = InComponent->SceneProxy;
		InComponent->SceneProxy = nullptr;
		RenderRemoveCommands.EnqueueCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->AddOrRemoveSkeletalMesh_RenderThread(SceneProxy, FALSE);
				delete SceneProxy;
			});
	}
	void RScene::UpdateSkeletalMesh(PSkeletalMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RSkeletalMeshSceneProxy* SceneProxy = InComponent->SceneProxy;

		UINT8 UpdateState = InComponent->GetUpdateRenderState();

		Check((InComponent->GetMobility() == EMobilityType::EMT_DYNAMIC));
		const BOOL32 InIsRenderHidden = InComponent->IsPrimitiveRenderHidden(),
			InIsCastShadow = InComponent->IsPrimitiveCastShadow(),
			InIsReceiveShadow = InComponent->IsPrimitiveReceiveShadow();
		ERenderPrimitiveMatrices* TempMatrices = nullptr;
		if ((UpdateState & PSkeletalMeshComponent::PSkeletalMeshUpdateState::SKELETAL_MESH_UPDATE_STATE_MATRIX) != 0u)
		{
			TempMatrices = new ERenderPrimitiveMatrices(
				InComponent->GetComponentWorldLocation(),
				InComponent->GetComponentWorldRotation(),
				InComponent->GetComponentWorldScale());
		}
		const ESkinnedMeshAsset* TempMeshAsset = nullptr;
		if ((UpdateState & PSkeletalMeshComponent::PSkeletalMeshUpdateState::SKELETAL_MESH_UPDATE_STATE_MESHASSET) != 0u)
		{
			TempMeshAsset = InComponent->GetMeshAsset();
		}
		const ESkeletonAsset* TempSkeletonAsset = nullptr;
		if ((UpdateState & PSkeletalMeshComponent::PSkeletalMeshUpdateState::SKELETAL_MESH_UPDATE_STATE_SKELETONASSET) != 0u)
		{
			TempSkeletonAsset = InComponent->GetSkeletonAsset();
		}
		BOOL32 TempUpdateBoneData = (UpdateState & PSkeletalMeshComponent::PSkeletalMeshUpdateState::SKELETAL_MESH_UPDATE_STATE_BONE_DATA) != 0u;
		const ESkeletonBoneMemoryPool& TempBoneMemoryPool = InComponent->GetBoneMemoryPool();

		RenderUpdateCommands.EnqueueCommand(
			[Scene, SceneProxy, InIsRenderHidden, InIsCastShadow, InIsReceiveShadow,
			TempMatrices, TempMeshAsset, TempSkeletonAsset,
			TempUpdateBoneData, TempBoneToRootTransforms = TempBoneMemoryPool.GetBoneToRootTransforms()
			]()->void
			{
				BOOL32 NeedUpdateRenderResource = FALSE;
				SceneProxy->SetPrimitiveSettings(InIsRenderHidden, TRUE, InIsCastShadow, InIsReceiveShadow);
				if (TempMatrices)
				{
					SceneProxy->UpdatePrimitiveMatrices(*TempMatrices);
					delete TempMatrices;
					NeedUpdateRenderResource = TRUE;
				}
				if (TempMeshAsset)
				{
					SceneProxy->UpdateMeshAsset(TempMeshAsset);
					NeedUpdateRenderResource = TRUE;
				}
				if (TempSkeletonAsset)
				{
					SceneProxy->UpdateSkeletonAsset(TempSkeletonAsset);
				}
				if (TempUpdateBoneData)
				{
					NeedUpdateRenderResource = TRUE;
				}
				if (NeedUpdateRenderResource)
				{
					SceneProxy->UpdateRenderResource();
					{
						TArray<Matrix4x4> TempBoneToRootMatrices;
						if (const UINT32 MatrixNum = TempBoneToRootTransforms.Length(); MatrixNum > 0u)
						{
							TempBoneToRootMatrices.Resize(MatrixNum);
							for (UINT32 MatrixIndex = 0u; MatrixIndex < MatrixNum; MatrixIndex++)
							{
#if (_USE_MATRIX_FOR_BONE_TO_ROOT)
								TempBoneToRootMatrices[MatrixIndex] = TempBoneToRootTransforms[MatrixIndex];
#else
								TempBoneToRootMatrices[MatrixIndex] = TempBoneToRootTransforms[MatrixIndex].ToMatrix4x4();
#endif
							}
						}
						SceneProxy->UpdateSkeletonRenderResource(TempBoneToRootMatrices);
					}
				}
			});
	}
	RCommand& RScene::GetAddCommands()
	{
		return RenderAddCommands;
	}
	const RCommand& RScene::GetAddCommands()const
	{
		return RenderAddCommands;
	}
	RCommand& RScene::GetRemoveCommands()
	{
		return RenderRemoveCommands;
	}
	const RCommand& RScene::GetRemoveCommands()const
	{
		return RenderRemoveCommands;
	}
	RCommand& RScene::GetUpdateCommands()
	{
		return RenderUpdateCommands;
	}
	const RCommand& RScene::GetUpdateCommands()const
	{
		return RenderUpdateCommands;
	}
	ROctree& RScene::GetSceneOctree()
	{
		return RenderSceneOctree;
	}
	const ROctree& RScene::GetSceneOctree()const
	{
		return RenderSceneOctree;
	}
	RSceneProxyMapping<RViewProxy>& RScene::GetViewProxies()
	{
		return ViewProxies;
	}
	const RSceneProxyMapping<RViewProxy>& RScene::GetViewProxies()const
	{
		return ViewProxies;
	}
	RSceneProxyMapping<RSkyLightSceneProxy>& RScene::GetSkyLightProxies()
	{
		return SkyLightProxies;
	}
	const RSceneProxyMapping<RSkyLightSceneProxy>& RScene::GetSkyLightProxies()const
	{
		return SkyLightProxies;
	}
	RSceneProxyMapping<RDirectionalLightSceneProxy>& RScene::GetDirectionalLightSceneProxies()
	{
		return DirectionalLightSceneProxies;
	}
	const RSceneProxyMapping<RDirectionalLightSceneProxy>& RScene::GetDirectionalLightSceneProxies()const
	{
		return DirectionalLightSceneProxies;
	}
	RSceneProxyMapping<RStaticMeshSceneProxy>& RScene::GetStaticMeshSceneProxies()
	{
		return StaticMeshSceneProxies;
	}
	const RSceneProxyMapping<RStaticMeshSceneProxy>& RScene::GetStaticMeshSceneProxies()const
	{
		return StaticMeshSceneProxies;
	}
	RSceneProxyMapping<RSkeletalMeshSceneProxy>& RScene::GetSkeletalMeshSceneProxies()
	{
		return SkeletalMeshSceneProxies;
	}
	const RSceneProxyMapping<RSkeletalMeshSceneProxy>& RScene::GetSkeletalMeshSceneProxies()const
	{
		return SkeletalMeshSceneProxies;
	}
	void RScene::AddOrRemoveCamera_RenderThread(RViewProxy* InSceneProxy, BOOL32 InIsAdd)
	{
		if (InIsAdd)
		{
			ViewProxies.AddSceneProxy(InSceneProxy);
		}
		else
		{
			ViewProxies.RemoveSceneProxy(InSceneProxy);
		}
	}
	void RScene::AddOrRemoveSkyLight_RenderThread(RSkyLightSceneProxy* InSceneProxy, BOOL32 InIsAdd)
	{
		if (InIsAdd)
		{
			SkyLightProxies.AddSceneProxy(InSceneProxy);
		}
		else
		{
			SkyLightProxies.RemoveSceneProxy(InSceneProxy);
		}
	}
	void RScene::AddOrRemoveDirectionalLight_RenderThread(RDirectionalLightSceneProxy* InSceneProxy, BOOL32 InIsAdd)
	{
		if (InIsAdd)
		{
			DirectionalLightSceneProxies.AddSceneProxy(InSceneProxy);
		}
		else
		{
			DirectionalLightSceneProxies.RemoveSceneProxy(InSceneProxy);
		}
	}
	void RScene::AddOrRemoveStaticMesh_RenderThread(RStaticMeshSceneProxy* InSceneProxy, BOOL32 InIsAdd)
	{
		if (InIsAdd)
		{
			StaticMeshSceneProxies.AddSceneProxy(InSceneProxy);
		}
		else
		{
			StaticMeshSceneProxies.RemoveSceneProxy(InSceneProxy);
		}
	}
	void RScene::AddOrRemoveSkeletalMesh_RenderThread(RSkeletalMeshSceneProxy* InSceneProxy, BOOL32 InIsAdd)
	{
		if (InIsAdd)
		{
			SkeletalMeshSceneProxies.AddSceneProxy(InSceneProxy);
		}
		else
		{
			SkeletalMeshSceneProxies.RemoveSceneProxy(InSceneProxy);
		}
	}

};