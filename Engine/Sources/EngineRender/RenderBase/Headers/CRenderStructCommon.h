#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../../EngineBase/Headers/CBaseType.h"

namespace CustomStruct
{
	class CRenderBaseSetting
	{
	public:
		constexpr static FLOAT RenderBoundMinimum		= 0.01f;
		constexpr static FLOAT RenderBoundHalfMinimum	= 0.005f;
	};

	struct CColor
	{
		CColor() { r = 1.f; g = 1.f; b = 1.f; a = 1.f; }
		CColor(const FLOAT& _r, const FLOAT& _g, const FLOAT& _b, const FLOAT& _a)
		{
			r = _r; g = _g;
			b = _b; a = _a;
		}
		CColor(const FLOAT& _r, const FLOAT& _g, const FLOAT& _b)
		{
			r = _r; g = _g;
			b = _b; a = 1.f;
		}
		union
		{
			FLOAT rgba[4];

			struct
			{
				FLOAT	r;
				FLOAT	g;
				FLOAT	b;
				FLOAT	a;
			};
		};
	};

	struct CRenderBoundingBox
	{
		CRenderBoundingBox(const CustomType::Vector3& anchor, const CustomType::Vector3& dimensions)
		{
			Anchor		= anchor;
			Dimensions	= dimensions;
		}
		CustomType::Vector3		Anchor;
		CustomType::Vector3		Dimensions;
	};

	struct CRenderBoundingSphere
	{
		CRenderBoundingSphere(const CustomType::Vector3& anchor, const FLOAT& radius)
		{
			Anchor	= anchor;
			Radius	= radius;
		}
		CustomType::Vector3		Anchor;
		FLOAT					Radius;
	};

	struct CSubMeshInfo
	{
		CSubMeshInfo() { ::ZeroMemory(this, sizeof(*this)); }
		UINT	VertexStart;
		UINT	VertexCount;
		UINT	IndexStart;
		UINT	IndexCount;
	};

	struct CShaderGlobalPerFrame
	{
		CShaderGlobalPerFrame() { ::ZeroMemory(this, sizeof(*this)); }
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

	struct CShaderGlobalPerDraw
	{
		CShaderGlobalPerDraw() { ::ZeroMemory(this, sizeof(*this)); }
		DirectX::XMFLOAT4X4		WorldMatrix;
		DirectX::XMFLOAT4X4		WorldInvMatrix;
		DirectX::XMFLOAT4X4		WorldInvTransposeMatrix;
		DirectX::XMFLOAT4		CustomParameter;
	};

	struct CShaderLightParams
	{
		CShaderLightParams() { ::ZeroMemory(this, sizeof(*this)); }
		DirectX::XMFLOAT4	Color;
		DirectX::XMFLOAT4	Params0;
		DirectX::XMFLOAT4	Params1;
	};

	struct CShaderGlobalLightData
	{
		CShaderGlobalLightData() { ::ZeroMemory(this, sizeof(*this)); }
		static const UINT& GetSupportLightMaxCount()
		{
			const static UINT supportLightMaxCount = 16u;
			return supportLightMaxCount;
		}
		DirectX::XMINT4			LightCount;
		CShaderLightParams		LightParams[16];
	};

	enum CEngineDefaultTexture2DType
	{
		ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE		= 0,
		ENGINE_DEFAULT_TEXTURE2D_TYPE_BLACK		= 1,
		ENGINE_DEFAULT_TEXTURE2D_TYPE_GRAY		= 2,
		ENGINE_DEFAULT_TEXTURE2D_TYPE_RED		= 3,
		ENGINE_DEFAULT_TEXTURE2D_TYPE_GREEN		= 4,
		ENGINE_DEFAULT_TEXTURE2D_TYPE_BLUE		= 5,
		ENGINE_DEFAULT_TEXTURE2D_TYPE_BUMP		= 6,
		ENGINE_DEFAULT_TEXTURE2D_TYPE_PROPERTY	= 7,
		ENGINE_DEFAULT_TEXTURE2D_TYPE_COUNT
	};

