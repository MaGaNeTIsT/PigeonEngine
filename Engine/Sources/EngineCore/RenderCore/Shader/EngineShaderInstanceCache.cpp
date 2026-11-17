#include "EngineShaderInstanceCache.h"

#include <Asset/AssetLoadManager.h>
#include <Main/MainManager.h>
#include <Renderer/ScenePass.h>
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
			// The indices address the array they were built against, so they go with it. Every
			// instance is owned by an entry now, including the copies of a material.
			InstanceIndicesByHash.Empty();
			EntryIndexByInstance.Empty();
			// A cache that comes back up starts its clone keys over. Nothing from the previous
			// table can survive to collide with them.
			NextOwnedInstanceKey = 1ull;
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

	// FNV-1a over the request fields, in the same order MakeInstanceRequestKey serializes them.
	// The hash is what a lookup compares first; the fields are compared only inside one bucket,
	// so two requests that share a bucket but not a field still answer correctly.
	static UINT64 HashRequestString(UINT64 InHash, const EString& InString) noexcept
	{
		const CHAR* Data = *InString;
		while (*Data != '\0')
		{
			InHash ^= static_cast<UINT64>(static_cast<UINT8>(*Data));
			InHash *= 1099511628211ull;
			Data++;
		}
		return InHash;
	}
	static UINT64 MixRequestHash(UINT64 InHash, const UINT64 InValue) noexcept
	{
		InHash ^= InValue;
		InHash *= 1099511628211ull;
		return InHash;
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
		* exactly what this refactor exists to keep out of a draw. A lookup does not call this:
		* it hashes the same fields (ComputeAssemblyRequestHash) and only a miss builds the key.
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
		RequestKey = RequestKey + EString(PE_TEXT("|")) + ToString(InRequest.InstanceKey);
		return RequestKey;
	}

	UINT64 REngineShaderInstanceCache::ComputeAssemblyRequestHash(const EShaderAssemblyRequest& InRequest) noexcept
	{
		// A name is hashed by its interned index: equal names share the index, and reading the
		// index avoids both a pool read and a second hash of the text.
		UINT64 Hash = 14695981039346656037ull;
		Hash = MixRequestHash(Hash, static_cast<UINT64>(InRequest.ShaderName.GetIndex()));
		Hash = HashRequestString(Hash, InRequest.MaterialName);
		Hash = HashRequestString(Hash, InRequest.DomainName);
		Hash = MixRequestHash(Hash, static_cast<UINT64>(InRequest.ProgramName.GetIndex()));
		Hash = MixRequestHash(Hash, static_cast<UINT64>(InRequest.VertexFactory));
		Hash = MixRequestHash(Hash, static_cast<UINT64>(InRequest.Profile.StreamProfile));
		Hash = MixRequestHash(Hash, static_cast<UINT64>(InRequest.StageMask));
		Hash = MixRequestHash(Hash, static_cast<UINT64>(static_cast<UINT32>(InRequest.SurfaceModeOverride)));
		Hash = MixRequestHash(Hash, InRequest.InstanceKey);
		return Hash;
	}

	BOOL32 REngineShaderInstanceCache::IsSameAssemblyRequest(const EShaderAssemblyRequest& InLeft
		, const EShaderAssemblyRequest& InRight) noexcept
	{
		return ((InLeft.ShaderName == InRight.ShaderName)
			&& (InLeft.MaterialName == InRight.MaterialName)
			&& (InLeft.DomainName == InRight.DomainName)
			&& (InLeft.ProgramName == InRight.ProgramName)
			&& (InLeft.VertexFactory == InRight.VertexFactory)
			&& (InLeft.Profile.StreamProfile == InRight.Profile.StreamProfile)
			&& (InLeft.StageMask == InRight.StageMask)
			&& (InLeft.SurfaceModeOverride == InRight.SurfaceModeOverride)
			&& (InLeft.InstanceKey == InRight.InstanceKey)) ? TRUE : FALSE;
	}

	BOOL32 REngineShaderInstanceCache::FindEntryIndexLocked(const EShaderInstance* InInstance, UINT32& OutIndex) const
	{
		if (!InInstance)
		{
			return FALSE;
		}
		const UINT32* EntryIndex = EntryIndexByInstance.FindValuePtr(InInstance);
		if (!EntryIndex)
		{
			return FALSE;
		}
		OutIndex = *EntryIndex;
		return TRUE;
	}

	const EShaderInstance* REngineShaderInstanceCache::TryFindInstance(const EShaderAssemblyRequest& InRequest) const
	{
		const UINT64 RequestHash = ComputeAssemblyRequestHash(InRequest);
		EScopedLock ScopedLock(TableMutex);
		const TArray<UINT32>* Bucket = InstanceIndicesByHash.FindValuePtr(RequestHash);
		if (!Bucket)
		{
			return nullptr;
		}
		for (UINT32 Index = 0u, Num = Bucket->Num<UINT32>(); Index < Num; Index++)
		{
			const FEntry& Entry = Instances[(*Bucket)[Index]];
			if ((Entry.bIsLookupable != FALSE) && (IsSameAssemblyRequest(Entry.Request, InRequest) != FALSE))
			{
				return Entry.Instance;
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

	const EShaderInstance* REngineShaderInstanceCache::RequirePassInstance(const EShaderInstance& InPrimary, const EName& InPassName)
	{
		// Fast path: the primary's entry and the pass instances already derived from it are both
		// addressed by identity. A draw that repeats a material for another view therefore costs
		// one hash lookup and a short scan of the passes, with no request string built.
		EShaderAssemblyRequest PrimaryRequest;
		{
			EScopedLock ScopedLock(TableMutex);
			UINT32 PrimaryIndex = 0u;
			if (FindEntryIndexLocked(&InPrimary, PrimaryIndex) == FALSE)
			{
				return nullptr;
			}
			FEntry& PrimaryEntry = Instances[PrimaryIndex];
			for (UINT32 Index = 0u, Num = PrimaryEntry.PassInstances.Num<UINT32>(); Index < Num; Index++)
			{
				const FPassInstance& PassInstance = PrimaryEntry.PassInstances[Index];
				if ((PassInstance.PassName == InPassName) && (PassInstance.Instance))
				{
					// The parent link is what makes a value or a texture set on the material reach
					// its pass instance without being copied. It is re-applied here because a
					// material swap may have replaced the set the primary answers with.
					EShaderInstance* MutableInstance = const_cast<EShaderInstance*>(PassInstance.Instance);
					const EShaderParameterSet& PrimaryParameters = const_cast<EShaderInstance&>(InPrimary).GetParameters();
					if (MutableInstance->GetParameters().GetParent() != &PrimaryParameters)
					{
						MutableInstance->GetParameters().SetParent(&PrimaryParameters);
					}
					return PassInstance.Instance;
				}
			}
			// The request is what the derived assembly is built from. Copied here, under the lock
			// that guards the entry, and only on the miss path that is about to build a key.
			PrimaryRequest = PrimaryEntry.Request;
		}
		const FRenderPassDesc* PassDesc = FRenderPassRegistry::FindDescByPassName(InPassName);
		if ((!PassDesc) || (!PassDesc->ProgramName))
		{
			return nullptr;
		}
		EShaderAssemblyRequest DerivedRequest = PrimaryRequest;
		DerivedRequest.ProgramName = EName(PassDesc->ProgramName);
		// A different program needs a different shader name: the assembly cache keys on it, and
		// the two programs carry different stage sources.
		EString DerivedName = PrimaryRequest.ShaderName.ToString();
		DerivedName += EString(PE_TEXT("_"));
		DerivedName += InPassName.ToString();
		DerivedRequest.ShaderName = EName(DerivedName);
		DerivedRequest.StageMask = ((PassDesc->StageMask != 0u)
			? PassDesc->StageMask : (SHADER_STAGE_MASK_VERTEX | SHADER_STAGE_MASK_PIXEL));

		if (const EShaderInstance* Cached = TryFindInstance(DerivedRequest))
		{
			// The instance is assembled; record it on the primary so the next request is a
			// pointer compare. The table may have gained the instance between the lookup and
			// this insert, so the scan is repeated rather than assumed empty.
			{
				EScopedLock ScopedLock(TableMutex);
				UINT32 PrimaryIndex = 0u;
				if (FindEntryIndexLocked(&InPrimary, PrimaryIndex) != FALSE)
				{
					FEntry& PrimaryEntry = Instances[PrimaryIndex];
					BOOL32 bAlreadyCached = FALSE;
					for (UINT32 Index = 0u, Num = PrimaryEntry.PassInstances.Num<UINT32>(); Index < Num; Index++)
					{
						if (PrimaryEntry.PassInstances[Index].PassName == InPassName)
						{
							PrimaryEntry.PassInstances[Index].Instance = Cached;
							bAlreadyCached = TRUE;
							break;
						}
					}
					if (bAlreadyCached == FALSE)
					{
						FPassInstance& PassInstance = PrimaryEntry.PassInstances.AddDefaultGetRef();
						PassInstance.PassName = InPassName;
						PassInstance.Instance = Cached;
					}
				}
			}
			// The parent link is what makes a value or a texture set on the material reach its pass
			// instance without being copied. It is re-applied here because a material swap may have
			// replaced the set the primary answers with.
			EShaderInstance* MutableInstance = const_cast<EShaderInstance*>(Cached);
			const EShaderParameterSet& PrimaryParameters = const_cast<EShaderInstance&>(InPrimary).GetParameters();
			if (MutableInstance->GetParameters().GetParent() != &PrimaryParameters)
			{
				MutableInstance->GetParameters().SetParent(&PrimaryParameters);
			}
			return Cached;
		}
		// Not assembled yet: record the work for the load thread and answer with nothing, the same
		// contract RequireInstance has. The draw that asked is skipped this frame.
		EAssetLoadManager::GetManagerSingleton()->RequestLoad(MakeInstanceRequestKey(DerivedRequest)
			, [DerivedRequest]() -> BOOL32
			{
				return ((REngineShaderInstanceCache::GetManagerSingleton()->CreateInstanceForRequest(DerivedRequest) != nullptr) ? TRUE : FALSE);
			}
			, [DerivedRequest](const BOOL32 InIsLoaded) -> void
			{
				(void)InIsLoaded;
				REngineShaderInstanceCache::GetManagerSingleton()->PublishShaderAssetSwap(DerivedRequest.ShaderName);
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
		* The request hash is recomputed from the request rather than passed in, so the two ends
		* of the request cannot disagree about the fields. A second request for the same assembly
		* can arrive while this one is queued, so the table is checked once more and the entry is
		* published once.
		*/
		if (const EShaderInstance* Existing = TryFindInstance(InRequest))
		{
			return Existing;
		}

		const EShaderAsset* Asset = nullptr;
		EName VariantName;
		BOOL32 bIsTwoSided = FALSE;
		if (!EShaderJobBuilder::EnsureAssembly(InRequest, Asset, VariantName, bIsTwoSided))
		{
			return nullptr;
		}
		// Note what is NOT here: publishing the swap. The assembly may have put a new asset object
		// behind this shader name, and moving the readers onto it is the caller's step - the
		// requested form does it in the request's completion, the synchronous form right after this
		// returns. Both run on this thread, right after the load.
		EShaderInstance* NewInstance = CreateShaderInstance(Asset, VariantName.ToString());
		if (!NewInstance)
		{
			// Not cached on purpose: the callers that retry until their shader is available rely
			// on the next request trying again.
			return nullptr;
		}
		// The material's rasterizer trait is part of what the draw reads from the instance, so it
		// is set before the entry is published below. A material with no such flag keeps the
		// default trait, which is the single sided one.
		EShaderRenderTraits Traits;
		Traits.bTwoSided = bIsTwoSided;
		NewInstance->SetTraits(Traits);

		FEntry NewEntry;
		NewEntry.Request	= InRequest;
		NewEntry.ShaderName	= InRequest.ShaderName;
		NewEntry.Instance	= NewInstance;
		NewEntry.bIsLookupable = TRUE;
		EScopedLock ScopedLock(TableMutex);
		const UINT64 RequestHash = ComputeAssemblyRequestHash(InRequest);
		const UINT32 NewIndex = Instances.Num<UINT32>();
		Instances.Add(NewEntry);
		InstanceIndicesByHash[RequestHash].Add(NewIndex);
		EntryIndexByInstance.Add(NewInstance, NewIndex);
		return NewInstance;
	}

	EShaderInstance* REngineShaderInstanceCache::CreateOwnedInstance(const EShaderInstance& InSource)
	{
		const EShaderAsset* SourceAsset = InSource.GetAsset();
		if (!SourceAsset)
		{
			return nullptr;
		}
		// The source's request is what a pass instance is derived from, so a copy of an instance
		// that did not come from this cache cannot be registered as one: it has no request to
		// derive and no shader name to be re-pointed under a reload.
		EShaderAssemblyRequest SourceRequest;
		EName SourceShaderName;
		BOOL32 bHasSourceEntry = FALSE;
		{
			EScopedLock ScopedLock(TableMutex);
			UINT32 SourceIndex = 0u;
			if (FindEntryIndexLocked(&InSource, SourceIndex) != FALSE)
			{
				SourceRequest	= Instances[SourceIndex].Request;
				SourceShaderName = Instances[SourceIndex].ShaderName;
				bHasSourceEntry	= TRUE;
			}
		}
		if (bHasSourceEntry == FALSE)
		{
			return nullptr;
		}
		// Same asset, same variant, so this resolves the program the source already resolved;
		// what the copy gains is its own parameter set, which is the whole point.
		EShaderInstance* Instance = CreateShaderInstance(SourceAsset, InSource.GetVariantName().ToString());
		if (!Instance)
		{
			return nullptr;
		}
		// The copy is a different instance of the same material, so it starts with the source's
		// render traits - the pipeline state a material asks for, not its values. A caller that
		// wants one material drawn both ways sets the trait on its own copy rather than on the
		// shared instance every other user of the material sees.
		Instance->SetTraits(InSource.GetTraits());
		// Recorded in the same table as the shared instances so ShutDown releases it with the
		// rest and a shader reload re-points it too. It is not lookupable by request: every copy
		// of a material answers to the same request fields, so only the instance pointer names it.
		EScopedLock ScopedLock(TableMutex);
		FEntry NewEntry;
		NewEntry.ShaderName	= SourceShaderName;
		NewEntry.Request	= SourceRequest;
		NewEntry.Request.InstanceKey = NextOwnedInstanceKey;
		NewEntry.Instance	= Instance;
		NewEntry.bIsLookupable = FALSE;
		NextOwnedInstanceKey++;
		if (NextOwnedInstanceKey == 0ull)
		{
			// Zero is the "shared instance" key, so the counter never hands it out again.
			NextOwnedInstanceKey = 1ull;
		}
		const UINT32 NewIndex = Instances.Num<UINT32>();
		Instances.Add(NewEntry);
		EntryIndexByInstance.Add(Instance, NewIndex);
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
			// The editor asset previews are renderers of their own, with their own pipeline
			// states and parameter blocks. They are told here so a reload reaches them in the
			// same frame it reaches the window.
			EMainManager::GetManagerSingleton()->NotifyFrameRenderersShaderReplaced(ReplacedShaderName);
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
