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
				for (UINT32 i = 0u, n = Other.LayoutNum; i < n; i++)
				{
					RawLayouts[i] = Other.RawLayouts[i];
				}
			}
		}
		virtual ~RVertexShaderResource() { ReleaseRenderResource(); }
		RVertexShaderResource& operator=(const RVertexShaderResource& Other)
		{
			ReleaseRenderResource();
			Shader		= Other.Shader;
			InputLayout	= Other.InputLayout;
			if ((!!(Other.RawLayouts)) && (Other.LayoutNum > 0u))
			{
				RawLayouts = new RInputLayoutDesc[Other.LayoutNum];
				LayoutNum = Other.LayoutNum;
				for (UINT32 i = 0u, n = Other.LayoutNum; i < n; i++)
				{
					RawLayouts[i] = Other.RawLayouts[i];
				}
			}
			return (*this);
		}
		virtual BOOL32 IsRenderResourceValid()const override
		{
			return ((!!Shader) && (!!InputLayout) && (!!RawLayouts) && (LayoutNum > 0u));
		}
		virtual BOOL32 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
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
		UINT32										LayoutNum;
	};
	class RPixelShaderResource : public RRenderResourceInterface
	{
	public:
		RPixelShaderResource() : Shader(nullptr) {}
		RPixelShaderResource(const RPixelShaderResource& Other) : Shader(Other.Shader) {}
		virtual ~RPixelShaderResource() { ReleaseRenderResource(); }
		RPixelShaderResource& operator=(const RPixelShaderResource& Other)
		{
			Shader = Other.Shader;
			return (*this);
		}
		virtual BOOL32 IsRenderResourceValid()const override
		{
			return (!!Shader);
		}
		virtual BOOL32 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			Shader = nullptr;
		};
		Microsoft::WRL::ComPtr<ID3D11PixelShader> Shader;
	};
	class RComputeShaderResource : public RRenderResourceInterface
	{
	public:
		RComputeShaderResource() : Shader(nullptr) {}
		RComputeShaderResource(const RComputeShaderResource& Other) : Shader(Other.Shader) {}
		virtual ~RComputeShaderResource() { ReleaseRenderResource(); }
		RComputeShaderResource& operator=(const RComputeShaderResource& Other)
		{
			Shader = Other.Shader;
			return (*this);
		}
		virtual BOOL32 IsRenderResourceValid()const override
		{
			return (!!Shader);
		}
		virtual BOOL32 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			Shader = nullptr;
		};
		Microsoft::WRL::ComPtr<ID3D11ComputeShader> Shader;
	};
	class RStructuredBuffer : public RRenderResourceInterface
	{
	public:
		RStructuredBuffer() : AccessMapRead(FALSE), AccessMapWrite(FALSE), Buffer(nullptr), UnorderedAccessView(nullptr), ShaderResourceView(nullptr) {}
		RStructuredBuffer(const RStructuredBuffer& Other) : AccessMapRead(Other.AccessMapRead), AccessMapWrite(Other.AccessMapWrite), Buffer(Other.Buffer), UnorderedAccessView(Other.UnorderedAccessView), ShaderResourceView(Other.ShaderResourceView) {}
		virtual ~RStructuredBuffer() { ReleaseRenderResource(); }
		RStructuredBuffer& operator=(const RStructuredBuffer& Other)
		{
			AccessMapRead		= Other.AccessMapRead;
			AccessMapWrite		= Other.AccessMapWrite;
			Buffer				= Other.Buffer;
			UnorderedAccessView	= Other.UnorderedAccessView;
			ShaderResourceView	= Other.ShaderResourceView;
			return (*this);
		}
		// StructuredBuffer can bind UAV or SRV, so this function only determines by buffer valid.
		virtual BOOL32 IsRenderResourceValid()const override
		{
			return (!!Buffer);
		}
		virtual BOOL32 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			AccessMapRead		= FALSE;
			AccessMapWrite		= FALSE;
			Buffer				= nullptr;
			UnorderedAccessView	= nullptr;
			ShaderResourceView	= nullptr;
		}
		BOOL32	AccessMapRead;
		BOOL32	AccessMapWrite;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				Buffer;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	UnorderedAccessView;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
	};
	class RRenderTexture2D : public RRenderResourceInterface
	{
	public:
		RRenderTexture2D() : Buffer(nullptr), RenderTargetView(nullptr), UnorderedAccessView(nullptr), ShaderResourceView(nullptr), DepthStencilView(nullptr) {}
		RRenderTexture2D(const RRenderTexture2D& Other) : Buffer(Other.Buffer), RenderTargetView(Other.RenderTargetView), UnorderedAccessView(Other.UnorderedAccessView), ShaderResourceView(Other.ShaderResourceView), DepthStencilView(Other.DepthStencilView) {}
		virtual ~RRenderTexture2D() { ReleaseRenderResource(); }
		RRenderTexture2D& operator=(const RRenderTexture2D& Other)
		{
			Buffer				= Other.Buffer;
			RenderTargetView	= Other.RenderTargetView;
			UnorderedAccessView	= Other.UnorderedAccessView;
			ShaderResourceView	= Other.ShaderResourceView;
			DepthStencilView	= Other.DepthStencilView;
			return (*this);
		}
		// RenderTexture2D can bind RTV, UAV, SRV or DSV, so this function only determines by buffer valid.
		virtual BOOL32 IsRenderResourceValid()const override
		{
			return (!!Buffer);
		}
		virtual BOOL32 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
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
	class RRenderTexture3D : public RRenderResourceInterface
	{
	public:
		RRenderTexture3D() : Buffer(nullptr), RenderTargetView(nullptr), UnorderedAccessView(nullptr), ShaderResourceView(nullptr) {}
		RRenderTexture3D(const RRenderTexture3D& Other) : Buffer(Other.Buffer), RenderTargetView(Other.RenderTargetView), UnorderedAccessView(Other.UnorderedAccessView), ShaderResourceView(Other.ShaderResourceView) {}
		virtual ~RRenderTexture3D() { ReleaseRenderResource(); }
		RRenderTexture3D& operator=(const RRenderTexture3D& Other)
		{
			Buffer				= Other.Buffer;
			RenderTargetView	= Other.RenderTargetView;
			UnorderedAccessView	= Other.UnorderedAccessView;
			ShaderResourceView	= Other.ShaderResourceView;
			return (*this);
		}
		// RenderTexture3D can bind RTV, UAV or SRV, so this function only determines by buffer valid.
		virtual BOOL32 IsRenderResourceValid()const override
		{
			return (!!Buffer);
		}
		virtual BOOL32 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
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
	class RTexture2DResource : public RRenderResourceInterface
	{
	public:
		RTexture2DResource() : Buffer(nullptr), ShaderResourceView(nullptr) {}
		RTexture2DResource(const RTexture2DResource& Other) : Buffer(Other.Buffer), ShaderResourceView(Other.ShaderResourceView) {}
		virtual ~RTexture2DResource() { ReleaseRenderResource(); }
		RTexture2DResource& operator=(const RTexture2DResource& Other)
		{
			Buffer				= Other.Buffer;
			ShaderResourceView	= Other.ShaderResourceView;
			return (*this);
		}
		// Texture2DResource must bind SRV, so this function determines by buffer valid and SRV valid.
		virtual BOOL32 IsRenderResourceValid()const override
		{
			return ((!!Buffer) && (!!ShaderResourceView));
		}
		virtual BOOL32 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			Buffer				= nullptr;
			ShaderResourceView	= nullptr;
		}
		Microsoft::WRL::ComPtr<ID3D11Texture2D>				Buffer;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
	};
	class RTextureCubeResource : public RRenderResourceInterface
	{
	public:
		RTextureCubeResource() : Buffer(nullptr), ShaderResourceView(nullptr) {}
		RTextureCubeResource(const RTextureCubeResource& Other) : Buffer(Other.Buffer), ShaderResourceView(Other.ShaderResourceView) {}
		virtual ~RTextureCubeResource() { ReleaseRenderResource(); }
		RTextureCubeResource& operator=(const RTextureCubeResource& Other)
		{
			Buffer				= Other.Buffer;
			ShaderResourceView	= Other.ShaderResourceView;
			return (*this);
		}
		// TextureCubeResource must bind SRV, so this function determines by buffer valid and SRV valid.
		virtual BOOL32 IsRenderResourceValid()const override
		{
			return ((!!Buffer) && (!!ShaderResourceView));
		}
		virtual BOOL32 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			Buffer				= nullptr;
			ShaderResourceView	= nullptr;
		}
		Microsoft::WRL::ComPtr<ID3D11Texture2D>				Buffer;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
	};
	class RBufferResource : public RRenderResourceInterface
	{
	public:
		RBufferResource() : Buffer(nullptr) {}
		RBufferResource(const RBufferResource& Other) : Buffer(Other.Buffer) {}
		virtual ~RBufferResource() { ReleaseRenderResource(); }
		RBufferResource& operator=(const RBufferResource& Other)
		{
			Buffer = Other.Buffer;
			return (*this);
		}
		// BufferResource is raw buffer resource, so this function only determines by buffer valid.
		virtual BOOL32 IsRenderResourceValid()const override
		{
			return (!!Buffer);
		}
		virtual BOOL32 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			Buffer = nullptr;
		}
		Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer;
	};
	class RSceneTextures final
	{
	public:
		void	InitResources(const Vector2Int& InTextureSize, const BOOL32 IsNeedStencil);
		void	ClearResources();
		void	ReleaseResources();
	public:
		RRenderTexture2D	SceneDepthStencil;
		RRenderTexture2D	SceneColor;
		RRenderTexture2D	GBufferA;
		RRenderTexture2D	GBufferB;
		RRenderTexture2D	GBufferC;
	public:
		const Vector2Int&	GetTextureSize()const;
		BOOL32				IsNeedStencil()const;
	private:
		Vector2Int	TextureSize;
		BOOL32		NeedStencil;
	public:
		RSceneTextures();
		~RSceneTextures();
		RSceneTextures(const RSceneTextures&) = delete;
		RSceneTextures& operator=(const RSceneTextures&) = delete;
	};
	class RShadowTexture final
	{
	public:
		void	InitResources(const TArray<Vector2Int>& InTextureSizes);
		void	ClearResources();
		void	ReleaseResources();
	public:
		TArray<RRenderTexture2D>	ShadowMaps;
	private:
		TArray<Vector2Int>	TextureSizes;
	public:
		RShadowTexture();
		~RShadowTexture();
		RShadowTexture(const RShadowTexture&) = delete;
		RShadowTexture& operator=(const RShadowTexture&) = delete;
	};

};