#pragma once

#include <CoreMinimal.h>
#include <RenderConfig/RenderConfig.h>

namespace PigeonEngine
{
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
		RLightConstantBuffer(const RLightConstantBuffer& other) : LightCount(other.LightCount) { for (USHORT i = 0u; i < RCommonSettings::SupportLightMax; i++) { LightParams[i] = other.LightParams[i]; } }
		DirectX::XMINT4		LightCount;
		RPerLightInfo		LightParams[RCommonSettings::SupportLightMax];
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
		RPerSkeletonConstantBuffer(const RPerSkeletonConstantBuffer& other) : SkeletonBoneNum(other.SkeletonBoneNum) { for (USHORT i = 0u; i < RCommonSettings::SkeletonBoneMax * 2u; i++) { SkeletonMatrix[i] = other.SkeletonMatrix[i]; } }
		DirectX::XMFLOAT4		SkeletonBoneNum;
		DirectX::XMFLOAT4X4		SkeletonMatrix[RCommonSettings::SkeletonBoneMax * 2u];
	};
	enum RShaderFrequencyType
	{
		SHADER_FREQUENCY_COMPUTE	= 0,
		SHADER_FREQUENCY_VERTEX		= 1,
		SHADER_FREQUENCY_PIXEL		= 2,
		SHADER_FREQUENCY_COUNT
	};
	enum RDefaultTextureType
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
	enum RStencilMaskType
	{
		STENCIL_MASK_SKY			= 0x1,
		STENCIL_MASK_LAND			= 0x2,
		STENCIL_MASK_OPAQUE_BASE	= 0x4
	};
	enum RComparisonFunctionType
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
	enum RCullModeType
	{
		CULL_NONE	= 1,
		CULL_FRONT	= 2,
		CULL_BACK	= 3
	};
	enum RFillModeType
	{
		FILL_WIREFRAME	= 2,
		FILL_SOLID		= 3
	};
	struct RRasterizerState
	{
		RRasterizerState() noexcept : CullMode(RCullModeType::CULL_BACK), FillMode(RFillModeType::FILL_SOLID) {}
		RRasterizerState(RCullModeType cull, RFillModeType fill = RFillModeType::FILL_SOLID) noexcept : CullMode(cull), FillMode(fill) {}
		RRasterizerState(const RRasterizerState& other) noexcept : CullMode(other.CullMode), FillMode(other.FillMode) {}
		constexpr RRasterizerState() noexcept : CullMode(RCullModeType::CULL_BACK), FillMode(RFillModeType::FILL_SOLID) {}
		constexpr RRasterizerState(RCullModeType cull, RFillModeType fill = RFillModeType::FILL_SOLID) noexcept : CullMode(cull), FillMode(fill) {}
		constexpr RRasterizerState(const RRasterizerState& other) noexcept : CullMode(other.CullMode), FillMode(other.FillMode) {}
		RCullModeType	CullMode;
		RFillModeType	FillMode;
	};
	enum RDepthWriteMaskType
	{
		DEPTH_WRITE_MASK_ZERO	= 0,
		DEPTH_WRITE_MASK_ALL	= 1
	};
	struct RDepthState
	{
		RDepthState() noexcept : DepthEnable(FALSE), DepthWriteMask(RDepthWriteMaskType::DEPTH_WRITE_MASK_ZERO), DepthFunc(RComparisonFunctionType::COMPARISON_NEVER) {}
		RDepthState(RComparisonFunctionType func, RDepthWriteMaskType writeMask = RDepthWriteMaskType::DEPTH_WRITE_MASK_ALL) noexcept : DepthEnable(TRUE), DepthWriteMask(writeMask), DepthFunc(func) {}
		RDepthState(const RDepthState& other) noexcept : DepthEnable(other.DepthEnable), DepthWriteMask(other.DepthWriteMask), DepthFunc(other.DepthFunc) {}
		constexpr RDepthState() noexcept : DepthEnable(FALSE), DepthWriteMask(RDepthWriteMaskType::DEPTH_WRITE_MASK_ZERO), DepthFunc(RComparisonFunctionType::COMPARISON_NEVER) {}
		constexpr RDepthState(RComparisonFunctionType func, RDepthWriteMaskType writeMask = RDepthWriteMaskType::DEPTH_WRITE_MASK_ALL) noexcept : DepthEnable(TRUE), DepthWriteMask(writeMask), DepthFunc(func) {}
		constexpr RDepthState(const RDepthState& other) noexcept : DepthEnable(other.DepthEnable), DepthWriteMask(other.DepthWriteMask), DepthFunc(other.DepthFunc) {}
		BOOL						DepthEnable;
		RDepthWriteMaskType			DepthWriteMask;
		RComparisonFunctionType		DepthFunc;
	};
	enum RStencilOperationType
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
		RStencilStateType(RComparisonFunctionType func, RStencilOperationType passOp, RStencilOperationType depthFailOp = RStencilOperationType::STENCIL_OP_KEEP, RStencilOperationType stencilFailOp = RStencilOperationType::STENCIL_OP_KEEP) noexcept : StencilFailOp(stencilFailOp), StencilDepthFailOp(depthFailOp), StencilPassOp(passOp), StencilFunc(func) {}
		RStencilStateType(const RStencilStateType& other) noexcept : StencilFailOp(other.StencilFailOp), StencilDepthFailOp(other.StencilDepthFailOp), StencilPassOp(other.StencilPassOp), StencilFunc(other.StencilFunc) {}
		constexpr RStencilStateType() noexcept : StencilFailOp(RStencilOperationType::STENCIL_OP_KEEP), StencilDepthFailOp(RStencilOperationType::STENCIL_OP_KEEP), StencilPassOp(RStencilOperationType::STENCIL_OP_KEEP), StencilFunc(RComparisonFunctionType::COMPARISON_NEVER) {}
		constexpr RStencilStateType(RComparisonFunctionType func, RStencilOperationType passOp, RStencilOperationType depthFailOp = RStencilOperationType::STENCIL_OP_KEEP, RStencilOperationType stencilFailOp = RStencilOperationType::STENCIL_OP_KEEP) noexcept : StencilFailOp(stencilFailOp), StencilDepthFailOp(depthFailOp), StencilPassOp(passOp), StencilFunc(func) {}
		constexpr RStencilStateType(const RStencilStateType& other) noexcept : StencilFailOp(other.StencilFailOp), StencilDepthFailOp(other.StencilDepthFailOp), StencilPassOp(other.StencilPassOp), StencilFunc(other.StencilFunc) {}
		RStencilOperationType		StencilFailOp;
		RStencilOperationType		StencilDepthFailOp;
		RStencilOperationType		StencilPassOp;
		RComparisonFunctionType		StencilFunc;
	};
	struct RStencilState
	{
		RStencilState() : StencilEnable(FALSE), StencilReadMask(ENGINE_DEFAULT_STENCIL_READ_MASK), StencilWriteMask(ENGINE_DEFAULT_STENCIL_WRITE_MASK), FrontFace(RStencilStateType(RComparisonFunctionType::COMPARISON_NEVER, RStencilOperationType::STENCIL_OP_KEEP)), BackFace(RStencilStateType(RComparisonFunctionType::COMPARISON_NEVER, RStencilOperationType::STENCIL_OP_KEEP)) {}
		RStencilState(RStencilMaskType readMask, RStencilMaskType writeMask, const RStencilStateType& frontFace, const RStencilStateType& backFace) noexcept : StencilEnable(TRUE), StencilReadMask(static_cast<UINT>(readMask)), StencilWriteMask(static_cast<UINT>(writeMask)), FrontFace(frontFace), BackFace(backFace) {}
		RStencilState(const UINT& readMask, const UINT& writeMask, const RStencilStateType& frontFace, const RStencilStateType& backFace) noexcept : StencilEnable(TRUE), StencilReadMask(EMath::Min(readMask, 0xffu)), StencilWriteMask(EMath::Min(writeMask, 0xffu)), FrontFace(frontFace), BackFace(backFace) {}
		RStencilState(const RStencilState& other) noexcept : StencilEnable(other.StencilEnable), StencilReadMask(other.StencilReadMask), StencilWriteMask(other.StencilWriteMask), FrontFace(other.FrontFace), BackFace(other.BackFace) {}
		BOOL				StencilEnable;
		UINT				StencilReadMask;
		UINT				StencilWriteMask;
		RStencilStateType	FrontFace;
		RStencilStateType	BackFace;
	};
	enum RBlendOptionType
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
	enum RBlendOperationType
	{
		BLEND_OP_ADD			= 1,
		BLEND_OP_SUBTRACT		= 2,
		BLEND_OP_REV_SUBTRACT	= 3,
		BLEND_OP_MIN			= 4,
		BLEND_OP_MAX			= 5
	};
	enum RColorWriteMaskType
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
		RBlendState(RBlendOptionType src, RBlendOptionType dst, RBlendOperationType op, RBlendOptionType srcA, RBlendOptionType dstA, RBlendOperationType opA, RColorWriteMaskType writeMask = RColorWriteMaskType::COLOR_WRITE_MASK_ALL) noexcept : BlendEnable(TRUE), SrcBlend(src), DstBlend(dst), BlendOp(op), SrcBlendAlpha(srcA), DstBlendAlpha(dstA), BlendOpAlpha(opA), RenderTargetWriteMask(writeMask) {}
		RBlendState(const RBlendState& other) noexcept : BlendEnable(other.BlendEnable), SrcBlend(other.SrcBlend), DstBlend(other.DstBlend), BlendOp(other.BlendOp), SrcBlendAlpha(other.SrcBlendAlpha), DstBlendAlpha(other.DstBlendAlpha), BlendOpAlpha(other.BlendOpAlpha), RenderTargetWriteMask(other.RenderTargetWriteMask) {}
		constexpr RBlendState() noexcept : BlendEnable(FALSE), SrcBlend(RBlendOptionType::BLEND_ONE), DstBlend(RBlendOptionType::BLEND_ZERO), BlendOp(RBlendOperationType::BLEND_OP_ADD), SrcBlendAlpha(RBlendOptionType::BLEND_ONE), DstBlendAlpha(RBlendOptionType::BLEND_ZERO), BlendOpAlpha(RBlendOperationType::BLEND_OP_ADD), RenderTargetWriteMask(RColorWriteMaskType::COLOR_WRITE_MASK_ALL) {}
		constexpr RBlendState(RBlendOptionType src, RBlendOptionType dst, RBlendOperationType op, RBlendOptionType srcA, RBlendOptionType dstA, RBlendOperationType opA, RColorWriteMaskType writeMask = RColorWriteMaskType::COLOR_WRITE_MASK_ALL) noexcept : BlendEnable(TRUE), SrcBlend(src), DstBlend(dst), BlendOp(op), SrcBlendAlpha(srcA), DstBlendAlpha(dstA), BlendOpAlpha(opA), RenderTargetWriteMask(writeMask) {}
		constexpr RBlendState(const RBlendState& other) noexcept : BlendEnable(other.BlendEnable), SrcBlend(other.SrcBlend), DstBlend(other.DstBlend), BlendOp(other.BlendOp), SrcBlendAlpha(other.SrcBlendAlpha), DstBlendAlpha(other.DstBlendAlpha), BlendOpAlpha(other.BlendOpAlpha), RenderTargetWriteMask(other.RenderTargetWriteMask) {}
		BOOL					BlendEnable;
		RBlendOptionType		SrcBlend;
		RBlendOptionType		DstBlend;
		RBlendOperationType		BlendOp;
		RBlendOptionType		SrcBlendAlpha;
		RBlendOptionType		DstBlendAlpha;
		RBlendOperationType		BlendOpAlpha;
		RColorWriteMaskType		RenderTargetWriteMask;
	};
	enum RFilterType
	{
		FILTER_POINT		= 1,
		FILTER_LINEAR		= 2,
		FILTER_ANISOTROPIC	= 3
	};
	enum RTextureAddressModeType
	{
		TEXTURE_ADDRESS_WRAP		= 1,
		TEXTURE_ADDRESS_MIRROR		= 2,
		TEXTURE_ADDRESS_CLAMP		= 3,
		TEXTURE_ADDRESS_BORDER		= 4,
		TEXTURE_ADDRESS_MIRROR_ONCE	= 5
	};
	struct RSamplerState
	{
		RSamplerState() noexcept : Filter(RFilterType::FILTER_LINEAR), AddressU(RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP), AddressV(RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP), AddressW(RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP), MipLODBias(0.f), MaxAnisotropy(1u), ComparisonFunc(RComparisonFunctionType::COMPARISON_ALWAYS), BorderColor(Color4(0.f, 0.f, 0.f, 0.f)), MinLOD(0.f), MaxLOD(ENGINE_FLOAT32_MAX) {}
		RSamplerState(RFilterType filter, RTextureAddressModeType u, RTextureAddressModeType v, RTextureAddressModeType w, const FLOAT& maxLOD = ENGINE_FLOAT32_MAX, const FLOAT& minLOD = 0.f, const UINT& maxAnisotropy = 1u, const FLOAT& mipLODBias = 0.f, const Color4& borderColor = Color4(0.f, 0.f, 0.f, 0.f), RComparisonFunctionType func = RComparisonFunctionType::COMPARISON_ALWAYS) noexcept : Filter(filter), AddressU(u), AddressV(v), AddressW(w), MipLODBias(mipLODBias), MaxAnisotropy(maxAnisotropy), ComparisonFunc(func), BorderColor(borderColor), MinLOD(minLOD), MaxLOD(maxLOD) {}
		RSamplerState(const RSamplerState& other) noexcept : Filter(other.Filter), AddressU(other.AddressU), AddressV(other.AddressV), AddressW(other.AddressW), MipLODBias(other.MipLODBias), MaxAnisotropy(other.MaxAnisotropy), ComparisonFunc(other.ComparisonFunc), BorderColor(other.BorderColor), MinLOD(other.MinLOD), MaxLOD(other.MaxLOD) {}
		RFilterType					Filter;
		RTextureAddressModeType		AddressU;
		RTextureAddressModeType		AddressV;
		RTextureAddressModeType		AddressW;
		FLOAT						MipLODBias;
		UINT						MaxAnisotropy;
		RComparisonFunctionType		ComparisonFunc;
		Color4						BorderColor;
		FLOAT						MinLOD;
		FLOAT						MaxLOD;
	};
	enum RBindFlagType
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
	enum RResourceMiscFlagType
	{
		RESOURCE_MISC_NONE					= 0,
		RESOURCE_MISC_GENERATE_MIPS			= (1 << 0),
		RESOURCE_MISC_TEXTURECUBE			= (1 << 1),
		RESOURCE_MISC_DRAWINDIRECT_ARGS		= (1 << 2),
		RESOURCE_MISC_BUFFER_STRUCTURED		= (1 << 3),
		RESOURCE_MISC_RESOURCE_CLAMP		= (1 << 4)
	};
	enum RUsageFlagType
	{
		USAGE_DEFAULT		= 0,
		USAGE_IMMUTABLE		= 1,
		USAGE_DYNAMIC		= 2,
		USAGE_STAGING		= 3
	};
	enum RCPUAccessFlagType
	{
		CPU_ACCESS_DEFAULT	= 0,
		CPU_ACCESS_WRITE	= (1 << 0),
		CPU_ACCESS_READ		= (1 << 1)
	};
	enum RUAVFlagType
	{
		UAV_FLAG_NONE		= 0,
		UAV_FLAG_RAW		= (1 << 0),
		UAV_FLAG_APPEND		= (1 << 1),
		UAV_FLAG_COUNTER	= (1 << 2)
	};
	struct RBufferDesc
	{
		RBufferDesc(const UINT& byteWidth, RBindFlagType bindFlag, const UINT& structureByteStride, RResourceMiscFlagType miscFlag = RResourceMiscFlagType::RESOURCE_MISC_NONE, RUsageFlagType usage = RUsageFlagType::USAGE_DEFAULT, RCPUAccessFlagType cpuAccessFlag = RCPUAccessFlagType::CPU_ACCESS_DEFAULT) noexcept : ByteWidth(byteWidth), Usage(usage), BindFlags(bindFlag), CPUAccessFlags(cpuAccessFlag), MiscFlags(miscFlag), StructureByteStride(structureByteStride) {}
		RBufferDesc(const RBufferDesc& other) noexcept : ByteWidth(other.ByteWidth), Usage(other.Usage), BindFlags(other.BindFlags), CPUAccessFlags(other.CPUAccessFlags), MiscFlags(other.MiscFlags), StructureByteStride(other.StructureByteStride) {}
		constexpr RBufferDesc(const UINT& byteWidth, RBindFlagType bindFlag, const UINT& structureByteStride, RResourceMiscFlagType miscFlag = RResourceMiscFlagType::RESOURCE_MISC_NONE, RUsageFlagType usage = RUsageFlagType::USAGE_DEFAULT, RCPUAccessFlagType cpuAccessFlag = RCPUAccessFlagType::CPU_ACCESS_DEFAULT) noexcept : ByteWidth(byteWidth), Usage(usage), BindFlags(bindFlag), CPUAccessFlags(cpuAccessFlag), MiscFlags(miscFlag), StructureByteStride(structureByteStride) {}
		constexpr RBufferDesc(const RBufferDesc& other) noexcept : ByteWidth(other.ByteWidth), Usage(other.Usage), BindFlags(other.BindFlags), CPUAccessFlags(other.CPUAccessFlags), MiscFlags(other.MiscFlags), StructureByteStride(other.StructureByteStride) {}
		UINT					ByteWidth;
		RUsageFlagType			Usage;
		RBindFlagType			BindFlags;
		RCPUAccessFlagType		CPUAccessFlags;
		RResourceMiscFlagType	MiscFlags;
		UINT					StructureByteStride;
	};
	struct RSubresourceDataDesc
	{
		RSubresourceDataDesc() noexcept : pSysMem(nullptr), SysMemPitch(0u), SysMemSlicePitch(0u) {}
		RSubresourceDataDesc(const void* mem, const UINT& sysMemPitch, const UINT& sysMemSlicePitch) noexcept : pSysMem(mem), SysMemPitch(sysMemPitch), SysMemSlicePitch(sysMemSlicePitch) {}
		RSubresourceDataDesc(const RSubresourceDataDesc& other) noexcept : pSysMem(other.pSysMem), SysMemPitch(other.SysMemPitch), SysMemSlicePitch(other.SysMemSlicePitch) {}
		constexpr RSubresourceDataDesc() noexcept : pSysMem(nullptr), SysMemPitch(0u), SysMemSlicePitch(0u) {}
		constexpr RSubresourceDataDesc(const void* mem, const UINT& sysMemPitch, const UINT& sysMemSlicePitch) noexcept : pSysMem(mem), SysMemPitch(sysMemPitch), SysMemSlicePitch(sysMemSlicePitch) {}
		constexpr RSubresourceDataDesc(const RSubresourceDataDesc& other) noexcept : pSysMem(other.pSysMem), SysMemPitch(other.SysMemPitch), SysMemSlicePitch(other.SysMemSlicePitch) {}
		const void*		pSysMem;
		UINT			SysMemPitch;
		UINT			SysMemSlicePitch;
	};
	enum RFormatType
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
		RTextureSampleDesc(const UINT& count, const UINT& quality) noexcept : Count(count), Quality(quality) {}
		RTextureSampleDesc(const RTextureSampleDesc& other) noexcept : Count(other.Count), Quality(other.Quality) {}
		constexpr RTextureSampleDesc() noexcept : Count(1u), Quality(0u) {}
		constexpr RTextureSampleDesc(const UINT& count, const UINT& quality) noexcept : Count(count), Quality(quality) {}
		constexpr RTextureSampleDesc(const RTextureSampleDesc& other) noexcept : Count(other.Count), Quality(other.Quality) {}
		UINT	Count;
		UINT	Quality;
	};
	struct RTextureDesc
	{
		RTextureDesc(
			const UINT& width, const UINT& height, RBindFlagType bindFlags, RFormatType bufferFormat, const RFormatType* srvFormat = nullptr
			, const RFormatType* rtvFormat = nullptr, const RFormatType* uavFormat = nullptr, const UINT& depth = 0u, const UINT& mipLevels = 1u
			, const UINT& arraySize = 1u, RUsageFlagType usageFlag = RUsageFlagType::USAGE_DEFAULT, RCPUAccessFlagType cpuAccessFlags = RCPUAccessFlagType::CPU_ACCESS_DEFAULT
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
		UINT					Width;
		UINT					Height;
		UINT					Depth;
		UINT					MipLevels;
		UINT					ArraySize;
		RFormatType				BufferFormat;
		RFormatType				SRVFormat;
		RFormatType				RTVFormat;
		RFormatType				UAVFormat;
		RTextureSampleDesc		SampleDesc;
		RUsageFlagType			UsageFlag;
		RBindFlagType			BindFlags;
		RCPUAccessFlagType		CPUAccessFlags;
		RResourceMiscFlagType	MiscFlags;
	};
	struct RStructuredBufferDesc
	{
		RStructuredBufferDesc(const UINT& structureSize, const UINT& numElements, const BOOL& writableGPU = FALSE, RCPUAccessFlagType accessFlags = RCPUAccessFlagType::CPU_ACCESS_DEFAULT, const UINT& firstElement = 0u, RUAVFlagType uavFlags = RUAVFlagType::UAV_FLAG_NONE) noexcept : GPUWritable(writableGPU), AccessFlags(accessFlags), StructureSize(structureSize), FirstElement(firstElement), NumElements(numElements), UAVFlags(uavFlags) {}
		RStructuredBufferDesc(const RStructuredBufferDesc& other) noexcept : GPUWritable(other.GPUWritable), AccessFlags(other.AccessFlags), StructureSize(other.StructureSize), FirstElement(other.FirstElement), NumElements(other.NumElements), UAVFlags(other.UAVFlags) {}
		constexpr RStructuredBufferDesc(const UINT& structureSize, const UINT& numElements, const BOOL& writableGPU = FALSE, RCPUAccessFlagType accessFlags = RCPUAccessFlagType::CPU_ACCESS_DEFAULT, const UINT& firstElement = 0u, RUAVFlagType uavFlags = RUAVFlagType::UAV_FLAG_NONE) noexcept : GPUWritable(writableGPU), AccessFlags(accessFlags), StructureSize(structureSize), FirstElement(firstElement), NumElements(numElements), UAVFlags(uavFlags) {}
		constexpr RStructuredBufferDesc(const RStructuredBufferDesc& other) noexcept : GPUWritable(other.GPUWritable), AccessFlags(other.AccessFlags), StructureSize(other.StructureSize), FirstElement(other.FirstElement), NumElements(other.NumElements), UAVFlags(other.UAVFlags) {}
		BOOL					GPUWritable;
		RCPUAccessFlagType		AccessFlags;
		UINT					StructureSize;
		UINT					FirstElement;
		UINT					NumElements;
		RUAVFlagType			UAVFlags;
	};
	enum RInputClassificationType
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

	enum RShaderSemanticType
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

#undef SHADER_SEMANTIC_DEFINE(Name)

	struct RInputLayoutDesc
	{
		RInputLayoutDesc() noexcept : SemanticName(RShaderSemanticType::SHADER_SEMANTIC_NONE), SemanticIndex(0u), InputSlot(0u), InputSlotClass(RInputClassificationType::INPUT_PER_VERTEX_DATA), InstanceDataStepRate(0u) {}
		RInputLayoutDesc(RShaderSemanticType name, const UINT& semanticIndex = 0u, const UINT& inputSlot = 0u, RInputClassificationType inputSlotClass = RInputClassificationType::INPUT_PER_VERTEX_DATA, const UINT& instanceDataStepRate = 0u) noexcept : SemanticName(name), SemanticIndex(semanticIndex), InputSlot(inputSlot), InputSlotClass(inputSlotClass), InstanceDataStepRate(instanceDataStepRate) {}
		RInputLayoutDesc(const RInputLayoutDesc& other) noexcept : SemanticName(other.SemanticName), SemanticIndex(other.SemanticIndex), InputSlot(other.InputSlot), InputSlotClass(other.InputSlotClass), InstanceDataStepRate(other.InstanceDataStepRate) {}
		constexpr RInputLayoutDesc(RShaderSemanticType name, const UINT& semanticIndex = 0u, const UINT& inputSlot = 0u, RInputClassificationType inputSlotClass = RInputClassificationType::INPUT_PER_VERTEX_DATA, const UINT& instanceDataStepRate = 0u) noexcept : SemanticName(name), SemanticIndex(semanticIndex), InputSlot(inputSlot), InputSlotClass(inputSlotClass), InstanceDataStepRate(instanceDataStepRate) {}
		constexpr RInputLayoutDesc(const RInputLayoutDesc& other) noexcept : SemanticName(other.SemanticName), SemanticIndex(other.SemanticIndex), InputSlot(other.InputSlot), InputSlotClass(other.InputSlotClass), InstanceDataStepRate(other.InstanceDataStepRate) {}
		RShaderSemanticType			SemanticName;
		UINT						SemanticIndex;
		UINT						InputSlot;
		RInputClassificationType	InputSlotClass;
		UINT						InstanceDataStepRate;
	};
	enum RResourceMapType
	{
		RESOURCE_MAP_READ				= 1,
		RESOURCE_MAP_WRITE				= 2,
		RESOURCE_MAP_READ_WRITE			= 3,
		RESOURCE_MAP_WRITE_DISCARD		= 4,
		RESOURCE_MAP_WRITE_NO_OVERWRITE = 5
	};
	enum RResourceMapFlagType
	{
		RESOURCE_MAP_FLAG_NONE			= 0,
		RESOURCE_MAP_FLAG_DO_NOT_WAIT	= 1
	};
	struct RMappedResource
	{
		RMappedResource() noexcept : pData(nullptr), RowPitch(0u), DepthPitch(0u) {}
		constexpr RMappedResource() noexcept : pData(nullptr), RowPitch(0u), DepthPitch(0u) {}
		void*	pData;
		UINT	RowPitch;
		UINT	DepthPitch;
	};
	enum RQueryType
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
	enum RQueryMiscFlagType
	{
		QUERY_MISC_DEFAULT			= 0,
		QUERY_MISC_PREDICATEHINT	= (1 << 0)
	};
	struct RQueryDesc
	{
		RQueryDesc(RQueryType queryType, RQueryMiscFlagType miscFlags = RQueryMiscFlagType::QUERY_MISC_DEFAULT) noexcept : QueryType(queryType), MiscFlags(miscFlags) {}
		constexpr RQueryDesc(RQueryType queryType, RQueryMiscFlagType miscFlags = RQueryMiscFlagType::QUERY_MISC_DEFAULT) noexcept : QueryType(queryType), MiscFlags(miscFlags) {}
		RQueryType			QueryType;
		RQueryMiscFlagType	MiscFlags;
	};
	enum RAsyncGetDataFlagType
	{
		ASYNC_GETDATA_DEFAULT		= 0,
		ASYNC_GETDATA_DONOTFLUSH	= (1 << 0)
	};
	struct RQueryTimestampDisjoint
	{
		RQueryTimestampDisjoint() noexcept : Frequency(1u), Disjoint(TRUE) {}
		void Reset() { Frequency = 1u; Disjoint = TRUE; }
		constexpr RQueryTimestampDisjoint() noexcept : Frequency(1u), Disjoint(TRUE) {}
		ULONGLONG	Frequency;
		BOOL		Disjoint;
	};
	enum RClearDepthStencilFlagType
	{
		CLEAR_NONE		= 0,
		CLEAR_DEPTH		= (1 << 0),
		CLEAR_STENCIL	= (1 << 1)
	};
	enum RPrimitiveTopologyType
	{
		PRIMITIVE_TOPOLOGY_UNDEFINED		= 0,
		PRIMITIVE_TOPOLOGY_POINTLIST		= 1,
		PRIMITIVE_TOPOLOGY_LINELIST			= 2,
		PRIMITIVE_TOPOLOGY_LINESTRIP		= 3,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST		= 4,
		PRIMITIVE_TOPOLOGY_TRIANGLESTRIP	= 5
	};
	class RCommonSettings
	{
	public:
		constexpr static FLOAT		BoundMinimum		= 0.01f;
		constexpr static FLOAT		BoundHalfMinimum	= 0.005f;
		constexpr static USHORT		SupportLightMax		= 16u;
		constexpr static USHORT		SkeletonBoneMax		= 286u;
	public:
		static std::string GetEngineDefaultTexturePath(RDefaultTextureType texType)
		{
			static std::map<RDefaultTextureType, std::string> engineDefaultTexturePathMap = {
				{ RDefaultTextureType::TEXTURE2D_WHITE, std::string(ENGINE_DEFAULT_TEXTURE2D_WHITE) },
				{ RDefaultTextureType::TEXTURE2D_BLACK, std::string(ENGINE_DEFAULT_TEXTURE2D_BLACK) },
				{ RDefaultTextureType::TEXTURE2D_GRAY, std::string(ENGINE_DEFAULT_TEXTURE2D_GRAY) },
				{ RDefaultTextureType::TEXTURE2D_RED, std::string(ENGINE_DEFAULT_TEXTURE2D_RED) },
				{ RDefaultTextureType::TEXTURE2D_GREEN, std::string(ENGINE_DEFAULT_TEXTURE2D_GREEN) },
				{ RDefaultTextureType::TEXTURE2D_BLUE, std::string(ENGINE_DEFAULT_TEXTURE2D_BLUE) },
				{ RDefaultTextureType::TEXTURE2D_BUMP, std::string(ENGINE_DEFAULT_TEXTURE2D_BUMP) },
				{ RDefaultTextureType::TEXTURE2D_PROPERTY, std::string(ENGINE_DEFAULT_TEXTURE2D_PROPERTY) } };

			return engineDefaultTexturePathMap[texType];
		}
		static UINT GetShaderSemanticSizeByByte(const RInputLayoutDesc& input)
		{
			if (input.SemanticName == RShaderSemanticType::SHADER_SEMANTIC_NONE) { return 0u; }
			UINT SemanticName = input.SemanticName;
			if ((SemanticName >> 15) & 0x1u) { /*SHADER_SEMANTIC_TEXCOORD[n]*/return 8u; }
			else if ((SemanticName >> 14) & 0x1u) { /*SHADER_SEMANTIC_POSITION[n]*/return 16u; }
			else if ((SemanticName >> 13) & 0x1u) { /*SHADER_SEMANTIC_NORMAL[n]*/return 16u; }
			else if ((SemanticName >> 12) & 0x1u) { /*SHADER_SEMANTIC_TANGENT[n]*/return 16u; }
			else if ((SemanticName >> 11) & 0x1u) { /*SHADER_SEMANTIC_COLOR[n]*/return 16u; }
			else if ((SemanticName >> 10) & 0x1u) { /*SHADER_SEMANTIC_BLENDINDICES[n]*/return 8u; }
			else if ((SemanticName >> 9) & 0x1u) { /*SHADER_SEMANTIC_BLENDWEIGHT[n]*/return 16u; }
			else if ((SemanticName >> 8) & 0x1u) { /*SHADER_SEMANTIC_BINORMAL[n]*/return 16u; }
			else if ((SemanticName >> 7) & 0x1u) { /*SHADER_SEMANTIC_POSITIONT[n]*/return 16u; }
			else if ((SemanticName >> 6) & 0x1u) { /*SHADER_SEMANTIC_PSIZE[n]*/return 8u; }
			return 0u;
		}
		static UINT GetShaderSemanticSizeBy32Bits(const RInputLayoutDesc& input)
		{
			if (input.SemanticName == RShaderSemanticType::SHADER_SEMANTIC_NONE) { return 0u; }
			UINT SemanticName = input.SemanticName;
			if ((SemanticName >> 15) & 0x1u) { /*SHADER_SEMANTIC_TEXCOORD[n]*/return 2u; }
			else if ((SemanticName >> 14) & 0x1u) { /*SHADER_SEMANTIC_POSITION[n]*/return 4u; }
			else if ((SemanticName >> 13) & 0x1u) { /*SHADER_SEMANTIC_NORMAL[n]*/return 4u; }
			else if ((SemanticName >> 12) & 0x1u) { /*SHADER_SEMANTIC_TANGENT[n]*/return 4u; }
			else if ((SemanticName >> 11) & 0x1u) { /*SHADER_SEMANTIC_COLOR[n]*/return 4u; }
			else if ((SemanticName >> 10) & 0x1u) { /*SHADER_SEMANTIC_BLENDINDICES[n]*/return 2u; }
			else if ((SemanticName >> 9) & 0x1u) { /*SHADER_SEMANTIC_BLENDWEIGHT[n]*/return 4u; }
			else if ((SemanticName >> 8) & 0x1u) { /*SHADER_SEMANTIC_BINORMAL[n]*/return 4u; }
			else if ((SemanticName >> 7) & 0x1u) { /*SHADER_SEMANTIC_POSITIONT[n]*/return 4u; }
			else if ((SemanticName >> 6) & 0x1u) { /*SHADER_SEMANTIC_PSIZE[n]*/return 2u; }
			return 0u;
		}
		static RShaderSemanticType GetShaderSemanticBaseType(RShaderSemanticType input)
		{
			if (input == RShaderSemanticType::SHADER_SEMANTIC_NONE) { return RShaderSemanticType::SHADER_SEMANTIC_NONE; }
			UINT SemanticType = input;
			if ((SemanticType >> 15) & 0x1u) { /*SHADER_SEMANTIC_TEXCOORD[n]*/return RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD; }
			else if ((SemanticType >> 14) & 0x1u) { /*SHADER_SEMANTIC_POSITION[n]*/return RShaderSemanticType::SHADER_SEMANTIC_POSITION; }
			else if ((SemanticType >> 13) & 0x1u) { /*SHADER_SEMANTIC_NORMAL[n]*/return RShaderSemanticType::SHADER_SEMANTIC_NORMAL; }
			else if ((SemanticType >> 12) & 0x1u) { /*SHADER_SEMANTIC_TANGENT[n]*/return RShaderSemanticType::SHADER_SEMANTIC_TANGENT; }
			else if ((SemanticType >> 11) & 0x1u) { /*SHADER_SEMANTIC_COLOR[n]*/return RShaderSemanticType::SHADER_SEMANTIC_COLOR; }
			else if ((SemanticType >> 10) & 0x1u) { /*SHADER_SEMANTIC_BLENDINDICES[n]*/return RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES; }
			else if ((SemanticType >> 9) & 0x1u) { /*SHADER_SEMANTIC_BLENDWEIGHT[n]*/return RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT; }
			else if ((SemanticType >> 8) & 0x1u) { /*SHADER_SEMANTIC_BINORMAL[n]*/return RShaderSemanticType::SHADER_SEMANTIC_BINORMAL; }
			else if ((SemanticType >> 7) & 0x1u) { /*SHADER_SEMANTIC_POSITIONT[n]*/return RShaderSemanticType::SHADER_SEMANTIC_POSITIONT; }
			else if ((SemanticType >> 6) & 0x1u) { /*SHADER_SEMANTIC_PSIZE[n]*/return RShaderSemanticType::SHADER_SEMANTIC_PSIZE; }
			return RShaderSemanticType::SHADER_SEMANTIC_NONE;
		}
		static UINT GetShaderSemanticTypeSlot(RShaderSemanticType input)
		{
			if (input == RShaderSemanticType::SHADER_SEMANTIC_NONE) { return 0u; }
			UINT SemanticType = input;
			return (SemanticType & 0xffu);
		}
		static void GetEngineDefaultMeshInputLayouts(const RShaderSemanticType*& OutLayouts, UINT& OutLayoutNum)
		{
			const static RShaderSemanticType _EngineDefaultMeshInputLayout[7u] = {
				RShaderSemanticType::SHADER_SEMANTIC_POSITION0,
				RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD0,
				RShaderSemanticType::SHADER_SEMANTIC_NORMAL0,
				RShaderSemanticType::SHADER_SEMANTIC_TANGENT0,
				RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD1,
				RShaderSemanticType::SHADER_SEMANTIC_COLOR0,
				RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD2 };

			OutLayouts = _EngineDefaultMeshInputLayout;
			OutLayoutNum = 7u;
		}
		static void GetEngineDefaultSkeletonMeshInputLayouts(const RShaderSemanticType*& OutLayouts, UINT& OutLayoutNum)
		{
			const static RShaderSemanticType _EngineDefaultSkeletonMeshInputLayout[9u] = {
				RShaderSemanticType::SHADER_SEMANTIC_POSITION0,
				RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD0,
				RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES0,
				RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT0,
				RShaderSemanticType::SHADER_SEMANTIC_NORMAL0,
				RShaderSemanticType::SHADER_SEMANTIC_TANGENT0,
				RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD1,
				RShaderSemanticType::SHADER_SEMANTIC_COLOR0,
				RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD2 };

			OutLayouts = _EngineDefaultSkeletonMeshInputLayout;
			OutLayoutNum = 9u;
		}
	public:
		RCommonSettings() {}
		~RCommonSettings() {}
	};
};