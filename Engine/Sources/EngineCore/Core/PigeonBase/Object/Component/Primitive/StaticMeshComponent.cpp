#include "StaticMeshComponent.h"
#include <Asset/AssetLoadManager.h>
#include <MeshAsset/MeshAsset.h>
#include <PigeonBase/Object/World/World.h>
#include <Renderer/RenderInterface.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PStaticMeshComponent, PMeshComponent>();
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
	const EStaticMeshAsset* PStaticMeshComponent::GetMeshAsset()const
	{
		return MeshAsset;
	}
	const EName& PStaticMeshComponent::GetRenderProxyTypeName() const
	{
		// Has to match the name RStaticMeshSceneProxy registers itself with, because that is
		// what the render side looks the proxy class up by.
		static const EName ProxyTypeName(PE_TEXT("StaticMeshPrimitive"));
		return ProxyTypeName;
	}
	void PStaticMeshComponent::SetMeshAsset(const EStaticMeshAsset* InMeshAsset)
	{
		MeshAsset = InMeshAsset;
		SetMeshAssetBase(InMeshAsset);
		if (!!InMeshAsset)
		{
			const EStaticMesh* Mesh = InMeshAsset->GetStoragedResource();
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Static mesh asset does not contain mesh resource."), (!!Mesh));
			if (Mesh)
#endif
			{
				SetLocalBound(Mesh->GetBoundAABB());
				SetMaterialSlotsNum(Mesh->GetSubmeshPart().Num<UINT32>());
			}
		}
		// A different mesh is an asset level change: the render side re-resolves the geometry and
		// the pass slots rather than only rewriting values.
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_ASSETS);
	}
	void PStaticMeshComponent::RequestMeshAsset(const EString& InAssetName, const FAssetImportSource* InImportSource)
	{
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

	// Render proxy functions START
	void PStaticMeshComponent::CreateRenderState()
	{
		PMeshComponent::CreateRenderState();
		if (ShouldRender())
		{
			this->GetWorld()->GetRenderScene()->AddStaticMesh(this);
		}
	}
	void PStaticMeshComponent::DestroyRenderState()
	{
		this->GetWorld()->GetRenderScene()->RemoveStaticMesh(this);
		PMeshComponent::DestroyRenderState();
	}
	void PStaticMeshComponent::SendUpdateRenderState()
	{
		if (ShouldRender() && IsRenderStateDirty())
		{
			this->GetWorld()->GetRenderScene()->UpdateStaticMesh(this);
		}
		PMeshComponent::SendUpdateRenderState();
	}
	// Render proxy functions END

}
