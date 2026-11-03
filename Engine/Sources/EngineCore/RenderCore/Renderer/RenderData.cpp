#include "RenderData.h"

#include <Main/MainManager.h>
#include <RenderProxy/PrimitiveSceneProxy.h>
#include <RenderProxy/ViewProxy.h>
#include <Renderer/ScenePass.h>
#include <Shader/ShaderAssetBinding.h>
#include <Shader/ShaderParameterBuffers.h>
#include <ShaderAsset/ShaderInstance.h>
#include <ShaderAsset/ShaderProgram.h>

namespace PigeonEngine
{

	BOOL32 FShaderInstanceKey::operator<(const FShaderInstanceKey& InOther) const
	{
		if (ShaderInstance != InOther.ShaderInstance)
		{
			return ((ShaderInstance < InOther.ShaderInstance) ? TRUE : FALSE);
		}
		return ((Program < InOther.Program) ? TRUE : FALSE);
	}

	BOOL32 FShaderInstanceKey::operator==(const FShaderInstanceKey& InOther) const
	{
		return (((ShaderInstance == InOther.ShaderInstance) && (Program == InOther.Program)) ? TRUE : FALSE);
	}

	FShaderInstanceRenderData::~FShaderInstanceRenderData()
	{
		if (ParameterBuffers)
		{
			delete ParameterBuffers;
			ParameterBuffers = nullptr;
		}
	}

	FFrameRenderData::FFrameRenderData()
	{
	}

	FFrameRenderData::~FFrameRenderData()
	{
		ReleaseShaderInstanceData();
	}

	void FFrameRenderData::BeginFrame(const UINT64 InFrameIndex)
	{
		(void)InFrameIndex;
		// A monotonic value, not the device's cycling frame index: a stamp has to be unique
		// across the whole session, or an object visited two frames later could compare equal
		// to the frame it was last published in.
		FrameCounter += 1ull;
		FrameIndex = FrameCounter;
		bPreparing = TRUE;
		BindingSets.SetNum(0);
		if (!RenderDevice)
		{
			RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		}
	}

	void FFrameRenderData::EndPrepare()
	{
		bPreparing = FALSE;
	}

	void FFrameRenderData::EndFrame()
	{
		bPreparing = FALSE;
	}

	void FFrameRenderData::ReleaseAll()
	{
		Views.Empty();
		Primitives.Empty();
		ShaderInstances.Empty();
		BindingSets.SetNum(0);
		RenderDevice = nullptr;
	}

