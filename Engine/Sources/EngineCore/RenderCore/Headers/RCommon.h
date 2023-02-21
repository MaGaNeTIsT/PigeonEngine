#pragma once

#include "../../Core/Headers/EMain.h"
#include "../../Base/Headers/EBaseType.h"

namespace PigeonEngine
{
	class RCommonSettings
	{
	public:
		constexpr static FLOAT		BoundMinimum			= 0.01f;
		constexpr static FLOAT		BoundHalfMinimum		= 0.005f;
		constexpr static USHORT		SupportLightMax			= 16u;
		constexpr static USHORT		SkeletonBoneMax			= 286u;
	public:
		RCommonSettings() {}
		~RCommonSettings() {}
	};
	struct RBoundingBox
	{
		RBoundingBox(const Vector3& anchor, const Vector3& dimensions) noexcept : Anchor(anchor), Dimensions(dimensions) {}
		Vector3		Anchor;
		Vector3		Dimensions;
	};
	struct RBoundingSphere
	{
		RBoundingSphere(const Vector3& anchor, const FLOAT& radius) noexcept : Anchor(anchor), Radius(radius) {}
		Vector3		Anchor;
		FLOAT		Radius;
	};
	struct RSubMeshInfo
	{
		RSubMeshInfo() noexcept :VertexStart(0u), VertexCount(0u), IndexStart(0u), IndexCount(0u) {}
		constexpr RSubMeshInfo() noexcept :VertexStart(0u), VertexCount(0u), IndexStart(0u), IndexCount(0u) {}
		UINT	VertexStart;
		UINT	VertexCount;
		UINT	IndexStart;
		UINT	IndexCount;
	};
	struct RPerLightInfo
	{
		RPerLightInfo() { ::ZeroMemory(this, sizeof(*this)); }
		DirectX::XMFLOAT4	Color;
		DirectX::XMFLOAT4	Params0;
		DirectX::XMFLOAT4	Params1;
	};
	struct RLightConstantBuffer
	{
		RLightConstantBuffer() { ::ZeroMemory(this, sizeof(*this)); }
		DirectX::XMINT4		LightCount;
		RPerLightInfo		LightParams[RCommonSettings::SupportLightMax];
	};
	struct RPerFrameConstantBuffer
	{
		RPerFrameConstantBuffer() { ::ZeroMemory(this, sizeof(*this)); }
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
		DirectX::XMFLOAT4X4		WorldMatrix;
		DirectX::XMFLOAT4X4		WorldInvMatrix;
		DirectX::XMFLOAT4X4		WorldInvTransposeMatrix;
		DirectX::XMFLOAT4		CustomParameter;
	};
	struct RPerSkeletonConstantBuffer
	{
		RPerSkeletonConstantBuffer() { ::ZeroMemory(this, sizeof(*this)); }
		DirectX::XMFLOAT4		SkeletonBoneNum;
		DirectX::XMFLOAT4X4		SkeletonMatrix[RCommonSettings::SkeletonBoneMax * 2u];
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
	struct RViewport
	{
		RViewport() { ::ZeroMemory(this, sizeof(*this)); }
		RViewport(const Vector2& startPos, const Vector2& rectSize, const Vector2& depthSize) noexcept : TopLeftX(rectSize.x), TopLeftY(rectSize.y), Width(rectSize.x), Height(rectSize.y), MinDepth(depthSize.x), MaxDepth(depthSize.y) {}
		constexpr RViewport(const Vector2& startPos, const Vector2& rectSize, const Vector2& depthSize) noexcept : TopLeftX(rectSize.x), TopLeftY(rectSize.y), Width(rectSize.x), Height(rectSize.y), MinDepth(depthSize.x), MaxDepth(depthSize.y) {}
		FLOAT	TopLeftX;
		FLOAT	TopLeftY;
		FLOAT	Width;
		FLOAT	Height;
		FLOAT	MinDepth;
		FLOAT	MaxDepth;
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
		FILL_WIREFRAME	= 1,
		FILL_SOLID		= 2
	};
	struct RRasterizerState
	{
		RRasterizerState() noexcept : CullMode(RCullModeType::CULL_BACK), FillMode(RFillModeType::FILL_SOLID) {}
		RRasterizerState(RCullModeType cull, RFillModeType fill) noexcept : CullMode(cull), FillMode(fill) {}
		constexpr RRasterizerState() noexcept : CullMode(RCullModeType::CULL_BACK), FillMode(RFillModeType::FILL_SOLID) {}
		constexpr RRasterizerState(RCullModeType cull, RFillModeType fill) noexcept : CullMode(cull), FillMode(fill) {}
		RCullModeType	CullMode;
		RFillModeType	FillMode;
	};
	enum RDepthWriteMaskType
	{
		DEPTH_WRITE_MASK_ZERO	= 1,
		DEPTH_WRITE_MASK_ALL	= 2
	};
	struct RDepthState
	{
		RDepthState() noexcept : DepthEnable(FALSE), DepthWriteMask(RDepthWriteMaskType::DEPTH_WRITE_MASK_ZERO), DepthFunc(RComparisonFunctionType::COMPARISON_NEVER) {}
		RDepthState(RComparisonFunctionType func, RDepthWriteMaskType writeMask = RDepthWriteMaskType::DEPTH_WRITE_MASK_ALL) noexcept : DepthEnable(TRUE), DepthWriteMask(writeMask), DepthFunc(func) {}
		constexpr RDepthState() noexcept : DepthEnable(FALSE), DepthWriteMask(RDepthWriteMaskType::DEPTH_WRITE_MASK_ZERO), DepthFunc(RComparisonFunctionType::COMPARISON_NEVER) {}
		constexpr RDepthState(RComparisonFunctionType func, RDepthWriteMaskType writeMask = RDepthWriteMaskType::DEPTH_WRITE_MASK_ALL) noexcept : DepthEnable(TRUE), DepthWriteMask(writeMask), DepthFunc(func) {}
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
		RStencilStateType(RComparisonFunctionType func, RStencilOperationType passOp, RStencilOperationType depthFailOp = RStencilOperationType::STENCIL_OP_KEEP, RStencilOperationType stencilFailOp = RStencilOperationType::STENCIL_OP_KEEP)
		{
			StencilFailOp		= stencilFailOp;
			StencilDepthFailOp	= depthFailOp;
			StencilPassOp		= passOp;
			StencilFunc			= func;
		}
		RStencilOperationType		StencilFailOp;
		RStencilOperationType		StencilDepthFailOp;
		RStencilOperationType		StencilPassOp;
		RComparisonFunctionType		StencilFunc;
	};
	struct RStencilState
	{
		RStencilState()
		{
			StencilEnable		= FALSE;
			StencilReadMask		= ENGINE_DEFAULT_STENCIL_READ_MASK;
			StencilWriteMask	= ENGINE_DEFAULT_STENCIL_WRITE_MASK;
			{
				FrontFace.StencilFailOp			= RStencilOperationType::STENCIL_OP_KEEP;
				FrontFace.StencilDepthFailOp	= RStencilOperationType::STENCIL_OP_KEEP;
				FrontFace.StencilPassOp			= RStencilOperationType::STENCIL_OP_KEEP;
				FrontFace.StencilFunc			= RComparisonFunctionType::COMPARISON_NEVER;
			}
			{
				BackFace.StencilFailOp			= RStencilOperationType::STENCIL_OP_KEEP;
				BackFace.StencilDepthFailOp		= RStencilOperationType::STENCIL_OP_KEEP;
				BackFace.StencilPassOp			= RStencilOperationType::STENCIL_OP_KEEP;
				BackFace.StencilFunc			= RComparisonFunctionType::COMPARISON_NEVER;
			}
		}
		RStencilState(RStencilMaskType readMask, RStencilMaskType writeMask, const RStencilStateType& frontFace, const RStencilStateType& backFace)
		{
			StencilEnable		= TRUE;
			StencilReadMask		= static_cast<UINT>(readMask);
			StencilWriteMask	= static_cast<UINT>(writeMask);
			FrontFace			= frontFace;
			BackFace			= backFace;
		}
		RStencilState(const UINT& readMask, const UINT& writeMask, const RStencilStateType& frontFace, const RStencilStateType& backFace)
		{
			StencilEnable		= TRUE;
			StencilReadMask		= EMath::Min(readMask, 0xffu);
			StencilWriteMask	= EMath::Min(writeMask, 0xffu);
			FrontFace			= frontFace;
			BackFace			= backFace;
		}
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
		RBlendState()
		{
			BlendEnable				= FALSE;
			SrcBlend				= RBlendOptionType::BLEND_ONE;
			DstBlend				= RBlendOptionType::BLEND_ZERO;
			BlendOp					= RBlendOperationType::BLEND_OP_ADD;
			SrcBlendAlpha			= RBlendOptionType::BLEND_ONE;
			DstBlendAlpha			= RBlendOptionType::BLEND_ZERO;
			BlendOpAlpha			= RBlendOperationType::BLEND_OP_ADD;
			RenderTargetWriteMask	= RColorWriteMaskType::COLOR_WRITE_MASK_ALL;
		}
		RBlendState(RBlendOptionType src, RBlendOptionType dst, RBlendOperationType op, RBlendOptionType srcA, RBlendOptionType dstA, RBlendOperationType opA, RColorWriteMaskType writeEnable = RColorWriteMaskType::COLOR_WRITE_MASK_ALL)
		{
			BlendEnable				= TRUE;
			SrcBlend				= src;
			DstBlend				= dst;
			BlendOp					= op;
			SrcBlendAlpha			= srcA;
			DstBlendAlpha			= dstA;
			BlendOpAlpha			= opA;
			RenderTargetWriteMask	= writeEnable;
		}
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
		RSamplerState()
		{
			Filter			= FILTER_LINEAR;
			AddressU		= AddressV = AddressW = TEXTURE_ADDRESS_CLAMP;
			MipLODBias		= 0.f;
			MaxAnisotropy	= 1u;
			ComparisonFunc	= COMPARISON_ALWAYS;
			BorderColor		= Color4(0.f, 0.f, 0.f, 0.f);
			MinLOD			= 0.f;
			MaxLOD			= ENGINE_FLOAT32_MAX;
		}
		RSamplerState(RFilterType filter, RTextureAddressModeType u, RTextureAddressModeType v, RTextureAddressModeType w, const FLOAT& maxLOD = ENGINE_FLOAT32_MAX, const FLOAT& minLOD = 0.f, const UINT& maxAnisotropy = 1u, const FLOAT& mipLODBias = 0.f, const Color4& borderColor = Color4(0.f, 0.f, 0.f, 0.f), RComparisonFunctionType func = RComparisonFunctionType::COMPARISON_ALWAYS)
		{
			Filter			= filter;
			AddressU		= u;
			AddressV		= v;
			AddressW		= w;
			MipLODBias		= mipLODBias;
			MaxAnisotropy	= maxAnisotropy;
			ComparisonFunc	= func;
			BorderColor		= borderColor;
			MinLOD			= minLOD;
			MaxLOD			= maxLOD;
		}
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
		UAV_FLAG_RAW		= 1,
		UAV_FLAG_APPEND		= 2,
		UAV_FLAG_COUNTER	= 3
	};
	struct RBufferDesc
	{
		RBufferDesc()
		{
			ByteWidth			= 0u;
			Usage				= RUsageFlagType::USAGE_DEFAULT;
			BindFlags			= RBindFlagType::BIND_NONE;
			CPUAccessFlags		= RCPUAccessFlagType::CPU_ACCESS_DEFAULT;
			MiscFlags			= RResourceMiscFlagType::RESOURCE_MISC_NONE;
			StructureByteStride	= sizeof(FLOAT);
		}
		RBufferDesc(const UINT& byteWidth, RBindFlagType bindFlag, const UINT& structureByteStride, RResourceMiscFlagType miscFlag = RResourceMiscFlagType::RESOURCE_MISC_NONE, RUsageFlagType usage = RUsageFlagType::USAGE_DEFAULT, RCPUAccessFlagType cpuAccessFlag = RCPUAccessFlagType::CPU_ACCESS_DEFAULT)
		{
			ByteWidth			= byteWidth;
			Usage				= usage;
			BindFlags			= bindFlag;
			CPUAccessFlags		= cpuAccessFlag;
			MiscFlags			= miscFlag;
			StructureByteStride	= structureByteStride;
		}
		UINT					ByteWidth;
		RUsageFlagType			Usage;
		RBindFlagType			BindFlags;
		RCPUAccessFlagType		CPUAccessFlags;
		RResourceMiscFlagType	MiscFlags;
		UINT					StructureByteStride;
	};
	struct RSubresourceDataDesc
	{
		RSubresourceDataDesc() { ::ZeroMemory(this, sizeof(*this)); }
		RSubresourceDataDesc(const void* mem, const UINT& sysMemPitch, const UINT& sysMemSlicePitch)
		{
			pSysMem				= mem;
			SysMemPitch			= sysMemPitch;
			SysMemSlicePitch	= sysMemSlicePitch;
		}
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
		RTextureSampleDesc()
		{
			Count = 1u;
			Quality = 0u;
		}
		RTextureSampleDesc(const UINT& count, const UINT& quality)
		{
			Count = count;
			Quality = quality;
		}
		UINT	Count;
		UINT	Quality;
	};