	struct CRenderViewport
	{
		CRenderViewport() { ::ZeroMemory(this, sizeof(*this)); }
		CRenderViewport(const CustomType::Vector4& rectSize, const CustomType::Vector2& depthSize)
		{
			TopLeftX	= rectSize.X();
			TopLeftY	= rectSize.Y();
			Width		= rectSize.Z();
			Height		= rectSize.W();
			MinDepth	= depthSize.X();
			MaxDepth	= depthSize.Y();
		}
		FLOAT TopLeftX;
		FLOAT TopLeftY;
		FLOAT Width;
		FLOAT Height;
		FLOAT MinDepth;
		FLOAT MaxDepth;
	};

	enum CRenderComparisonFunction
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

	enum CRenderCullMode
	{
		CULL_NONE	= 1,
		CULL_FRONT	= 2,
		CULL_BACK	= 3
	};

	enum CRenderFillMode
	{
		FILL_WIREFRAME	= 1,
		FILL_SOLID		= 2
	};

	struct CRenderRasterizerState
	{
		CRenderRasterizerState()
		{
			CullMode = CRenderCullMode::CULL_BACK;
			FillMode = CRenderFillMode::FILL_SOLID;
		}
		CRenderRasterizerState(CRenderCullMode cull, CRenderFillMode fill)
		{
			CullMode = cull;
			FillMode = fill;
		}
		CRenderCullMode CullMode;
		CRenderFillMode FillMode;
	};

	enum CRenderDepthWriteMask
	{
		DEPTH_WRITE_MASK_ZERO	= 1,
		DEPTH_WRITE_MASK_ALL	= 2
	};

	struct CRenderDepthState
	{
		CRenderDepthState()
		{
			DepthEnable		= FALSE;
			DepthWriteMask	= CRenderDepthWriteMask::DEPTH_WRITE_MASK_ZERO;
			DepthFunc		= CRenderComparisonFunction::COMPARISON_NEVER;
		}
		CRenderDepthState(CRenderComparisonFunction func, CRenderDepthWriteMask writeMask = CRenderDepthWriteMask::DEPTH_WRITE_MASK_ALL)
		{
			DepthEnable		= TRUE;
			DepthWriteMask	= writeMask;
			DepthFunc		= func;
		}
		BOOL						DepthEnable;
		CRenderDepthWriteMask		DepthWriteMask;
		CRenderComparisonFunction	DepthFunc;
	};

	enum CRenderStencilMask
	{
		STENCIL_MASK_SKY			= 0x1,
		STENCIL_MASK_LAND			= 0x2,
		STENCIL_MASK_OPAQUE_BASE	= 0x4
	};

	enum CRenderStencilOperation
	{
		STENCIL_OP_KEEP		= 1,
		STENCIL_OP_ZERO		= 2,
		STENCIL_OP_REPLACE	= 3,
		STENCIL_OP_INCR_SAT	= 4,
		STENCIL_OP_DECR_SAT	= 5,
		STENCIL_OP_INVERT	= 6,
		STENCIL_OP_INCR		= 7,
		STENCIL_OP_DECR		= 8
	};

	struct CRenderStencilOp
	{
		CRenderStencilOp()
		{
			StencilFailOp		= CRenderStencilOperation::STENCIL_OP_KEEP;
			StencilDepthFailOp	= CRenderStencilOperation::STENCIL_OP_KEEP;
			StencilPassOp		= CRenderStencilOperation::STENCIL_OP_KEEP;
			StencilFunc			= CRenderComparisonFunction::COMPARISON_NEVER;
		}
		CRenderStencilOp(CRenderComparisonFunction func, CRenderStencilOperation passOp, CRenderStencilOperation depthFailOp = CRenderStencilOperation::STENCIL_OP_KEEP, CRenderStencilOperation stencilFailOp = CRenderStencilOperation::STENCIL_OP_KEEP)
		{
			StencilFailOp		= stencilFailOp;
			StencilDepthFailOp	= depthFailOp;
			StencilPassOp		= passOp;
			StencilFunc			= func;
		}
		CRenderStencilOperation		StencilFailOp;
		CRenderStencilOperation		StencilDepthFailOp;
		CRenderStencilOperation		StencilPassOp;
		CRenderComparisonFunction	StencilFunc;
	};

