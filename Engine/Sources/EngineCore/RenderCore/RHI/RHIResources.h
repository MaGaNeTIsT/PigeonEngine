#pragma once

#include <CoreMinimal.h>
#include <RenderCommon.h>
#include <RHI/RHITypes.h>
#include <RHI/IRRHIResource.h>

namespace PigeonEngine
{

	class IRRHIDevice;

	/*
	* Engine-side RHI resource wrappers. Each wrapper carries
	*   - an abstract IRRHIxxx* pointer (the actual GPU object)
	*   - the engine-level metadata needed at binding / PSO time
	* and nothing else. No D3D11* / D3D12* type ever appears here, so
	* engine code (assets, scene proxies, renderer) only ever references
	* these wrappers and the IRRHIDevice / IRCommandList interfaces.
	*
	* Construction: IRRHIDevice::CreateXxxResource fills an out-parameter
	* wrapper. Release: returns the IRRHIxxx ownership back to the device
	* for deferred deletion (N+FRAME_IN_FLIGHT) via IRRHIDevice::DestroyResource.
	*
	* Validity: IsRenderResourceValid() returns true when the abstract
	* pointer is non-null. Inheriting RRenderResourceInterface lets these
	* plug straight into the existing TRenderBaseAsset framework.
	*
	* Shader stages are deliberately absent: a compiled stage is an
	* RShaderStageResource (RenderCore/Shader/ShaderStageResource.h), owned by its
	* EShaderStageAsset, and it already carries the abstract IRRHIShader plus the
	* reflection derived input layout. Duplicating it here would give the engine two
	* representations of one GPU object again.
	*/
	struct RRHIVertexBuffer : public RRenderResourceInterface
	{
		IRRHIBuffer*		Buffer;
		UINT32				Stride;
		UINT32				VertexCount;

		RRHIVertexBuffer() noexcept
			: Buffer{ nullptr }
			, Stride{ 0u }
			, VertexCount{ 0u }
		{
		}
		virtual BOOL8 IsRenderResourceValid() const noexcept override { return (!(!Buffer)); }
		virtual BOOL8 InitRenderResource() noexcept override { return TRUE; }
		virtual void  ReleaseRenderResource() noexcept override;
		void Release(IRRHIDevice* InDevice) noexcept;
	};

	struct RRHIIndexBuffer : public RRenderResourceInterface
	{
		IRRHIBuffer*		Buffer;
		UINT32				IndexCount;
		ERHIIndexFormat		Format;

		RRHIIndexBuffer() noexcept
			: Buffer{ nullptr }
			, IndexCount{ 0u }
			, Format{ ERHIIndexFormat::RHI_INDEX_FORMAT_UINT32 }
		{
		}
		virtual BOOL8 IsRenderResourceValid() const noexcept override { return ((!(!Buffer)) && (IndexCount > 0u)); }
		virtual BOOL8 InitRenderResource() noexcept override { return TRUE; }
		virtual void  ReleaseRenderResource() noexcept override;
		void Release(IRRHIDevice* InDevice) noexcept;
	};

	struct RRHIConstantBuffer : public RRenderResourceInterface
	{
		IRRHIBuffer*		Buffer;
		// Byte width, matching IRRHIBuffer::GetByteSize / RBufferDesc::ByteWidth (UINT64).
		UINT64				SizeInBytes;

		RRHIConstantBuffer() noexcept
			: Buffer{ nullptr }
			, SizeInBytes{ 0u }
		{
		}
		virtual BOOL8 IsRenderResourceValid() const noexcept override { return (!(!Buffer)); }
		virtual BOOL8 InitRenderResource() noexcept override { return TRUE; }
		virtual void  ReleaseRenderResource() noexcept override;
		void Release(IRRHIDevice* InDevice) noexcept;
	};

	struct RRHIStructuredBuffer : public RRenderResourceInterface
	{
		RRHIStructuredBuffer() noexcept
			: Buffer(nullptr), Stride(0u), ElementCount(0u)
		{
		}

		IRRHIBuffer* Buffer;
		UINT32       Stride;
		UINT32       ElementCount;

		virtual BOOL8 IsRenderResourceValid() const noexcept override { return (!(!Buffer)); }
		virtual BOOL8 InitRenderResource() noexcept override { return TRUE; }
		virtual void  ReleaseRenderResource() noexcept override;
		void Release(IRRHIDevice* InDevice) noexcept;
	};

	struct RRHITexture2D : public RRenderResourceInterface
	{
		RRHITexture2D() noexcept
			: Texture(nullptr), Width(0u), Height(0u), MipLevels(1u)
			, Format(RFormatType::FORMAT_UNKNOWN), BindFlags(0u)
		{
		}

		IRRHITexture* Texture;
		UINT32        Width;
		UINT32        Height;
		UINT32        MipLevels;
		RFormatType   Format;
		UINT8         BindFlags;

		virtual BOOL8 IsRenderResourceValid() const noexcept override { return (!(!Texture)); }
		virtual BOOL8 InitRenderResource() noexcept override { return TRUE; }
		virtual void  ReleaseRenderResource() noexcept override;
		void Release(IRRHIDevice* InDevice) noexcept;
	};

	struct RRHITextureCube : public RRenderResourceInterface
	{
		RRHITextureCube() noexcept
			: Texture(nullptr), Size(0u), MipLevels(1u)
			, Format(RFormatType::FORMAT_UNKNOWN), BindFlags(0u)
		{
		}

		IRRHITexture* Texture;
		UINT32        Size;
		UINT32        MipLevels;
		RFormatType   Format;
		UINT8         BindFlags;

		virtual BOOL8 IsRenderResourceValid() const noexcept override { return (!(!Texture)); }
		virtual BOOL8 InitRenderResource() noexcept override { return TRUE; }
		virtual void  ReleaseRenderResource() noexcept override;
		void Release(IRRHIDevice* InDevice) noexcept;
	};

	// Off-screen render target / depth-stencil. BindFlags records the
	// combo of RT / DS / SRV / UAV the resource was created with so
	// downstream binding code can pick the right view.
	struct RRHIRenderTexture2D : public RRenderResourceInterface
	{
		RRHIRenderTexture2D() noexcept
			: Texture(nullptr), Width(0u), Height(0u)
			, Format(RFormatType::FORMAT_UNKNOWN), BindFlags(0u)
		{
		}

		IRRHITexture* Texture;
		UINT32        Width;
		UINT32        Height;
		RFormatType   Format;
		UINT8         BindFlags;

		virtual BOOL8 IsRenderResourceValid() const noexcept override { return (!(!Texture)); }
		virtual BOOL8 InitRenderResource() noexcept override { return TRUE; }
		virtual void  ReleaseRenderResource() noexcept override;
		void Release(IRRHIDevice* InDevice) noexcept;
	};

	struct RRHISamplerHandle : public RRenderResourceInterface
	{
		RRHISamplerHandle() noexcept : Sampler(nullptr) {}

		IRRHISampler* Sampler;

		virtual BOOL8 IsRenderResourceValid() const noexcept override { return (!(!Sampler)); }
		virtual BOOL8 InitRenderResource() noexcept override { return TRUE; }
		virtual void  ReleaseRenderResource() noexcept override;
		void Release(IRRHIDevice* InDevice) noexcept;
	};

};
