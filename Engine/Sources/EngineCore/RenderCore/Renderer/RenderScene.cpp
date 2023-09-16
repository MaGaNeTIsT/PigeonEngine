#include "RenderScene.h"
#include <MeshAsset/MeshAsset.h>
#include <RenderProxy/ViewProxy.h>
#include <RenderProxy/LightSceneProxy.h>
#include <RenderProxy/PrimitiveSceneProxy.h>
#include <RenderProxy/MeshSceneProxy.h>
#include <RenderProxy/StaticMeshSceneProxy.h>
#include <RenderProxy/SkeletalMeshSceneProxy.h>
#include <PigeonBase/Object/Component/CameraAndLight/CameraComponent.h>
#include <PigeonBase/Object/Component/CameraAndLight/DirectionalLightComponent.h>
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>
#include <PigeonBase/Object/Component/Primitive/SkeletalMeshComponent.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RCommand, ERegisterBase>();
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

	}
	void RScene::ClearAll()
	{
		UnbindErrorCheck();
		RenderSceneOctree.ClearPrimitives();
	}
	void RScene::UnbindErrorCheck()
	{
		Check((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all static meshes' mapping failed"), (StaticMeshSceneProxies.SceneProxyMapping.Length() == 0u));
		Check((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all static meshes failed"), (StaticMeshSceneProxies.SceneProxies.Length() == 0u));
		Check((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all skeletal meshes' mapping failed"), (SkeletalMeshSceneProxies.SceneProxyMapping.Length() == 0u));
		Check((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all skeletal meshes failed"), (SkeletalMeshSceneProxies.SceneProxies.Length() == 0u));
	}
	void RScene::AddCamera(PCameraComponent* InComponent)
	{
		RScene* Scene = this;
		RViewProxy* SceneProxy = InComponent->CreateSceneProxy();

		ERenderViewMatrices* TempMatrices = new ERenderViewMatrices(
			InComponent->GetComponentWorldLocation(),
			InComponent->GetComponentWorldRotation(),
			InComponent->GetComponentWorldScale(),
			InComponent->GetCameraMatrix());
		ERenderViewParams* TempParams = new ERenderViewParams(InComponent->GetCameraFrustum(), InComponent->GetCameraViewInfo());

		RenderAddRemoveCommands.EnqueueCommand(
			[Scene, SceneProxy, TempMatrices, TempParams]()->void
			{
				SceneProxy->SetupProxy(*TempMatrices, *TempParams);
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
		RenderAddRemoveCommands.EnqueueCommand(
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
			[Scene, SceneProxy, TempMatrices, TempParams]()->void
			{
				// View params MUST be updating before matrices.
				if (TempParams)
				{
					SceneProxy->UpdateViewParams(*TempParams);
					delete TempParams;
				}
				if (TempMatrices)
				{
					SceneProxy->UpdateMatrices(*TempMatrices);
					delete TempMatrices;
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

		RenderAddRemoveCommands.EnqueueCommand(
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
		RenderAddRemoveCommands.EnqueueCommand(
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
				if (TempMatrices)
				{
					SceneProxy->UpdateMatrices(*TempMatrices);
					delete TempMatrices;
				}
				if (TempParams)
				{
					SceneProxy->UpdateLightParams(*TempParams);
					delete TempParams;
				}
				if (TempUsedCascadeShadowData)
				{
					SceneProxy->UpdateCascadeData(TempUsedCascadeShadowData);
					delete TempUsedCascadeShadowData;
				}
			});
	}
	void RScene::AddStaticMesh(PStaticMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RStaticMeshSceneProxy* SceneProxy = InComponent->CreateSceneProxy();

		const BOOL32 InIsMovable = InComponent->GetMobility() != EMobilityType::EMT_STATIC,
			InIsCastShadow = InComponent->IsPrimitiveCastShadow(),
			InIsReceiveShadow = InComponent->IsPrimitiveReceiveShadow();
		ERenderPrimitiveMatrices* TempMatrices = new ERenderPrimitiveMatrices(
			InComponent->GetComponentWorldLocation(),
			InComponent->GetComponentWorldRotation(),
			InComponent->GetComponentWorldScale());
		const EStaticMeshAsset* TempMeshAsset = InComponent->GetMeshAsset();

		RenderAddRemoveCommands.EnqueueCommand(
			[Scene, SceneProxy, InIsMovable, InIsCastShadow, InIsReceiveShadow, TempMatrices, TempMeshAsset]()->void
			{
				SceneProxy->SetupProxy(InIsMovable, InIsCastShadow, InIsReceiveShadow, *TempMatrices, TempMeshAsset);
				delete TempMatrices;
				Scene->AddOrRemoveStaticMesh_RenderThread(SceneProxy, TRUE);
			});
	}
	void RScene::RemoveStaticMesh(PStaticMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RStaticMeshSceneProxy* SceneProxy = InComponent->SceneProxy;
		InComponent->SceneProxy = nullptr;
		RenderAddRemoveCommands.EnqueueCommand(
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

		const BOOL32 InIsMovable = InComponent->GetMobility() != EMobilityType::EMT_STATIC,
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
			[Scene, SceneProxy, InIsMovable, InIsCastShadow, InIsReceiveShadow, TempMatrices, TempMeshAsset]()->void
			{
				SceneProxy->SetPrimitiveSettings(InIsMovable, InIsCastShadow, InIsReceiveShadow);
				if (TempMatrices)
				{
					SceneProxy->UpdatePrimitiveMatrices(*TempMatrices);
					delete TempMatrices;
				}
				if (TempMeshAsset)
				{
					SceneProxy->UpdateMeshAsset(TempMeshAsset);
				}
			});
	}
	void RScene::AddSkeletalMesh(PSkeletalMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RSkeletalMeshSceneProxy* SceneProxy = InComponent->CreateSceneProxy();
		RenderAddRemoveCommands.EnqueueCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->AddOrRemoveSkeletalMesh_RenderThread(SceneProxy, TRUE);
			});
	}
	void RScene::RemoveSkeletalMesh(PSkeletalMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RSkeletalMeshSceneProxy* SceneProxy = InComponent->GetSceneProxy();
		RenderAddRemoveCommands.EnqueueCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->AddOrRemoveSkeletalMesh_RenderThread(SceneProxy, TRUE);
				delete SceneProxy;
			});
	}
	void RScene::UpdateSkeletalMesh(PSkeletalMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RSkeletalMeshSceneProxy* SceneProxy = InComponent->GetSceneProxy();
		//TODO
		RenderUpdateCommands.EnqueueCommand(
			[Scene, SceneProxy]()->void
			{

			});
	}
	RCommand& RScene::GetAddOrRemoveCommands()
	{
		return RenderAddRemoveCommands;
	}
	const RCommand& RScene::GetAddOrRemoveCommands()const
	{
		return RenderAddRemoveCommands;
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