	struct CRenderStencilState
	{
		CRenderStencilState()
		{
			StencilEnable		= FALSE;
			StencilReadMask		= ENGINE_DEFAULT_STENCIL_READ_MASK;
			StencilWriteMask	= ENGINE_DEFAULT_STENCIL_WRITE_MASK;
			{
				FrontFace.StencilFailOp			= CRenderStencilOperation::STENCIL_OP_KEEP;
				FrontFace.StencilDepthFailOp	= CRenderStencilOperation::STENCIL_OP_KEEP;
				FrontFace.StencilPassOp			= CRenderStencilOperation::STENCIL_OP_KEEP;
				FrontFace.StencilFunc			= CRenderComparisonFunction::COMPARISON_NEVER;
			}

			{
				BackFace.StencilFailOp		= CRenderStencilOperation::STENCIL_OP_KEEP;
				BackFace.StencilDepthFailOp = CRenderStencilOperation::STENCIL_OP_KEEP;
				BackFace.StencilPassOp		= CRenderStencilOperation::STENCIL_OP_KEEP;
				BackFace.StencilFunc		= CRenderComparisonFunction::COMPARISON_NEVER;
			}
		}
		CRenderStencilState(CRenderStencilMask readMask, CRenderStencilMask writeMask, const CRenderStencilOp& frontFace, const CRenderStencilOp& backFace)
		{
			StencilEnable		= TRUE;
			StencilReadMask		= static_cast<UINT>(readMask);
			StencilWriteMask	= static_cast<UINT>(writeMask);
			FrontFace			= frontFace;
			BackFace			= backFace;
		}
		CRenderStencilState(UINT readMask, UINT writeMask, const CRenderStencilOp& frontFace, const CRenderStencilOp& backFace)
		{
			StencilEnable		= TRUE;
			StencilReadMask		= CustomType::CMath::Min(readMask, 0xffu);
			StencilWriteMask	= CustomType::CMath::Min(writeMask, 0xffu);
			FrontFace			= frontFace;
			BackFace			= backFace;
		}
		BOOL				StencilEnable;
		UINT				StencilReadMask;
		UINT				StencilWriteMask;
		CRenderStencilOp	FrontFace;
		CRenderStencilOp	BackFace;
	};

	enum CRenderBlendOption
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

	enum CRenderBlendOperation
	{
		BLEND_OP_ADD			= 1,
		BLEND_OP_SUBTRACT		= 2,
		BLEND_OP_REV_SUBTRACT	= 3,
		BLEND_OP_MIN			= 4,
		BLEND_OP_MAX			= 5
	};

	enum CRenderBlendWriteEnable
	{
		COLOR_WRITE_ENABLE_RED		= 1,
		COLOR_WRITE_ENABLE_GREEN	= 2,
		COLOR_WRITE_ENABLE_BLUE		= 4,
		COLOR_WRITE_ENABLE_ALPHA	= 8,
		COLOR_WRITE_ENABLE_ALL		= (((D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN) | D3D11_COLOR_WRITE_ENABLE_BLUE) | D3D11_COLOR_WRITE_ENABLE_ALPHA)
	};

	struct CRenderBlendState
	{
		CRenderBlendState()
		{
			BlendEnable				= FALSE;
			SrcBlend				= CRenderBlendOption::BLEND_ONE;
			DstBlend				= CRenderBlendOption::BLEND_ZERO;
			BlendOp					= CRenderBlendOperation::BLEND_OP_ADD;
			SrcBlendAlpha			= CRenderBlendOption::BLEND_ONE;
			DstBlendAlpha			= CRenderBlendOption::BLEND_ZERO;
			BlendOpAlpha			= CRenderBlendOperation::BLEND_OP_ADD;
			RenderTargetWriteMask	= CRenderBlendWriteEnable::COLOR_WRITE_ENABLE_ALL;
		}
		CRenderBlendState(CRenderBlendOption src, CRenderBlendOption dst, CRenderBlendOperation op, CRenderBlendOption srcA, CRenderBlendOption dstA, CRenderBlendOperation opA, CRenderBlendWriteEnable writeEnable = CRenderBlendWriteEnable::COLOR_WRITE_ENABLE_ALL)
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
		CRenderBlendOption		SrcBlend;
		CRenderBlendOption		DstBlend;
		CRenderBlendOperation	BlendOp;
		CRenderBlendOption		SrcBlendAlpha;
		CRenderBlendOption		DstBlendAlpha;
		CRenderBlendOperation	BlendOpAlpha;
		CRenderBlendWriteEnable	RenderTargetWriteMask;
	};

