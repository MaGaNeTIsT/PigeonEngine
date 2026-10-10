#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <MultiThread/Atomic.h>
#include <RenderResourceLifetime.h>
#include <Shader/ShaderRenderTraits.h>
#include <ShaderAsset/ShaderProgram.h>
#include <ShaderAsset/ShaderParameterSet.h>

namespace PigeonEngine
{

	class EShaderAsset;

	/*
	* ShaderAsset - the object the rest of the engine references.
	*
	* Two reference styles, one implementation:
	*   - referencing an EShaderAsset directly uses the asset level default parameter
	*     set, so every user of that asset shares one set of values.
	*   - referencing an EShaderInstance uses the instance's own set, whose parent is
	*     the asset level set. Two instances never share values, and an instance that
	*     overrides nothing keeps following the asset.
	*
	* Traits (the abstract pipeline data) exist on the instance only: a shader asset
	* does not carry them, and this module never interprets them.
	*
	* The program pointer is cached together with the asset revision it was resolved
	* from, so a rebuilt (hot reloaded) asset is picked up automatically on the next
	* use instead of leaving a dangling pointer behind.
	*
	* Values are game thread data and the instance keeps them; the GPU constant buffers
	* they feed are a render tier object. The instance only holds the reference into the
	* registry, reserved here and published by the render thread.
	*/

	class EShaderInstance
	{
	public:
		EShaderInstance(const EShaderAsset* InAsset, const EName& InVariantName);
		~EShaderInstance();
	public:
		PE_NODISCARD const EShaderAsset*	GetAsset() const { return Asset.Load(); }
		PE_NODISCARD const EName&			GetVariantName() const { return VariantName; }
		PE_NODISCARD EShaderParameterSet&		GetParameters() { return Parameters; }
		PE_NODISCARD const EShaderParameterSet&	GetParameters() const { return Parameters; }
		PE_NODISCARD const EShaderRenderTraits&	GetTraits() const { return Traits; }
		PE_NODISCARD UINT32					GetRevision() const { return Revision; }
		// Render thread: the constant buffers built from this instance's values. The caller
		// resolves it through RRenderResourceRegistry; it is invalid before the render thread
		// ran the queued creation and after the instance gave it back.
		PE_NODISCARD FRenderResourceRef	GetParameterBuffersRef() const { return ParameterBuffersRef; }
	public:
		void	SetTraits(const EShaderRenderTraits& InTraits);
		void	ResetTraits();
		void	MarkDirty();
		/*
		* Any thread: points the instance at a different asset object - same name, new object,
		* which is what a reload publishes. Nothing else is touched: the resolved program and the
		* parameter mirrors are re-derived by whoever reads the program next, on the thread that
		* owns them, because this call runs where the load ran.
		*
		* The pointer is atomic for the same reason: the swap happens on the load thread while a
		* draw on the render thread may be reading it, and the instance keeps working against the
		* old object until its own next resolution.
		*/
		void	RebindAsset(const EShaderAsset* InAsset);
	public:
		// Resolves the program for (asset, variant) and rebuilds the parameter set
		// when the asset revision moved on. Returns FALSE when the asset has no such
		// program, or none of its stages is valid.
		BOOL32	RefreshProgram();
		PE_NODISCARD const EShaderProgram*	GetProgram();
		// Same resolution through a const instance: the resolved program is cached, not
		// owned, so reading it is a const operation from the caller's point of view.
		PE_NODISCARD const EShaderProgram*	GetProgram() const;
		/*
		* The asset object the program currently resolved by GetProgram/Prepare came from. A caller
		* that caches that program records this next to it, because it is what tells the two apart
		* after a reload: the program comes from the object that was current when it was resolved,
		* which is not necessarily the object the instance is bound to a moment later.
		*
		* Written by whoever resolves the program, so it is read on that same thread.
		*/
		PE_NODISCARD const EShaderAsset*	GetResolvedAsset() const { return CachedAsset; }
		// Re-evaluates the cpu side values. Binding the result is the render layer's job:
		// it uploads the buffers this call leaves dirty and reads the binding plan.
		BOOL32	Prepare();
		// Same call through a const instance: Prepare only re-evaluates the cpu side caches
		// (program resolution + parameter mirrors), so the caller can treat it as a read.
		BOOL32	Prepare() const;
		PE_NODISCARD BOOL32	IsValid();
	private:
		// Game thread: claims the render tier slot and queues the creation of the constant
		// buffers. The device is fetched by the queued command, which runs on the render
		// thread.
		void	CreateParameterBuffersRenderResource();
		void	ReleaseParameterBuffersRenderResource();
	private:
		// The asset object this instance resolves its program from. Atomic: a reload swaps it on
		// the load thread while a draw may read it.
		TAtomic<const EShaderAsset*>	Asset;
		EName					VariantName;
		EShaderParameterSet		Parameters;
		EShaderRenderTraits		Traits;
		const EShaderProgram*	CachedProgram;
		// The object CachedProgram was resolved from. A reload publishes a new object for the same
		// name, so the program is only still valid when the OBJECT and its revision both match.
		const EShaderAsset*		CachedAsset;
		UINT32					CachedAssetRevision;
		UINT32					Revision;
		// The render tier buffers live in the registry, not here.
		FRenderResourceRef		ParameterBuffersRef;

	public:
		EShaderInstance() = delete;

		CLASS_REMOVE_COPY_BODY(EShaderInstance)

	};

} // namespace PigeonEngine
