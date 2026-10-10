#include "RenderScene.h"
#include <MeshAsset/MeshAsset.h>
#include <TextureAsset/TextureAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <ShaderAsset/ShaderInstance.h>
#include <RenderProxy/ViewProxy.h>
#include <RenderProxy/SkyLightProxy.h>
#include <RenderProxy/LightSceneProxy.h>
#include <RenderProxy/PrimitiveSceneProxy.h>
#include <RenderProxy/PrimitiveSceneProxyFactory.h>
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
#if _EDITOR_ONLY
#include <Editor/EditorLogManager.h>
#endif


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

		// The drained commands deleted every proxy whose remove was still queued. What is left
		// here belongs to a primitive that was never removed, so the mapping is dropped with it.
		PrimitiveProxyMapping.Empty();
		Primitives.Empty();

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
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all primitives' mapping failed"), (PrimitiveProxyMapping.Num() == 0));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all primitives failed"), (Primitives.Num() == 0));
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
			InComponent->GetIlluminanceLux(),
			InComponent->GetSourceAngleDegrees(),
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
				InComponent->GetIlluminanceLux(),
				InComponent->GetSourceAngleDegrees(),
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
		// Forwarded onto the generic path: the component reports what it is made of, the proxy
		// class is looked up by the name it states, and the proxy itself is built on the render
		// thread. There is nothing static-mesh specific left to do here.
		AddPrimitive(InComponent);
	}
	void RScene::RemoveStaticMesh(PStaticMeshComponent* InComponent)
	{
		// Forwarded onto the generic path: the proxy is found by the component's identity, since
		// the component itself holds no proxy pointer any more.
		RemovePrimitive(InComponent);
	}
	void RScene::UpdateStaticMesh(PStaticMeshComponent* InComponent)
	{
		// Forwarded onto the generic path: the component states which of its channels moved
		// while it builds the snapshot, and the render side rebuilds only what changed.
		UpdatePrimitive(InComponent);
	}
	// ---------------------------------------------------------------------------
	// The generic primitive path
	// ---------------------------------------------------------------------------

	void RScene::AddPrimitive(PPrimitiveComponent* InComponent)
	{
		if (!InComponent)
		{
			return;
		}
		RScene* Scene = this;
		// What crosses to the render thread is a value snapshot. The proxy is built from it on
		// the render thread, so the game thread never hands one of its own objects over.
		FPrimitiveRenderSnapshot* Snapshot = new FPrimitiveRenderSnapshot();
		if (!(InComponent->BuildRenderSnapshot(*Snapshot)))
		{
			delete Snapshot;
			return;
		}
		// The create snapshot carries every channel, so nothing is left pending afterwards.
		InComponent->ClearRenderChannels(PRIMITIVE_UPDATE_TRANSFORM | PRIMITIVE_UPDATE_VALUES | PRIMITIVE_UPDATE_ASSETS);

		EnqueueAddCommand(
			[Scene, Snapshot]()->void
			{
				RPrimitiveSceneProxy* SceneProxy = RPrimitiveSceneProxyFactory::Create(Snapshot->ProxyTypeName);
				if (SceneProxy)
				{
					SceneProxy->SetupProxy(*Snapshot);
					Scene->AddPrimitive_RenderThread(SceneProxy);
				}
				delete Snapshot;
			});
	}
	void RScene::UpdatePrimitive(PPrimitiveComponent* InComponent)
	{
		if (!InComponent)
		{
			return;
		}
		const ObjectIdentityType Identity = InComponent->GetUniqueID();
		// Only the channels that moved are carried, and only those are cleared once the command
		// is enqueued: a change made while the snapshot was being built keeps its channel.
		FPrimitiveRenderSnapshot* Snapshot = new FPrimitiveRenderSnapshot();
		if (!(InComponent->BuildRenderSnapshot(*Snapshot)))
		{
			delete Snapshot;
			return;
		}
		InComponent->ClearRenderChannels(Snapshot->DirtyChannels);

		EnqueueUpdateCommand(
			[this, Identity, Snapshot]()->void
			{
				RPrimitiveSceneProxy* SceneProxy = FindPrimitiveProxy(Identity);
				if (SceneProxy)
				{
					SceneProxy->SetPrimitiveSettings(Snapshot->bMovable, Snapshot->bCastShadow, Snapshot->bReceiveShadow);
					if ((Snapshot->DirtyChannels & PRIMITIVE_UPDATE_TRANSFORM) != 0u)
					{
						SceneProxy->UpdatePrimitiveMatrices(ERenderPrimitiveMatrices(
							Snapshot->WorldLocation, Snapshot->WorldRotation, Snapshot->WorldScaling));
					}
					if ((Snapshot->DirtyChannels & PRIMITIVE_UPDATE_ASSETS) != 0u)
					{
						SceneProxy->ApplyAssets(Snapshot->Assets);
					}
					if ((Snapshot->DirtyChannels & (PRIMITIVE_UPDATE_TRANSFORM | PRIMITIVE_UPDATE_VALUES)) != 0u)
					{
						SceneProxy->ApplyValues(Snapshot->Values);
						SceneProxy->ApplyRuntimePayload(*Snapshot);
					}
				}
				delete Snapshot;
			});
	}
	void RScene::RemovePrimitive(PPrimitiveComponent* InComponent)
	{
		if (!InComponent)
		{
			return;
		}
		RScene* Scene = this;
		const ObjectIdentityType Identity = InComponent->GetUniqueID();
		EnqueueRemoveCommand(
			[Scene, Identity]()->void
			{
				RPrimitiveSceneProxy* SceneProxy = Scene->FindPrimitiveProxy(Identity);
				if (SceneProxy)
				{
					Scene->RemovePrimitive_RenderThread(SceneProxy);
					delete SceneProxy;
				}
			});
	}
	RPrimitiveSceneProxy* RScene::FindPrimitiveProxy(const ObjectIdentityType& InIdentity)const
	{
		RPrimitiveSceneProxy* const* Found = PrimitiveProxyMapping.FindValueAsPtr(InIdentity);
		return (Found ? (*Found) : nullptr);
	}
	const TArray<RPrimitiveSceneProxy*>& RScene::GetPrimitives()const
	{
		return Primitives;
	}
	void RScene::AddPrimitive_RenderThread(RPrimitiveSceneProxy* InSceneProxy)
	{
		if (!InSceneProxy)
		{
			return;
		}
		const ObjectIdentityType& Identity = InSceneProxy->GetPrimitiveIdentity();
		if (PrimitiveProxyMapping.FindValueAsPtr(Identity) != nullptr)
		{
			// Two proxies under one identity would make every later lookup ambiguous, so the
			// second is refused rather than silently replacing the first.
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("A primitive scene proxy is already registered under that identity."));
			return;
		}
		PrimitiveProxyMapping.Add(Identity, InSceneProxy);
		Primitives.Add(InSceneProxy);
	}
	void RScene::RemovePrimitive_RenderThread(RPrimitiveSceneProxy* InSceneProxy)
	{
		if (!InSceneProxy)
		{
			return;
		}
		PrimitiveProxyMapping.Remove(InSceneProxy->GetPrimitiveIdentity());
		for (UINT32 Index = 0u, Num = Primitives.Num<UINT32>(); Index < Num; Index++)
		{
			if (Primitives[Index] == InSceneProxy)
			{
				// Swap remove: the list is walked whole every frame and nothing outside it holds
				// an index into it.
				Primitives[Index] = Primitives[Num - 1u];
				Primitives.Pop();
				break;
			}
		}
	}
	void RScene::AddSkeletalMesh(PSkeletalMeshComponent* InComponent)
	{
		Check((InComponent->GetMobility() == EMobilityType::EMT_DYNAMIC));
		// Forwarded onto the generic path: the snapshot the component builds carries the mesh,
		// the skeleton and this frame's bone matrices, and the proxy is built on the render
		// thread from it.
		AddPrimitive(InComponent);
	}
	void RScene::RemoveSkeletalMesh(PSkeletalMeshComponent* InComponent)
	{
		RemovePrimitive(InComponent);
	}
	void RScene::UpdateSkeletalMesh(PSkeletalMeshComponent* InComponent)
	{
		Check((InComponent->GetMobility() == EMobilityType::EMT_DYNAMIC));
		// The bone matrices travel as snapshot payload, which is what makes a skinned primitive
		// the one kind whose update has to reach past the assets and the values.
		UpdatePrimitive(InComponent);
	}
	void RScene::AddBezierGrass(PBezierGrassComponent* InComponent)
	{
		// Parked with the proxy: the grass render path is being rebuilt from its shaders.
		(void)InComponent;
#if 0
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
#endif
	}
	void RScene::RemoveBezierGrass(PBezierGrassComponent* InComponent)
	{
		// Parked with the proxy: the grass render path is being rebuilt from its shaders.
		(void)InComponent;
#if 0
		RScene* Scene = this;
		RBezierGrassSceneProxy* SceneProxy = InComponent->SceneProxy;
		InComponent->SceneProxy = nullptr;
		EnqueueRemoveCommand(
			[Scene, SceneProxy]()->void
			{
					Scene->AddOrRemoveBezierGrass_RenderThread(SceneProxy, FALSE);
					delete SceneProxy;
				});
#endif
	}
	void RScene::UpdateBezierGrass(PBezierGrassComponent* InComponent)
	{
		// Parked with the proxy: the grass render path is being rebuilt from its shaders.
		(void)InComponent;
#if 0
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
#endif
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
	RSceneProxyMapping<RLocalLightSceneProxy>& RScene::GetLocalLightSceneProxies()
	{
		return LocalLightSceneProxies;
	}
	const RSceneProxyMapping<RLocalLightSceneProxy>& RScene::GetLocalLightSceneProxies() const
	{
		return LocalLightSceneProxies;
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
	void RScene::AddLocalLight(PLocalLightComponent* InComponent)
	{
		RScene* Scene = this;
		RLocalLightSceneProxy* SceneProxy = InComponent->CreateSceneProxy();

		// The marshal is a value copy of everything the render side needs, taken here on the
		// game thread: the proxy is built on the render thread and must not read the component,
		// which the game thread is free to change in the meantime.
		ERenderTransformInfo* TempTransform = new ERenderTransformInfo(
			InComponent->GetComponentWorldLocation(),
			InComponent->GetComponentWorldRotation(),
			InComponent->GetComponentWorldScale());
		ELightData* TempData = new ELightData(InComponent->GetLightData());

		EnqueueAddCommand(
			[Scene, SceneProxy, TempTransform, TempData]()->void
			{
				SceneProxy->SetupProxy(*TempTransform, *TempData);
				delete TempTransform;
				delete TempData;
				Scene->AddOrRemoveLocalLight_RenderThread(SceneProxy, TRUE);
			});
	}
	void RScene::RemoveLocalLight(PLocalLightComponent* InComponent)
	{
		RScene* Scene = this;
		RLocalLightSceneProxy* SceneProxy = InComponent->SceneProxy;
		InComponent->SceneProxy = nullptr;
		EnqueueRemoveCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->AddOrRemoveLocalLight_RenderThread(SceneProxy, FALSE);
				delete SceneProxy;
			});
	}
	void RScene::UpdateLocalLight(PLocalLightComponent* InComponent)
	{
		RScene* Scene = this;
		RLocalLightSceneProxy* SceneProxy = InComponent->SceneProxy;

		UINT8 UpdateState = InComponent->GetUpdateRenderState();

		ERenderTransformInfo* TempTransform = nullptr;
		if ((UpdateState & PLightComponent::PLightUpdateState::LIGHT_UPDATE_STATE_MATRIX) != 0u)
		{
			TempTransform = new ERenderTransformInfo(
				InComponent->GetComponentWorldLocation(),
				InComponent->GetComponentWorldRotation(),
				InComponent->GetComponentWorldScale());
		}
		ELightData* TempData = nullptr;
		if ((UpdateState & PLightComponent::PLightUpdateState::LIGHT_UPDATE_STATE_DATA) != 0u)
		{
			TempData = new ELightData(InComponent->GetLightData());
		}

		EnqueueUpdateCommand(
			[Scene, SceneProxy, TempTransform, TempData]()->void
			{
				if (TempTransform)
				{
					SceneProxy->UpdateTransform(*TempTransform);
					delete TempTransform;
				}
				if (TempData)
				{
					SceneProxy->UpdateLightParams(*TempData);
					delete TempData;
				}
			});
	}
	void RScene::AddOrRemoveLocalLight_RenderThread(RLocalLightSceneProxy* InSceneProxy, BOOL32 InIsAdd)
	{
		if (InIsAdd)
		{
			LocalLightSceneProxies.AddSceneProxy(InSceneProxy);
		}
		else
		{
			LocalLightSceneProxies.RemoveSceneProxy(InSceneProxy);
		}
	}
	void RScene::AddOrRemoveBezierGrass_RenderThread(RBezierGrassSceneProxy* InSceneProxy, BOOL32 InIsAdd)
	{
#if 0
		if (InIsAdd)
		{
			BezierGrassSceneProxies.AddSceneProxy(InSceneProxy);
		}
		else
		{
			BezierGrassSceneProxies.RemoveSceneProxy(InSceneProxy);
		}
#endif
		(void)InSceneProxy;
		(void)InIsAdd;
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
		const EShaderInstance* TempMatAsset = InComponent->GetMaterialInstance();

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