	enum CRenderFilter
	{
		FILTER_POINT		= 1,
		FILTER_LINEAR		= 2,
		FILTER_ANISOTROPIC	= 3,
	};

	enum CRenderTextureAddressMode
	{
		TEXTURE_ADDRESS_WRAP		= 1,
		TEXTURE_ADDRESS_MIRROR		= 2,
		TEXTURE_ADDRESS_CLAMP		= 3,
		TEXTURE_ADDRESS_BORDER		= 4,
		TEXTURE_ADDRESS_MIRROR_ONCE = 5
	};

	struct CRenderSamplerState
	{
		CRenderSamplerState()
		{
			Filter			= FILTER_LINEAR;
			AddressU = AddressV = AddressW = TEXTURE_ADDRESS_CLAMP;
			MipLODBias		= 0.f;
			MaxAnisotropy	= 1u;
			ComparisonFunc	= COMPARISON_ALWAYS;
			BorderColor		= CColor(0.f, 0.f, 0.f, 0.f);
			MinLOD			= 0.f;
			MaxLOD			= ENGINE_FLOAT32_MAX;
		}
		CRenderSamplerState(CRenderFilter filter, CRenderTextureAddressMode u, CRenderTextureAddressMode v, CRenderTextureAddressMode w, const FLOAT& maxLOD = ENGINE_FLOAT32_MAX, const FLOAT& minLOD = 0.f, const UINT& maxAnisotropy = 1u, const FLOAT& mipLODBias = 0.f, const CColor& borderColor = CColor(0.f, 0.f, 0.f, 0.f), CRenderComparisonFunction func = CRenderComparisonFunction::COMPARISON_ALWAYS)
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
		CRenderFilter				Filter;
		CRenderTextureAddressMode	AddressU;
		CRenderTextureAddressMode	AddressV;
		CRenderTextureAddressMode	AddressW;
		FLOAT						MipLODBias;
		UINT						MaxAnisotropy;
		CRenderComparisonFunction	ComparisonFunc;
		CColor						BorderColor;
		FLOAT						MinLOD;
		FLOAT						MaxLOD;
	};

	enum CRenderBindFlag
	{
		BIND_NONE				= 0,
		BIND_VERTEX_BUFFER		= 1,
		BIND_INDEX_BUFFER		= 2,
		BIND_CONSTANT_BUFFER	= 3,
		BIND_SHADER_RESOURCE	= 4,
		BIND_STREAM_OUTPUT		= 5,
		BIND_RENDER_TARGET		= 6,
		BIND_DEPTH_STENCIL		= 7,
		BIND_UNORDERED_ACCESS	= 8,
		BIND_SRV_UAV			= 9,
		BIND_RTV_SRV			= 10,
		BIND_RTV_SRV_UAV		= 11,
	};

	enum CRenderResourceMiscFlag
	{
		RESOURCE_MISC_NONE				= 0,
		RESOURCE_MISC_GENERATE_MIPS		= 1,
		RESOURCE_MISC_TEXTURECUBE		= 2,
		RESOURCE_MISC_DRAWINDIRECT_ARGS	= 3,
		RESOURCE_MISC_BUFFER_STRUCTURED	= 4,
		RESOURCE_MISC_RESOURCE_CLAMP	= 5
	};

