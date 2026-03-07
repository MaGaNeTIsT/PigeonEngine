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


	extern PE_INLINE FLOAT		TranslateUploadBaseType(const FLOAT InData);
	extern PE_INLINE INT32		TranslateUploadBaseType(const INT32 InData);
	extern PE_INLINE INT32		TranslateUploadBaseType(const UINT32 InData);
	extern PE_INLINE Matrix4x4	TranslateUploadMatrixType(const Matrix4x4& InData);
	extern PE_INLINE Matrix4x4	TranslateUploadTransposeMatrixType(const Matrix4x4& InData);
	extern PE_INLINE Vector3	TranslateUploadVectorType(const Vector3& InData);
	extern PE_INLINE Vector4	TranslateUploadVectorType(const Vector4& InData);
	extern PE_INLINE Vector2Int	TranslateUploadVectorType(const Vector2Int& InData);
	extern PE_INLINE Vector4Int	TranslateUploadVectorType(const Vector4Int& InData);
	extern PE_INLINE Vector3	TranslateUploadColorType(const Color3& InData);
	extern PE_INLINE Vector4	TranslateUploadColorType(const Color4& InData);

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
		UINT8							ValueType;
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
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Shader parameter can not greater than 2G."), ((sizeof(_TValueType) * InElementNum) < ((UINT32)(-1))));
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check shader parameter raw pointer failed."), (!!InRawPtr));
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check shader parameter value type failed."), (
				(InValueType > EShaderParameterValueType::SHADER_PARAMETER_TYPE_UNKNOWN) &&
				(InValueType < EShaderParameterValueType::SHADER_PARAMETER_TYPE_COUNT)));
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check shader parameter element num failed."), (InElementNum > 0u));

			ValuePtr				= InRawPtr;
			ValueType				= InValueType;
			ParameterInfo.Offset	= InOffset;
			ParameterInfo.Size		= sizeof(_TValueType);
			ParameterInfo.Element	= InElementNum;
			if (InInitValuePtr)
			{
				const UINT32 MemSize = sizeof(_TValueType) * InElementNum;
				BYTE* DstPtr = &(ValuePtr[ParameterInfo.Offset]);
				::memcpy_s(DstPtr, MemSize, InInitValuePtr, MemSize);
			}
		}
		EShaderParameter() : ValuePtr(nullptr)
			, ValueType(EShaderParameterValueType::SHADER_PARAMETER_TYPE_UNKNOWN)
		{
			ParameterInfo.Offset	= 0u;
			ParameterInfo.Size		= 0u;
			ParameterInfo.Element	= 0u;
		}
		EShaderParameter(const EShaderParameter& Other)
			: ValuePtr(Other.ValuePtr), ValueType(Other.ValueType)
			, ParameterInfo(Other.ParameterInfo)
		{
		}
		EShaderParameter& operator=(const EShaderParameter& Other)
		{
			ValuePtr		= Other.ValuePtr;
			ValueType		= Other.ValueType;
			ParameterInfo	= Other.ParameterInfo;
			return (*this);
		}
		void operator=(const void* InValuePtr)
		{
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Shader parameter is invalid, so can not copy value."), (
				(!!ValuePtr) &&
				(ValueType > EShaderParameterValueType::SHADER_PARAMETER_TYPE_UNKNOWN) &&
				(ValueType < EShaderParameterValueType::SHADER_PARAMETER_TYPE_COUNT) &&
				(ParameterInfo.Element > 0u) &&
				(ParameterInfo.Size > 0u) &&
				(ParameterInfo.Size < ((UINT32)(-1)))));
			if (InValuePtr)
			{
				const UINT32 MemSize = ParameterInfo.Size * ParameterInfo.Element;
				BYTE* DstPtr = &(ValuePtr[ParameterInfo.Offset]);
				::memcpy_s(DstPtr, MemSize, InValuePtr, MemSize);
			}
		}
	};

	class EShaderStruct
	{
	public:
		virtual void			SetupParameters() {}
		virtual void			ClearParameter();
		virtual void			CreateBuffer()	= 0;
		virtual void			UploadBuffer()	= 0;
		virtual void			ClearBuffer()	= 0;
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
			ERegisterBase<EVoidFunctionType>	RawCommands;
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
			EShaderParameterRaw& TempRawData = RawData;
			InitCommands.EnqueueCommand(
				[InValueName, InInitValuePtr, InElementNum, TempOffset, this, &TempRawData]()->void
				{
#if _EDITOR_ONLY
					if (ShaderParameterNames.ContainsKey(InValueName))
					{
						PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Shader parameter is exist, so can not add same value."));
						return;
					}
#endif
					ShaderParameterNames.Add(InValueName, ShaderParameters.Num<UINT32>());
					EShaderParameter& ShaderParameter = ShaderParameters.AddDefaultGetRef();
					ShaderParameter.SetupParameter<_TValueType>(TempRawData.Datas, TempOffset, __TParameterValueType, InInitValuePtr, InElementNum);
				});
		}
		virtual EShaderParameter*	FindParameter(const EString& InParamName);
		virtual void				BeginSetupParameter();
		virtual void				EndSetupParameter();
	protected:
		ECommand					InitCommands;
		EShaderParameterRaw			RawData;
		TMap<EString, UINT32>		ShaderParameterNames;
		TArray<EShaderParameter>	ShaderParameters;
	public:
		EShaderStruct();
		EShaderStruct(const EShaderStruct& Other);
		virtual ~EShaderStruct();
		EShaderStruct& operator=(const EShaderStruct&) = delete;
	};

	class EMaterialParameter : public EShaderStruct
	{
	public:
		virtual void			ClearParameter()override;
		virtual void			CreateBuffer()override;
		virtual void			UploadBuffer()override;
		virtual void			ClearBuffer()override;
		EShaderParameter&		operator[](const EString& InParamName);
		RBufferResource&		GetConstantBuffer();
		const RBufferResource&	GetConstantBuffer()const;
		BOOL32					IsHasPaddings()const;
	protected:
		virtual void			BeginSetupParameter()override;
		virtual void			EndSetupParameter()override;
	private:
		BOOL32					HasPaddings;
		RBufferResource			ConstantBuffer;
	public:
		EMaterialParameter();
		EMaterialParameter(const EMaterialParameter& Other);
		virtual ~EMaterialParameter();
		EMaterialParameter& operator=(const EMaterialParameter&) = delete;
	};

	class EMaterialStructParameter : public EShaderStruct
	{
	public:
		virtual void				ClearParameter()override;
		virtual void				CreateBuffer()override;
		virtual void				UploadBuffer()override;
		virtual void				ClearBuffer()override;
		virtual EShaderParameter*	FindParameter(const EString& InParamName)override;
		EShaderParameter			FindParameter(const UINT32 InStructIndex, const EString& InParamName);
		void						SetElementNum(const UINT32 InElementNum);
		UINT32						GetElementNum()const;
		RStructuredBuffer&			GetStructBuffer();
		const RStructuredBuffer&	GetStructBuffer()const;
		void						BeginSetupStructParameter(const UINT32 InElementNum);
		void						EndSetupStructParameter();
	protected:
		virtual void				BeginSetupParameter()override;
		virtual void				EndSetupParameter()override;
	private:
		UINT32						ElementNum;
		RStructuredBuffer			StructuredBuffer;
	public:
		EMaterialStructParameter();
		EMaterialStructParameter(const EMaterialStructParameter& Other);
		virtual ~EMaterialStructParameter();
		EMaterialStructParameter& operator=(const EMaterialStructParameter&) = delete;
	};

};