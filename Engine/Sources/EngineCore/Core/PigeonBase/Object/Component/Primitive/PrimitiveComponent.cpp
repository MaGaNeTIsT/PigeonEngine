#include "PrimitiveComponent.h"
#include <Shader/ShaderEngineParameters.h>

#if _EDITOR_ONLY
#include <RenderProxy/StaticMeshSceneProxy.h>
#include <imgui.h>
#endif

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PPrimitiveComponent, PSceneComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PPrimitiveComponent::PPrimitiveComponent()
		: PSceneComponent()
		, bMaterialDirty(FALSE)
		, bCastShadow(FALSE)
		, bReceiveShadow(FALSE)
	{
		POBJ_DEBUGNAME_SET(this, "PrimitiveComponent");
	}
	PPrimitiveComponent::~PPrimitiveComponent()
	{
	}
	BOOL32 PPrimitiveComponent::IsCastShadow() const
	{
		return bCastShadow;
	}
	BOOL32 PPrimitiveComponent::IsReceiveShadow() const
	{
		return bReceiveShadow;
	}
	BOOL32 PPrimitiveComponent::IsMaterialDirty() const
	{
		return bMaterialDirty;
	}
	void PPrimitiveComponent::SetCastShadow(BOOL32 InIsCastShadow)
	{
		bCastShadow = InIsCastShadow;
	}
	void PPrimitiveComponent::SetReceiveShadow(BOOL32 InIsReceiveShadow)
	{
		bReceiveShadow = InIsReceiveShadow;
	}

	// What this primitive reports to the render tier START
	const EName& PPrimitiveComponent::GetRenderProxyTypeName() const
	{
		// An invalid name: RPrimitiveSceneProxyFactory refuses it, so the scene drops this
		// primitive instead of building a proxy it cannot draw with. A migrated subclass
		// overrides this with the name its proxy registers under.
		static const EName UnregisteredProxyTypeName;
		return UnregisteredProxyTypeName;
	}
	void PPrimitiveComponent::CollectRenderAssets(FPrimitiveRenderAssetReport& InOutReport) const
	{
		// Reports nothing, so an unmigrated subclass is inert rather than wrong. A subclass
		// adds its mesh, its skeleton, its per pass material instances and its named assets
		// here - this layer deliberately knows none of them.
		(void)InOutReport;
	}
	void PPrimitiveComponent::CollectRenderValues(FShaderValueReport& InOutValues) const
	{
		// The per draw block belongs to every primitive, so the base fills it and a subclass
		// only adds its own values.
		//
		// The matrices go in as the CPU holds them, never transposed: the shader side matrix is
		// the transpose of this one, which is what the library's mul(Matrix, Vector) has to
		// receive for a row vector cpu matrix. WorldInvTransposeMatrix carries a transpose in
		// its VALUE because that is what the normal transform is - it is not an upload step.
		const Matrix4x4 LocalToWorld = MakeMatrix4x4(
			GetComponentWorldLocation(), GetComponentWorldRotation(), GetComponentWorldScale());
		const Matrix4x4 InverseWorld(LocalToWorld.Inverse());

		const EName BlockName(SHADER_BLOCK_DRAW_PARAMS);
		InOutValues.SetMatrix(BlockName, EName(SHADER_FIELD_WORLD_MATRIX), LocalToWorld);
		InOutValues.SetMatrix(BlockName, EName(SHADER_FIELD_WORLD_INV_MATRIX), InverseWorld);
		InOutValues.SetMatrix(BlockName, EName(SHADER_FIELD_WORLD_INV_TRANSPOSE_MATRIX), InverseWorld.Transpose());
		// The proxy's own identity, which is what a debug or picking pass would look a
		// primitive up by. A dense scene assigned index is a later change.
		InOutValues.SetUInt(BlockName, EName(SHADER_FIELD_PRIMITIVE_INDEX), static_cast<UINT32>(GetUniqueID()));
		// No instanced draw path yet: one instance per draw.
		InOutValues.SetUInt(BlockName, EName(SHADER_FIELD_INSTANCE_COUNT), 1u);
	}
	BOOL32 PPrimitiveComponent::BuildRenderSnapshot(FPrimitiveRenderSnapshot& OutSnapshot) const
	{
		OutSnapshot.Identity = GetUniqueID();
		OutSnapshot.ProxyTypeName = GetRenderProxyTypeName();
		OutSnapshot.WorldLocation = GetComponentWorldLocation();
		OutSnapshot.WorldRotation = GetComponentWorldRotation();
		OutSnapshot.WorldScaling = GetComponentWorldScale();
		OutSnapshot.bMovable = ((GetMobility() != EMobilityType::EMT_STATIC) ? TRUE : FALSE);
		OutSnapshot.bCastShadow = bCastShadow;
		OutSnapshot.bReceiveShadow = bReceiveShadow;
		OutSnapshot.DirtyChannels = PendingRenderChannels;
		OutSnapshot.Assets = FPrimitiveRenderAssetReport();
		OutSnapshot.Values = FShaderValueReport();
		CollectRenderAssets(OutSnapshot.Assets);
		CollectRenderValues(OutSnapshot.Values);
		return TRUE;
	}
	UINT32 PPrimitiveComponent::GetPendingRenderChannels() const
	{
		return PendingRenderChannels;
	}
	void PPrimitiveComponent::MarkRenderChannelsDirty(const UINT32 InChannels)
	{
		PendingRenderChannels |= (InChannels & (PRIMITIVE_UPDATE_TRANSFORM | PRIMITIVE_UPDATE_VALUES | PRIMITIVE_UPDATE_ASSETS));
		MarkRenderStateAsDirty();
	}
	void PPrimitiveComponent::ClearRenderChannels(const UINT32 InChannels)
	{
		PendingRenderChannels &= ~InChannels;
		if (PendingRenderChannels == PRIMITIVE_UPDATE_NONE)
		{
			CleanMarkRenderStateDirty();
		}
	}
	// What this primitive reports to the render tier END

	// Render proxy functions START
	void PPrimitiveComponent::CreateRenderState()
	{
		PSceneComponent::CreateRenderState();
		if (ShouldRender())
		{
			//TODO
		}
	}
	void PPrimitiveComponent::DestroyRenderState()
	{
		//TODO
		PSceneComponent::DestroyRenderState();
	}
	void PPrimitiveComponent::SendUpdateRenderState()
	{
		// The chain stops here on purpose. The scene clears exactly the channels the snapshot
		// it enqueued covered, and the state only stops being dirty once none are left; an
		// unconditional clean at this point would drop a channel that was marked while the
		// snapshot was being built.
		if (ShouldRender() && IsRenderStateDirty())
		{
			// A subclass that reaches the scene does it here.
		}
	}
	void PPrimitiveComponent::MarkRenderTransformAsDirty()
	{
		PendingRenderChannels |= PRIMITIVE_UPDATE_TRANSFORM;
		PSceneComponent::MarkRenderTransformAsDirty();
	}
	void PPrimitiveComponent::MarkRenderStateAsDirty()
	{
		PSceneComponent::MarkRenderStateAsDirty();
	}
	void PPrimitiveComponent::CleanMarkRenderStateDirty()
	{
		//TODO
		PPrimitiveComponent::CleanMaterialDirty();
		PSceneComponent::CleanMarkRenderStateDirty();
	}
	void PPrimitiveComponent::MarkMaterialAsDirty()
	{
		bMaterialDirty = TRUE;
		// A material assignment is an asset level change: the render side has to rebuild the
		// pass slot and its pipeline state, not merely rewrite a value.
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_ASSETS);
	}
	void PPrimitiveComponent::CleanMaterialDirty()
	{
		bMaterialDirty = FALSE;
	}
	// Render proxy functions END

#if _EDITOR_ONLY
	void PPrimitiveComponent::GenerateComponentDetail()
	{
		PSceneComponent::GenerateComponentDetail();
		// The material slot panel is intentionally absent: the old material helper panel
		// was deleted together with the material asset system, and the reflection driven
		// replacement of this UI lands in a later phase. So today an editor build shows
		// the primitive component without any material slot editing section.
	}
#endif // _EDITOR_ONLY

};
