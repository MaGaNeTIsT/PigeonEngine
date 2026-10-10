#include "ShaderParameter.h"
#include <RenderDevice/DeviceD3D11.h>
#include <RHI/IRRHIDevice.h>
#include <RHI/IRCommandList.h>
#include <Main/MainManager.h>
#include <TextureAsset/TextureAsset.h>
#if _EDITOR_ONLY
#include <imgui.h>
#endif

namespace PigeonEngine
{

	PE_INLINE FLOAT TranslateUploadBaseType(const FLOAT InData)
	{
		return InData;
	}
	PE_INLINE INT32 TranslateUploadBaseType(const INT32 InData)
	{
		return InData;
	}
	PE_INLINE INT32 TranslateUploadBaseType(const UINT32 InData)
	{
		union Bit32
		{
			INT32	A;
			UINT32	B;
		};
		Bit32 Data;
		Data.B = InData;
		return (Data.A);
	}
	PE_INLINE Matrix4x4	TranslateUploadMatrixType(const Matrix4x4& InData)
	{
#if 0
		return Matrix4x4(InData.GetDirectXTransposeValue4x4());
#else
		return InData;
#endif
	}
	PE_INLINE Matrix4x4 TranslateUploadTransposeMatrixType(const Matrix4x4& InData)
	{
		return InData;
	}
	PE_INLINE Vector3 TranslateUploadVectorType(const Vector3& InData)
	{
		return InData;
	}
	PE_INLINE Vector4 TranslateUploadVectorType(const Vector4& InData)
	{
		return InData;
	}
	PE_INLINE Vector2Int TranslateUploadVectorType(const Vector2Int& InData)
	{
		return InData;
	}
	PE_INLINE Vector4Int TranslateUploadVectorType(const Vector4Int& InData)
	{
		return InData;
	}
	PE_INLINE Vector3 TranslateUploadColorType(const Color3& InData)
	{
		return Vector3(InData.r, InData.g, InData.b);
	}
	PE_INLINE Vector4 TranslateUploadColorType(const Color4& InData)
	{
		return Vector4(InData.r, InData.g, InData.b, InData.a);
	}

	// The default constructor and the destructor come from CLASS_SHADER_PARAMETER; the
	// destructor releases the GPU buffer it owns, which the macro's empty body cannot do,
	// so releasing stays in ReleaseRenderResource paths of the owning block.
	void EConstantBufferParameter::BeginSetupVariable() noexcept
	{
		bHasPaddings = FALSE;
		BeginSetupCBuffer();
	}
	void EConstantBufferParameter::EndSetupVariable(UINT32& OutHeapSize) noexcept
	{
		EndSetupCBuffer();
		OutHeapSize = InitStructSize;
#if _EDITOR_ONLY
		// Only a block that declared no field at all is a mistake. The queued init commands
		// are NOT part of this test: AddStructVariable only enqueues when the caller passed an
		// initial value, so a block whose fields are all written by hand through SetFieldValue
		// leaves the queue empty - which is the normal case for every engine block. Testing the
		// queue length here rejected exactly those blocks and tripped this Check.
		Check((InitStructSize > 0u), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("[EConstantBufferParameter::EndSetupVariable] Invalid Empty Struct")));
#endif	// _EDITOR_ONLY
		// A constant buffer is read in 16 byte registers, so the allocation is rounded up the
		// way the reflection layout rounds a block. This runs in every build on purpose: an
		// editor build that returned early would size its buffers differently from a shipping
		// one, and the two would disagree about the block the shader reads.
		const UINT32 AlignedSize = (InitStructSize + 15u) & (~15u);
		if (AlignedSize != InitStructSize)
		{
			bHasPaddings = TRUE;
		}
		OutHeapSize = AlignedSize;
	}
	void EConstantBufferParameter::SetupParameters() noexcept
	{
		SetupStruct();
		CreateBuffer();
	}
	void EConstantBufferParameter::CreateBuffer() noexcept
	{
		Check((GetMemSize() > 0u), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("[EConstantBufferParameter::CreateBuffer] Create constant buffer failed(size is zero).")));
		Check((!(Buffer.IsRenderResourceValid())), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("[EConstantBufferParameter::CreateBuffer] Forgot release old constant buffer.")));
#if _EDITOR_ONLY
		BOOL32 Result =
#endif	// _EDITOR_ONLY
			Buffer.InitFromRHI(EMainManager::GetManagerSingleton()->GetRenderDevice(),
				RBufferDesc(GetMemSize(), RBindFlagType::BIND_CONSTANT_BUFFER, 0u));
#if _EDITOR_ONLY
		if (!Result)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("[EConstantBufferParameter::CreateBuffer] Create constant buffer failed(render resource create failed).")));
		}
