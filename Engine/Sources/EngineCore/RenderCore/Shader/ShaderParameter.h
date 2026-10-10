#pragma once

#include <CoreMinimal.h>
#include <Core/Base/DataStructure/Text/Name.h>
#include <RenderResource.h>

namespace PigeonEngine
{

// Declares the ctor / dtor a concrete constant block needs and forbids copying: a block
// owns a GPU buffer and a field name table, so duplicating it would give two objects the
// same layout but separate storage. Both members are defined in class so a block that
// adds nothing of its own does not need an out of line definition.
#define CLASS_SHADER_PARAMETER(__Name) \
	public:\
		__Name() noexcept {}\
		virtual ~__Name() noexcept {}\
		__Name(const __Name&) = delete;\
		__Name(__Name&&) = delete;\
		__Name& operator=(const __Name&) = delete;\
		__Name& operator=(__Name&&) = delete;\


	// Terse constructor for the field formats the engine internal constant blocks use.
	// Field declaration happens once per block, so the call sites stay readable.
	PE_NODISCARD PE_INLINE EVariableFormat	MakeShaderVariableFormat(const EVariableFormatPrefixType InPrefix, const EVariableFormatSuffixType InSuffix) noexcept
	{
		EVariableFormat Format;
		Format.Prefix = InPrefix;
		Format.Suffix = InSuffix;
		return Format;
	}

	extern PE_INLINE FLOAT			TranslateUploadBaseType(const FLOAT InData);	extern PE_INLINE INT32			TranslateUploadBaseType(const INT32 InData);
	extern PE_INLINE INT32			TranslateUploadBaseType(const UINT32 InData);
	extern PE_INLINE Matrix4x4		TranslateUploadMatrixType(const Matrix4x4& InData);
	extern PE_INLINE Matrix4x4		TranslateUploadTransposeMatrixType(const Matrix4x4& InData);
	extern PE_INLINE Vector3		TranslateUploadVectorType(const Vector3& InData);
	extern PE_INLINE Vector4		TranslateUploadVectorType(const Vector4& InData);
	extern PE_INLINE Vector2Int		TranslateUploadVectorType(const Vector2Int& InData);
	extern PE_INLINE Vector4Int		TranslateUploadVectorType(const Vector4Int& InData);
	extern PE_INLINE Vector3		TranslateUploadColorType(const Color3& InData);
	extern PE_INLINE Vector4		TranslateUploadColorType(const Color4& InData);

	class EConstantBufferParameter : public EExclusiveStruct
	{
	public:
		virtual void				BeginSetupVariable() noexcept override;
		virtual void				EndSetupVariable(UINT32& OutHeapSize) noexcept override;
		// Declares this block's fields through DoSetupVariable and creates the GPU
		// buffer. A concrete block only implements DoSetupVariable.
		void						SetupParameters() noexcept;
		virtual void				CreateBuffer() noexcept;
		virtual void				UploadBuffer() noexcept;
		virtual void				ReleaseBuffer() noexcept;
		RBufferResource&			GetBuffer() noexcept;
		const RBufferResource&		GetBuffer() const noexcept;
		BOOL8						IsHasPaddings() const noexcept;
		// Writes one field by name. The value type has to match the format the field was
		// declared with, which is the same contract AddStructVariable already has; the
		// back-end copy is a plain memcpy of sizeof(_TValueType) bytes.
		template<typename _TValueType>
		BOOL32						SetFieldValue(const EName& InVarName, const _TValueType& InValue) noexcept
		{
			EStructVariable Variable;
			if (!FindVariable(InVarName, Variable))
			{
				return FALSE;
			}
			Variable.SetValue(reinterpret_cast<const UINT8*>(&InValue), static_cast<UINT32>(sizeof(_TValueType)));
			return TRUE;
		}
	protected:
		virtual void				BeginSetupCBuffer() noexcept;
		virtual void				EndSetupCBuffer() noexcept;
	protected:
		RBufferResource				Buffer{};
		BOOL8						bHasPaddings = FALSE;
	public:

		CLASS_SHADER_PARAMETER(EConstantBufferParameter)

	};

	class EStructuredBufferParameter : public EStructArray
	{
	public:
		virtual void					BeginSetupVariable() noexcept override;
		virtual void					EndSetupVariable(UINT32& OutHeapSize) noexcept override;
		virtual void					CreateBuffer() noexcept;
		virtual void					UploadBuffer() noexcept;
		virtual void					ReleaseBuffer() noexcept;
		void							ResizeElement(const UINT32 InNumElements) noexcept;
		RStructuredBuffer&				GetBuffer() noexcept;
		const RStructuredBuffer&		GetBuffer() const noexcept;
	protected:
		virtual void					BeginSetupStructuredBuffer() noexcept;
		virtual void					EndSetupStructuredBuffer() noexcept;
	protected:
		RStructuredBuffer				Buffer;
	public:
		EStructuredBufferParameter(UINT32 InNumElements) noexcept;

	protected:
		// Element count the caller asked for. ClearStruct() resets the base class count on
		// every setup, so the request is kept here and restored by BeginSetupVariable.
		UINT32						ElementCount = 0u;

		CLASS_SHADER_PARAMETER(EStructuredBufferParameter)
	};

};