	enum CRenderUsage
	{
		USAGE_DEFAULT	= 0,
		USAGE_IMMUTABLE	= 1,
		USAGE_DYNAMIC	= 2,
		USAGE_STAGING	= 3
	};

	enum CRenderCPUAccessFlag
	{
		CPU_ACCESS_NONE			= 0,
		CPU_ACCESS_WRITE		= 1,
		CPU_ACCESS_READ			= 2,
		CPU_ACCESS_READ_WRITE	= 3
	};

	enum CRenderBufferUAVFlag
	{
		BUFFER_UAV_FLAG_NONE	= 0,
		BUFFER_UAV_FLAG_RAW		= 1,
		BUFFER_UAV_FLAG_APPEND	= 2,
		BUFFER_UAV_FLAG_COUNTER	= 3
	};

	struct CRenderBufferDesc
	{
		CRenderBufferDesc()
		{
			ByteWidth			= 0u;
			Usage				= CRenderUsage::USAGE_DEFAULT;
			BindFlags			= CRenderBindFlag::BIND_NONE;
			CPUAccessFlags		= CRenderCPUAccessFlag::CPU_ACCESS_NONE;
			MiscFlags			= CRenderResourceMiscFlag::RESOURCE_MISC_NONE;
			StructureByteStride	= sizeof(FLOAT);
		}
		CRenderBufferDesc(const UINT& byteWidth, CRenderBindFlag bindFlag, const UINT& structureByteStride, CRenderResourceMiscFlag miscFlag = CRenderResourceMiscFlag::RESOURCE_MISC_NONE, CRenderUsage usage = CRenderUsage::USAGE_DEFAULT, CRenderCPUAccessFlag cpuAccessFlag = CRenderCPUAccessFlag::CPU_ACCESS_NONE)
		{
			ByteWidth			= byteWidth;
			Usage				= usage;
			BindFlags			= bindFlag;
			CPUAccessFlags		= cpuAccessFlag;
			MiscFlags			= miscFlag;
			StructureByteStride	= structureByteStride;
		}
		UINT					ByteWidth;
		CRenderUsage			Usage;
		CRenderBindFlag			BindFlags;
		CRenderCPUAccessFlag	CPUAccessFlags;
		CRenderResourceMiscFlag	MiscFlags;
		UINT					StructureByteStride;
	};

	struct CRenderSubresourceData
	{
		CRenderSubresourceData() { ::ZeroMemory(this, sizeof(*this)); }
		CRenderSubresourceData(const void* mem, const UINT& sysMemPitch, const UINT& sysMemSlicePitch)
		{
			pSysMem				= mem;
			SysMemPitch			= sysMemPitch;
			SysMemSlicePitch	= sysMemSlicePitch;
		}
		const void*		pSysMem;
		UINT			SysMemPitch;
		UINT			SysMemSlicePitch;
	};

	struct CRenderBox
	{
		CRenderBox() { ::ZeroMemory(this, sizeof(*this)); }
		CRenderBox(const UINT _left, const UINT _top, const UINT _front, const UINT _right, const UINT _bottom, const UINT _back)
		{
			left	= _left;
			top		= _top;
			front	= _front;
			right	= _right;
			bottom	= _bottom;
			back	= _back;
		}
		UINT	left;
		UINT	top;
		UINT	front;
		UINT	right;
		UINT	bottom;
		UINT	back;
	};

	enum CRenderFormat
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

	struct CRenderTextureSampleDesc
	{
		CRenderTextureSampleDesc()
		{
			Count	= 1u;
			Quality	= 0u;
		}
		CRenderTextureSampleDesc(const UINT& count, const UINT& quality)
		{
			Count	= count;
			Quality = quality;
		}
		UINT	Count;
		UINT	Quality;
	};