#endif	// _EDITOR_ONLY
	}
	void EConstantBufferParameter::UploadBuffer() noexcept
	{
		Check((GetMemSize() > 0u), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("[EConstantBufferParameter::UploadBuffer] Check constant buffer failed(size is zero).")));
		Check((Buffer.IsRenderResourceValid()), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("[EConstantBufferParameter::UploadBuffer] Check constant buffer failed(buffer is null).")));
		Buffer.UploadDataViaRHI(EMainManager::GetManagerSingleton()->GetRenderDevice(), GetData(), GetMemSize());
	}
	void EConstantBufferParameter::ReleaseBuffer() noexcept
	{
#if _EDITOR_ONLY
		if (Buffer.IsRenderResourceValid())
#endif	// _EDITOR_ONLY
		{
			Buffer.ReleaseRenderResource();
		}
	}
	RBufferResource& EConstantBufferParameter::GetBuffer() noexcept
	{
		return Buffer;
	}
	const RBufferResource& EConstantBufferParameter::GetBuffer() const noexcept
	{
		return Buffer;
	}
	BOOL32 EConstantBufferParameter::IsHasPaddings() const noexcept
	{
		return bHasPaddings;
	}
	void EConstantBufferParameter::BeginSetupCBuffer() noexcept
	{
	}
	void EConstantBufferParameter::EndSetupCBuffer() noexcept
	{
	}

	EStructuredBufferParameter::EStructuredBufferParameter(UINT32 InNumElements) noexcept
		: EStructArray{}
		, Buffer{}
		, ElementCount(InNumElements)
	{
	}
	void EStructuredBufferParameter::BeginSetupVariable() noexcept
	{
		// ClearStruct (run by SetupStruct before this) resets the base class element count,
		// so the requested count is restored here. Resize then gets a non zero count and
		// allocates one sub struct per element.
		NumElements = ElementCount;
		BeginSetupStructuredBuffer();
	}
	void EStructuredBufferParameter::EndSetupVariable(UINT32& OutHeapSize) noexcept
	{
		EndSetupStructuredBuffer();
		Check(((GetMemSize(0u) % 4u) == 0u), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("[EStructuredBufferParameter::EndSetupVariable] StructuredBuffer MUST be aligned with 4 bytes.")));
		OutHeapSize = GetNumElements() * GetMemSize(0u);
	}
	void EStructuredBufferParameter::CreateBuffer() noexcept
	{
		const UINT32 TotalMemSize = GetMemSize();
		Check((TotalMemSize > 0u), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("[EStructuredBufferParameter::CreateBuffer] Check structured buffer failed(size is zero).")));
		Check((((TotalMemSize % GetNumElements()) == 0u) && ((TotalMemSize % 4u) == 0u)), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("[EStructuredBufferParameter::CreateBuffer] Invalid Memory Size")));
		Check((!(Buffer.IsRenderResourceValid())), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("[EStructuredBufferParameter::CreateBuffer] Forgot release old constant buffer.")));
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
#if _EDITOR_ONLY
		if (RenderDevice)
#endif	// _EDITOR_ONLY
		{
			// The element stride is what makes this a structured resource on both
			// back-ends, so it rides in the buffer description instead of in a
			// back-end specific create call.
			const UINT32 ElementSize = TotalMemSize / GetNumElements();

#if _EDITOR_ONLY
			BOOL32 Result =
#endif	// _EDITOR_ONLY
				Buffer.InitFromRHI(RenderDevice, RBufferDesc(TotalMemSize
					, RBindFlagType::BIND_SHADER_RESOURCE | RBindFlagType::BIND_UNORDERED_ACCESS
					, ElementSize, RResourceMiscFlagType::RESOURCE_MISC_BUFFER_STRUCTURED));
#if _EDITOR_ONLY
			if (!Result)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("[EStructuredBufferParameter::CreateBuffer] Structured buffer create buffer failed(render resource create failed).")));
			}
#endif	// _EDITOR_ONLY
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("[EStructuredBufferParameter::CreateBuffer] RenderDevice is null.")));
		}
#endif	// _EDITOR_ONLY
	}
	void EStructuredBufferParameter::UploadBuffer() noexcept
	{
		const UINT32 TotalMemSize = GetMemSize();
		Check((TotalMemSize > 0u), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("[EStructuredBufferParameter::UploadBuffer] Check structured buffer failed(size is zero).")));
		Check((((TotalMemSize % GetNumElements()) == 0u) && ((TotalMemSize % 4u) == 0u)), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("[EStructuredBufferParameter::UploadBuffer] Invalid Memory Size")));
		Check((Buffer.IsRenderResourceValid()), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("[EStructuredBufferParameter::UploadBuffer] Check constant buffer failed(buffer is null).")));
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
#if _EDITOR_ONLY
		if (RenderDevice)
#endif	// _EDITOR_ONLY
		{
			Buffer.UploadDataViaRHI(RenderDevice, GetData(), TotalMemSize);
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("[EStructuredBufferParameter::UploadBuffer] RenderDevice is null.")));
		}
#endif	// _EDITOR_ONLY
	}
	void EStructuredBufferParameter::ReleaseBuffer() noexcept
	{
#if _EDITOR_ONLY
		if (Buffer.IsRenderResourceValid())
#endif	// _EDITOR_ONLY
		{
			Buffer.ReleaseRenderResource();
		}
	}
	void EStructuredBufferParameter::ResizeElement(const UINT32 InNumElements) noexcept
	{
		ElementCount = InNumElements;
		NumElements = InNumElements;
		Resize();
		ReleaseBuffer();
		if (GetNumElements() > 0u)
		{
			CreateBuffer();
		}
	}
	RStructuredBuffer& EStructuredBufferParameter::GetBuffer() noexcept
	{
		return Buffer;
	}
	const RStructuredBuffer& EStructuredBufferParameter::GetBuffer() const noexcept
	{
		return Buffer;
	}
	void EStructuredBufferParameter::BeginSetupStructuredBuffer() noexcept
	{

	}
	void EStructuredBufferParameter::EndSetupStructuredBuffer() noexcept
	{

	}

};