	struct CRenderTextureDesc
	{
		CRenderTextureDesc()
		{
			Width = 2u;
			Height = 2u;
			Depth = 0u;
			MipLevels = 1u;
			ArraySize = 1u;
			TextureFormat = CRenderFormat::FORMAT_UNKNOWN;
			SRVFormat = CRenderFormat::FORMAT_UNKNOWN;
			RTVFormat = CRenderFormat::FORMAT_UNKNOWN;
			UAVFormat = CRenderFormat::FORMAT_UNKNOWN;
			SampleDesc = CRenderTextureSampleDesc(1u, 0u);
			Usage = CRenderUsage::USAGE_DEFAULT;
			BindFlags = CRenderBindFlag::BIND_NONE;
			CPUAccessFlags = CRenderCPUAccessFlag::CPU_ACCESS_NONE;
			MiscFlags = CRenderResourceMiscFlag::RESOURCE_MISC_NONE;
		}
		CRenderTextureDesc(const UINT& width, const UINT& height, CRenderBindFlag bindFlags, CRenderFormat texFormat, const CRenderFormat* srvFormat = NULL, const CRenderFormat* rtvFormat = NULL, const CRenderFormat* uavFormat = NULL, const UINT& depth = 0u, const UINT& mipLevels = 1u, const UINT& arraySize = 1u, CRenderUsage usage = CRenderUsage::USAGE_DEFAULT, CRenderCPUAccessFlag cpuAccessFlags = CRenderCPUAccessFlag::CPU_ACCESS_NONE, CRenderResourceMiscFlag miscFlags = CRenderResourceMiscFlag::RESOURCE_MISC_NONE, CRenderTextureSampleDesc sampleDesc = CRenderTextureSampleDesc(1u, 0u))
		{
			Width = width;
			Height = height;
			Depth = depth;
			MipLevels = mipLevels;
			ArraySize = arraySize;
			SampleDesc = sampleDesc;
			Usage = usage;
			BindFlags = bindFlags;
			CPUAccessFlags = cpuAccessFlags;
			MiscFlags = miscFlags;
			TextureFormat = SRVFormat = UAVFormat = RTVFormat = texFormat;
			if (srvFormat != NULL)
				SRVFormat = (*srvFormat);
			if (rtvFormat != NULL)
				RTVFormat = (*rtvFormat);
			if (uavFormat != NULL)
				UAVFormat = (*uavFormat);
		}
		UINT						Width;
		UINT						Height;
		UINT						Depth;
		UINT						MipLevels;
		UINT						ArraySize;
		CRenderFormat				TextureFormat;
		CRenderFormat				SRVFormat;
		CRenderFormat				RTVFormat;
		CRenderFormat				UAVFormat;
		CRenderTextureSampleDesc	SampleDesc;
		CRenderUsage				Usage;
		CRenderBindFlag				BindFlags;
		CRenderCPUAccessFlag		CPUAccessFlags;
		CRenderResourceMiscFlag		MiscFlags;
	};