	struct CRenderTextureDesc
	{
		CRenderTextureDesc()
		{
			Width			= 2u;
			Height			= 2u;
			Depth			= 0u;
			MipLevels		= 1u;
			ArraySize		= 1u;
			TextureFormat	= CRenderFormat::FORMAT_UNKNOWN;
			SRVFormat		= CRenderFormat::FORMAT_UNKNOWN;
			RTVFormat		= CRenderFormat::FORMAT_UNKNOWN;
			UAVFormat		= CRenderFormat::FORMAT_UNKNOWN;
			SampleDesc		= CRenderTextureSampleDesc(1u, 0u);
			Usage			= CRenderUsage::USAGE_DEFAULT;
			BindFlags		= CRenderBindFlag::BIND_NONE;
			CPUAccessFlags	= CRenderCPUAccessFlag::CPU_ACCESS_NONE;
			MiscFlags		= CRenderResourceMiscFlag::RESOURCE_MISC_NONE;
		}
		CRenderTextureDesc(const UINT& width, const UINT& height, CRenderBindFlag bindFlags, CRenderFormat texFormat, const CRenderFormat* srvFormat = NULL, const CRenderFormat* rtvFormat = NULL, const CRenderFormat* uavFormat = NULL, const UINT& depth = 0u, const UINT& mipLevels = 1u, const UINT& arraySize = 1u, CRenderUsage usage = CRenderUsage::USAGE_DEFAULT, CRenderCPUAccessFlag cpuAccessFlags = CRenderCPUAccessFlag::CPU_ACCESS_NONE, CRenderResourceMiscFlag miscFlags = CRenderResourceMiscFlag::RESOURCE_MISC_NONE, CRenderTextureSampleDesc sampleDesc = CRenderTextureSampleDesc(1u, 0u))
		{
			Width			= width;
			Height			= height;
			Depth			= depth;
			MipLevels		= mipLevels;
			ArraySize		= arraySize;
			SampleDesc		= sampleDesc;
			Usage			= usage;
			BindFlags		= bindFlags;
			CPUAccessFlags	= cpuAccessFlags;
			MiscFlags		= miscFlags;
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
			GPUWritable		= writableGPU;
			AccessFlag		= accessFlag;
			StructureSize	= structureSize;
			FirstElement	= firstElement;
			NumElements		= numElements;
			BufferFlag		= uavFlag;
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
		SHADER_SEMANTIC_POSITION		= 0,
		SHADER_SEMANTIC_TEXCOORD		= 1,
		SHADER_SEMANTIC_NORMAL			= 2,
		SHADER_SEMANTIC_TANGENT			= 3,
		SHADER_SEMANTIC_COLOR			= 4,
		SHADER_SEMANTIC_BLENDINDICES	= 5,
		SHADER_SEMANTIC_BLENDWEIGHT		= 6,
		SHADER_SEMANTIC_POSITIONT		= 7,
		SHADER_SEMANTIC_PSIZE			= 8,
		SHADER_SEMANTIC_BINORMAL		= 9
	};

	enum CRenderInputClassification
	{
		INPUT_PER_VERTEX_DATA	= 0,
		INPUT_PER_INSTANCE_DATA	= 1
	};

	struct CRenderInputLayoutDesc
	{
		CRenderInputLayoutDesc()
		{
			SemanticName			= CRenderShaderSemantic::SHADER_SEMANTIC_POSITION;
			SemanticIndex			= 0u;
			InputSlot				= 0u;
			InputSlotClass			= CRenderInputClassification::INPUT_PER_VERTEX_DATA;
			InstanceDataStepRate	= 0u;
		}
		CRenderInputLayoutDesc(const CRenderInputLayoutDesc& desc)
		{
			SemanticName			= desc.SemanticName;
			SemanticIndex			= desc.SemanticIndex;
			InputSlot				= desc.InputSlot;
			InputSlotClass			= desc.InputSlotClass;
			InstanceDataStepRate	= desc.InstanceDataStepRate;
		}
		CRenderInputLayoutDesc(CRenderShaderSemantic name, const UINT& semanticIndex = 0u, const UINT& inputSlot = 0u, CRenderInputClassification inputSlotClass = CRenderInputClassification::INPUT_PER_VERTEX_DATA, const UINT& instanceDataStepRate = 0u)
		{
			SemanticName			= name;
			SemanticIndex			= semanticIndex;
			InputSlot				= inputSlot;
			InputSlotClass			= inputSlotClass;
			InstanceDataStepRate	= instanceDataStepRate;
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

			inputLayoutDesc	= _EngineDefaultMeshInputLayout;
			inputLayoutNum	= 4u;
		}
	};

