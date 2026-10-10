#include "ShaderInstance.h"

#include <Shader/ShaderParameterBuffers.h>
#include <ShaderAsset/ShaderAsset.h>
#include <Main/MainManager.h>
#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	EShaderInstance::EShaderInstance(const EShaderAsset* InAsset, const EName& InVariantName)
		: Asset(InAsset)
		, VariantName(InVariantName)
		, Parameters{}
		, Traits{}
		, CachedProgram(nullptr)
		, CachedAsset(nullptr)
		, CachedAssetRevision(0u)
		, Revision(1u)
		, ParameterBuffersRef()
	{
		CreateParameterBuffersRenderResource();
	}

	EShaderInstance::~EShaderInstance()
	{
		ReleaseParameterBuffersRenderResource();
	}

	void EShaderInstance::CreateParameterBuffersRenderResource()
	{
		RRenderResourceManager* ResourceManager = RRenderResourceManager::GetManagerSingleton();
		ParameterBuffersRef = ResourceManager->GetRegistry().ReserveSlot();
		EShaderInstance* InstancePtr = this;
		const FRenderResourceRef TargetRef = ParameterBuffersRef;
		ResourceManager->EnqueueCommand([InstancePtr, TargetRef]() -> void
		{
		// The command captures the instance pointer and runs on the render thread, where it
		// reads the parameter set's cpu mirrors to build the buffers. The caller resolves the
		// program right after construction, so the mirrors are already built by the time the
		// queue drains; the instance itself has to outlive the command, which the owner of the
		// instance guarantees by releasing it through the manager's retirement rather than by
		// dropping it directly.
		//
		// The check below is the guard for an owner that drops one anyway: such an instance has
		// already handed its reservation back, and this command must not read it then.
			IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
			if (!RenderDevice)
			{
				return;
			}
			if (!(RRenderResourceManager::GetManagerSingleton()->GetRegistry().IsReservationPending(TargetRef)))
			{
				return;
			}
			RShaderParameterBuffers* Buffers = new RShaderParameterBuffers();
			if (!(Buffers->CreateFromParameterSet(RenderDevice, InstancePtr->GetParameters())))
			{
				Buffers->ReleaseRenderResource();
				delete Buffers;
				return;
			}
			if (!(RRenderResourceManager::GetManagerSingleton()->GetRegistry().Commit(TargetRef, Buffers)))
			{
				// Backstop: the reservation was outstanding when this command started, so a
				// failure here means the slot went away while the buffers were built. Nothing
				// could ever reach them again, so they are torn down here.
				Buffers->ReleaseRenderResource();
				delete Buffers;
				return;
			}
		});
	}

	void EShaderInstance::ReleaseParameterBuffersRenderResource()
	{
		if (!ParameterBuffersRef.IsValid())
		{
			return;
		}
		const FRenderResourceRef TargetRef = ParameterBuffersRef;
		ParameterBuffersRef.Reset();
		RRenderResourceManager::GetManagerSingleton()->EnqueueDestroyRef(TargetRef);
	}

	void EShaderInstance::SetTraits(const EShaderRenderTraits& InTraits)
	{
		if (Traits.IsSameAs(InTraits))
		{
			return;
		}
		Traits = InTraits;
		MarkDirty();
	}

	void EShaderInstance::ResetTraits()
	{
		if (Traits.IsAllUnset())
		{
			return;
		}
		Traits.Reset();
		MarkDirty();
	}

	void EShaderInstance::MarkDirty()
	{
		Revision++;
	}

	void EShaderInstance::RebindAsset(const EShaderAsset* InAsset)
	{
		// Only the pointer, and only here: the resolved program, its cached revision and the
		// parameter mirrors are left as they are, so this call is safe from the thread that loaded
		// the new object. The next RefreshProgram on the reading thread notices the object changed
		// and re-derives all three.
		Asset.Store(InAsset);
	}

	BOOL32 EShaderInstance::RefreshProgram()
	{
		const EShaderAsset* CurrentAsset = Asset.Load();
		if (!CurrentAsset)
		{
			CachedProgram = nullptr;
			CachedAsset = nullptr;
			CachedAssetRevision = 0u;
			return FALSE;
		}

		// The cached program is only valid for one asset OBJECT and one revision of it: a reload
		// publishes a new object under the same name, and nothing carries over from the old one.
		if (CachedProgram && (CachedAsset == CurrentAsset) && (CachedAssetRevision == CurrentAsset->GetRevision()))
		{
			return TRUE;
		}

		EShaderProgram* Program = const_cast<EShaderAsset*>(CurrentAsset)->FindProgram(VariantName);
		if (!Program)
		{
			PE_LOG_ERROR(EString("[ShaderInstance] Asset '") + CurrentAsset->GetName().ToString()
				+ EString("' has no variant '") + VariantName.ToString() + EString("'."));
			CachedProgram = nullptr;
			CachedAsset = CurrentAsset;
			CachedAssetRevision = CurrentAsset->GetRevision();
			return FALSE;
		}
		if (!Program->IsValid())
		{
			PE_LOG_WARN(EString("[ShaderInstance] Variant '") + VariantName.ToString()
				+ EString("' of asset '") + CurrentAsset->GetName().ToString() + EString("' has no valid stage."));
			CachedProgram = nullptr;
			CachedAsset = CurrentAsset;
			CachedAssetRevision = CurrentAsset->GetRevision();
			return FALSE;
		}

		CachedProgram = Program;
		CachedAsset = CurrentAsset;
		CachedAssetRevision = CurrentAsset->GetRevision();

		// The value side follows the program: the buffers are sized from its
		// reflection and seeded with its defaults. The parent stays pointed at the
		// asset level set, so an instance without overrides follows the asset.
		Parameters.InitializeFromProgram(*Program);
		EShaderParameterSet* AssetParameterSet = const_cast<EShaderAsset*>(CurrentAsset)->GetOrCreateDefaultParameterSet(VariantName);
		Parameters.SetParent(AssetParameterSet);
		MarkDirty();
		return TRUE;
	}

	const EShaderProgram* EShaderInstance::GetProgram()
	{
		if (!RefreshProgram())
		{
			return nullptr;
		}
		return CachedProgram;
	}

	const EShaderProgram* EShaderInstance::GetProgram() const
	{
		// The cache is an implementation detail of the lookup, so a const instance can
		// still resolve its program.
		return const_cast<EShaderInstance*>(this)->GetProgram();
	}

	BOOL32 EShaderInstance::Prepare()
	{
		if (!RefreshProgram())
		{
			return FALSE;
		}
		// Value layer only: the cpu mirrors are re-evaluated and left dirty for the render
		// side, which owns the gpu buffers (RShaderParameterBuffers) and uploads them.
		return Parameters.PrepareBuffers();
	}

	BOOL32 EShaderInstance::Prepare() const
	{
		// The resolved program and the cpu mirrors are caches of the lookup rather than
		// observable state, the same reason GetProgram() const casts through this.
		return const_cast<EShaderInstance*>(this)->Prepare();
	}

	BOOL32 EShaderInstance::IsValid()
	{
		return ((GetProgram() != nullptr) ? TRUE : FALSE);
	}

} // namespace PigeonEngine
