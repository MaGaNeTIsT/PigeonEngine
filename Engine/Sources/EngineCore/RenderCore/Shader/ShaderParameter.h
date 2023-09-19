#pragma once

#include <CoreMinimal.h>
#include <RenderResource.h>

namespace PigeonEngine
{

#define CLASS_MATERIAL_PARAMETER(__Name) \
	public:\
		__Name() {}\
		virtual ~__Name() {}\
		__Name(const __Name&) = delete;\
		__Name& operator=(const __Name&) = delete;\


	extern PE_INLINE Matrix4x4	TranslateUploadMatrixType(const Matrix4x4& InData);
	extern PE_INLINE Matrix4x4	TranslateUploadTransposeMatrixType(const Matrix4x4& InData);
	extern PE_INLINE Vector4	TranslateUploadVectorType(const Vector4& InData);

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
		void Resize(const ULONGLONG& InSize)
		{
			if (Datas)
			{
				delete[]Datas;
				Datas = nullptr;
			}
			Size = 0u;
			if ((InSize > 0u) && ((InSize % 2u) == 0u))
			{
				Size	= InSize;
				Datas	= new BYTE[Size];
				::memset(Datas, 0, Size);
			}
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
		EShaderParameterValueType		ValueType;
		EShaderParameterBase			ParameterInfo;

		BOOL32 IsValid()const
		{
			return ((!!ValuePtr) &&
				(ValueType > EShaderParameterValueType::SHADER_PARAMETER_TYPE_UNKNOWN) &&
				(ValueType < EShaderParameterValueType::SHADER_PARAMETER_TYPE_COUNT) &&
				(ParameterInfo.Size > 0u) &&
				(ParameterInfo.Element > 0u));
		}
		template<typename _TValueType>
		void SetupParameter(BYTE* InRawPtr, const UINT32 InOffset, EShaderParameterValueType InValueType,
			const _TValueType* InInitValuePtr = nullptr, const UINT32 InElementNum = 1u)
		{
			Check((ENGINE_RENDER_CORE_ERROR), ("Shader parameter can not greater than 2G."), ((sizeof(_TValueType) * InElementNum) < ((UINT32)(-1))));
			Check((ENGINE_RENDER_CORE_ERROR), ("Check shader parameter raw pointer failed."), (!!InRawPtr));
			Check((ENGINE_RENDER_CORE_ERROR), ("Check shader parameter value type failed."), (
				(InValueType > EShaderParameterValueType::SHADER_PARAMETER_TYPE_UNKNOWN) &&
				(InValueType < EShaderParameterValueType::SHADER_PARAMETER_TYPE_COUNT)));
			Check((ENGINE_RENDER_CORE_ERROR), ("Check shader parameter element num failed."), (InElementNum > 0u));

			ValuePtr				= &(InRawPtr[InOffset]);
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
			, ValueType(EShaderParameterValueType::SHADER_PARAMETER_TYPE_UNKNOWN)
		{
			ParameterInfo.Offset	= 0u;
			ParameterInfo.Size		= 0u;
			ParameterInfo.Element	= 0u;
		}
		EShaderParameter(const EShaderParameter& Other) : ValuePtr(nullptr)
			, ParameterInfo(Other.ParameterInfo)
		{
		}
		EShaderParameter& operator=(const EShaderParameter& Other)
		{
			ValuePtr		= nullptr;
			ValueType		= Other.ValueType;
			ParameterInfo	= Other.ParameterInfo;
			return (*this);
		}
		void operator=(const void* InValuePtr)
		{
			Check((ENGINE_RENDER_CORE_ERROR), ("Shader parameter is invalid, so can not copy value."), (
				(!!ValuePtr) &&
				(ValueType > EShaderParameterValueType::SHADER_PARAMETER_TYPE_UNKNOWN) &&
				(ValueType < EShaderParameterValueType::SHADER_PARAMETER_TYPE_COUNT) &&
				(ParameterInfo.Element > 0u) &&
				(ParameterInfo.Size > 0u) &&
				(ParameterInfo.Size < ((UINT32)(-1)))));
			if (InValuePtr)
			{
				const UINT32 MemSize = ParameterInfo.Size * ParameterInfo.Element;
				::memcpy_s(ValuePtr, MemSize, InValuePtr, MemSize);
			}
		}
	};

	class EMaterialParameter
	{
	public:
		virtual void			SetupParameters() {}
		EShaderParameter&		operator[](const EString& InParamName);
		void					CreateConstantBuffer();
		void					UploadConstantBuffer();
		RBufferResource&		GetConstantBuffer();
		const RBufferResource&	GetConstantBuffer()const;
		void					ClearParameter();
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
			UINT32 GetQueueCount()const
			{
				return (RawCommands.GetRegisterNum());
			}
		private:
			ERegisterBase	RawCommands;
		public:
			ECommand() {}
			ECommand(const ECommand&) = delete;
			~ECommand() {}
			ECommand& operator=(const ECommand&) = delete;
		};
		template<typename _TValueType, EShaderParameterValueType __TParameterValueType>
		void AddParameter(const EString& InValueName, const _TValueType* InInitValuePtr = nullptr, const UINT32 InElementNum = 1u)
		{
			const UINT32 TempOffset = static_cast<UINT32>(RawData.Size);
			RawData.Size += sizeof(_TValueType) * InElementNum;
			InitCommands.EnqueueCommand(
				[InValueName, InInitValuePtr, InElementNum, this, TempOffset]()->void
				{
#if _EDITOR_ONLY
					if (ShaderParameterNames.ContainsKey(InValueName))
					{
						PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Shader parameter is exist, so can not add same value."));
						return;
					}
#endif
					ShaderParameterNames.Add(InValueName, ShaderParameters.Length());
					EShaderParameter& ShaderParameter = ShaderParameters.Add_Default_GetRef();
					ShaderParameter.SetupParameter<_TValueType>(RawData.Datas, TempOffset, __TParameterValueType, InInitValuePtr, InElementNum);
				});
		}
		void BeginSetupParameter();
		void EndSetupParameter();
	protected:
		BOOL32						HasPaddings;
	private:
		ECommand					InitCommands;
		EShaderParameterRaw			RawData;
		TMap<EString, UINT32>		ShaderParameterNames;
		TArray<EShaderParameter>	ShaderParameters;
		RBufferResource				ConstantBuffer;
	public:
		EMaterialParameter();
		EMaterialParameter(const EMaterialParameter& Other);
		virtual ~EMaterialParameter();
		EMaterialParameter& operator=(const EMaterialParameter&) = delete;
	};

};