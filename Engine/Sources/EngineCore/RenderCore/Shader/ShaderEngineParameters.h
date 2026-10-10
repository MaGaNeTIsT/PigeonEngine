#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <ShaderAsset/ShaderParameterSet.h>
#include <Shader/ShaderParameterBuffers.h>

namespace PigeonEngine
{

	class EShaderProgram;
	class IRCommandList;
	class IRRHIDevice;

	/*
	* ShaderAsset - engine parameter publishing.
	*
	* The engine wide blocks (GlobalParams / ViewParams / DrawParams) are declared by the
	* shader source exactly like a material block is, so the C++ side publishes them the
	* same way: by (block, field) name into an EShaderParameterSet, uploaded into the gpu
	* buffers the render tier owns, and bound through the binding plan of whatever program
	* is current. No root index and no register is written down in this file, and a name the
	* shader source does not declare is simply a write that binds nothing.
	*
	* Matrix upload convention, which is the one thing here that cannot be inferred:
	*   - Matrix4x4 stores a DirectXMath row major matrix and the engine transforms a
	*     vector as mul(Vector, Matrix).
	*   - The shader library writes mul(Matrix, Vector) everywhere and is compiled with
	*     column major constant buffer packing, so the matrix the shader sees is the
	*     transpose of the cpu matrix - which is exactly what mul(Matrix, Vector) has to
	*     receive for a row vector cpu matrix.
	*   - Writing the cpu matrix as it is (row major flatten, no transpose) is therefore
	*     the whole conversion. A field whose name carries a transpose
	*     (WorldInvTransposeMatrix) gets it from the caller's math, never from the upload.
	*/

	// OutScalars[Row * 4 + Column] = InMatrix(Row, Column). No transpose.
	extern void	FlattenShaderMatrix(const Matrix4x4& InMatrix, FLOAT* OutScalars);

	// The write helpers return FALSE only for a malformed request (a field the reflection
	// declares as another kind, a null array). A field the compiler culled is not an error:
	// it is a write that binds nothing, which is why a discarded result is not a mistake.
	extern BOOL32	WriteShaderMatrix(EShaderParameterSet& InOutParameters
		, const EName& InBlockName, const EName& InFieldName, const Matrix4x4& InMatrix);
	extern BOOL32	WriteShaderVector4(EShaderParameterSet& InOutParameters
		, const EName& InBlockName, const EName& InFieldName, const Vector4& InValue);
	// For a field with fewer than four components. A float2 fed through WriteShaderVector4
	// carries two values the field has no room for; the set drops them and reports a mismatched
	// write, so the count here is the caller stating how wide the field really is.
	extern BOOL32	WriteShaderFloatArray(EShaderParameterSet& InOutParameters
		, const EName& InBlockName, const EName& InFieldName, const FLOAT* InValues, const UINT32 InScalarNum);
	extern BOOL32	WriteShaderScalar(EShaderParameterSet& InOutParameters
		, const EName& InBlockName, const EName& InFieldName, const FLOAT InValue);
	extern BOOL32	WriteShaderUIntArray(EShaderParameterSet& InOutParameters
		, const EName& InBlockName, const EName& InFieldName, const UINT32* InValues, const UINT32 InScalarNum);

	/*
	* One engine block, published as a unit: the cpu values, the gpu buffers they are
	* uploaded into, and the bind that walks a program's plan.
	*
	* A block is bound to the program it was initialized from: the program's reflection
	* supplies the field layout and the buffer sizes. Every program shares the engine
	* components, so one resolved layout serves all of them, and the block is initialized
	* from the first program that actually declares it. A program that declares no such
	* block leaves it uninitialized rather than latching an empty layout, so a later
	* program can still supply one.
	*
	* One object publishes one block, and the name is its identity: it is taken at
	* construction so an unnamed block cannot exist. That matters because the set, the gpu
	* buffers and the bind are all scoped to this one name - the object uploads only its own
	* block and can only write its own root. An unnamed object would have been inert (no
	* mirror, no buffer, no bind) and silent about it, which is exactly the state this
	* signature exists to make impossible.
	*/
	class RShaderParameterBlock
	{
	public:
		explicit RShaderParameterBlock(const EName& InBlockName);
		// The gpu side is per instance, so a copy carries the name only and starts empty: the
		// copies the engine makes are render proxies copied from a registered prototype, and
		// those rebuild their buffers from the parameter set on first use.
		RShaderParameterBlock(const RShaderParameterBlock& Other);
		~RShaderParameterBlock();
	public:
		PE_NODISCARD const EName&	GetBlockName() const { return BlockName; }
	public:
		// Builds the cpu mirrors and the gpu buffers from the first program that carries
		// this block. FALSE while no such program was seen: the caller then skips its own
		// publish step and tries again with the next program.
		BOOL32	EnsureInitialized(IRRHIDevice* InRenderDevice, const EShaderProgram& InProgram);
		// Releases the gpu buffers and the cpu mirrors and returns the block to its
		// uninitialized state, so the next EnsureInitialized builds both again from whichever
		// program is current then. What an owner that can be shut down and started again calls
		// before the device its buffers were created on goes away.
		void	ReleaseRenderResources();
		PE_NODISCARD BOOL32	IsInitialized() const { return bInitialized; }
		PE_NODISCARD EShaderParameterSet&		GetParameters() { return Parameters; }
		PE_NODISCARD const EShaderParameterSet&	GetParameters() const { return Parameters; }
	public:
		// Re-evaluates what moved and pushes it into the gpu buffers. Cheap when nothing
		// moved: the set reports only the mirrors whose cpu bytes are stale. The set holds
		// this block's mirrors only, so the upload cannot reach another block's buffer.
		BOOL32	Upload(IRRHIDevice* InRenderDevice);
		// Binds this block - and only this block - at the root index the program's plan
		// reports, for every stage the program has. A plan lists every block its stage uses, so
		// the object's own name is passed down as a filter and another block's binding is
		// skipped rather than satisfied from this object's table. A stage whose plan does not
		// carry the block, and a program without it at all, leave earlier bindings untouched.
		void	Bind(IRCommandList* InCommandList, const EShaderProgram& InProgram) const;
	private:
		PE_NODISCARD static BOOL32	HasBlock(const EShaderProgram& InProgram, const EName& InBlockName);
	private:
		EName					BlockName;
		BOOL32					bInitialized;
		EShaderParameterSet		Parameters;
		RShaderParameterBuffers	Buffers;

