#include "ShaderParameter.h"
#include <RenderDevice/DeviceD3D11.h>

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
		return Matrix4x4(InData.GetDirectXTransposeValue4x4());
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

	EShaderStruct::EShaderStruct()
	{
	}
	EShaderStruct::EShaderStruct(const EShaderStruct& Other)
	{
	}
	EShaderStruct::~EShaderStruct()
	{
		ClearParameter();
	}
	void EShaderStruct::ClearParameter()
	{
		InitCommands.DoCommands();
		InitCommands.EmptyQueue();
		RawData.Release();
		ShaderParameterNames.Empty();
		ShaderParameters.Empty();
	}
	EShaderParameter* EShaderStruct::FindParameter(const EString& InParamName)
	{
#if _EDITOR_ONLY
		if (UINT32 ParamIndex = (UINT32)(-1); ShaderParameterNames.FindValue(InParamName, ParamIndex))
#else
		const UINT32 ParamIndex = ShaderParameterNames[InParamName];
#endif
		{
#if _EDITOR_ONLY
			if (ParamIndex < ShaderParameters.Num<UINT32>())
#endif
			{
				return (&(ShaderParameters[ParamIndex]));
			}
		}
#if _EDITOR_ONLY
		EString ErrorInfo("Could not find name[");
		ErrorInfo = ErrorInfo + InParamName + "] parameter.";
		PE_FAILED((ENGINE_RENDER_CORE_ERROR), (*ErrorInfo));
		return nullptr;
#endif
	}
	void EShaderStruct::BeginSetupParameter()
	{
	}
	void EShaderStruct::EndSetupParameter()
	{
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
#if _EDITOR_ONLY
		if (ConstantBuffer.IsRenderResourceValid())
#endif
		{
			ClearBuffer();
		}
	}
	void EMaterialParameter::ClearParameter()
	{
		EShaderStruct::ClearParameter();
		HasPaddings = FALSE;
	}
	void EMaterialParameter::CreateBuffer()
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
	void EMaterialParameter::UploadBuffer()
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
	void EMaterialParameter::ClearBuffer()
	{
#if _EDITOR_ONLY
		if (ConstantBuffer.IsRenderResourceValid())
#endif
		{
			ConstantBuffer.ReleaseRenderResource();
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Try to clear a not init constant buffer."));
		}
#endif
	}
	EShaderParameter& EMaterialParameter::operator[](const EString& InParamName)
	{
		if (EShaderParameter* Parameter = FindParameter(InParamName); !!Parameter)
		{
			return (*Parameter);
		}
		static EShaderParameter _FailedParameter;
		return _FailedParameter;
	}
	RBufferResource& EMaterialParameter::GetConstantBuffer()
	{
		return ConstantBuffer;
	}
	const RBufferResource& EMaterialParameter::GetConstantBuffer()const
	{
		return ConstantBuffer;
	}
	BOOL32 EMaterialParameter::IsHasPaddings()const
	{
		return HasPaddings;
	}
	void EMaterialParameter::BeginSetupParameter()
	{
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

	EMaterialStructParameter::EMaterialStructParameter()
		: ElementNum(0u)
	{
	}
	EMaterialStructParameter::EMaterialStructParameter(const EMaterialStructParameter& Other)
		: ElementNum(Other.ElementNum)
	{
	}
	EMaterialStructParameter::~EMaterialStructParameter()
	{
		ClearBuffer();
	}
	void EMaterialStructParameter::ClearParameter()
	{
		EShaderStruct::ClearParameter();
		ElementNum = 0u;
	}
	void EMaterialStructParameter::CreateBuffer()
	{
#if _EDITOR_ONLY
		if ((ElementNum > 0u) && (ElementNum <= 65536u) && (RawData.Size > 0u))
#endif
		{
			Check((RawData.Size < ((ULONGLONG)((UINT32)(-1)))));
			Check(((RawData.Size % ElementNum) == 0u), (ENGINE_RENDER_CORE_ERROR));
			Check((!(StructuredBuffer.IsRenderResourceValid())), (ENGINE_RENDER_CORE_ERROR));
#if _EDITOR_ONLY
			BOOL32 Result =
#endif
				RDeviceD3D11::GetDeviceSingleton()->CreateStructuredBuffer(StructuredBuffer,
					RStructuredBufferDesc(static_cast<UINT32>(RawData.Size) / ElementNum, ElementNum));
#if _EDITOR_ONLY
			if (!Result)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Material struct parameter create buffer failed(render resource create failed)."));
			}
#endif
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Try to create a error memory size struct buffer."));
		}
#endif
	}
	void EMaterialStructParameter::UploadBuffer()
	{
#if _EDITOR_ONLY
		if ((StructuredBuffer.IsRenderResourceValid()) && (RawData.Size > 0u) && (!!(RawData.Datas)))
#endif
		{
			Check((RawData.Size < ((ULONGLONG)((UINT32)(-1)))));
			const UINT32 TempSize = static_cast<UINT32>(RawData.Size) * ElementNum;
			RDeviceD3D11::GetDeviceSingleton()->UploadBuffer(StructuredBuffer.Buffer, RawData.Datas, TempSize, TempSize);
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Try to create a error memory size struct buffer."));
		}
#endif
	}
	void EMaterialStructParameter::ClearBuffer()
	{
#if _EDITOR_ONLY
		if (StructuredBuffer.IsRenderResourceValid())
#endif
		{
			StructuredBuffer.ReleaseRenderResource();
		}
	}
	EShaderParameter* EMaterialStructParameter::FindParameter(const EString& InParamName)
	{
		return nullptr;
	}
	EShaderParameter EMaterialStructParameter::FindParameter(const UINT32 InStructIndex, const EString& InParamName)
	{
		EShaderParameter Result;
#if _EDITOR_ONLY
		if (InStructIndex < ElementNum && (!!(RawData.Datas)) && (RawData.Size > 0u))
#endif
		{
#if _EDITOR_ONLY
			if (UINT32 ParamIndex = (UINT32)(-1); ShaderParameterNames.FindValue(InParamName, ParamIndex))
#else
			const UINT32 ParamIndex = ShaderParameterNames[InParamName];
#endif
			{
#if _EDITOR_ONLY
				if (ParamIndex < ShaderParameters.Num<UINT32>())
#endif
				{
					Check((RawData.Size < ((ULONGLONG)((UINT32)(-1)))));
					const UINT32 StructElementIndex = (static_cast<UINT32>(RawData.Size) / ElementNum) * InStructIndex;
					Result.ValuePtr			= &(RawData.Datas[StructElementIndex]);
					Result.ValueType		= ShaderParameters[ParamIndex].ValueType;
					Result.ParameterInfo	= ShaderParameters[ParamIndex].ParameterInfo;
				}
#if _EDITOR_ONLY
				else
				{
					EString ErrorInfo("Could not find parameter by index[");
					ErrorInfo = ErrorInfo + ToString(ParamIndex) + "].";
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), (*ErrorInfo));
				}
#endif
			}
