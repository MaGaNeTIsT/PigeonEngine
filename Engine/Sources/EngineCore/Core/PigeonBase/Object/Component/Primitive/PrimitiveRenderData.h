#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <Base/DataStructure/BaseType.h>
#include <RenderConfig/RenderConfig.h>

namespace PigeonEngine
{

	class EAssetBase;
	class EShaderInstance;
	class EShaderProgram;

	/*
	* PrimitiveRenderData - what a primitive tells the render tier, and how it travels.
	*
	* Three channels, deliberately separate because they change at different rates:
	*
	*   assets   FPrimitiveRenderAssetReport   what the primitive is made of: its mesh, its
	*                                          skeleton and one shader instance per (material
	*                                          slot, pass). Changes when a pointer changes.
	*   values   FShaderValueReport            named values the primitive computes, keyed by
	*                                          the block / field names the shader source
	*                                          declares. Changes when a number changes.
	*   structure FPrimitiveRenderSnapshot     the identity, the transform and the flags, plus
	*                                          the two reports above when they moved.
	*
	* All three are value types: they are built on the game thread, copied into the frame's
	* command queue and consumed on the render thread, which never reads a game object. That
	* is what keeps the primitive's own members game thread private.
	*
	* The primitive layer declares the SHAPE of what is reported and nothing else. Which
	* shader instances a component holds, and under which pass it reports them, is the
	* subclass's business - a primitive never declares a shader asset member of its own.
	*/

	// What one reported value is, so the render side knows which setter to use. The words in
	// FShaderValueReportEntry are 32 bit and format independent, exactly like the value layer
	// of the shader asset system stores them.
	enum EPrimitiveValueKindType : UINT8
	{
		PRIMITIVE_VALUE_SCALAR		= 0,	// Raw32[0] as float
		PRIMITIVE_VALUE_VECTOR		= 1,	// Raw32[0..3] as floats
		PRIMITIVE_VALUE_MATRIX		= 2,	// Raw32[0..15] as floats, row major, no transpose
		PRIMITIVE_VALUE_UINT		= 3,	// Raw32[0] as uint
		PRIMITIVE_VALUE_UINT4		= 4,	// Raw32[0..3] as uints
		PRIMITIVE_VALUE_COUNT
	};

	static constexpr UINT32 PRIMITIVE_VALUE_WORD_MAX = 16u;

	// The two ways one reported value is read, over the same 32 bits. Which one is live is
	// what Kind says, so a value crosses the thread boundary without a format of its own.
	union FShaderValueWords
	{
	public:
		FLOAT	Float32[PRIMITIVE_VALUE_WORD_MAX];
		UINT32	UInt32[PRIMITIVE_VALUE_WORD_MAX];
	};

	struct FShaderValueReportEntry
	{
	public:
		EName					BlockName;
		EName					FieldName;
		EPrimitiveValueKindType	Kind	= EPrimitiveValueKindType::PRIMITIVE_VALUE_SCALAR;
		FShaderValueWords		Words	= {};
	};

	/*
	* The named values of one primitive, in the order they were reported. A name the shader
	* does not declare is not an error: it becomes a write that binds nothing, which is what
	* lets a shader field be commented out and come back without a C++ change.
	*/
	class FShaderValueReport
	{
	public:
		void	SetScalar(const EName& InBlockName, const EName& InFieldName, const FLOAT InValue);
		void	SetVector4(const EName& InBlockName, const EName& InFieldName, const Vector4& InValue);
		// Row major, no transpose: the shader side matrix is the transpose of this one, which
		// is what the shader library's mul(Matrix, Vector) expects. A field whose name carries
		// a transpose (WorldInvTransposeMatrix) gets it from the caller's math.
		void	SetMatrix(const EName& InBlockName, const EName& InFieldName, const Matrix4x4& InValue);
		void	SetUInt(const EName& InBlockName, const EName& InFieldName, const UINT32 InValue);
		void	SetUInt4(const EName& InBlockName, const EName& InFieldName, const UINT32* InValues);
		void	Clear();
	public:
		PE_NODISCARD BOOL32	IsEmpty() const { return ((Values.Num<UINT32>() == 0u) ? TRUE : FALSE); }
		PE_NODISCARD UINT32	GetValueNum() const { return Values.Num<UINT32>(); }
		PE_NODISCARD const FShaderValueReportEntry*	GetValueByIndex(const UINT32 InIndex) const;
	private:
		PE_NODISCARD FShaderValueReportEntry*	FindOrAdd(const EName& InBlockName, const EName& InFieldName);
	private:
		TArray<FShaderValueReportEntry>	Values;
	};

	// One (pass, instance) pair of one material slot.
	struct FPrimitiveMaterialPassReport
	{
	public:
		EName					PassName;
		const EShaderInstance*	Instance	= nullptr;
	};

	// One material slot of a primitive: the passes its material was assembled for. A slot with
	// no entry at all means "no material", which the render side answers with the engine
	// fallback for the passes the fallback covers.
	struct FPrimitiveMaterialSlotReport
	{
	public:
		TArray<FPrimitiveMaterialPassReport>	Passes;
	public:
		PE_NODISCARD const EShaderInstance*	Find(const EName& InPassName) const;
	};

