#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	extern PE_INLINE DirectX::XMFLOAT4X4 TranslateUploadType(const Matrix4x4& InData);
	extern PE_INLINE DirectX::XMFLOAT4 TranslateUploadType(const Vector4& InData);

	enum EShaderParameterValueType : UINT8
	{
		SHADER_PARAMETER_TYPE_UNKNOWN	= 0,
		SHADER_PARAMETER_TYPE_FLOAT1,
		SHADER_PARAMETER_TYPE_FLOAT2,
		SHADER_PARAMETER_TYPE_FLOAT3,
		SHADER_PARAMETER_TYPE_FLOAT4,
		SHADER_PARAMETER_TYPE_INT1,
		SHADER_PARAMETER_TYPE_INT2,
		SHADER_PARAMETER_TYPE_INT3,
		SHADER_PARAMETER_TYPE_INT4,
		SHADER_PARAMETER_TYPE_UINT1,
		SHADER_PARAMETER_TYPE_UINT2,
		SHADER_PARAMETER_TYPE_UINT3,
		SHADER_PARAMETER_TYPE_UINT4,
		SHADER_PARAMETER_TYPE_MATRIX33,
		SHADER_PARAMETER_TYPE_MATRIX34,
		SHADER_PARAMETER_TYPE_MATRIX44,
		SHADER_PARAMETER_TYPE_COUNT
	};

	enum EShaderParameterContainerType : UINT8
	{
		SHADER_PARAMETER_CONTAINER_TYPE_UNKNOWN = 0,
		SHADER_PARAMETER_CONTAINER_TYPE_SINGLE,
		SHADER_PARAMETER_CONTAINER_TYPE_ARRAY,
		SHADER_PARAMETER_CONTAINER_TYPE_COUNT
	};

	struct EShaderParameterData
	{
	public:
		UINT32	Offset;
		UINT32	Size;

		EShaderParameterData()noexcept : Offset(0u), Size(0u) {}
		EShaderParameterData(const EShaderParameterData& Other)noexcept : Offset(Other.Offset), Size(Other.Size) {}
		EShaderParameterData& operator=(const EShaderParameterData& Other)
		{
			CopyFromOther(Other);
			return (*this);
		}
		void CopyFromOther(const EShaderParameterData& Other)
		{
			Offset	= Other.Offset;
			Size	= Other.Size;
		}
	};

	struct EShaderParameterBase : public EShaderParameterData
	{
	public:
		UINT32	Element;

		EShaderParameterBase()noexcept : EShaderParameterData(), Element(0u) {}
		EShaderParameterBase(const EShaderParameterBase& Other)noexcept : EShaderParameterData(Other), Element(Other.Element) {}
		EShaderParameterBase& operator=(const EShaderParameterBase& Other)
		{
			CopyFromOther(Other);
			Element = Other.Element;
			return (*this);
		}
	};

	struct EShaderParameterRaw
	{
	public:
		BYTE*		Datas;
		ULONGLONG	Size;

		EShaderParameterRaw(const ULONGLONG& InSize)noexcept : Datas(nullptr), Size(0u)
		{
			if ((InSize > 0u) && ((InSize % 2u) == 0u))
			{
				Size = InSize;
				Datas = new BYTE[Size];
				::memset(Datas, 0, Size);
			}
		}
		EShaderParameterRaw()noexcept : Datas(nullptr), Size(0u) {}
		EShaderParameterRaw(const EShaderParameterRaw& Other) : Datas(nullptr), Size(0u)
		{
			if ((Other.Size > 0u) && ((Other.Size % 2u) == 0u) && (!!(Other.Datas)))
			{
				Size	= Other.Size;
				Datas	= new BYTE[Size];
				::memcpy_s(Datas, Size, Other.Datas, Other.Size);
			}
		}
		EShaderParameterRaw& operator=(const EShaderParameterRaw& Other)
		{
			Release();
			if ((Other.Size > 0u) && ((Other.Size % 2u) == 0u) && (!!(Other.Datas)))
			{
				Size	= Other.Size;
				Datas	= new BYTE[Size];
				::memcpy_s(Datas, Size, Other.Datas, Other.Size);
			}
			return (*this);
		}
		void Release()
		{
			if (Datas)
			{
				delete[]Datas;
				Datas = nullptr;
			}
			Size = 0u;
		}
	};

	struct EShaderParameter
	{
	public:
		BYTE*							ValuePtr;
		EShaderParameterContainerType	ContainerType;
		EShaderParameterValueType		ValueType;
		EShaderParameterBase			ParameterInfo;

		template<typename _TValueType>
		void SetupParameter(BYTE* InRawPtr, const UINT32 InOffset, EShaderParameterValueType InValueType, const _TValueType* InInitValuePtr = nullptr,
			EShaderParameterContainerType InContainerType = EShaderParameterContainerType::SHADER_PARAMETER_CONTAINER_TYPE_SINGLE,
			const UINT32 InElementNum = 1u)
		{
			Check((ENGINE_RENDER_CORE_ERROR), ("Shader parameter can not greater than 2G."), ((sizeof(_TValueType) * InElementNum) <= ((UINT32)(-1))));
			Check((ENGINE_RENDER_CORE_ERROR), ("Check shader parameter raw pointer failed."), (!!InRawPtr));
			Check((ENGINE_RENDER_CORE_ERROR), ("Check shader parameter value type failed."), (
				(InValueType > EShaderParameterValueType::SHADER_PARAMETER_TYPE_UNKNOWN) &&
				(InValueType < EShaderParameterValueType::SHADER_PARAMETER_TYPE_COUNT)));
			Check((ENGINE_RENDER_CORE_ERROR), ("Check shader parameter container type failed."), (
				(InContainerType > EShaderParameterContainerType::SHADER_PARAMETER_CONTAINER_TYPE_UNKNOWN) &&
				(InContainerType < EShaderParameterContainerType::SHADER_PARAMETER_CONTAINER_TYPE_COUNT)));
			Check((ENGINE_RENDER_CORE_ERROR), ("Check shader parameter element num failed."), (InElementNum > 0u));

			ValuePtr				= &(InRawPtr[InOffset]);
			ContainerType			= InContainerType;
			ValueType				= InValueType;
			ParameterInfo.Offset	= InOffset;
			ParameterInfo.Size		= sizeof(_TValueType);
			ParameterInfo.Element	= InElementNum;
			if (InInitValuePtr)
			{
				const UINT32 MemSize = sizeof(_TValueType) * InElementNum;
				::memcpy_s(ValuePtr, MemSize, InInitValuePtr, MemSize);
			}
		}
		EShaderParameter() : ValuePtr(nullptr)
			, ContainerType(EShaderParameterContainerType::SHADER_PARAMETER_CONTAINER_TYPE_UNKNOWN)
			, ValueType(EShaderParameterValueType::SHADER_PARAMETER_TYPE_UNKNOWN)
		{
			ParameterInfo.Offset	= 0u;
			ParameterInfo.Size		= 0u;
			ParameterInfo.Element	= 0u;
		}
		EShaderParameter(const EShaderParameter& Other) : ValuePtr(nullptr)
			, ContainerType(Other.ContainerType), ValueType(Other.ValueType)
			, ParameterInfo(Other.ParameterInfo)
		{
		}
		EShaderParameter& operator=(const EShaderParameter& Other)
		{
			ValuePtr		= nullptr;
			ContainerType	= Other.ContainerType;
			ValueType		= Other.ValueType;
			ParameterInfo	= Other.ParameterInfo;
			return (*this);
		}
		void operator=(const void* InValuePtr)
		{
			Check((ENGINE_RENDER_CORE_ERROR), ("Shader parameter can not greater than 2G."), ((sizeof(ParameterInfo.Size) * InElementNum) <= ((UINT32)(-1))));
			Check((ENGINE_RENDER_CORE_ERROR), ("Check shader parameter raw pointer failed."), (!!InRawPtr));
			Check((ENGINE_RENDER_CORE_ERROR), ("Check shader parameter value type failed."), (
				(InValueType > EShaderParameterValueType::SHADER_PARAMETER_TYPE_UNKNOWN) &&
				(InValueType < EShaderParameterValueType::SHADER_PARAMETER_TYPE_COUNT)));
			Check((ENGINE_RENDER_CORE_ERROR), ("Check shader parameter container type failed."), (
				(InContainerType > EShaderParameterContainerType::SHADER_PARAMETER_CONTAINER_TYPE_UNKNOWN) &&
				(InContainerType < EShaderParameterContainerType::SHADER_PARAMETER_CONTAINER_TYPE_COUNT)));
			Check((ENGINE_RENDER_CORE_ERROR), ("Check shader parameter element num failed."), (InElementNum > 0u));
		}
	};

	class EMaterialParameter
	{
	public:
		virtual void	SetupParameters() {}
	protected:
		class ECommand final
		{
		public:
			template<typename _TFunctionType>
			void EnqueueCommand(_TFunctionType InFunction)
			{
				RawCommands.AddRegisterFunction<_TFunctionType>(InFunction);
			}
			void DoCommands()
			{
				RawCommands.DoRegister();
			}
			void EmptyQueue()
			{
				RawCommands.ClearRegister();
			}
		private:
			ERegisterBase	RawCommands;
		public:
			ECommand() {}
			ECommand(const ECommand&) = delete;
			~ECommand() {}
			ECommand& operator=(const ECommand&) = delete;
		};
		template<typename _TValueType>
		void AddParameterInfo(const _TValueType* InInitValuePtr = nullptr, const UINT32 InElementNum = 1u)
		{
			RawData.Size += sizeof(_TValueType) * InElementNum;
		}
	public:
		void	ReleaseParameter();
	private:
		ECommand					InitCommands;
		EShaderParameterRaw			RawData;
		TMap<EString, UINT32>		ShaderParameterNames;
		TArray<EShaderParameter>	ShaderParameters;
	public:
		EMaterialParameter();
		EMaterialParameter(const EMaterialParameter& Other);
		virtual ~EMaterialParameter();
		EMaterialParameter& operator=(const EMaterialParameter&) = delete;
	};

};