	enum CRenderMapType
	{
		MAP_READ				= 0,
		MAP_WRITE				= 1,
		MAP_READ_WRITE			= 2,
		MAP_WRITE_DISCARD		= 3,
		MAP_WRITE_NO_OVERWRITE	= 4
	};

	enum CRenderMapFlag
	{
		MAP_FLAG_NONE			= 0,
		MAP_FLAG_DO_NOT_WAIT	= 1
	};

	struct CRenderMappedResource
	{
		CRenderMappedResource() { ::ZeroMemory(this, sizeof(*this)); }
		void*	pData;
		UINT	RowPitch;
		UINT	DepthPitch;
	};

	enum CRenderQueryType
	{
		QUERY_EVENT							= 0,
		QUERY_OCCLUSION						= 1,
		QUERY_TIMESTAMP						= 2,
		QUERY_TIMESTAMP_DISJOINT			= 3,
		QUERY_PIPELINE_STATISTICS			= 4,
		QUERY_OCCLUSION_PREDICATE			= 5,
		QUERY_SO_STATISTICS					= 6,
		QUERY_SO_OVERFLOW_PREDICATE			= 7,
		QUERY_SO_STATISTICS_STREAM0			= 8,
		QUERY_SO_OVERFLOW_PREDICATE_STREAM0	= 9,
		QUERY_SO_STATISTICS_STREAM1			= 10,
		QUERY_SO_OVERFLOW_PREDICATE_STREAM1	= 11,
		QUERY_SO_STATISTICS_STREAM2			= 12,
		QUERY_SO_OVERFLOW_PREDICATE_STREAM2	= 13,
		QUERY_SO_STATISTICS_STREAM3			= 14,
		QUERY_SO_OVERFLOW_PREDICATE_STREAM3	= 15
	};

	enum CRenderQueryMiscFlag
	{
		QUERY_MISC_DEFAULT			= 0x0,
		QUERY_MISC_PREDICATEHINT	= 0x1
	};

	struct CRenderQueryDesc
	{
		CRenderQueryDesc()
		{
			Query		= CRenderQueryType::QUERY_EVENT;
			MiscFlags	= CRenderQueryMiscFlag::QUERY_MISC_DEFAULT;
		}
		CRenderQueryDesc(CRenderQueryType type, CRenderQueryMiscFlag flag = CRenderQueryMiscFlag::QUERY_MISC_DEFAULT)
		{
			Query		= type;
			MiscFlags	= flag;
		}
		CRenderQueryType		Query;
		CRenderQueryMiscFlag	MiscFlags;
	};

	enum CRenderAsyncGetDataFlag
	{
		D3D11_ASYNC_GETDATA_DEFAULT		= 0x0,
		D3D11_ASYNC_GETDATA_DONOTFLUSH	= 0x1
	};

	struct CRenderQueryTimestampDisjoint
	{
		CRenderQueryTimestampDisjoint()
		{
			Frequency	= 1u;
			Disjoint	= TRUE;
		}
		void Reset()
		{
			Frequency	= 1u;
			Disjoint	= TRUE;
		}
		ULONGLONG	Frequency;
		BOOL		Disjoint;
	};

	enum CRenderClearDepthStencilFlag
	{
		CLEAR_DEPTH			= 1,
		CLEAR_STENCIL		= 2,
		CLEAR_DEPTH_STENCIL	= 3
	};

	enum CRenderPrimitiveTopology
	{
		PRIMITIVE_TOPOLOGY_UNDEFINED		= 0,
		PRIMITIVE_TOPOLOGY_POINTLIST		= 1,
		PRIMITIVE_TOPOLOGY_LINELIST			= 2,
		PRIMITIVE_TOPOLOGY_LINESTRIP		= 3,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST		= 4,
		PRIMITIVE_TOPOLOGY_TRIANGLESTRIP	= 5
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
}