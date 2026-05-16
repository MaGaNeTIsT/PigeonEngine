#include "RenderScene.h"
#include <MeshAsset/MeshAsset.h>
#include <TextureAsset/TextureAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <MaterialAsset/MaterialAsset.h>
#include <RenderProxy/ViewProxy.h>
#include <RenderProxy/SkyLightProxy.h>
#include <RenderProxy/LightSceneProxy.h>
#include <RenderProxy/PrimitiveSceneProxy.h>
#include <RenderProxy/MeshSceneProxy.h>
#include <RenderProxy/StaticMeshSceneProxy.h>
#include <RenderProxy/SkeletalMeshSceneProxy.h>
#include <RenderProxy/BezierGrassSceneProxy.h>
#include <RenderProxy/FluidWaterSceneProxy.h>
#include <PigeonBase/Object/Component/CameraAndLight/CameraComponent.h>
#include <PigeonBase/Object/Component/Primitive/SkyLightComponent.h>
#include <PigeonBase/Object/Component/CameraAndLight/DirectionalLightComponent.h>
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>
#include <PigeonBase/Object/Component/Primitive/SkeletalMeshComponent.h>
#include <PigeonBase/Object/Component/Primitive/BezierGrassComponent.h>
#include <PigeonBase/Object/Component/Primitive/FluidWaterComponent.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RScene, EObjectBase, RSceneInterface>();
		RegisterClassType<RSceneNull, EObjectBase, RSceneInterface>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RScene::RScene()
		: BackSlotIndex(0u), FrontSlotIndex(1u)
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

		// Drain both slots so any pending lambda is replayed and freed.
		for (UINT32 SlotIndex = 0u; SlotIndex < 2u; SlotIndex++)
		{
			RenderAddCommands[SlotIndex].DoCommands();
			RenderAddCommands[SlotIndex].EmptyQueue();
			RenderUpdateCommands[SlotIndex].DoCommands();
			RenderUpdateCommands[SlotIndex].EmptyQueue();
			RenderRemoveCommands[SlotIndex].DoCommands();
			RenderRemoveCommands[SlotIndex].EmptyQueue();
		}

		UnbindErrorCheck();
	}
	void RScene::UnbindErrorCheck()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all views' mapping failed"), (ViewProxies.SceneProxyMapping.Num() == 0));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all views failed"), (ViewProxies.SceneProxies.Num() == 0));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all sky lights' failed"), (SkyLightProxies.SceneProxyMapping.Num() == 0));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all sky lights failed"), (SkyLightProxies.SceneProxies.Num() == 0));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all directional lights' mapping failed"), (DirectionalLightSceneProxies.SceneProxyMapping.Num() == 0));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all directional lights failed"), (DirectionalLightSceneProxies.SceneProxies.Num() == 0));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all static meshes' mapping failed"), (StaticMeshSceneProxies.SceneProxyMapping.Num() == 0));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all static meshes failed"), (StaticMeshSceneProxies.SceneProxies.Num() == 0));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all skeletal meshes' mapping failed"), (SkeletalMeshSceneProxies.SceneProxyMapping.Num() == 0));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all skeletal meshes failed"), (SkeletalMeshSceneProxies.SceneProxies.Num() == 0));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all bezier grasses' mapping failed"), (BezierGrassSceneProxies.SceneProxyMapping.Num() == 0));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all bezier grasses failed"), (BezierGrassSceneProxies.SceneProxies.Num() == 0));
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

		EnqueueAddCommand(
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
		EnqueueRemoveCommand(
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

		EnqueueUpdateCommand(
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

		EnqueueAddCommand(
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
		EnqueueRemoveCommand(
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

		EnqueueUpdateCommand(
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

		EnqueueAddCommand(
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
		EnqueueRemoveCommand(
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

		EnqueueUpdateCommand(
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

		const BOOL32 bMovable = InComponent->GetMobility() != EMobilityType::EMT_STATIC;
		const BOOL32 bCastShadow = InComponent->IsCastShadow();
		const BOOL32 bReceiveShadow = InComponent->IsReceiveShadow();
		ERenderPrimitiveMatrices* TempMatrices = new ERenderPrimitiveMatrices(
			InComponent->GetComponentWorldLocation(),
			InComponent->GetComponentWorldRotation(),
			InComponent->GetComponentWorldScale());
		const EStaticMeshAsset* TempMeshAsset = InComponent->GetMeshAsset();
		const EMaterialAsset* TempMatAsset = InComponent->GetMaterialAsset(0u);

		EnqueueAddCommand(
			[Scene, SceneProxy, bMovable, bCastShadow, bReceiveShadow, TempMatrices, TempMeshAsset, TempMatAsset]()->void
			{
				SceneProxy->SetupProxy(bMovable, bCastShadow, bReceiveShadow, *TempMatrices, TempMeshAsset, TempMatAsset);
				delete TempMatrices;
				Scene->AddOrRemoveStaticMesh_RenderThread(SceneProxy, TRUE);
			});
	}
	void RScene::RemoveStaticMesh(PStaticMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RStaticMeshSceneProxy* SceneProxy = InComponent->SceneProxy;
		InComponent->SceneProxy = nullptr;
		EnqueueRemoveCommand(
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

		const BOOL32 bMovable = InComponent->GetMobility() != EMobilityType::EMT_STATIC;
		const BOOL32 bCastShadow = InComponent->IsCastShadow();
		const BOOL32 bReceiveShadow = InComponent->IsReceiveShadow();
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
		const BOOL32 bMatAssetUpdated = (UpdateState & PStaticMeshComponent::PStaticMeshUpdateState::STATIC_MESH_UPDATE_STATE_MATERIAL) != 0u;
		const EMaterialAsset* TempMatAsset = bMatAssetUpdated ? InComponent->GetMaterialAsset(0u) : nullptr;

		TArray<TArray<BYTE>>* TempCBDatas = nullptr;
		TArray<RMaterialTextureSRV>* TempTexs = nullptr;
		if (InComponent->IsMaterialDirty())
		{
			TempCBDatas = new TArray<TArray<BYTE>>();
			InComponent->GetMaterialConstantBufferDataBySlot(0u, *TempCBDatas);

			TempTexs = new TArray<RMaterialTextureSRV>();
			InComponent->GetMaterialTextureBySlot(0u, *TempTexs);
		}

		EnqueueUpdateCommand(
			[Scene
			, SceneProxy
			, bMovable
			, bCastShadow
			, bReceiveShadow
			, TempMatrices
			, TempMeshAsset
			, bMatAssetUpdated
			, TempMatAsset
			, TempCBDatas
			, TempTexs
			]()->void
			{
				BOOL32 NeedUpdateRenderResource = FALSE;
				SceneProxy->SetPrimitiveSettings(bMovable, bCastShadow, bReceiveShadow);
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
				if (bMatAssetUpdated)
				{
					SceneProxy->UpdateMaterialAsset(TempMatAsset);
				}
				if (TempCBDatas)
				{
					SceneProxy->UpdateMaterialCBData(*TempCBDatas);
					delete TempCBDatas;
				}
				if(TempTexs)
				{
					SceneProxy->UpdateMaterialTextures(*TempTexs);
					delete TempTexs;
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
		const BOOL32 bCastShadow = InComponent->IsCastShadow();
		const BOOL32 bReceiveShadow = InComponent->IsReceiveShadow();
		ERenderPrimitiveMatrices* TempMatrices = new ERenderPrimitiveMatrices(
			InComponent->GetComponentWorldLocation(),
			InComponent->GetComponentWorldRotation(),
			InComponent->GetComponentWorldScale());
		const ESkinnedMeshAsset* TempMeshAsset = InComponent->GetMeshAsset();
		const ESkeletonAsset* TempSkeletonAsset = InComponent->GetSkeletonAsset();
		const ESkeletonBoneMemoryPool& TempBoneMemoryPool = InComponent->GetBoneMemoryPool();

		EnqueueAddCommand(
			[Scene, SceneProxy, bCastShadow, bReceiveShadow,
			TempMatrices, TempMeshAsset, TempSkeletonAsset,
			TempBoneToRootTransforms = TempBoneMemoryPool.GetBoneToRootTransforms()
			]()->void
			{
				TArray<Matrix4x4> TempBoneToRootMatrices;
				if (const UINT32 MatrixNum = TempBoneToRootTransforms.Num<UINT32>(); MatrixNum > 0u)
				{
					TempBoneToRootMatrices.SetNum(MatrixNum);
					for (UINT32 MatrixIndex = 0u; MatrixIndex < MatrixNum; MatrixIndex++)
					{
#if (_USE_MATRIX_FOR_BONE_TO_ROOT)
						TempBoneToRootMatrices[MatrixIndex] = TempBoneToRootTransforms[MatrixIndex];
#else
						TempBoneToRootMatrices[MatrixIndex] = TempBoneToRootTransforms[MatrixIndex].ToMatrix4x4();
#endif
					}
				}
				SceneProxy->SetupProxy(TRUE, bCastShadow, bReceiveShadow, *TempMatrices, TempMeshAsset, TempSkeletonAsset, TempBoneToRootMatrices);
				delete TempMatrices;
				Scene->AddOrRemoveSkeletalMesh_RenderThread(SceneProxy, TRUE);
			});
	}
	void RScene::RemoveSkeletalMesh(PSkeletalMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RSkeletalMeshSceneProxy* SceneProxy = InComponent->SceneProxy;
		InComponent->SceneProxy = nullptr;
		EnqueueRemoveCommand(
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
		const BOOL32 bCastShadow = InComponent->IsCastShadow();
		const BOOL32 bReceiveShadow = InComponent->IsReceiveShadow();
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

		EnqueueUpdateCommand(
			[Scene, SceneProxy, bCastShadow, bReceiveShadow,
			TempMatrices, TempMeshAsset, TempSkeletonAsset,
			TempUpdateBoneData, TempBoneToRootTransforms = TempBoneMemoryPool.GetBoneToRootTransforms()
			]()->void
			{
				BOOL32 NeedUpdateRenderResource = FALSE;
				SceneProxy->SetPrimitiveSettings(TRUE, bCastShadow, bReceiveShadow);
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
						if (const UINT32 MatrixNum = TempBoneToRootTransforms.Num<UINT32>(); MatrixNum > 0u)
						{
							TempBoneToRootMatrices.SetNum(MatrixNum);
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
	void RScene::AddBezierGrass(PBezierGrassComponent* InComponent)
	{
		RScene* Scene = this;
		RBezierGrassSceneProxy* SceneProxy = InComponent->CreateSceneProxy();

		const BOOL32 bMovable = InComponent->GetMobility() != EMobilityType::EMT_STATIC;
		const BOOL32 bCastShadow = InComponent->IsCastShadow();
		const BOOL32 bReceiveShadow = InComponent->IsReceiveShadow();
		ERenderPrimitiveMatrices* TempMatrices = new ERenderPrimitiveMatrices(
			InComponent->GetComponentWorldLocation(),
			InComponent->GetComponentWorldRotation(),
			InComponent->GetComponentWorldScale());
		EBezierGrassProperty* TempProperty = new EBezierGrassProperty(InComponent->Property);

		EnqueueAddCommand(
			[Scene, SceneProxy, bMovable, bCastShadow, bReceiveShadow, TempMatrices, TempProperty]()->void
			{
				SceneProxy->SetupProxy(bMovable, bCastShadow, bReceiveShadow, *TempMatrices);
				delete TempMatrices;
				if (TempProperty)
				{
					SceneProxy->UpdateProperty(*TempProperty);
					delete TempProperty;
				}
				Scene->AddOrRemoveBezierGrass_RenderThread(SceneProxy, TRUE);
			});
	}
	void RScene::RemoveBezierGrass(PBezierGrassComponent* InComponent)
	{
		RScene* Scene = this;
		RBezierGrassSceneProxy* SceneProxy = InComponent->SceneProxy;
		InComponent->SceneProxy = nullptr;
		EnqueueRemoveCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->AddOrRemoveBezierGrass_RenderThread(SceneProxy, FALSE);
				delete SceneProxy;
			});
	}
	void RScene::UpdateBezierGrass(PBezierGrassComponent* InComponent)
	{
		RScene* Scene = this;
		RBezierGrassSceneProxy* SceneProxy = InComponent->SceneProxy;

		UINT8 UpdateState = InComponent->GetUpdateRenderState();

		const BOOL32 bMovable = InComponent->GetMobility() != EMobilityType::EMT_STATIC;
		const BOOL32 bCastShadow = InComponent->IsCastShadow();
		const BOOL32 bReceiveShadow = InComponent->IsReceiveShadow();
		ERenderPrimitiveMatrices* TempMatrices = nullptr;
		if ((UpdateState & PBezierGrassComponent::PBezierGrassUpdateState::BEZIER_GRASS_UPDATE_STATE_MATRIX) != 0u)
		{
			TempMatrices = new ERenderPrimitiveMatrices(
				InComponent->GetComponentWorldLocation(),
				InComponent->GetComponentWorldRotation(),
				InComponent->GetComponentWorldScale());
		}
		EBezierGrassProperty* TempProperty = nullptr;
		EBezierGrassLayerTypeData* TempLayerData = nullptr;
		Vector2* TempTileAnchor = nullptr;
		Vector2* TempTileSize = nullptr;
		UINT32 TempNumTilesX = 0;
		UINT32 TempNumTilesZ = 0;
		Vector3* TempWindDirection = nullptr;
		FLOAT TempWindStrength = 0.f;
		if ((UpdateState & PBezierGrassComponent::PBezierGrassUpdateState::BEZIER_GRASS_UPDATE_STATE_ASSET) != 0u)
		{
			TempProperty = new EBezierGrassProperty(InComponent->Property);
			TempLayerData = new EBezierGrassLayerTypeData(InComponent->LayerTypeData);
#if 0
			TempTileAnchor = new Vector2(InComponent->TileAnchor);
			TempTileSize = new Vector2(InComponent->TileSize);
			TempNumTilesX = InComponent->NumTilesX;
			TempNumTilesZ = InComponent->NumTilesZ;
			TempWindDirection = new Vector3(InComponent->WindDirection);
			TempWindStrength = InComponent->WindStrength;
#endif
		}

		EnqueueUpdateCommand(
			[Scene, SceneProxy, bMovable, bCastShadow, bReceiveShadow, TempMatrices, TempProperty, TempLayerData, TempTileAnchor, TempTileSize, TempNumTilesX, TempNumTilesZ, TempWindDirection, TempWindStrength]()->void
			{
				BOOL32 NeedUpdateRenderResource = FALSE;
				SceneProxy->SetPrimitiveSettings(bMovable, bCastShadow, bReceiveShadow);
				if (TempMatrices)
				{
					SceneProxy->UpdatePrimitiveMatrices(*TempMatrices);
					delete TempMatrices;
					NeedUpdateRenderResource = TRUE;
				}
				if (TempProperty)
				{
					SceneProxy->UpdateProperty(*TempProperty);
					delete TempProperty;
					NeedUpdateRenderResource = TRUE;
				}
				if (TempLayerData)
				{
					SceneProxy->UpdateLayerTypeData(*TempLayerData);
					delete TempLayerData;
				}
				if (TempTileAnchor && TempTileSize)
				{
					SceneProxy->UpdateTileParams(*TempTileAnchor, *TempTileSize, TempNumTilesX, TempNumTilesZ);
					delete TempTileAnchor;
					delete TempTileSize;
				}
				if (TempWindDirection)
				{
					SceneProxy->UpdateWindParams(*TempWindDirection, TempWindStrength);
					delete TempWindDirection;
				}
				if (NeedUpdateRenderResource)
				{
					SceneProxy->UpdateRenderResource();
				}
			});
	}
	RCommand& RScene::GetAddCommandsForRender()
	{
		return RenderAddCommands[FrontSlotIndex];
	}
	const RCommand& RScene::GetAddCommandsForRender()const
	{
		return RenderAddCommands[FrontSlotIndex];
	}
	RCommand& RScene::GetRemoveCommandsForRender()
	{
		return RenderRemoveCommands[FrontSlotIndex];
	}
	const RCommand& RScene::GetRemoveCommandsForRender()const
	{
		return RenderRemoveCommands[FrontSlotIndex];
	}
	RCommand& RScene::GetUpdateCommandsForRender()
	{
		return RenderUpdateCommands[FrontSlotIndex];
	}
	const RCommand& RScene::GetUpdateCommandsForRender()const
	{
		return RenderUpdateCommands[FrontSlotIndex];
	}
	void RScene::SwapCommandSlots()
	{
		const UINT32 NewBack = FrontSlotIndex;
		FrontSlotIndex = BackSlotIndex;
		BackSlotIndex = NewBack;
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
	RSceneProxyMapping<RBezierGrassSceneProxy>& RScene::GetBezierGrassSceneProxies()
	{
		return BezierGrassSceneProxies;
	}
	const RSceneProxyMapping<RBezierGrassSceneProxy>& RScene::GetBezierGrassSceneProxies()const
	{
		return BezierGrassSceneProxies;
	}
	RSceneProxyMapping<RFluidWaterSceneProxy>& RScene::GetFluidWaterSceneProxies()
	{
		return FluidWaterSceneProxies;
	}
	const RSceneProxyMapping<RFluidWaterSceneProxy>& RScene::GetFluidWaterSceneProxies()const
	{
		return FluidWaterSceneProxies;
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
	void RScene::AddOrRemoveBezierGrass_RenderThread(RBezierGrassSceneProxy* InSceneProxy, BOOL32 InIsAdd)
	{
		if (InIsAdd)
		{
			BezierGrassSceneProxies.AddSceneProxy(InSceneProxy);
		}
		else
		{
			BezierGrassSceneProxies.RemoveSceneProxy(InSceneProxy);
		}
	}

	void RScene::AddFluidWater(PFluidWaterComponent* InComponent)
	{
		RScene* Scene = this;
		RFluidWaterSceneProxy* SceneProxy = InComponent->CreateSceneProxy();

		const BOOL32 bMovable = InComponent->GetMobility() != EMobilityType::EMT_STATIC;
		const BOOL32 bCastShadow = InComponent->IsCastShadow();
		const BOOL32 bReceiveShadow = InComponent->IsReceiveShadow();
		ERenderPrimitiveMatrices* TempMatrices = new ERenderPrimitiveMatrices(
			InComponent->GetComponentWorldLocation(),
			InComponent->GetComponentWorldRotation(),
			InComponent->GetComponentWorldScale());
		const EMaterialAsset* TempMatAsset = InComponent->GetMaterialAsset(0u);

		EnqueueAddCommand(
			[Scene, SceneProxy, bMovable, bCastShadow, bReceiveShadow, TempMatrices, TempMatAsset]()->void
			{
				SceneProxy->SetupProxy(bMovable, bCastShadow, bReceiveShadow, *TempMatrices, TempMatAsset);
				delete TempMatrices;
				Scene->AddOrRemoveFluidWater_RenderThread(SceneProxy, TRUE);
			});
	}

	void RScene::RemoveFluidWater(PFluidWaterComponent* InComponent)
	{
		RScene* Scene = this;
		RFluidWaterSceneProxy* SceneProxy = InComponent->SceneProxy;
		InComponent->SceneProxy = nullptr;
		EnqueueRemoveCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->AddOrRemoveFluidWater_RenderThread(SceneProxy, FALSE);
				delete SceneProxy;
			});
	}

	void RScene::UpdateFluidWater(PFluidWaterComponent* InComponent)
	{

	}

	void RScene::AddOrRemoveFluidWater_RenderThread(RFluidWaterSceneProxy* InSceneProxy, BOOL32 InIsAdd)
	{
		if (InIsAdd)
		{
			FluidWaterSceneProxies.AddSceneProxy(InSceneProxy);
		}
		else
		{
			FluidWaterSceneProxies.RemoveSceneProxy(InSceneProxy);
		}
	}

};