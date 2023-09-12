#pragma once

#include <CoreMinimal.h>
#include <RenderConfig/RenderConfig.h>

namespace PigeonEngine
{

	class RRenderResourceInterface
	{
	public:
		RRenderResourceInterface() = default;
		RRenderResourceInterface(const RRenderResourceInterface&) = default;
		virtual ~RRenderResourceInterface() = default;
		RRenderResourceInterface& operator=(const RRenderResourceInterface&) = default;
	public:
		virtual BOOL32 IsRenderResourceValid()const = 0;
		virtual BOOL32 InitRenderResource() = 0;
		virtual void ReleaseRenderResource() = 0;
	};
	struct RBoundingBox
	{
		RBoundingBox(const RBoundingBox& other) noexcept : Anchor(other.Anchor), Dimensions(other.Dimensions) {}
		RBoundingBox(const Vector3& anchor, const Vector3& dimensions) noexcept : Anchor(anchor), Dimensions(dimensions) {}
		Vector3		Anchor;
		Vector3		Dimensions;
	};
	struct RBoundingSphere
	{
		RBoundingSphere(const RBoundingSphere& other) noexcept : Anchor(other.Anchor), Radius(other.Radius) {}
		RBoundingSphere(const Vector3& anchor, const FLOAT& radius) noexcept : Anchor(anchor), Radius(radius) {}
		Vector3		Anchor;
		FLOAT		Radius;
	};
	struct RPerLightInfo
	{
		RPerLightInfo() { ::ZeroMemory(this, sizeof(*this)); }
		RPerLightInfo(const RPerLightInfo& other) : Color(other.Color), Params0(other.Params0), Params1(other.Params1) {}
		DirectX::XMFLOAT4	Color;
		DirectX::XMFLOAT4	Params0;
		DirectX::XMFLOAT4	Params1;
	};
	struct RLightConstantBuffer
	{
		RLightConstantBuffer() { ::ZeroMemory(this, sizeof(*this)); }
		RLightConstantBuffer(const RLightConstantBuffer& other) : LightCount(other.LightCount) { for (USHORT i = 0u; i < RCommonSettings::RENDER_SUPPORT_LIGHT_NUM_MAX; i++) { LightParams[i] = other.LightParams[i]; } }
		DirectX::XMINT4		LightCount;
		RPerLightInfo		LightParams[RCommonSettings::RENDER_SUPPORT_LIGHT_NUM_MAX];
	};
	struct RPerFrameConstantBuffer
	{
		RPerFrameConstantBuffer() { ::ZeroMemory(this, sizeof(*this)); }
		RPerFrameConstantBuffer(const RPerFrameConstantBuffer& other)
			: ViewMatrix(other.ViewMatrix)
			, ViewInvMatrix(other.ViewInvMatrix)
			, ProjectionMatrix(other.ProjectionMatrix)
			, ProjectionInvMatrix(other.ProjectionInvMatrix)
			, ViewProjectionMatrix(other.ViewProjectionMatrix)
			, ViewProjectionInvMatrix(other.ViewProjectionInvMatrix)
			, LightViewProjectionMatrix(other.LightViewProjectionMatrix)
			, TimeParams(other.TimeParams)
			, DepthMultiAdd(other.DepthMultiAdd)
			, ScreenToViewSpaceParams(other.ScreenToViewSpaceParams)
			, CameraViewportMinSizeAndInvBufferSize(other.CameraViewportMinSizeAndInvBufferSize)
			, CameraViewportSizeAndInvSize(other.CameraViewportSizeAndInvSize)
			, CameraViewportRect(other.CameraViewportRect)
			, CameraWorldPosition(other.CameraWorldPosition) {}
		DirectX::XMFLOAT4X4		ViewMatrix;
		DirectX::XMFLOAT4X4		ViewInvMatrix;
		DirectX::XMFLOAT4X4		ProjectionMatrix;
		DirectX::XMFLOAT4X4		ProjectionInvMatrix;
		DirectX::XMFLOAT4X4		ViewProjectionMatrix;
		DirectX::XMFLOAT4X4		ViewProjectionInvMatrix;
		DirectX::XMFLOAT4X4		LightViewProjectionMatrix;
		DirectX::XMFLOAT4		TimeParams;
		DirectX::XMFLOAT4		DepthMultiAdd;
		DirectX::XMFLOAT4		ScreenToViewSpaceParams;
		DirectX::XMFLOAT4		CameraViewportMinSizeAndInvBufferSize;
		DirectX::XMFLOAT4		CameraViewportSizeAndInvSize;
		DirectX::XMFLOAT4		CameraViewportRect;
		DirectX::XMFLOAT4		CameraWorldPosition;
	};
	struct RPerDrawConstantBuffer
	{
		RPerDrawConstantBuffer() { ::ZeroMemory(this, sizeof(*this)); }
		RPerDrawConstantBuffer(const RPerDrawConstantBuffer& other) : WorldMatrix(other.WorldInvMatrix), WorldInvMatrix(other.WorldInvMatrix), WorldInvTransposeMatrix(other.WorldInvTransposeMatrix), CustomParameter(other.CustomParameter) {}
		DirectX::XMFLOAT4X4		WorldMatrix;
		DirectX::XMFLOAT4X4		WorldInvMatrix;
		DirectX::XMFLOAT4X4		WorldInvTransposeMatrix;
		DirectX::XMFLOAT4		CustomParameter;
	};
	struct RPerSkeletonConstantBuffer
	{
		RPerSkeletonConstantBuffer() { ::ZeroMemory(this, sizeof(*this)); }
		RPerSkeletonConstantBuffer(const RPerSkeletonConstantBuffer& other) : SkeletonBoneNum(other.SkeletonBoneNum) { for (USHORT i = 0u; i < RCommonSettings::RENDER_MESH_BONE_NUM_MAX * 2u; i++) { SkeletonMatrix[i] = other.SkeletonMatrix[i]; } }
		DirectX::XMFLOAT4		SkeletonBoneNum;
		DirectX::XMFLOAT4X4		SkeletonMatrix[RCommonSettings::RENDER_MESH_BONE_NUM_MAX * 2u];
	};
	enum RShaderFrequencyType : UINT8
	{
		SHADER_FREQUENCY_COMPUTE	= 0,
		SHADER_FREQUENCY_VERTEX		= 1,
		SHADER_FREQUENCY_PIXEL		= 2,
		SHADER_FREQUENCY_COUNT
	};
	enum RDefaultTextureType : UINT8
	{
		TEXTURE2D_WHITE		= 0,
		TEXTURE2D_BLACK		= 1,
		TEXTURE2D_GRAY		= 2,
		TEXTURE2D_RED		= 3,
		TEXTURE2D_GREEN		= 4,
		TEXTURE2D_BLUE		= 5,
		TEXTURE2D_BUMP		= 6,
		TEXTURE2D_PROPERTY	= 7,
		TEXTURE2D_COUNT
	};
	enum RStencilMaskType : UINT8
	{
		STENCIL_MASK_SKY			= 0x1,
		STENCIL_MASK_LAND			= 0x2,
		STENCIL_MASK_OPAQUE_BASE	= 0x4
	};
	enum RComparisonFunctionType : UINT8
	{
		COMPARISON_NEVER			= 1,
		COMPARISON_LESS				= 2,
		COMPARISON_EQUAL			= 3,
		COMPARISON_LESS_EQUAL		= 4,
		COMPARISON_GREATER			= 5,
		COMPARISON_NOT_EQUAL		= 6,
		COMPARISON_GREATER_EQUAL	= 7,
		COMPARISON_ALWAYS			= 8
	};
	enum RCullModeType : UINT8
	{
		CULL_NONE	= 1,
		CULL_FRONT	= 2,
		CULL_BACK	= 3
	};
	enum RFillModeType : UINT8
	{
		FILL_WIREFRAME	= 2,
		FILL_SOLID		= 3
	};
	struct RRasterizerState
	{
		RRasterizerState() noexcept : CullMode(RCullModeType::CULL_BACK), FillMode(RFillModeType::FILL_SOLID) {}
		RRasterizerState(const RRasterizerState& other) noexcept : CullMode(other.CullMode), FillMode(other.FillMode) {}
		constexpr RRasterizerState(RCullModeType cull = RCullModeType::CULL_BACK, RFillModeType fill = RFillModeType::FILL_SOLID) noexcept : CullMode(cull), FillMode(fill) {}
		RCullModeType	CullMode;
		RFillModeType	FillMode;
	};
	enum RDepthWriteMaskType : UINT8
	{
		DEPTH_WRITE_MASK_ZERO	= 0,
		DEPTH_WRITE_MASK_ALL	= 1
	};
	struct RDepthState
	{
		RDepthState() noexcept : DepthEnable(FALSE), DepthWriteMask(RDepthWriteMaskType::DEPTH_WRITE_MASK_ZERO), DepthFunc(RComparisonFunctionType::COMPARISON_NEVER) {}
		RDepthState(const RDepthState& other) noexcept : DepthEnable(other.DepthEnable), DepthWriteMask(other.DepthWriteMask), DepthFunc(other.DepthFunc) {}
		constexpr RDepthState(RComparisonFunctionType func, RDepthWriteMaskType writeMask = RDepthWriteMaskType::DEPTH_WRITE_MASK_ALL) noexcept : DepthEnable(TRUE), DepthWriteMask(writeMask), DepthFunc(func) {}
		BOOL32						DepthEnable;
		RDepthWriteMaskType			DepthWriteMask;
		RComparisonFunctionType		DepthFunc;
	};
	enum RStencilOperationType : UINT8
	{
		STENCIL_OP_KEEP			= 1,
		STENCIL_OP_ZERO			= 2,
		STENCIL_OP_REPLACE		= 3,
		STENCIL_OP_INCR_SAT		= 4,
		STENCIL_OP_DECR_SAT		= 5,
		STENCIL_OP_INVERT		= 6,
		STENCIL_OP_INCR			= 7,
		STENCIL_OP_DECR			= 8
	};
	struct RStencilStateType
	{
		RStencilStateType() noexcept : StencilFailOp(RStencilOperationType::STENCIL_OP_KEEP), StencilDepthFailOp(RStencilOperationType::STENCIL_OP_KEEP), StencilPassOp(RStencilOperationType::STENCIL_OP_KEEP), StencilFunc(RComparisonFunctionType::COMPARISON_NEVER) {}
		RStencilStateType(const RStencilStateType& other) noexcept : StencilFailOp(other.StencilFailOp), StencilDepthFailOp(other.StencilDepthFailOp), StencilPassOp(other.StencilPassOp), StencilFunc(other.StencilFunc) {}
		constexpr RStencilStateType(RComparisonFunctionType func, RStencilOperationType passOp, RStencilOperationType depthFailOp = RStencilOperationType::STENCIL_OP_KEEP, RStencilOperationType stencilFailOp = RStencilOperationType::STENCIL_OP_KEEP) noexcept : StencilFailOp(stencilFailOp), StencilDepthFailOp(depthFailOp), StencilPassOp(passOp), StencilFunc(func) {}
		RStencilOperationType		StencilFailOp;
		RStencilOperationType		StencilDepthFailOp;
		RStencilOperationType		StencilPassOp;
		RComparisonFunctionType		StencilFunc;
	};
	struct RStencilState
	{
		RStencilState() : StencilEnable(FALSE), StencilReadMask(ENGINE_DEFAULT_STENCIL_READ_MASK), StencilWriteMask(ENGINE_DEFAULT_STENCIL_WRITE_MASK), FrontFace(RStencilStateType(RComparisonFunctionType::COMPARISON_NEVER, RStencilOperationType::STENCIL_OP_KEEP)), BackFace(RStencilStateType(RComparisonFunctionType::COMPARISON_NEVER, RStencilOperationType::STENCIL_OP_KEEP)) {}
		RStencilState(RStencilMaskType readMask, RStencilMaskType writeMask, const RStencilStateType& frontFace, const RStencilStateType& backFace) noexcept : StencilEnable(TRUE), StencilReadMask(static_cast<UINT32>(readMask)), StencilWriteMask(static_cast<UINT32>(writeMask)), FrontFace(frontFace), BackFace(backFace) {}
		RStencilState(const UINT32& readMask, const UINT32& writeMask, const RStencilStateType& frontFace, const RStencilStateType& backFace) noexcept : StencilEnable(TRUE), StencilReadMask(EMath::Min(readMask, 0xffu)), StencilWriteMask(EMath::Min(writeMask, 0xffu)), FrontFace(frontFace), BackFace(backFace) {}
		RStencilState(const RStencilState& other) noexcept : StencilEnable(other.StencilEnable), StencilReadMask(other.StencilReadMask), StencilWriteMask(other.StencilWriteMask), FrontFace(other.FrontFace), BackFace(other.BackFace) {}
		BOOL32				StencilEnable;
		UINT32				StencilReadMask;
		UINT32				StencilWriteMask;
		RStencilStateType	FrontFace;
		RStencilStateType	BackFace;
	};
	enum RBlendOptionType : UINT8
	{
		BLEND_ZERO				= 1,
		BLEND_ONE				= 2,
		BLEND_SRC_COLOR			= 3,
		BLEND_INV_SRC_COLOR		= 4,
		BLEND_SRC_ALPHA			= 5,
		BLEND_INV_SRC_ALPHA		= 6,
		BLEND_DEST_ALPHA		= 7,
		BLEND_INV_DEST_ALPHA	= 8,
		BLEND_DEST_COLOR		= 9,
		BLEND_INV_DEST_COLOR	= 10,
		BLEND_SRC_ALPHA_SAT		= 11,
		BLEND_BLEND_FACTOR		= 12,
		BLEND_INV_BLEND_FACTOR	= 13
	};
	enum RBlendOperationType : UINT8
	{
		BLEND_OP_ADD			= 1,
		BLEND_OP_SUBTRACT		= 2,
		BLEND_OP_REV_SUBTRACT	= 3,
		BLEND_OP_MIN			= 4,
		BLEND_OP_MAX			= 5
	};
	enum RColorWriteMaskType : UINT8
	{
		COLOR_WRITE_MASK_RED	= 0x1,
		COLOR_WRITE_MASK_GREEN	= 0x2,
		COLOR_WRITE_MASK_BLUE	= 0x4,
		COLOR_WRITE_MASK_ALPHA	= 0x8,
		COLOR_WRITE_MASK_ALL	= (((COLOR_WRITE_MASK_RED | COLOR_WRITE_MASK_GREEN) | COLOR_WRITE_MASK_BLUE) | COLOR_WRITE_MASK_ALPHA)
	};
	struct RBlendState
	{
		RBlendState() noexcept : BlendEnable(FALSE), SrcBlend(RBlendOptionType::BLEND_ONE), DstBlend(RBlendOptionType::BLEND_ZERO), BlendOp(RBlendOperationType::BLEND_OP_ADD), SrcBlendAlpha(RBlendOptionType::BLEND_ONE), DstBlendAlpha(RBlendOptionType::BLEND_ZERO), BlendOpAlpha(RBlendOperationType::BLEND_OP_ADD), RenderTargetWriteMask(RColorWriteMaskType::COLOR_WRITE_MASK_ALL) {}
		RBlendState(const RBlendState& other) noexcept : BlendEnable(other.BlendEnable), SrcBlend(other.SrcBlend), DstBlend(other.DstBlend), BlendOp(other.BlendOp), SrcBlendAlpha(other.SrcBlendAlpha), DstBlendAlpha(other.DstBlendAlpha), BlendOpAlpha(other.BlendOpAlpha), RenderTargetWriteMask(other.RenderTargetWriteMask) {}
		constexpr RBlendState(RBlendOptionType src, RBlendOptionType dst, RBlendOperationType op, RBlendOptionType srcA, RBlendOptionType dstA, RBlendOperationType opA, RColorWriteMaskType writeMask = RColorWriteMaskType::COLOR_WRITE_MASK_ALL) noexcept : BlendEnable(TRUE), SrcBlend(src), DstBlend(dst), BlendOp(op), SrcBlendAlpha(srcA), DstBlendAlpha(dstA), BlendOpAlpha(opA), RenderTargetWriteMask(writeMask) {}
		BOOL32					BlendEnable;
		RBlendOptionType		SrcBlend;
		RBlendOptionType		DstBlend;
		RBlendOperationType		BlendOp;
		RBlendOptionType		SrcBlendAlpha;
		RBlendOptionType		DstBlendAlpha;
		RBlendOperationType		BlendOpAlpha;
		RColorWriteMaskType		RenderTargetWriteMask;
	};
	enum RFilterType : UINT8
	{
		FILTER_POINT		= 1,
		FILTER_LINEAR		= 2,
		FILTER_ANISOTROPIC	= 3
	};
	enum RTextureAddressModeType : UINT8
	{
		TEXTURE_ADDRESS_WRAP		= 1,
		TEXTURE_ADDRESS_MIRROR		= 2,
		TEXTURE_ADDRESS_CLAMP		= 3,
		TEXTURE_ADDRESS_BORDER		= 4,
		TEXTURE_ADDRESS_MIRROR_ONCE	= 5
	};
	struct RSamplerState
	{
		RSamplerState() noexcept : Filter(RFilterType::FILTER_LINEAR), AddressU(RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP), AddressV(RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP), AddressW(RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP), MipLODBias(0.f), MaxAnisotropy(1u), ComparisonFunc(RComparisonFunctionType::COMPARISON_ALWAYS), BorderColor(Color4(0.f, 0.f, 0.f, 0.f)), MinLOD(0.f), MaxLOD(PE_FLOAT32_MAX) {}
		RSamplerState(RFilterType filter, RTextureAddressModeType u, RTextureAddressModeType v, RTextureAddressModeType w, const FLOAT& maxLOD = PE_FLOAT32_MAX, const FLOAT& minLOD = 0.f, const UINT32& maxAnisotropy = 1u, const FLOAT& mipLODBias = 0.f, const Color4& borderColor = Color4(0.f, 0.f, 0.f, 0.f), RComparisonFunctionType func = RComparisonFunctionType::COMPARISON_ALWAYS) noexcept : Filter(filter), AddressU(u), AddressV(v), AddressW(w), MipLODBias(mipLODBias), MaxAnisotropy(maxAnisotropy), ComparisonFunc(func), BorderColor(borderColor), MinLOD(minLOD), MaxLOD(maxLOD) {}
		RSamplerState(const RSamplerState& other) noexcept : Filter(other.Filter), AddressU(other.AddressU), AddressV(other.AddressV), AddressW(other.AddressW), MipLODBias(other.MipLODBias), MaxAnisotropy(other.MaxAnisotropy), ComparisonFunc(other.ComparisonFunc), BorderColor(other.BorderColor), MinLOD(other.MinLOD), MaxLOD(other.MaxLOD) {}
		RFilterType					Filter;
		RTextureAddressModeType		AddressU;
		RTextureAddressModeType		AddressV;
		RTextureAddressModeType		AddressW;
		FLOAT						MipLODBias;
		UINT32						MaxAnisotropy;
		RComparisonFunctionType		ComparisonFunc;
		Color4						BorderColor;
		FLOAT						MinLOD;
		FLOAT						MaxLOD;
	};
	enum RBindFlagType : UINT8
	{
		BIND_NONE				= 0,
		BIND_VERTEX_BUFFER		= (1 << 0),
		BIND_INDEX_BUFFER		= (1 << 1),
		BIND_CONSTANT_BUFFER	= (1 << 2),
		BIND_SHADER_RESOURCE	= (1 << 3),
		BIND_STREAM_OUTPUT		= (1 << 4),
		BIND_RENDER_TARGET		= (1 << 5),
		BIND_DEPTH_STENCIL		= (1 << 6),
		BIND_UNORDERED_ACCESS	= (1 << 7)
	};
	enum RResourceMiscFlagType : UINT8
	{
		RESOURCE_MISC_NONE					= 0,
		RESOURCE_MISC_GENERATE_MIPS			= (1 << 0),
		RESOURCE_MISC_TEXTURECUBE			= (1 << 1),
		RESOURCE_MISC_DRAWINDIRECT_ARGS		= (1 << 2),
		RESOURCE_MISC_BUFFER_STRUCTURED		= (1 << 3),
		RESOURCE_MISC_RESOURCE_CLAMP		= (1 << 4)
	};
	enum RUsageFlagType : UINT8
	{
		USAGE_DEFAULT		= 0,
		USAGE_IMMUTABLE		= 1,
		USAGE_DYNAMIC		= 2,
		USAGE_STAGING		= 3
	};
	enum RCPUAccessFlagType : UINT8
	{
		CPU_ACCESS_DEFAULT	= 0,
		CPU_ACCESS_WRITE	= (1 << 0),
		CPU_ACCESS_READ		= (1 << 1)
	};
	enum RUAVFlagType : UINT8
	{
		UAV_FLAG_NONE		= 0,
		UAV_FLAG_RAW		= (1 << 0),
		UAV_FLAG_APPEND		= (1 << 1),
		UAV_FLAG_COUNTER	= (1 << 2)
	};
	struct RBufferDesc
	{
		RBufferDesc()noexcept : ByteWidth(0u), Usage(RUsageFlagType::USAGE_DEFAULT), BindFlags(RBindFlagType::BIND_NONE), CPUAccessFlags(RCPUAccessFlagType::CPU_ACCESS_DEFAULT), MiscFlags(RResourceMiscFlagType::RESOURCE_MISC_NONE), StructureByteStride(0u) {}
		RBufferDesc(const RBufferDesc& other) noexcept : ByteWidth(other.ByteWidth), Usage(other.Usage), BindFlags(other.BindFlags), CPUAccessFlags(other.CPUAccessFlags), MiscFlags(other.MiscFlags), StructureByteStride(other.StructureByteStride) {}
		constexpr RBufferDesc(const UINT32& byteWidth, RBindFlagType bindFlag, const UINT32& structureByteStride, RResourceMiscFlagType miscFlag = RResourceMiscFlagType::RESOURCE_MISC_NONE, RUsageFlagType usage = RUsageFlagType::USAGE_DEFAULT, RCPUAccessFlagType cpuAccessFlag = RCPUAccessFlagType::CPU_ACCESS_DEFAULT) noexcept : ByteWidth(byteWidth), Usage(usage), BindFlags(bindFlag), CPUAccessFlags(cpuAccessFlag), MiscFlags(miscFlag), StructureByteStride(structureByteStride) {}
		UINT32					ByteWidth;
		RUsageFlagType			Usage;
		RBindFlagType			BindFlags;
		RCPUAccessFlagType		CPUAccessFlags;
		RResourceMiscFlagType	MiscFlags;
		UINT32					StructureByteStride;
	};
	struct RSubresourceDataDesc
	{
		RSubresourceDataDesc() noexcept : pSysMem(nullptr), SysMemPitch(0u), SysMemSlicePitch(0u) {}
		RSubresourceDataDesc(const RSubresourceDataDesc& other) noexcept : pSysMem(other.pSysMem), SysMemPitch(other.SysMemPitch), SysMemSlicePitch(other.SysMemSlicePitch) {}
		constexpr RSubresourceDataDesc(const void* mem, const UINT32& sysMemPitch, const UINT32& sysMemSlicePitch) noexcept : pSysMem(mem), SysMemPitch(sysMemPitch), SysMemSlicePitch(sysMemSlicePitch) {}
		const void*		pSysMem;
		UINT32			SysMemPitch;
		UINT32			SysMemSlicePitch;
	};
	enum RFormatType : UINT32
	{
		FORMAT_UNKNOWN						= 0,
		FORMAT_R32G32B32A32_TYPELESS		= 1,
		FORMAT_R32G32B32A32_FLOAT			= 2,
		FORMAT_R32G32B32A32_UINT			= 3,
		FORMAT_R32G32B32A32_SINT			= 4,
		FORMAT_R32G32B32_TYPELESS			= 5,
		FORMAT_R32G32B32_FLOAT				= 6,
		FORMAT_R32G32B32_UINT				= 7,
		FORMAT_R32G32B32_SINT				= 8,
		FORMAT_R16G16B16A16_TYPELESS		= 9,
		FORMAT_R16G16B16A16_FLOAT			= 10,
		FORMAT_R16G16B16A16_UNORM			= 11,
		FORMAT_R16G16B16A16_UINT			= 12,
		FORMAT_R16G16B16A16_SNORM			= 13,
		FORMAT_R16G16B16A16_SINT			= 14,
		FORMAT_R32G32_TYPELESS				= 15,
		FORMAT_R32G32_FLOAT					= 16,
		FORMAT_R32G32_UINT					= 17,
		FORMAT_R32G32_SINT					= 18,
		FORMAT_R10G10B10A2_TYPELESS			= 23,
		FORMAT_R10G10B10A2_UNORM			= 24,
		FORMAT_R10G10B10A2_UINT				= 25,
		FORMAT_R11G11B10_FLOAT				= 26,
		FORMAT_R8G8B8A8_TYPELESS			= 27,
		FORMAT_R8G8B8A8_UNORM				= 28,
		FORMAT_R8G8B8A8_UNORM_SRGB			= 29,
		FORMAT_R8G8B8A8_UINT				= 30,
		FORMAT_R8G8B8A8_SNORM				= 31,
		FORMAT_R8G8B8A8_SINT				= 32,
		FORMAT_R16G16_TYPELESS				= 33,
		FORMAT_R16G16_FLOAT					= 34,
		FORMAT_R16G16_UNORM					= 35,
		FORMAT_R16G16_UINT					= 36,
		FORMAT_R16G16_SNORM					= 37,
		FORMAT_R16G16_SINT					= 38,
		FORMAT_R32_TYPELESS					= 39,
		FORMAT_R32_FLOAT					= 41,
		FORMAT_R32_UINT						= 42,
		FORMAT_R32_SINT						= 43,
		FORMAT_R24G8_TYPELESS				= 44,
		FORMAT_R8G8_TYPELESS				= 48,
		FORMAT_R8G8_UNORM					= 49,
		FORMAT_R8G8_UINT					= 50,
		FORMAT_R8G8_SNORM					= 51,
		FORMAT_R8G8_SINT					= 52,
		FORMAT_R16_TYPELESS					= 53,
		FORMAT_R16_FLOAT					= 54,
		FORMAT_R16_UNORM					= 56,
		FORMAT_R16_UINT						= 57,
		FORMAT_R16_SNORM					= 58,
		FORMAT_R16_SINT						= 59,
		FORMAT_R8_TYPELESS					= 60,
		FORMAT_R8_UNORM						= 61,
		FORMAT_R8_UINT						= 62,
		FORMAT_R8_SNORM						= 63,
		FORMAT_R8_SINT						= 64,
		FORMAT_A8_UNORM						= 65,
		FORMAT_R1_UNORM						= 66,
		FORMAT_R9G9B9E5_SHAREDEXP			= 67,
		FORMAT_R8G8_B8G8_UNORM				= 68,
		FORMAT_G8R8_G8B8_UNORM				= 69,
		FORMAT_BC1_TYPELESS					= 70,
		FORMAT_BC1_UNORM					= 71,
		FORMAT_BC1_UNORM_SRGB				= 72,
		FORMAT_BC2_TYPELESS					= 73,
		FORMAT_BC2_UNORM					= 74,
		FORMAT_BC2_UNORM_SRGB				= 75,
		FORMAT_BC3_TYPELESS					= 76,
		FORMAT_BC3_UNORM					= 77,
		FORMAT_BC3_UNORM_SRGB				= 78,
		FORMAT_BC4_TYPELESS					= 79,
		FORMAT_BC4_UNORM					= 80,
		FORMAT_BC4_SNORM					= 81,
		FORMAT_BC5_TYPELESS					= 82,
		FORMAT_BC5_UNORM					= 83,
		FORMAT_BC5_SNORM					= 84,
		FORMAT_B5G6R5_UNORM					= 85,
		FORMAT_B5G5R5A1_UNORM				= 86,
		FORMAT_B8G8R8A8_UNORM				= 87,
		FORMAT_B8G8R8X8_UNORM				= 88,
		FORMAT_R10G10B10_XR_BIAS_A2_UNORM	= 89,
		FORMAT_B8G8R8A8_TYPELESS			= 90,
		FORMAT_B8G8R8A8_UNORM_SRGB			= 91,
		FORMAT_B8G8R8X8_TYPELESS			= 92,
		FORMAT_B8G8R8X8_UNORM_SRGB			= 93,
		FORMAT_BC6H_TYPELESS				= 94,
		FORMAT_BC6H_UF16					= 95,
		FORMAT_BC6H_SF16					= 96,
		FORMAT_BC7_TYPELESS					= 97,
		FORMAT_BC7_UNORM					= 98,
		FORMAT_BC7_UNORM_SRGB				= 99,
		FORMAT_AYUV							= 100,
		FORMAT_Y410							= 101,
		FORMAT_Y416							= 102,
		FORMAT_NV12							= 103,
		FORMAT_P010							= 104,
		FORMAT_P016							= 105,
		FORMAT_420_OPAQUE					= 106,
		FORMAT_YUY2							= 107,
		FORMAT_Y210							= 108,
		FORMAT_Y216							= 109,
		FORMAT_NV11							= 110,
		FORMAT_AI44							= 111,
		FORMAT_IA44							= 112,
		FORMAT_P8							= 113,
		FORMAT_A8P8							= 114,
		FORMAT_B4G4R4A4_UNORM				= 115,
		FORMAT_P208							= 130,
		FORMAT_V208							= 131,
		FORMAT_V408							= 132,
		FORMAT_FORCE_UINT					= 0xffffffff
	};
	struct RTextureSampleDesc
	{
		RTextureSampleDesc() noexcept : Count(1u), Quality(0u) {}
		RTextureSampleDesc(const RTextureSampleDesc& other) noexcept : Count(other.Count), Quality(other.Quality) {}
		constexpr RTextureSampleDesc(const UINT32& count, const UINT32& quality) noexcept : Count(count), Quality(quality) {}
		UINT32	Count;
		UINT32	Quality;
	};
	struct RTextureDesc
	{
		RTextureDesc(
			const UINT32& width, const UINT32& height, UINT8 bindFlags, RFormatType bufferFormat, const RFormatType* srvFormat = nullptr
			, const RFormatType* rtvFormat = nullptr, const RFormatType* uavFormat = nullptr, const UINT32& depth = 0u, const UINT32& mipLevels = 1u
			, const UINT32& arraySize = 1u, RUsageFlagType usageFlag = RUsageFlagType::USAGE_DEFAULT, RCPUAccessFlagType cpuAccessFlags = RCPUAccessFlagType::CPU_ACCESS_DEFAULT
			, RResourceMiscFlagType miscFlags = RResourceMiscFlagType::RESOURCE_MISC_NONE, RTextureSampleDesc sampleDesc = RTextureSampleDesc(1u, 0u)) noexcept
			: Width(width), Height(height), Depth(depth), MipLevels(mipLevels), ArraySize(arraySize), BufferFormat(bufferFormat), SampleDesc(sampleDesc)
			, UsageFlag(usageFlag), BindFlags(bindFlags), CPUAccessFlags(cpuAccessFlags), MiscFlags(miscFlags)
		{
			SRVFormat = srvFormat ? (*srvFormat) : bufferFormat;
			RTVFormat = rtvFormat ? (*rtvFormat) : bufferFormat;
			UAVFormat = uavFormat ? (*uavFormat) : bufferFormat;
		}
		RTextureDesc(const RTextureDesc& other) noexcept
			: Width(other.Width), Height(other.Height), Depth(other.Depth), MipLevels(other.MipLevels)
			, ArraySize(other.ArraySize), BufferFormat(other.BufferFormat), SRVFormat(other.SRVFormat)
			, RTVFormat(other.RTVFormat), UAVFormat(other.UAVFormat), SampleDesc(other.SampleDesc)
			, UsageFlag(other.UsageFlag), BindFlags(other.BindFlags), CPUAccessFlags(other.CPUAccessFlags)
			, MiscFlags(other.MiscFlags) {}
		UINT32					Width;
		UINT32					Height;
		UINT32					Depth;
		UINT32					MipLevels;
		UINT32					ArraySize;
		RFormatType				BufferFormat;
		RFormatType				SRVFormat;
		RFormatType				RTVFormat;
		RFormatType				UAVFormat;
		RTextureSampleDesc		SampleDesc;
		RUsageFlagType			UsageFlag;
		UINT8					BindFlags;
		RCPUAccessFlagType		CPUAccessFlags;
		RResourceMiscFlagType	MiscFlags;
	};
	struct RStructuredBufferDesc
	{
		RStructuredBufferDesc() noexcept : GPUWritable(FALSE), AccessFlags(RCPUAccessFlagType::CPU_ACCESS_DEFAULT), StructureSize(0u), FirstElement(0u), NumElements(0u), UAVFlags(RUAVFlagType::UAV_FLAG_NONE) {}
		RStructuredBufferDesc(const RStructuredBufferDesc& other) noexcept : GPUWritable(other.GPUWritable), AccessFlags(other.AccessFlags), StructureSize(other.StructureSize), FirstElement(other.FirstElement), NumElements(other.NumElements), UAVFlags(other.UAVFlags) {}
		constexpr RStructuredBufferDesc(const UINT32& structureSize, const UINT32& numElements, const BOOL32& writableGPU = FALSE, RCPUAccessFlagType accessFlags = RCPUAccessFlagType::CPU_ACCESS_DEFAULT, const UINT32& firstElement = 0u, RUAVFlagType uavFlags = RUAVFlagType::UAV_FLAG_NONE) noexcept : GPUWritable(writableGPU), AccessFlags(accessFlags), StructureSize(structureSize), FirstElement(firstElement), NumElements(numElements), UAVFlags(uavFlags) {}
		BOOL32					GPUWritable;
		RCPUAccessFlagType		AccessFlags;
		UINT32					StructureSize;
		UINT32					FirstElement;
		UINT32					NumElements;
		RUAVFlagType			UAVFlags;
	};
	enum RInputClassificationType : UINT8
	{
		INPUT_PER_VERTEX_DATA		= 0,
		INPUT_PER_INSTANCE_DATA		= 1
	};


#define SHADER_SEMANTIC_DEFINE(Name)\
	SHADER_SEMANTIC##Name##0 =  SHADER_SEMANTIC##Name + 0,\
	SHADER_SEMANTIC##Name##1 =  SHADER_SEMANTIC##Name + 1,\
	SHADER_SEMANTIC##Name##2 =  SHADER_SEMANTIC##Name + 2,\
	SHADER_SEMANTIC##Name##3 =  SHADER_SEMANTIC##Name + 3,\
	SHADER_SEMANTIC##Name##4 =  SHADER_SEMANTIC##Name + 4,\
	SHADER_SEMANTIC##Name##5 =  SHADER_SEMANTIC##Name + 5,\
	SHADER_SEMANTIC##Name##6 =  SHADER_SEMANTIC##Name + 6,\
	SHADER_SEMANTIC##Name##7 =  SHADER_SEMANTIC##Name + 7,\
	SHADER_SEMANTIC##Name##8 =  SHADER_SEMANTIC##Name + 8,\
	SHADER_SEMANTIC##Name##9 =  SHADER_SEMANTIC##Name + 9,\
	SHADER_SEMANTIC##Name##10 =  SHADER_SEMANTIC##Name + 10,\
	SHADER_SEMANTIC##Name##11 =  SHADER_SEMANTIC##Name + 11,\
	SHADER_SEMANTIC##Name##12 =  SHADER_SEMANTIC##Name + 12,\
	SHADER_SEMANTIC##Name##13 =  SHADER_SEMANTIC##Name + 13,\
	SHADER_SEMANTIC##Name##14 =  SHADER_SEMANTIC##Name + 14,\
	SHADER_SEMANTIC##Name##15 =  SHADER_SEMANTIC##Name + 15\


