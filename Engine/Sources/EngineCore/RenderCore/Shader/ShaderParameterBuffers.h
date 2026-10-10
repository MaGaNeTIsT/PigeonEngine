#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderCommon.h>
#include <RenderResource.h>
#include <ShaderAsset/ShaderParameterSet.h>

namespace PigeonEngine
{

	class IRRHIDevice;

/*
* ShaderAsset - render tier counterpart of the value layer.
*
* The asset side owns the values (EShaderParameterSet keeps one cpu mirror per
* (stage, block)); this owns the constant buffers they are uploaded into, one buffer
* per mirror, and is created, uploaded and destroyed on the render thread.
*
* The (stage, block) key comes from the mirror itself, so this never reaches into the
* set's container: it walks the read only accessors the set exposes. A dirty flag is
* the whole staleness protocol - the asset side marks a mirror dirty when it writes a
* value, the render side uploads the dirty ones and clears the flag.
*
* A lookup by (stage, block) falls back to a buffer of the same block in another stage,
* because the stage set of a block is a property of the program that first declared it,
* not of the block: the view block is laid out from a mesh program whose vertex stage
* reads it, while the sky and lighting programs read the same block in their pixel
* stage. Both stages see the same values - a block publisher writes fields, not stages -
* so the stage whose buffer is missing binds the one that exists. A block that is
* genuinely per stage (none today) would need its own publisher rather than a second
* buffer here.
*
* Entries are heap allocated because RBufferResource has to keep one address for the
* whole lookup: the entries array holds pointers, not values.
*/

	class RShaderParameterBuffers : public RRenderResourceInterface
	{
	public:
		RShaderParameterBuffers();
		virtual ~RShaderParameterBuffers() override;
	public:
		// A copy starts with an empty table. The entries own GPU buffers, so two tables
		// sharing them would release the same buffer twice; the copies the engine makes are
		// render proxies copied by class-registration boilerplate, and those rebuild their
		// buffers from the parameter set on first use. Assignment drops the current table
		// the same way.
		RShaderParameterBuffers(const RShaderParameterBuffers& Other);
		RShaderParameterBuffers& operator=(const RShaderParameterBuffers& Other);
	public:
		// One buffer per (stage, block). Built on the render thread from the set's CPU mirrors.
		BOOL32  CreateFromParameterSet(IRRHIDevice* InRenderDevice, const EShaderParameterSet& InParameters);
		BOOL32  UploadFromParameterSet(IRRHIDevice* InRenderDevice, const EShaderParameterSet& InParameters);
		PE_NODISCARD const RBufferResource* FindBuffer(const EShaderStageType InStage, const EName& InBlockName) const;
		// The IRRHIBuffer to hand to IRCommandList. Null when the block has no buffer here,
		// which is what lets a caller bind the blocks it has and skip the rest.
		PE_NODISCARD IRRHIBuffer* GetResolvedBuffer(const EShaderStageType InStage, const EName& InBlockName) const;
		// CPU image of a block the shader source marked 'RootConstant'. Such a block is fed
		// through root 32 bit constants instead of being read from a buffer, so its bytes have
		// to stay reachable here. Returns FALSE when the block is not a root constant one, has
		// no value yet, or its size is not a whole number of 32 bit values.
		PE_NODISCARD BOOL32 GetRootConstantData(const EShaderStageType InStage, const EName& InBlockName
			, const BYTE*& OutData, UINT32& OutNum32BitValues) const;
	public:
		virtual BOOL8 IsRenderResourceValid() const noexcept override;
		virtual BOOL8 InitRenderResource() noexcept override;
		virtual void  ReleaseRenderResource() noexcept override;
	private:
		struct FEntry
		{
			EShaderStageType	Stage;
			EName				BlockName;
			RBufferResource		Buffer;
			// TRUE when the reflection marked this block 'RootConstant'. The buffer is still
			// created - the D3D11 back-end has no root constants and binds it as a plain CB -
			// but the platforms that do use RootConstantData instead.
			BOOL32				bIsRootConstant = FALSE;
			// Latest CPU bytes of a root constant block, kept current by the upload path.
			TArray<BYTE>		RootConstantData;
		};
		PE_NODISCARD FEntry*	FindEntry(const EShaderStageType InStage, const EName& InBlockName) const;
		// Same table, the stage left out: the fallback when the stage the caller asks for has
		// no mirror of the block (see the note above).
		PE_NODISCARD FEntry*	FindEntryOfBlock(const EName& InBlockName) const;
		// Creates the constant buffer of one mirror. Returns null when the mirror is empty
		// or the device refused the buffer; the caller owns the result.
		PE_NODISCARD FEntry*	CreateEntry(IRRHIDevice* InRenderDevice, const EShaderParameterBuffer& InMirror) const;
		// Releases one entry's buffer, drops it from the table and deletes it.
		void	DestroyEntry(FEntry* InEntry);
		// Uploads one mirror into its entry, creating the entry first when the set grew
		// after the table was built and rebuilding it when the block changed size. Clears
		// the mirror's dirty flag on success.
		BOOL32	UploadEntry(IRRHIDevice* InRenderDevice, const EShaderParameterBuffer& InMirror);
	private:
		// Heap allocated: RBufferResource is not copyable, and the entries have to survive
		// the array growing.
		TArray<FEntry*> Entries;

	};

} // namespace PigeonEngine