	struct CRenderStructuredBufferDesc
	{
		CRenderStructuredBufferDesc(const UINT& structureSize, const UINT& numElements, const BOOL& writableGPU = FALSE, CRenderCPUAccessFlag accessFlag = CRenderCPUAccessFlag::CPU_ACCESS_NONE, const UINT& firstElement = 0u, CRenderBufferUAVFlag uavFlag = CRenderBufferUAVFlag::BUFFER_UAV_FLAG_NONE)
		{
			GPUWritable = writableGPU;
			AccessFlag = accessFlag;
			StructureSize = structureSize;
			FirstElement = firstElement;
			NumElements = numElements;
			BufferFlag = uavFlag;
		}

		BOOL					GPUWritable;
		CRenderCPUAccessFlag	AccessFlag;
		UINT					StructureSize;
		UINT					FirstElement;
		UINT					NumElements;
		CRenderBufferUAVFlag	BufferFlag;
	};

	enum CRenderShaderSemantic
	{
		SHADER_SEMANTIC_POSITION = 0,
		SHADER_SEMANTIC_TEXCOORD = 1,
		SHADER_SEMANTIC_NORMAL = 2,
		SHADER_SEMANTIC_TANGENT = 3,
		SHADER_SEMANTIC_COLOR = 4,
		SHADER_SEMANTIC_BLENDINDICES = 5,
		SHADER_SEMANTIC_BLENDWEIGHT = 6,
		SHADER_SEMANTIC_POSITIONT = 7,
		SHADER_SEMANTIC_PSIZE = 8,
		SHADER_SEMANTIC_BINORMAL = 9
	};

