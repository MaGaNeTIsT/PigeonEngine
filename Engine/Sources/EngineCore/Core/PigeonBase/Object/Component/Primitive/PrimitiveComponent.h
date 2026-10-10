#pragma once

#include "EngineCommon.h"
#include <PigeonBase/Object/Component/SceneComponent.h>
#include "PrimitiveRenderData.h"
#include <ShaderAsset/ShaderAsset.h>
#include <ShaderAsset/ShaderInstance.h>

namespace PigeonEngine
{

	struct ERenderPrimitiveMatrices : public ERenderTransformInfo
	{
	public:

	public:
		ERenderPrimitiveMatrices(const Vector3& InWorldLocation, const Quaternion& InWorldRotation, const Vector3& InWorldScaling)
			: ERenderTransformInfo(InWorldLocation, InWorldRotation, InWorldScaling)
		{
		}
		ERenderPrimitiveMatrices() = default;
		ERenderPrimitiveMatrices(const ERenderPrimitiveMatrices& Other)
			: ERenderTransformInfo(Other)
		{
		}
		ERenderPrimitiveMatrices& operator=(const ERenderPrimitiveMatrices& Other)
		{
			ERenderTransformInfo::CopyFromOther(Other);
			//TODO
			return (*this);
		}
	};

	class PPrimitiveComponent : public PSceneComponent
	{

		CLASS_VIRTUAL_NOCOPY_BODY(PPrimitiveComponent)

	public:
		PE_NODISCARD BOOL32                 IsCastShadow() const;
		PE_NODISCARD BOOL32                 IsReceiveShadow() const;
		PE_NODISCARD BOOL32                 IsMaterialDirty() const;
	public:
		void            SetCastShadow(BOOL32 InIsCastShadow);
		void            SetReceiveShadow(BOOL32 InIsReceiveShadow);

		// Materials are deliberately NOT stored here: how a subclass holds its shader
		// instances, and under which pass it reports them, is the subclass's decision. What
		// this layer provides is the channel that says "they changed" and the report those
		// subclasses fill in.

		// What this primitive reports to the render tier START
	public:
		// Registered per primitive subclass. The render side builds the proxy by looking this
		// name up in RPrimitiveSceneProxyFactory, so it never has to touch this object.
		//
		// The default is an invalid name, which the factory refuses: a subclass that has not
		// stated its proxy type yet submits nothing. That default exists so a primitive can be
		// migrated on its own, and a migrated subclass overrides it with its registered name.
		PE_NODISCARD virtual const EName&   GetRenderProxyTypeName() const;
		// Everything the render tier has to mirror about this primitive. The primitive layer
		// declares the SHAPE only: which shader instances a component holds, and under which
		// pass it reports them, stays the subclass's business - this layer declares no shader
		// asset member of its own.
		//
		// The default reports nothing, for the same reason as above: a primitive that reports
		// no assets draws nothing, so an unmigrated subclass is inert rather than wrong.
		virtual void    CollectRenderAssets(FPrimitiveRenderAssetReport& InOutReport) const;
		// The named values of this primitive, keyed by the block / field names the shader
		// source declares. The base reports the per draw block, so a subclass only adds its
		// own values.
		virtual void    CollectRenderValues(FShaderValueReport& InOutValues) const;
		// Identity, transform, flags and the two reports above, in one value type that can
		// cross to the render thread. FALSE when the primitive has nothing to submit.
		PE_NODISCARD virtual BOOL32 BuildRenderSnapshot(FPrimitiveRenderSnapshot& OutSnapshot) const;
	public:
		PE_NODISCARD UINT32 GetPendingRenderChannels() const;
		void    MarkRenderChannelsDirty(const UINT32 InChannels);
		// Called by the scene once a snapshot covering these channels has been enqueued. Only
		// the channels that snapshot actually carried are cleared, so a change made while the
		// command was being built is not lost.
		void    ClearRenderChannels(const UINT32 InChannels);
	protected:
		UINT32  PendingRenderChannels = PRIMITIVE_UPDATE_NONE;
		// What this primitive reports to the render tier END

		// Render proxy functions START
	public:
		virtual void    CreateRenderState()override;
		virtual void    DestroyRenderState()override;
		virtual void    SendUpdateRenderState()override;
	protected:
		virtual void    MarkRenderTransformAsDirty()override;
		virtual void    MarkRenderStateAsDirty()override;
		virtual void    CleanMarkRenderStateDirty()override;
		virtual void    MarkMaterialAsDirty();
		virtual void    CleanMaterialDirty();
		// Render proxy functions END

#if _EDITOR_ONLY
	public:
		virtual void    GenerateComponentDetail() override;
#endif
	protected:
		BOOL32                              bMaterialDirty;
		BOOL32                              bCastShadow;
		BOOL32                              bReceiveShadow;
#if _EDITOR_ONLY
		BOOL8                               bEmitShaderDebug = FALSE;
#endif

	};

};
