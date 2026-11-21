#include "StaticMeshComponent.h"
#include <Asset/AssetLoadManager.h>
#include <MeshAsset/MeshAsset.h>
#include <ShaderAsset/ShaderJobBuilder.h>
#include <PigeonBase/Object/World/World.h>
#include <Renderer/RenderInterface.h>
#include <Reflection/Property.h>

namespace PigeonEngine
{

	static void RegisterClassProperties()
	{
		// Asset reference by name: the file stores the mesh asset's name and the setter
		// re-requests it through the asset manager. The asset must already be importable
		// (the demo/level pipeline) - loading a level never imports new files.
		AddClassAssetPropertyAccessors<PStaticMeshComponent>(GetClassHashCode<PStaticMeshComponent>(), PE_TEXT("MeshAsset"),
			[](const PStaticMeshComponent* Component) -> EString
			{
				return Component->GetMeshAssetName();
			},
			[](PStaticMeshComponent* Component, const EString& Value) -> void
			{
				if (Value.Length() > 0u)
				{
					Component->RequestMeshAsset(Value, nullptr);
				}
				else
				{
					Component->ClearMeshAsset();
				}
			},
			EPropertyAssetType::STATIC_MESH,
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		// Which class of surface this mesh contributes to the navmesh. A static mesh is the
		// only thing the collector bakes, so the mark lives here and not on the primitive base.
		AddClassEnumPropertyAccessors<PStaticMeshComponent, ENavigationAreaFlagType>(GetClassHashCode<PStaticMeshComponent>(), PE_TEXT("NavigationArea"),
			[](const PStaticMeshComponent* Component) -> ENavigationAreaFlagType { return Component->GetNavigationArea(); },
			[](PStaticMeshComponent* Component, const ENavigationAreaFlagType& Value) -> void { Component->SetNavigationArea(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<PStaticMeshComponent, PMeshComponent>();
		RegisterClassProperties();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PStaticMeshComponent::PStaticMeshComponent()
		: MeshAsset(nullptr)
	{
		POBJ_DEBUGNAME_SET(this, "StaticMeshComponent");
	}
	PStaticMeshComponent::~PStaticMeshComponent()
	{
		// Nothing to release here: the proxy belongs to the render scene and is deleted by the
		// remove command, so the component never holds a pointer to it.
	}
	const EStaticMeshAsset* PStaticMeshComponent::GetMeshAsset() const
	{
		return MeshAsset;
	}
	const EString& PStaticMeshComponent::GetMeshAssetName() const
	{
		if (MeshAsset)
		{
			return MeshAsset->GetAssetName();
		}
		return RequestedMeshAssetName;
	}
	const EName& PStaticMeshComponent::GetRenderProxyTypeName() const
	{
		// Has to match the name RStaticMeshSceneProxy registers itself with, because that is
		// what the render side looks the proxy class up by.
		static const EName ProxyTypeName(PE_TEXT("StaticMeshPrimitive"));
		return ProxyTypeName;
	}
	ENavigationAreaFlagType PStaticMeshComponent::GetNavigationArea() const
	{
		return NavigationArea;
	}
	void PStaticMeshComponent::SetNavigationArea(const ENavigationAreaFlagType& InArea)
	{
		NavigationArea = InArea;
	}
	void PStaticMeshComponent::SetMeshAsset(const EStaticMeshAsset* InMeshAsset)
	{
		MeshAsset = InMeshAsset;
		if (!!InMeshAsset)
	{
		const EStaticMesh* Mesh = InMeshAsset->GetStoragedResource();
#if ENGINE_EDITOR_ONLY
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Static mesh asset does not contain mesh resource."), (!!Mesh));
		if (Mesh)
#endif
		{
			SetLocalBound(Mesh->GetBoundAABB());
			SetMaterialSlotsNum(Mesh->GetSubmeshPart().Num<UINT32>());
		}
	}
	// Publish the base only after the slot count is ready, then re-resolve authoring
	// material references against the new vertex axis.
	SetMeshAssetBase(InMeshAsset);
	RefreshMaterialSlotReferences();
	// A different mesh is an asset level change: the render side re-resolves the geometry and
	// the pass slots rather than only rewriting values.
	MarkRenderChannelsDirty(PRIMITIVE_UPDATE_ASSETS);
}

BOOL32 PStaticMeshComponent::ResolveMaterialVertexAxis(EShaderVertexFactoryType& OutVertexFactory, EShaderVertexProfile& OutProfile) const
{
	if (!MeshAsset)
	{
		return FALSE;
	}
	const EStaticMesh* Mesh = MeshAsset->GetStoragedResource();
	if (!Mesh)
	{
		return FALSE;
	}
	return EShaderJobBuilder::ResolveVertexAxis(Mesh, OutVertexFactory, OutProfile);
}

	void PStaticMeshComponent::RequestMeshAsset(const EString& InAssetName, const FAssetImportSource* InImportSource)
	{
		RequestedMeshAssetName = InAssetName;
		// Copied by value: the binding outlives this call.
		FAssetImportSource ImportSource;
		if (InImportSource)
		{
			ImportSource = *InImportSource;
		}
		PStaticMeshComponent* Component = this;
		TFunction<BOOL32()> Step = [Component, InAssetName, ImportSource]() -> BOOL32
		{
			const EStaticMeshAsset* MeshAsset = nullptr;
			if (!EMeshAssetManager::GetManagerSingleton()->RequireLoadStaticMeshAsset(
				EBaseSettings::ENGINE_MESH_PATH, InAssetName, MeshAsset, &ImportSource))
			{
				return FALSE;	// not loaded yet: the component retries on its next tick
			}
			Component->SetMeshAsset(MeshAsset);
			return TRUE;
		};
		// Tried once here as well, so an asset that is already loaded is applied - and its render state
		// marked dirty - before this call returns rather than a frame later.
		if (Step() == FALSE)
		{
			AddDeferredApply(static_cast<TFunction<BOOL32()>&&>(Step));
		}
	}

	void PStaticMeshComponent::ClearMeshAsset()
	{
		RequestedMeshAssetName = EString();
		SetMeshAsset(nullptr);
		SetLocalBound(EBoundAABB(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), FALSE));
		SetMaterialSlotsNum(0u);
	}

	// Render proxy functions START
	void PStaticMeshComponent::CreateRenderState()
	{
		PMeshComponent::CreateRenderState();
		if (ShouldRender())
		{
			if (PWorld* World = this->GetWorld())
			{
				if (RSceneInterface* RenderScene = World->GetRenderScene())
				{
					RenderScene->AddStaticMesh(this);
				}
			}
		}
	}
	void PStaticMeshComponent::DestroyRenderState()
	{
		// A component that never joined a world - or whose world already released its
		// render scene - has nothing registered; the guard turns that teardown into a
		// no-op instead of a null dereference.
		if (PWorld* World = this->GetWorld())
		{
			if (RSceneInterface* RenderScene = World->GetRenderScene())
			{
				RenderScene->RemoveStaticMesh(this);
			}
		}
		PMeshComponent::DestroyRenderState();
	}
	void PStaticMeshComponent::SendUpdateRenderState()
	{
		if (ShouldRender() && IsRenderStateDirty())
		{
			if (PWorld* World = this->GetWorld())
			{
				if (RSceneInterface* RenderScene = World->GetRenderScene())
				{
					RenderScene->UpdateStaticMesh(this);
				}
			}
		}
		PMeshComponent::SendUpdateRenderState();
	}
	// Render proxy functions END

}