	enum CRenderInputClassification
	{
		INPUT_PER_VERTEX_DATA = 0,
		INPUT_PER_INSTANCE_DATA = 1
	};

	struct CRenderInputLayoutDesc
	{
		CRenderInputLayoutDesc()
		{
			SemanticName = CRenderShaderSemantic::SHADER_SEMANTIC_POSITION;
			SemanticIndex = 0u;
			InputSlot = 0u;
			InputSlotClass = CRenderInputClassification::INPUT_PER_VERTEX_DATA;
			InstanceDataStepRate = 0u;
		}
		CRenderInputLayoutDesc(const CRenderInputLayoutDesc& desc)
		{
			SemanticName = desc.SemanticName;
			SemanticIndex = desc.SemanticIndex;
			InputSlot = desc.InputSlot;
			InputSlotClass = desc.InputSlotClass;
			InstanceDataStepRate = desc.InstanceDataStepRate;
		}
		CRenderInputLayoutDesc(CRenderShaderSemantic name, const UINT& semanticIndex = 0u, const UINT& inputSlot = 0u, CRenderInputClassification inputSlotClass = CRenderInputClassification::INPUT_PER_VERTEX_DATA, const UINT& instanceDataStepRate = 0u)
		{
			SemanticName = name;
			SemanticIndex = semanticIndex;
			InputSlot = inputSlot;
			InputSlotClass = inputSlotClass;
			InstanceDataStepRate = instanceDataStepRate;
		}
		const UINT& GetSemanticSizeByByte()const
		{
			static std::map<CRenderShaderSemantic, UINT> semanticSizeByByteMap = {
				{ CRenderShaderSemantic::SHADER_SEMANTIC_POSITION, 16u },
				{ CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD, 8u },
				{ CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL, 16u },
				{ CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT, 16u },
				{ CRenderShaderSemantic::SHADER_SEMANTIC_COLOR, 16u },
				{ CRenderShaderSemantic::SHADER_SEMANTIC_BLENDINDICES, 8u },
				{ CRenderShaderSemantic::SHADER_SEMANTIC_BLENDWEIGHT, 16u },
				{ CRenderShaderSemantic::SHADER_SEMANTIC_POSITIONT, 16u },
				{ CRenderShaderSemantic::SHADER_SEMANTIC_PSIZE, 16u },
				{ CRenderShaderSemantic::SHADER_SEMANTIC_BINORMAL, 16u } };

			return semanticSizeByByteMap[SemanticName];
		}
		const UINT& GetSemanticSizeByFloat()const
		{
			static std::map<CRenderShaderSemantic, UINT> semanticSizeByFloatMap = {
				{ CRenderShaderSemantic::SHADER_SEMANTIC_POSITION, 4u },
				{ CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD, 2u },
				{ CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL, 4u },
				{ CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT, 4u },
				{ CRenderShaderSemantic::SHADER_SEMANTIC_COLOR, 4u },
				{ CRenderShaderSemantic::SHADER_SEMANTIC_BLENDWEIGHT, 4u },
				{ CRenderShaderSemantic::SHADER_SEMANTIC_POSITIONT, 4u },
				{ CRenderShaderSemantic::SHADER_SEMANTIC_PSIZE, 4u },
				{ CRenderShaderSemantic::SHADER_SEMANTIC_BINORMAL, 4u } };

			return semanticSizeByFloatMap[SemanticName];
		}
		CRenderShaderSemantic		SemanticName;
		UINT						SemanticIndex;
		UINT						InputSlot;
		CRenderInputClassification	InputSlotClass;
		UINT						InstanceDataStepRate;
		static void GetEngineDefaultMeshInputLayouts(const CustomStruct::CRenderInputLayoutDesc*& inputLayoutDesc, UINT& inputLayoutNum)
		{
			const static CustomStruct::CRenderInputLayoutDesc _EngineDefaultMeshInputLayout[4u] = {
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };

			inputLayoutDesc = _EngineDefaultMeshInputLayout;
			inputLayoutNum = 4u;
		}
		static void GetEngineSkeletonMeshInputLayouts(const CustomStruct::CRenderInputLayoutDesc*& inputLayoutDesc, UINT& inputLayoutNum)
		{
			const static CustomStruct::CRenderInputLayoutDesc _EngineSkeletonMeshInputLayout[6u] = {
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD),
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_BLENDINDICES),
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_BLENDWEIGHT) };

