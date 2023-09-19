#include "ShaderParameter.h"
#include <RenderDevice/DeviceD3D11.h>

namespace PigeonEngine
{

	PE_INLINE Matrix4x4	TranslateUploadMatrixType(const Matrix4x4& InData)
	{
		return Matrix4x4(InData.GetDirectXTransposeValue4x4());
	}
	PE_INLINE Matrix4x4 TranslateUploadTransposeMatrixType(const Matrix4x4& InData)
	{
		return InData;
	}
	PE_INLINE Vector4 TranslateUploadVectorType(const Vector4& InData)
	{
		return InData;
	}

	EMaterialParameter::EMaterialParameter()
		: HasPaddings(FALSE)
	{
	}
	EMaterialParameter::EMaterialParameter(const EMaterialParameter& Other)
		: HasPaddings(FALSE)
	{
	}
	EMaterialParameter::~EMaterialParameter()
	{
		ClearParameter();
	}
	EShaderParameter& EMaterialParameter::operator[](const EString& InParamName)
	{
		static EShaderParameter _StaticParameter;
#if _EDITOR_ONLY
		if (UINT32 ParamIndex = (UINT32)(-1); ShaderParameterNames.FindValue(InParamName, ParamIndex))
#else
		const UINT32 ParamIndex = ShaderParameterNames[InParamName];
#endif
		{
#if _EDITOR_ONLY
			if (ParamIndex < ShaderParameters.Length())
#endif
			{
				return (ShaderParameters[ParamIndex]);
			}
		}
#if _EDITOR_ONLY
		EString ErrorInfo("Could not find name[");
		ErrorInfo = ErrorInfo + InParamName + "] parameter.";
		PE_FAILED((ENGINE_RENDER_CORE_ERROR), (*ErrorInfo));
#endif
		return _StaticParameter;
	}
	void EMaterialParameter::CreateConstantBuffer()
	{
#if _EDITOR_ONLY
		if (RawData.Size == 0u)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Material parameter create constant buffer failed(size is zero)."));
			return;
		}
		if (ConstantBuffer.IsRenderResourceValid())
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Material parameter create constant buffer failed(already created)."));
			return;
		}
#endif
#if _EDITOR_ONLY
		BOOL32 Result =
#endif
			RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(ConstantBuffer.Buffer,
				RBufferDesc(static_cast<UINT32>(RawData.Size), RBindFlagType::BIND_CONSTANT_BUFFER, sizeof(FLOAT)));
#if _EDITOR_ONLY
		if (!Result)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Material parameter create constant buffer failed(render resource create failed)."));
		}
#endif
	}
	void EMaterialParameter::UploadConstantBuffer()
	{
#if _EDITOR_ONLY
		if ((RawData.Size == 0u) || (!(RawData.Datas)))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Check material parameter constant buffer failed(size is zero)."));
			return;
		}
		if (!(ConstantBuffer.IsRenderResourceValid()))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Check material parameter constant buffer failed(buffer is null)."));
			return;
		}
#endif
		RDeviceD3D11::GetDeviceSingleton()->UploadBuffer(ConstantBuffer.Buffer, RawData.Datas);
	}
	RBufferResource& EMaterialParameter::GetConstantBuffer()
	{
		return ConstantBuffer;
	}
	const RBufferResource& EMaterialParameter::GetConstantBuffer()const
	{
		return ConstantBuffer;
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

		CreateConstantBuffer();
	}

};