#if _EDITOR_ONLY
			else
			{
				EString ErrorInfo("Could not find name[");
				ErrorInfo = ErrorInfo + InParamName + "] parameter.";
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), (*ErrorInfo));
			}
#endif
		}
#if _EDITOR_ONLY
		else
		{
			EString ErrorInfo("Material struct parameter does not contain this index[");
			ErrorInfo += ToString(InStructIndex);
			ErrorInfo += "].";
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), (*ErrorInfo));
		}
#endif
		return Result;
	}
	void EMaterialStructParameter::SetElementNum(const UINT32 InElementNum)
	{
#if _EDITOR_ONLY
		if ((InElementNum == 0u) || (InElementNum > 65536u))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Try set 0 to material struct parameter."));
		}
#endif
		if (ElementNum != 0u)
		{
			ClearBuffer();
		}
		ElementNum = InElementNum;
	}
	UINT32 EMaterialStructParameter::GetElementNum()const
	{
		Check((ElementNum <= 65536u), (ENGINE_RENDER_CORE_ERROR));
		return ElementNum;
	}
	RStructuredBuffer& EMaterialStructParameter::GetStructBuffer()
	{
		return StructuredBuffer;
	}
	const RStructuredBuffer& EMaterialStructParameter::GetStructBuffer()const
	{
		return StructuredBuffer;
	}
	void EMaterialStructParameter::BeginSetupStructParameter(const UINT32 InElementNum)
	{
		SetElementNum(InElementNum);
		BeginSetupParameter();
	}
	void EMaterialStructParameter::EndSetupStructParameter()
	{
		EndSetupParameter();
	}
	void EMaterialStructParameter::BeginSetupParameter()
	{
	}
	void EMaterialStructParameter::EndSetupParameter()
	{
		ULONGLONG UsedSize = RawData.Size * ElementNum;
#if _EDITOR_ONLY
		if ((ElementNum == 0u) || (ElementNum > 65536u))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Try setup 0 element material struct parameter."));
		}
		if (UsedSize == 0u || InitCommands.GetQueueCount() == 0u)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Error shader parameter try to allocating 0 size buffer."));
			return;
		}
#endif
		RawData.Resize(UsedSize);

		InitCommands.DoCommands();
		InitCommands.EmptyQueue();
	}

};