	FViewRenderData* FFrameRenderData::EnsureView(RViewProxy& InView)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("RenderData: EnsureView ran outside the Prepare phase.")), (bPreparing != FALSE));

		const ObjectIdentityType ViewIdentity = InView.GetUniqueID();
		TUniquePtr<FViewRenderData>& Entry = Views[ViewIdentity];
		if (!Entry)
		{
			Entry = EMemory::MakeUnique<FViewRenderData>();
			Entry->View = &InView;
			Entry->ViewKind = InView.GetViewKind();
		}
		FViewRenderData* Data = Entry.get();
		if (Data->ResolvedFrame != FrameIndex)
		{
			// The view owns the values and the GPU buffers. The stamp is what makes this the
			// one publish of the frame: every later pass asking for the same view returns here
			// without touching the block again.
			InView.UpdateRenderResource();
			Data->ResolvedFrame = FrameIndex;
		}
		return Data;
	}

	FPrimitiveRenderData* FFrameRenderData::EnsurePrimitive(RPrimitiveSceneProxy& InPrimitive)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("RenderData: EnsurePrimitive ran outside the Prepare phase.")), (bPreparing != FALSE));

		const ObjectIdentityType PrimitiveIdentity = InPrimitive.GetUniqueID();
		TUniquePtr<FPrimitiveRenderData>& Entry = Primitives[PrimitiveIdentity];
		if (!Entry)
		{
			Entry = EMemory::MakeUnique<FPrimitiveRenderData>();
			Entry->Primitive = &InPrimitive;
		}
		FPrimitiveRenderData* Data = Entry.get();
		if (Data->ResolvedFrame != FrameIndex)
		{
			InPrimitive.EnsurePublishedDrawParameters();
			Data->ResolvedFrame = FrameIndex;
		}
		return Data;
	}

	FPrimitivePassData* FFrameRenderData::EnsurePrimitivePass(RPrimitiveSceneProxy& InPrimitive, const FRenderPassDesc& InPassDesc
		, const EShaderInstance* InViewShaderInstanceOverride, const UINT32 InSlotIndex)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("RenderData: EnsurePrimitivePass ran outside the Prepare phase.")), (bPreparing != FALSE));
		// The pass slot cache lives on the primitive, because that is where its lifetime and
		// its invalidation already are. This call is the publish entry: it resolves the pass
		// specific instance, the pipeline state, the geometry binding and the shared instance
		// data, all once per frame.
		return InPrimitive.EnsurePublishedPassData(InSlotIndex, FRenderPassRegistry::GetPassNameId(InPassDesc)
			, InPassDesc, InViewShaderInstanceOverride, *this);
	}

	FShaderInstanceRenderData* FFrameRenderData::EnsureShaderInstance(const EShaderInstance& InShaderInstance, const EShaderProgram& InProgram)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("RenderData: EnsureShaderInstance ran outside the Prepare phase.")), (bPreparing != FALSE));

		FShaderInstanceKey Key;
		Key.ShaderInstance	= &InShaderInstance;
		Key.Program			= &InProgram;

		TUniquePtr<FShaderInstanceRenderData>& Entry = ShaderInstances[Key];
		if (!Entry)
		{
			Entry = EMemory::MakeUnique<FShaderInstanceRenderData>();
			Entry->Key				= Key;
			Entry->Parameters		= &(const_cast<EShaderInstance&>(InShaderInstance).GetParameters());
			Entry->ParameterBuffers	= new RShaderParameterBuffers();
		}
		FShaderInstanceRenderData* Data = Entry.get();
		// A material instance that was reloaded can land on a recycled address; re-reading the
		// set pointer here is what keeps the entry from binding the previous object's values.
		Data->Parameters = &(const_cast<EShaderInstance&>(InShaderInstance).GetParameters());
		if (Data->ResolvedFrame == FrameIndex)
		{
			return Data;
		}
		if (!RenderDevice)
		{
			RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		}
		if (!RenderDevice)
		{
			return nullptr;
		}

		// The value layer re-evaluates its CPU mirrors first; the publish then writes the
		// bindless indices into the same set and uploads whatever moved. One call per
		// (instance, program) per frame, no matter how many draws reference it.
		const_cast<EShaderInstance&>(InShaderInstance).Prepare();
		if (PublishShaderParameterValues(RenderDevice, *Data->Parameters, *Data->ParameterBuffers
			, InProgram, EName(SHADER_BINDING_SCOPE_MATERIAL)) == FALSE)
		{
			return nullptr;
		}
		Data->ResolvedRevision	= InShaderInstance.GetRevision();
		Data->ResolvedFrame		= FrameIndex;
		return Data;
	}

	FBindingSet* FFrameRenderData::EnsureBindingSet(const FPrimitivePassData& InPrimitivePassData, const FViewRenderData& InViewData)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("RenderData: EnsureBindingSet ran outside the Prepare phase.")), (bPreparing != FALSE));
		// The cached descriptor table is the L6 work item: the command list currently opens a
		// set per draw and the cached path needs a table allocator that is not part of this
		// change. The key and the item field already exist, so switching the record path over
		// is local to the binding layer.
		(void)InPrimitivePassData;
		(void)InViewData;
		return nullptr;
	}

	FShaderInstanceRenderData* FFrameRenderData::FindShaderInstance(const FShaderInstanceKey& InKey) const
	{
		const TUniquePtr<FShaderInstanceRenderData>* Entry = ShaderInstances.FindValueAsPtr(InKey);
		return ((Entry && (*Entry)) ? Entry->get() : nullptr);
	}

	void FFrameRenderData::ReleaseShaderInstanceData()
	{
		ShaderInstances.Empty();
	}

}	// namespace PigeonEngine