	enum RShaderSemanticType : UINT16
	{
		SHADER_SEMANTIC_NONE			= 0,
		SHADER_SEMANTIC_PSIZE			= (1 << 6),
		SHADER_SEMANTIC_POSITIONT		= (1 << 7),
		SHADER_SEMANTIC_BINORMAL		= (1 << 8),
		SHADER_SEMANTIC_BLENDWEIGHT		= (1 << 9),
		SHADER_SEMANTIC_BLENDINDICES	= (1 << 10),
		SHADER_SEMANTIC_COLOR			= (1 << 11),
		SHADER_SEMANTIC_TANGENT			= (1 << 12),
		SHADER_SEMANTIC_NORMAL			= (1 << 13),
		SHADER_SEMANTIC_POSITION		= (1 << 14),
		SHADER_SEMANTIC_TEXCOORD		= (1 << 15),
		SHADER_SEMANTIC_DEFINE(_PSIZE),
		SHADER_SEMANTIC_DEFINE(_POSITIONT),
		SHADER_SEMANTIC_DEFINE(_BINORMAL),
		SHADER_SEMANTIC_DEFINE(_BLENDWEIGHT),
		SHADER_SEMANTIC_DEFINE(_BLENDINDICES),
		SHADER_SEMANTIC_DEFINE(_COLOR),
		SHADER_SEMANTIC_DEFINE(_TANGENT),
		SHADER_SEMANTIC_DEFINE(_NORMAL),
		SHADER_SEMANTIC_DEFINE(_POSITION),
		SHADER_SEMANTIC_DEFINE(_TEXCOORD)
	};


#undef SHADER_SEMANTIC_DEFINE


