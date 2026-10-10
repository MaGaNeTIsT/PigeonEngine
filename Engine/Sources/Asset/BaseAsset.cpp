#include "BaseAsset.h"

#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	// The abstract asset layers register themselves here, one link per layer, rather than
	// having a family registration try to be idempotent for them. There are only a handful of
	// asset types in the engine, so the whole chain is readable by inspection:
	//
	//   EObjectBase  ->  EAssetBase      (this file)
	//   EAssetBase   ->  TBaseAsset<T>   (RegisterBaseAssetClassTypes<T>, BaseAsset.h)
	//   TBaseAsset<T> -> concrete asset  (each asset's own RegisterClassTypes)
	static void RegisterClassTypes()
	{
		RegisterClassType<EAssetBase, EObjectBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	EAssetBase::EAssetBase(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
	)
		: bIsInitialized(FALSE)
		, AssetPath(InAssetPath)
		, AssetName(InAssetName)
		// Cached here because GetAssetFullPathName hands out a reference: building it on every
		// call returned a reference to a temporary.
		, AssetFullPathName(InAssetPath + InAssetName + EEngineSettings::ENGINE_ASSET_NAME_TYPE)
	{
#if _EDITOR_ONLY
		DebugName = InDebugName;
#endif
	}

	EAssetBase::~EAssetBase()
	{
	}

	RAssetRenderBinding::RAssetRenderBinding() noexcept
		: RenderRef()
	{
	}

	RAssetRenderBinding::~RAssetRenderBinding()
	{
		// Hand the proxy back to the render thread; this object never deletes it. The queued
		// command captures the registry reference, not this object, so it runs safely after the
		// binding is gone.
		Release();
	}

	RRenderResourceInterface* RAssetRenderBinding::Resolve() const
	{
		return RRenderResourceManager::GetManagerSingleton()->GetRegistry().Resolve(RenderRef);
	}

	void RAssetRenderBinding::Release()
	{
		if (!RenderRef.IsValid())
		{
			return;
		}
		const FRenderResourceRef TargetRef = RenderRef;
		RenderRef.Reset();
		RRenderResourceManager::GetManagerSingleton()->EnqueueDestroyRef(TargetRef);
	}

} // namespace PigeonEngine
