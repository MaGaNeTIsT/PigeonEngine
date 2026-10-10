#include "EngineShaderInstanceCache.h"

#include <Asset/AssetLoadManager.h>
#include <Main/MainManager.h>
#include <Renderer/SceneRenderer.h>
#include <Shader/ShaderAssetBinding.h>
#include <ShaderAsset/ShaderAsset.h>
#include <ShaderAsset/ShaderInstance.h>
#include <RenderResourceLifetime.h>
#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<REngineShaderInstanceCache, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	// The materials whose failure has already been reported, so a scene full of meshes sharing
	// one broken material reports it once instead of once per object. Only ever grows, and holds
	// one entry per broken material, which is a handful in the worst case.
	static TArray<EString>& GetReportedMaterialFailures()
	{
		static TArray<EString> ReportedMaterialFailures;
		return ReportedMaterialFailures;
	}

	// The shader that stands in for a material that could not be assembled. It is an asset of its
	// own, so a publication that follows an assembly has to publish it as well.
	static const EName& GetEngineErrorShaderName()
	{
		static const EName ErrorShaderName(PE_TEXT("M_EngineError"));
		return ErrorShaderName;
	}

	REngineShaderInstanceCache::REngineShaderInstanceCache()
	{
	}
	REngineShaderInstanceCache::~REngineShaderInstanceCache()
	{
	}

	void REngineShaderInstanceCache::Initialize()
	{
		// Nothing to prepare: the table starts empty and fills on demand.
	}
	void REngineShaderInstanceCache::ShutDown()
	{
		ReleaseAllInstances();
	}
	void REngineShaderInstanceCache::Uninit()
	{
		// The same release as ShutDown: either of the two may be the one that runs on a given
		// teardown path, and both are idempotent because the table is emptied with the release.
		ReleaseAllInstances();
	}
	void REngineShaderInstanceCache::ReleaseAllInstances()
	{
		// The table is emptied under its lock and the instances are retired outside it: a
		// retirement takes the render resource manager's own list lock, and nesting the two would
		// put a lock order between this cache and that manager for no reason.
		TArray<const EShaderInstance*> ReleasedInstances;
		{
			EScopedLock ScopedLock(TableMutex);
			for (UINT32 Index = 0u, Num = Instances.Num<UINT32>(); Index < Num; Index++)
			{
				if (Instances[Index].Instance)
				{
					ReleasedInstances.Add(Instances[Index].Instance);
				}
			}
			Instances.Empty();
		}
		for (UINT32 Index = 0u, Num = ReleasedInstances.Num<UINT32>(); Index < Num; Index++)
		{
			const EShaderInstance* Instance = ReleasedInstances[Index];
			// Handed back through the render resource manager's retirement rather than released
			// here: the command that builds an instance's constant buffers may still be queued,
			// and it reads the instance's parameter mirrors. ShutDown runs before that manager
			// drains, so the destroy the release enqueues is still executed.
			RRenderResourceManager::GetManagerSingleton()->RetireRelease([Instance]() -> void { delete Instance; });
		}
	}

	EString REngineShaderInstanceCache::MakeInstanceRequestKey(const EShaderAssemblyRequest& InRequest)
	{
		/*
		* Every field that shapes the resolved desc, so two requests share an instance exactly
		* when the assembly behind them would be the same one. The fields the desc derives from
		* the material json (its variant defines, its domain, whether it has vertex ops) are
		* covered by carrying the material name: within one run the same name reads the same file.
		*
		* Pure string work on purpose. This runs inside a draw, where reading the program or the
		* material json to resolve the desc would be a file read the frame cannot afford - and
		* exactly what this refactor exists to keep out of a draw.
		*/
		EString RequestKey(PE_TEXT("Instance|"));
		RequestKey = RequestKey + InRequest.ShaderName.ToString();
		RequestKey = RequestKey + EString(PE_TEXT("|")) + InRequest.MaterialName;
		RequestKey = RequestKey + EString(PE_TEXT("|")) + InRequest.DomainName;
		RequestKey = RequestKey + EString(PE_TEXT("|")) + InRequest.ProgramName.ToString();
		RequestKey = RequestKey + EString(PE_TEXT("|")) + ToString(static_cast<UINT32>(InRequest.VertexFactory));
		RequestKey = RequestKey + EString(PE_TEXT("|")) + ToString(InRequest.Profile.StreamProfile);
		RequestKey = RequestKey + EString(PE_TEXT("|")) + ToString(InRequest.StageMask);
		RequestKey = RequestKey + EString(PE_TEXT("|")) + ToString(InRequest.SurfaceModeOverride);
		return RequestKey;
	}

	const EShaderInstance* REngineShaderInstanceCache::TryFindInstance(const EShaderAssemblyRequest& InRequest) const
	{
		const EString RequestKey = MakeInstanceRequestKey(InRequest);
		EScopedLock ScopedLock(TableMutex);
		for (UINT32 Index = 0u, Num = Instances.Num<UINT32>(); Index < Num; Index++)
		{
			if (Instances[Index].RequestKey == RequestKey)
			{
				return Instances[Index].Instance;
			}
		}
		return nullptr;
	}

	const EShaderInstance* REngineShaderInstanceCache::RequireInstance(const EShaderAssemblyRequest& InRequest)
	{
		if (const EShaderInstance* Cached = TryFindInstance(InRequest))
		{
			return Cached;
		}
		// Not assembled yet: hand the work to the load thread and answer with nothing. The caller
		// draws without it this frame and asks again next frame. A draw never reaches the disk or
		// the compiler - the request is queued and the frame moves on.
		//
		// The completion is what makes the swap visible: it runs on the load thread right after the
		// assembly, which is where a new asset object may have been published for this shader.
		EAssetLoadManager::GetManagerSingleton()->RequestLoad(MakeInstanceRequestKey(InRequest),
			[InRequest]() -> BOOL32
			{
				return ((REngineShaderInstanceCache::GetManagerSingleton()->CreateInstanceForRequest(InRequest) != nullptr) ? TRUE : FALSE);
			},
			[InRequest](const BOOL32 InIsLoaded) -> void
			{
				if (InIsLoaded != FALSE)
				{
					REngineShaderInstanceCache::GetManagerSingleton()->PublishShaderAssetSwap(InRequest.ShaderName);
				}
			});
		return nullptr;
	}

	const EShaderInstance* REngineShaderInstanceCache::RequireMaterialInstance(const EShaderAssemblyRequest& InRequest)
	{
		if (const EShaderInstance* Cached = TryFindInstance(InRequest))
		{
			return Cached;
		}
		// As above, with the material half of the semantics: what the load thread builds may be
		// the engine error material when this one cannot be assembled, and that substitution is
		// made there rather than here.
		EAssetLoadManager::GetManagerSingleton()->RequestLoad(MakeInstanceRequestKey(InRequest),
			[InRequest]() -> BOOL32
			{
				return ((REngineShaderInstanceCache::GetManagerSingleton()->CreateMaterialInstanceForRequest(InRequest) != nullptr) ? TRUE : FALSE);
			},
			[InRequest](const BOOL32 InIsLoaded) -> void
			{
				if (InIsLoaded != FALSE)
				{
					REngineShaderInstanceCache* Cache = REngineShaderInstanceCache::GetManagerSingleton();
					Cache->PublishShaderAssetSwap(InRequest.ShaderName);
					// The substitute is an asset of its own and the same request may have rebuilt it,
					// so it is published too.
					Cache->PublishShaderAssetSwap(GetEngineErrorShaderName());
				}
			});
		return nullptr;
	}

	const EShaderInstance* REngineShaderInstanceCache::FindOrCreate(const EShaderAssemblyRequest& InRequest)
	{
		// The load thread's synchronous form, for the callers that resolve a program during engine
		// start up. A frame's requests go through RequireInstance.
		if (const EShaderInstance* Cached = TryFindInstance(InRequest))
		{
			return Cached;
		}
		const EShaderInstance* Instance = CreateInstanceForRequest(InRequest);
		// The same publication step the requested form runs in its completion. At start up nothing
		// has been bound to the previous object yet, so this finds nothing to re-point.
		if (Instance)
		{
			PublishShaderAssetSwap(InRequest.ShaderName);
		}
		return Instance;
	}

	const EShaderInstance* REngineShaderInstanceCache::FindOrCreateMaterial(const EShaderAssemblyRequest& InRequest)
	{
		if (const EShaderInstance* Cached = TryFindInstance(InRequest))
		{
			return Cached;
		}
		const EShaderInstance* Instance = CreateMaterialInstanceForRequest(InRequest);
		if (Instance)
		{
			PublishShaderAssetSwap(InRequest.ShaderName);
			PublishShaderAssetSwap(GetEngineErrorShaderName());
		}
		return Instance;
	}

	const EShaderInstance* REngineShaderInstanceCache::CreateInstanceForRequest(const EShaderAssemblyRequest& InRequest)
	{
		/*
		* The load thread's half, and the only place an instance is assembled: the program and
		* material json reads, the generated sources, the compiler and the PAsset loads all hang
		* off this call, which is why no draw may reach it.
		*
		* The key is recomputed from the request rather than passed in, so the two ends of the
		* request cannot disagree about the string. A second request for the same assembly can
		* arrive while this one is queued, so the table is checked once more and the entry is
		* published once.
		*/
		if (const EShaderInstance* Existing = TryFindInstance(InRequest))
		{
			return Existing;
		}

		const EShaderAsset* Asset = nullptr;
		EName VariantName;
		if (!EShaderJobBuilder::EnsureAssembly(InRequest, Asset, VariantName))
		{
			return nullptr;
		}
		// Note what is NOT here: publishing the swap. The assembly may have put a new asset object
		// behind this shader name, and moving the readers onto it is the caller's step - the
		// requested form does it in the request's completion, the synchronous form right after this
		// returns. Both run on this thread, right after the load.
		const EShaderInstance* Instance = CreateShaderInstance(Asset, VariantName.ToString());
		if (!Instance)
		{
			// Not cached on purpose: the callers that retry until their shader is available rely
			// on the next request trying again.
			return nullptr;
		}

		FEntry NewEntry;
		NewEntry.RequestKey	= MakeInstanceRequestKey(InRequest);
		NewEntry.ShaderName	= InRequest.ShaderName;
		NewEntry.Instance	= Instance;
		EScopedLock ScopedLock(TableMutex);
		Instances.Add(NewEntry);
		return Instance;
	}

	void REngineShaderInstanceCache::PublishShaderAssetSwap(const EName& InShaderName)
	{
		// What the manager hands out for this name right now. Null means the assembly failed, so
		// there is nothing to move anyone onto.
		const EShaderAsset* CurrentAsset = EShaderAssetManager::GetManagerSingleton()->FindShaderAsset(InShaderName);
		if (!CurrentAsset)
		{
			return;
		}
		BOOL32 bAnyRebound = FALSE;
		{
			// Under the table lock, because a draw on the render thread walks the same entries.
			EScopedLock ScopedLock(TableMutex);
			for (UINT32 Index = 0u, Num = Instances.Num<UINT32>(); Index < Num; Index++)
			{
				FEntry& Entry = Instances[Index];
				if ((!Entry.Instance) || (Entry.ShaderName != InShaderName))
				{
					continue;
				}
				if (Entry.Instance->GetAsset() == CurrentAsset)
				{
					continue;	// already on the current object
				}
				// The cache owns the instance and the const is only the type it hands out, so the
				// retarget goes through it here - the same reason RefreshProgram casts to resolve a
				// program through a const asset.
				const_cast<EShaderInstance*>(Entry.Instance)->RebindAsset(CurrentAsset);
				bAnyRebound = TRUE;
			}
		}
		if (!bAnyRebound)
		{
			return;
		}
		/*
		* The render thread derived things from the object that was just replaced - pipeline states,
		* binding layouts, the parameter blocks initialized against its programs - and it owns them,
		* so it is the thread that has to drop them. Recording the command here is what "each module
		* marshals to its own thread" looks like in practice: the command queue may be written from
		* any thread, and this one is the load thread.
		*/
		const EName ReplacedShaderName = InShaderName;
		RRenderResourceManager::GetManagerSingleton()->EnqueueCommand([ReplacedShaderName]() -> void
		{
			RSceneRenderer* Renderer = EMainManager::GetManagerSingleton()->GetSceneRenderer();
			if (Renderer)
			{
				Renderer->OnShaderAssetReplaced(ReplacedShaderName);
			}
		});
	}

	const EShaderInstance* REngineShaderInstanceCache::CreateMaterialInstanceForRequest(const EShaderAssemblyRequest& InRequest)
	{
		const EShaderInstance* Instance = CreateInstanceForRequest(InRequest);
		if (Instance)
		{
			return Instance;
		}

		// What failed is this material's half of the program, not the program and not the vertex
		// axis, so the substitute keeps both and changes only the names.
		EShaderAssemblyRequest ErrorRequest = InRequest;
		ErrorRequest.ShaderName = GetEngineErrorShaderName();
		ErrorRequest.MaterialName = GetEngineErrorShaderName().ToString();
		const EShaderInstance* ErrorInstance = CreateInstanceForRequest(ErrorRequest);

		const EString ReportKey = InRequest.MaterialName + EString(PE_TEXT("|")) + InRequest.ProgramName.ToString();
		TArray<EString>& ReportedFailures = GetReportedMaterialFailures();
		if (!ReportedFailures.Contains(ReportKey))
		{
			ReportedFailures.Add(ReportKey);
			// The compile layer reports the failure itself and what a draw does about it; this line
			// is the one that names the material and the program, which is what a reader has and
			// what the log does not otherwise get.
			PE_LOG_ERROR(EString(PE_TEXT("[EngineShaderInstanceCache] The material '")) + InRequest.MaterialName
				+ EString(PE_TEXT("' could not be assembled for '")) + InRequest.ProgramName.ToString()
				+ EString(PE_TEXT("'; objects using it are drawn with M_EngineError.")));
		}
		return ErrorInstance;
	}

}	// PigeonEngine