			inputLayoutDesc = _EngineSkeletonMeshInputLayout;
			inputLayoutNum = 6u;
		}
	};

	enum CRenderMapType
	{
		MAP_READ = 0,
		MAP_WRITE = 1,
		MAP_READ_WRITE = 2,
		MAP_WRITE_DISCARD = 3,
		MAP_WRITE_NO_OVERWRITE = 4
	};

	enum CRenderMapFlag
	{
		MAP_FLAG_NONE = 0,
		MAP_FLAG_DO_NOT_WAIT = 1
	};

	struct CRenderMappedResource
	{
		CRenderMappedResource() { ::ZeroMemory(this, sizeof(*this)); }
		void* pData;
		UINT	RowPitch;
		UINT	DepthPitch;
	};

	enum CRenderQueryType
	{
		QUERY_EVENT = 0,
		QUERY_OCCLUSION = 1,
		QUERY_TIMESTAMP = 2,
		QUERY_TIMESTAMP_DISJOINT = 3,
		QUERY_PIPELINE_STATISTICS = 4,
		QUERY_OCCLUSION_PREDICATE = 5,
		QUERY_SO_STATISTICS = 6,
		QUERY_SO_OVERFLOW_PREDICATE = 7,
		QUERY_SO_STATISTICS_STREAM0 = 8,
		QUERY_SO_OVERFLOW_PREDICATE_STREAM0 = 9,
		QUERY_SO_STATISTICS_STREAM1 = 10,
		QUERY_SO_OVERFLOW_PREDICATE_STREAM1 = 11,
		QUERY_SO_STATISTICS_STREAM2 = 12,
		QUERY_SO_OVERFLOW_PREDICATE_STREAM2 = 13,
		QUERY_SO_STATISTICS_STREAM3 = 14,
		QUERY_SO_OVERFLOW_PREDICATE_STREAM3 = 15
	};

	enum CRenderQueryMiscFlag
	{
		QUERY_MISC_DEFAULT = 0x0,
		QUERY_MISC_PREDICATEHINT = 0x1
	};

	struct CRenderQueryDesc
	{
		CRenderQueryDesc()
		{
			Query = CRenderQueryType::QUERY_EVENT;
			MiscFlags = CRenderQueryMiscFlag::QUERY_MISC_DEFAULT;
		}
		CRenderQueryDesc(CRenderQueryType type, CRenderQueryMiscFlag flag = CRenderQueryMiscFlag::QUERY_MISC_DEFAULT)
		{
			Query = type;
			MiscFlags = flag;
		}
		CRenderQueryType		Query;
		CRenderQueryMiscFlag	MiscFlags;
	};

	enum CRenderAsyncGetDataFlag
	{
		D3D11_ASYNC_GETDATA_DEFAULT = 0x0,
		D3D11_ASYNC_GETDATA_DONOTFLUSH = 0x1
	};

	struct CRenderQueryTimestampDisjoint
	{
		CRenderQueryTimestampDisjoint()
		{
			Frequency = 1u;
			Disjoint = TRUE;
		}
		void Reset()
		{
			Frequency = 1u;
			Disjoint = TRUE;
		}
		ULONGLONG	Frequency;
		BOOL		Disjoint;
	};

	enum CRenderClearDepthStencilFlag
	{
		CLEAR_DEPTH = 1,
		CLEAR_STENCIL = 2,
		CLEAR_DEPTH_STENCIL = 3
	};

	enum CRenderPrimitiveTopology
	{
		PRIMITIVE_TOPOLOGY_UNDEFINED = 0,
		PRIMITIVE_TOPOLOGY_POINTLIST = 1,
		PRIMITIVE_TOPOLOGY_LINELIST = 2,
		PRIMITIVE_TOPOLOGY_LINESTRIP = 3,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST = 4,
		PRIMITIVE_TOPOLOGY_TRIANGLESTRIP = 5
	};

	class CEngineDefaultDefines
	{
	public:
		CEngineDefaultDefines() {}
		~CEngineDefaultDefines() {}
	public:
		static std::string GetDefaultTexturePath(const INT& input)
		{
			static std::map<CEngineDefaultTexture2DType, std::string> engineDefaultTexturePathMap = {
				{ CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE, std::string(ENGINE_DEFAULT_TEXTURE2D_WHITE) },
				{ CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BLACK, std::string(ENGINE_DEFAULT_TEXTURE2D_BLACK) },
				{ CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_GRAY, std::string(ENGINE_DEFAULT_TEXTURE2D_GRAY) },
				{ CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_RED, std::string(ENGINE_DEFAULT_TEXTURE2D_RED) },
				{ CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_GREEN, std::string(ENGINE_DEFAULT_TEXTURE2D_GREEN) },
				{ CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BLUE, std::string(ENGINE_DEFAULT_TEXTURE2D_BLUE) },
				{ CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BUMP, std::string(ENGINE_DEFAULT_TEXTURE2D_BUMP) },
				{ CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_PROPERTY, std::string(ENGINE_DEFAULT_TEXTURE2D_PROPERTY) } };

			return engineDefaultTexturePathMap[static_cast<CEngineDefaultTexture2DType>(input)];
		}
	};
};