	public:
		RShaderParameterBlock& operator=(const RShaderParameterBlock&) = delete;

	};

	/*
	* The engine block and field names, in one place. The shader source is the single
	* source of truth for these strings; this table only keeps the C++ side from spelling
	* them out at every call site, and a rename on one side is a compile time change here
	* plus a json change there.
	*/
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BLOCK_GLOBAL_PARAMS	= "GlobalParams";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BLOCK_VIEW_PARAMS	= "ViewParams";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BLOCK_SKY_LIGHT_PARAMS = "SkyLightParams";
	// The environment's brightness and tint, published by the renderer rather than by the sky:
	// the sky owns its own block for its own draw, and this one is read where the baked
	// coefficients are consumed.
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BLOCK_INDIRECT_LIGHTING_PARAMS = "IndirectLightingParams";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BLOCK_DRAW_PARAMS	= "DrawParams";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BLOCK_MATERIAL_PARAMS = "MaterialParams";

	/*
	* The blocks whose publisher is the render tier itself: the renderer's global block, a
	* view's block, a primitive's draw block, and any block a proxy owns. A block object
	* registers the name it was built with, so this table is filled by the code that owns
	* the blocks and the engine block names are never listed a second time.
	*
	* It answers one question, asked by the material path: may this bind write that root?
	* A material supplies its own blocks; a root the render tier binds belongs to its block
	* object, and whoever else is holding that block's mirror (a never written, all zero
	* one) must not push it over the real values.
	*/
	PE_NODISCARD extern BOOL32	IsRenderTierShaderBlock(const EName& InBlockName);
	// Called by RShaderParameterBlock's constructor; nothing else registers a block.
	extern void	RegisterRenderTierShaderBlock(const EName& InBlockName);

	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_TIME_PARAMS				= "TimeParams";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_DELTA_TIME					= "DeltaTime";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_FRAME_INDEX				= "FrameIndex";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_RENDER_FLAGS				= "RenderFlags";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_DIRECTIONAL_LIGHT_COUNT	= "DirectionalLightCount";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_LOCAL_LIGHT_COUNT			= "LocalLightCount";

	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_VIEW_MATRIX				= "ViewMatrix";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_VIEW_INV_MATRIX			= "ViewInvMatrix";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_PROJECTION_MATRIX			= "ProjectionMatrix";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_PROJECTION_INV_MATRIX		= "ProjectionInvMatrix";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_VIEW_PROJECTION_MATRIX		= "ViewProjectionMatrix";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_VIEW_PROJECTION_INV_MATRIX	= "ViewProjectionInvMatrix";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_VIEW_POSITION				= "ViewPosition";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_VIEW_DIRECTION				= "ViewDirection";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_SCREEN_SIZE				= "ScreenSize";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_INV_SCREEN_SIZE			= "InvScreenSize";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_NEAR_FAR_PARAMS			= "NearFarParams";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_VIEWPORT_RECT				= "ViewportRect";
	// Exposure belongs to the view and not to the frame: every camera has its own EV100 and
	// ISO, so the value that turns scene luminance into a display signal is per view.
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_EXPOSURE_PARAMS			= "ExposureParams";

	// The sky is the one light with a block of its own rather than a slot in a light list:
	// it is a fullscreen pass with a single instance, so it needs no array and no index.
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_LIGHT_ADJUST_COLOR			= "LightAdjustColor";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_LIGHT_ADJUST_INTENSITY		= "LightAdjustIntensity";

	// rgb is the sky's tint and w its mean luminance in cd/m2: the pair the bake leaves out of
	// the coefficients on purpose, so that changing either one costs no dispatch.
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_SKY_LIGHT_COLOR_LUMINANCE	= "SkyLightColorLuminance";

	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_WORLD_MATRIX				= "WorldMatrix";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_WORLD_INV_MATRIX			= "WorldInvMatrix";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_WORLD_INV_TRANSPOSE_MATRIX	= "WorldInvTransposeMatrix";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_PRIMITIVE_INDEX			= "PrimitiveIndex";
	PE_CONSTEXPR_STATIC const CHAR* SHADER_FIELD_INSTANCE_COUNT				= "InstanceCount";

} // namespace PigeonEngine
