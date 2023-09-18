#include "ShaderParameter.h"

namespace PigeonEngine
{

	PE_INLINE DirectX::XMFLOAT4X4 TranslateUploadType(const Matrix4x4& InData)
	{
		DirectX::XMFLOAT4X4 Result(InData.GetDirectXTransposeValue4x4());
		return Result;
	}
	PE_INLINE DirectX::XMFLOAT4 TranslateUploadType(const Vector4& InData)
	{
		DirectX::XMFLOAT4 Result(InData.GetDirectXValue4());
		return Result;
	}

	EMaterialParameter::EMaterialParameter()
		: HasPaddings(FALSE)
	{
		SetupParameters();
	}
	EMaterialParameter::EMaterialParameter(const EMaterialParameter& Other)
		: HasPaddings(FALSE)
	{
		SetupParameters();
	}
	EMaterialParameter::~EMaterialParameter()
	{
		ClearParameter();
	}
	void EMaterialParameter::BeginSetupParameter()
	{
		ClearParameter();
	}
	void EMaterialParameter::EndSetupParameter()
	{
		ULONGLONG UsedSize = RawData.Size;
#if _EDITOR_ONLY
		if (UsedSize == 0u || InitCommands.GetQueueCount() == 0u)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Error shader parameter try to allocating 0 size buffer."));
			return;
		}
#endif
		if ((UsedSize % 2u) != 0u)
		{
			HasPaddings = TRUE;
			UsedSize = UsedSize + 1u;
		}
		RawData.Resize(UsedSize);

		InitCommands.DoCommands();
		InitCommands.EmptyQueue();
	}
	void EMaterialParameter::ClearParameter()
	{
		HasPaddings = FALSE;
		InitCommands.DoCommands();
		InitCommands.EmptyQueue();
		RawData.Release();
		ShaderParameterNames.Clear();
		ShaderParameters.Clear();
	}

};