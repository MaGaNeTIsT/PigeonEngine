#pragma once

#include <CoreMinimal.h>
#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "xaudio2.lib")
#include <RenderConfig/RenderConfig.h>
#include <RenderCommon.h>

namespace PigeonEngine
{

	class RVertexShaderResource : public RRenderResourceInterface
	{
	public:
		RVertexShaderResource() : Shader(nullptr), InputLayout(nullptr), RawLayouts(nullptr), LayoutNum(0u) {}
		RVertexShaderResource(const RVertexShaderResource& Other)
			: Shader(Other.Shader), InputLayout(Other.InputLayout), RawLayouts(nullptr), LayoutNum(0u)
		{
			if ((!!(Other.RawLayouts)) && (Other.LayoutNum > 0u))
			{
				RawLayouts = new RInputLayoutDesc[Other.LayoutNum];
				LayoutNum = Other.LayoutNum;
				for (UINT i = 0u, n = Other.LayoutNum; i < n; i++)
				{
					RawLayouts[i] = Other.RawLayouts[i];
				}
			}
		}
		virtual ~RVertexShaderResource() { Release(); }
		RVertexShaderResource& operator=(const RVertexShaderResource& Other)
		{
			Release();
			Shader		= Other.Shader;
			InputLayout	= Other.InputLayout;
			if ((!!(Other.RawLayouts)) && (Other.LayoutNum > 0u))
			{
				RawLayouts = new RInputLayoutDesc[Other.LayoutNum];
				LayoutNum = Other.LayoutNum;
				for (UINT i = 0u, n = Other.LayoutNum; i < n; i++)
				{
					RawLayouts[i] = Other.RawLayouts[i];
				}
			}
		}
		virtual void Release()override
		{
			Shader		= nullptr;
			InputLayout	= nullptr;
			if (RawLayouts)
			{
				delete[]RawLayouts;
				RawLayouts = nullptr;
				LayoutNum = 0u;
			}
		};
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	Shader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	InputLayout;
		RInputLayoutDesc*							RawLayouts;
		UINT										LayoutNum;
	};
	struct RPixelShaderResource : public RRenderResourceInterface
	{
	public:
		RPixelShaderResource() : Shader(nullptr) {}
		RPixelShaderResource(const RPixelShaderResource& Other) : Shader(Other.Shader) {}
		virtual ~RPixelShaderResource() { Release(); }
		RPixelShaderResource& operator=(const RPixelShaderResource& Other) { Shader = Other.Shader; }
		virtual void Release()override
		{
			Shader = nullptr;
		};
		Microsoft::WRL::ComPtr<ID3D11PixelShader> Shader;
	};
	struct RComputeShaderResource : public RRenderResourceInterface
	{
	public:
		RComputeShaderResource() : Shader(nullptr) {}
		RComputeShaderResource(const RComputeShaderResource& Other) : Shader(Other.Shader) {}
		virtual ~RComputeShaderResource() { Release(); }
		RComputeShaderResource& operator=(const RComputeShaderResource& Other) { Shader = Other.Shader; }
		virtual void Release()override
		{
			Shader = nullptr;
		};
		Microsoft::WRL::ComPtr<ID3D11ComputeShader> Shader;
	};
	struct RStructuredBuffer : public RRenderResourceInterface
	{
	public:
		RStructuredBuffer() : AccessMapRead(FALSE), AccessMapWrite(FALSE), Buffer(nullptr), UnorderedAccessView(nullptr), ShaderResourceView(nullptr) {}
		RStructuredBuffer(const RStructuredBuffer& Other) : AccessMapRead(Other.AccessMapRead), AccessMapWrite(Other.AccessMapWrite), Buffer(Other.Buffer), UnorderedAccessView(Other.UnorderedAccessView), ShaderResourceView(Other.ShaderResourceView) {}
		virtual ~RStructuredBuffer() { Release(); }
		RStructuredBuffer& operator=(const RStructuredBuffer& Other)
		{
			AccessMapRead		= Other.AccessMapRead;
			AccessMapWrite		= Other.AccessMapWrite;
			Buffer				= Other.Buffer;
			UnorderedAccessView	= Other.UnorderedAccessView;
			ShaderResourceView	= Other.ShaderResourceView;
		}
		virtual void Release()override
		{
			AccessMapRead		= FALSE;
			AccessMapWrite		= FALSE;
			Buffer				= nullptr;
			UnorderedAccessView	= nullptr;
			ShaderResourceView	= nullptr;
		}
		BOOL	AccessMapRead;
		BOOL	AccessMapWrite;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				Buffer;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	UnorderedAccessView;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
	};
	struct RRenderTexture2D : public RRenderResourceInterface
	{
	public:
		RRenderTexture2D() : Buffer(nullptr), RenderTargetView(nullptr), UnorderedAccessView(nullptr), ShaderResourceView(nullptr), DepthStencilView(nullptr) {}
		RRenderTexture2D(const RRenderTexture2D& Other) : Buffer(Other.Buffer), RenderTargetView(Other.RenderTargetView), UnorderedAccessView(Other.UnorderedAccessView), ShaderResourceView(Other.ShaderResourceView), DepthStencilView(Other.DepthStencilView) {}
		virtual ~RRenderTexture2D() { Release(); }
		RRenderTexture2D& operator=(const RRenderTexture2D& Other)
		{
			Buffer				= Other.Buffer;
			RenderTargetView	= Other.RenderTargetView;
			UnorderedAccessView	= Other.UnorderedAccessView;
			ShaderResourceView	= Other.ShaderResourceView;
			DepthStencilView	= Other.DepthStencilView;
		}
		virtual void Release()override
		{
			Buffer				= nullptr;
			RenderTargetView	= nullptr;
			UnorderedAccessView	= nullptr;
			ShaderResourceView	= nullptr;
			DepthStencilView	= nullptr;
		}
		Microsoft::WRL::ComPtr<ID3D11Texture2D>				Buffer;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>		RenderTargetView;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	UnorderedAccessView;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView>		DepthStencilView;
	};
	struct RRenderTexture3D : public RRenderResourceInterface
	{
		RRenderTexture3D() : Buffer(nullptr), RenderTargetView(nullptr), UnorderedAccessView(nullptr), ShaderResourceView(nullptr) {}
		RRenderTexture3D(const RRenderTexture3D& Other) : Buffer(Other.Buffer), RenderTargetView(Other.RenderTargetView), UnorderedAccessView(Other.UnorderedAccessView), ShaderResourceView(Other.ShaderResourceView) {}
		virtual ~RRenderTexture3D() { Release(); }
		RRenderTexture3D& operator=(const RRenderTexture3D& Other)
		{
			Buffer				= Other.Buffer;
			RenderTargetView	= Other.RenderTargetView;
			UnorderedAccessView	= Other.UnorderedAccessView;
			ShaderResourceView	= Other.ShaderResourceView;
		}
		virtual void Release()override
		{
			Buffer				= nullptr;
			RenderTargetView	= nullptr;
			UnorderedAccessView	= nullptr;
			ShaderResourceView	= nullptr;
		};
		Microsoft::WRL::ComPtr<ID3D11Texture3D>				Buffer;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>		RenderTargetView;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	UnorderedAccessView;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
	};
	struct RTexture2DResource : public RRenderResourceInterface
	{
		RTexture2DResource() : Buffer(nullptr), ShaderResourceView(nullptr) {}
		RTexture2DResource(const RTexture2DResource& Other) : Buffer(Other.Buffer), ShaderResourceView(Other.ShaderResourceView) {}
		virtual ~RTexture2DResource() { Release(); }
		RTexture2DResource& operator=(const RTexture2DResource& Other)
		{
			Buffer				= Other.Buffer;
			ShaderResourceView	= Other.ShaderResourceView;
		}
		virtual void Release()override
		{
			Buffer				= nullptr;
			ShaderResourceView	= nullptr;
		}
		Microsoft::WRL::ComPtr<ID3D11Texture2D>				Buffer;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
	};
	struct RTextureCubeResource : public RRenderResourceInterface
	{
		RTextureCubeResource() : Buffer(nullptr), ShaderResourceView(nullptr) {}
		RTextureCubeResource(const RTextureCubeResource& Other) : Buffer(Other.Buffer), ShaderResourceView(Other.ShaderResourceView) {}
		virtual ~RTextureCubeResource() { Release(); }
		RTextureCubeResource& operator=(const RTextureCubeResource& Other)
		{
			Buffer				= Other.Buffer;
			ShaderResourceView	= Other.ShaderResourceView;
		}
		virtual void Release()override
		{
			Buffer				= nullptr;
			ShaderResourceView	= nullptr;
		}
		Microsoft::WRL::ComPtr<ID3D11Texture2D>				Buffer;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
	};
	struct RBufferResource : public RRenderResourceInterface
	{
		RBufferResource() : Buffer(nullptr) {}
		RBufferResource(const RBufferResource& Other) : Buffer(Other.Buffer) {}
		virtual ~RBufferResource() { Release(); }
		RBufferResource& operator=(const RBufferResource& Other)
		{
			Buffer = Other.Buffer;
		}
		virtual void Release()override
		{
			Buffer = nullptr;
		}
		Microsoft::WRL::ComPtr<ID3D11Buffer>				Buffer;
	};

};