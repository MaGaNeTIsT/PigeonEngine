#pragma once

#include <CoreMinimal.h>
#include "PrimitiveComponent.h"

namespace PigeonEngine
{

	class EAssetBase;

	/*
	* What the mesh primitives share: the material slots every mesh asset drives one per
	* submesh, and the mesh asset itself reported to the render tier as a generic handle.
	*
	* The material storage lives HERE rather than on the primitive layer on purpose: a
	* primitive declares no shader asset member of its own, because how a subclass uses its
	* shader instances is a subclass decision. A mesh uses one instance per submesh and reports
	* them under the primary pass; the procedural primitives hold theirs singly.
	*/
	class PMeshComponent : public PPrimitiveComponent
	{

		CLASS_VIRTUAL_NOCOPY_BODY(PMeshComponent)

	public:
		// One entry per submesh. A null entry means "no material assigned", which the render
		// side answers with the engine fallback shader for that mesh.
		PE_NODISCARD const EShaderInstance* GetMaterialInstance(UINT32 InSlotIdx = 0u) const;
		PE_NODISCARD UINT32                 GetMaterialSlotCount() const;
		void    SetMaterialInstance(const EShaderInstance* InMaterialInstance);
		void    SetMaterialInstance(UINT32 InSlotIdx, const EShaderInstance* InMaterialInstance);
		// Grows or shrinks the slot array. New slots start empty; the caller assigns into them.
		void    SetMaterialSlotsNum(UINT32 InNum);
	public:
		// The mesh asset as the render tier sees it. The report carries only this generic
		// handle: a subclass keeps its own typed pointer for its own use, and the shared part
		// of the report needs no typed access.
		PE_NODISCARD const EAssetBase*  GetMeshAssetBase() const;
		void    SetMeshAssetBase(const EAssetBase* InMeshAsset);
	public:
		// Reports the mesh and every assigned material slot under the primary pass. A subclass
		// that has more to report (a skeleton, a compute instance, a named texture) overrides
		// this, calls the base first and then adds its own.
		virtual void    CollectRenderAssets(FPrimitiveRenderAssetReport& InOutReport) const override;
	protected:
		const EAssetBase*               MeshAssetBase = nullptr;
		TArray<const EShaderInstance*>  MaterialSlots;

		// Render proxy functions START
	public:
		virtual void	CreateRenderState()override;
		virtual void	DestroyRenderState()override;
		virtual void	SendUpdateRenderState()override;
	protected:
		virtual void    MarkRenderTransformAsDirty()override;
		virtual void    MarkRenderStateAsDirty()override;
		virtual void    CleanMarkRenderStateDirty()override;
		// Render proxy functions END
	};

};