	// A named asset a primitive reaches for by a role name it and its shader source agree on:
	// a cube map for the sky, a height map for the grass, and so on.
	struct FPrimitiveNamedAssetReport
	{
	public:
		EName			RoleName;
		const EAssetBase*	Asset	= nullptr;
	};

	// A named shader instance a primitive uses outside the material slot model, such as the
	// compute instance of a construction pass.
	struct FPrimitiveNamedInstanceReport
	{
	public:
		EName					RoleName;
		const EShaderInstance*	Instance	= nullptr;
	};

	/*
	* What a primitive is made of. Built on the game thread by the component, consumed on the
	* render thread by its proxy. Every pointer stays a game side asset pointer: the render
	* thread resolves it to a render side object and never owns it.
	*/
	class FPrimitiveRenderAssetReport
	{
	public:
		// The mesh whose render resource supplies the index buffer and the vertex streams, or
		// null for a primitive that generates its own geometry.
		void	SetMeshAsset(const EAssetBase* InAsset) { MeshAsset = InAsset; }
		void	SetSkeletonAsset(const EAssetBase* InAsset) { SkeletonAsset = InAsset; }
		// SlotIndex is the submesh index the slot serves; a mesh with fewer slots than submeshes
		// reuses its last slot, which is the render side's fallback rule and not a reporting one.
		void	SetMaterialInstance(const UINT32 InSlotIndex, const EName& InPassName, const EShaderInstance* InInstance);
		void	SetNamedInstance(const EName& InRoleName, const EShaderInstance* InInstance);
		void	SetNamedAsset(const EName& InRoleName, const EAssetBase* InAsset);
	public:
		PE_NODISCARD const EAssetBase*	GetMeshAsset() const { return MeshAsset; }
		PE_NODISCARD const EAssetBase*	GetSkeletonAsset() const { return SkeletonAsset; }
		PE_NODISCARD UINT32				GetMaterialSlotNum() const { return MaterialSlots.Num<UINT32>(); }
		PE_NODISCARD const FPrimitiveMaterialSlotReport*	GetMaterialSlot(const UINT32 InSlotIndex) const;
		PE_NODISCARD const EShaderInstance*	FindMaterialInstance(const UINT32 InSlotIndex, const EName& InPassName) const;
		PE_NODISCARD const EShaderInstance*	FindNamedInstance(const EName& InRoleName) const;
		PE_NODISCARD const EAssetBase*		FindNamedAsset(const EName& InRoleName) const;
	private:
		const EAssetBase*					MeshAsset		= nullptr;
		const EAssetBase*					SkeletonAsset	= nullptr;
		TArray<FPrimitiveMaterialSlotReport>	MaterialSlots;
		TArray<FPrimitiveNamedInstanceReport>	NamedInstances;
		TArray<FPrimitiveNamedAssetReport>		NamedAssets;
	};

	/*
	* Which channel of a primitive moved. The create snapshot carries every channel, an update
	* snapshot carries only the ones that changed, and the component clears exactly the
	* channels the enqueued snapshot covered - never more, which is what keeps a change from
	* being dropped between two frames.
	*/
	enum EPrimitiveUpdateChannel : UINT32
	{
		PRIMITIVE_UPDATE_NONE		= 0u,
		// The world transform. A static primitive reports it once, at creation.
		PRIMITIVE_UPDATE_TRANSFORM	= (1u << 0u),
		// The named values of the primitive.
		PRIMITIVE_UPDATE_VALUES		= (1u << 1u),
		// The asset report: mesh, skeleton, materials, named assets.
		PRIMITIVE_UPDATE_ASSETS		= (1u << 2u),
	};

	/*
	* One frame's worth of primitive state, by value. The render thread builds the proxy from
	* it on the add command and patches an existing proxy from it on the update command, so no
	* game thread object is ever reached across the boundary.
	*/
	struct FPrimitiveRenderSnapshot
	{
	public:
		// Identity of the owning component. It is what Add / Update / Remove agree on, and it
		// is stable for the component's lifetime.
		ObjectIdentityType		Identity		= 0ull;
		// Registered per primitive subclass; the render side looks the proxy class up by it.
		EName					ProxyTypeName;
		// Structure and flags. The transform travels as its three components rather than as an
		// ETransform so the snapshot stays a plain value type with no accessor dependency.
		Vector3					WorldLocation;
		Quaternion				WorldRotation;
		Vector3					WorldScaling;
		BOOL32					bMovable		= FALSE;
		BOOL32					bCastShadow		= FALSE;
		BOOL32					bReceiveShadow	= FALSE;
		// Payload. An update snapshot only fills the channels its DirtyChannels names.
		FPrimitiveRenderAssetReport	Assets;
		FShaderValueReport			Values;
		// Bone matrices of a skinned primitive, in bone order, and empty for every other kind.
		// They are runtime data rather than an asset, so they travel with the snapshot instead
		// of through the asset report: the report says what the primitive is made of, this says
		// where its bones are this frame.
		TArray<Matrix4x4>		BoneToRootMatrices;
		UINT32					DirtyChannels	= PRIMITIVE_UPDATE_NONE;
	};

} // namespace PigeonEngine