	struct RInputLayoutDesc
	{
		RInputLayoutDesc() noexcept : SemanticName(RShaderSemanticType::SHADER_SEMANTIC_NONE), SemanticIndex(0u), InputSlot(0u), InputSlotClass(RInputClassificationType::INPUT_PER_VERTEX_DATA), InstanceDataStepRate(0u) {}
		RInputLayoutDesc(const RInputLayoutDesc& other) noexcept : SemanticName(other.SemanticName), SemanticIndex(other.SemanticIndex), InputSlot(other.InputSlot), InputSlotClass(other.InputSlotClass), InstanceDataStepRate(other.InstanceDataStepRate) {}
		constexpr RInputLayoutDesc(RShaderSemanticType name, const UINT32& semanticIndex = 0u, const UINT32& inputSlot = 0u, RInputClassificationType inputSlotClass = RInputClassificationType::INPUT_PER_VERTEX_DATA, const UINT32& instanceDataStepRate = 0u) noexcept : SemanticName(name), SemanticIndex(semanticIndex), InputSlot(inputSlot), InputSlotClass(inputSlotClass), InstanceDataStepRate(instanceDataStepRate) {}
		RShaderSemanticType			SemanticName;
		UINT32						SemanticIndex;
		UINT32						InputSlot;
		RInputClassificationType	InputSlotClass;
		UINT32						InstanceDataStepRate;
	};
	enum RResourceMapType : UINT8
	{
		RESOURCE_MAP_READ				= 1,
		RESOURCE_MAP_WRITE				= 2,
		RESOURCE_MAP_READ_WRITE			= 3,
		RESOURCE_MAP_WRITE_DISCARD		= 4,
		RESOURCE_MAP_WRITE_NO_OVERWRITE = 5
	};
	enum RResourceMapFlagType : UINT8
	{
		RESOURCE_MAP_FLAG_NONE			= 0,
		RESOURCE_MAP_FLAG_DO_NOT_WAIT	= 1
	};
	struct RMappedResource
	{
		RMappedResource() noexcept : pData(nullptr), RowPitch(0u), DepthPitch(0u) {}
		RMappedResource(const RMappedResource& other) noexcept : pData(other.pData), RowPitch(other.RowPitch), DepthPitch(other.DepthPitch) {}
		constexpr RMappedResource(void* InData, UINT32 InRowPitch, UINT32 InDepthPitch) noexcept : pData(InData), RowPitch(InRowPitch), DepthPitch(InDepthPitch) {}
		void*	pData;
		UINT32	RowPitch;
		UINT32	DepthPitch;
	};
	enum RQueryType : UINT8
	{
		QUERY_EVENT								= 0,
		QUERY_OCCLUSION							= 1,
		QUERY_TIMESTAMP							= 2,
		QUERY_TIMESTAMP_DISJOINT				= 3,
		QUERY_PIPELINE_STATISTICS				= 4,
		QUERY_OCCLUSION_PREDICATE				= 5,
		QUERY_SO_STATISTICS						= 6,
		QUERY_SO_OVERFLOW_PREDICATE				= 7,
		QUERY_SO_STATISTICS_STREAM0				= 8,
		QUERY_SO_OVERFLOW_PREDICATE_STREAM0		= 9,
		QUERY_SO_STATISTICS_STREAM1				= 10,
		QUERY_SO_OVERFLOW_PREDICATE_STREAM1		= 11,
		QUERY_SO_STATISTICS_STREAM2				= 12,
		QUERY_SO_OVERFLOW_PREDICATE_STREAM2		= 13,
		QUERY_SO_STATISTICS_STREAM3				= 14,
		QUERY_SO_OVERFLOW_PREDICATE_STREAM3		= 15
	};
	enum RQueryMiscFlagType : UINT8
	{
		QUERY_MISC_DEFAULT			= 0,
		QUERY_MISC_PREDICATEHINT	= (1 << 0)
	};
	struct RQueryDesc
	{
		RQueryDesc() noexcept : QueryType(RQueryType::QUERY_EVENT), MiscFlags(RQueryMiscFlagType::QUERY_MISC_DEFAULT) {}
		RQueryDesc(const RQueryDesc& other) noexcept : QueryType(other.QueryType), MiscFlags(other.MiscFlags) {}
		constexpr RQueryDesc(RQueryType queryType, RQueryMiscFlagType miscFlags = RQueryMiscFlagType::QUERY_MISC_DEFAULT) noexcept : QueryType(queryType), MiscFlags(miscFlags) {}
		RQueryType			QueryType;
		RQueryMiscFlagType	MiscFlags;
	};
	enum RAsyncGetDataFlagType : UINT8
	{
		ASYNC_GETDATA_DEFAULT		= 0,
		ASYNC_GETDATA_DONOTFLUSH	= (1 << 0)
	};
	struct RQueryTimestampDisjoint
	{
		RQueryTimestampDisjoint() noexcept : Frequency(1u), Disjoint(TRUE) {}
		RQueryTimestampDisjoint(const RQueryTimestampDisjoint& other) noexcept : Frequency(other.Frequency), Disjoint(other.Disjoint) {}
		constexpr RQueryTimestampDisjoint(ULONGLONG	InFrequency, BOOL32 InDisjoint) noexcept : Frequency(InFrequency), Disjoint(InDisjoint) {}
		void Reset() { Frequency = 1u; Disjoint = TRUE; }
		ULONGLONG	Frequency;
		BOOL32		Disjoint;
	};
	enum RClearDepthStencilFlagType : UINT8
	{
		CLEAR_NONE		= 0,
		CLEAR_DEPTH		= (1 << 0),
		CLEAR_STENCIL	= (1 << 1)
	};
	enum RPrimitiveTopologyType : UINT8
	{
		PRIMITIVE_TOPOLOGY_UNDEFINED		= 0,
		PRIMITIVE_TOPOLOGY_POINTLIST		= 1,
		PRIMITIVE_TOPOLOGY_LINELIST			= 2,
		PRIMITIVE_TOPOLOGY_LINESTRIP		= 3,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST		= 4,
		PRIMITIVE_TOPOLOGY_TRIANGLESTRIP	= 5
	};

	extern EString GetEngineDefaultTexturePath(RDefaultTextureType texType);
	extern UINT32 GetShaderSemanticSizeByByte(const RInputLayoutDesc& input);
	extern UINT32 GetShaderSemanticSizeBy32Bits(const RInputLayoutDesc& input);
	extern RShaderSemanticType GetShaderSemanticBaseType(RShaderSemanticType input);
	extern UINT32 GetShaderSemanticTypeSlot(RShaderSemanticType input);
	extern void GetEngineDefaultMeshInputLayouts(const RShaderSemanticType*& OutLayouts, UINT32& OutLayoutNum);
	extern void GetEngineDefaultSkeletalMeshInputLayouts(const RShaderSemanticType*& OutLayouts, UINT32& OutLayoutNum);

};