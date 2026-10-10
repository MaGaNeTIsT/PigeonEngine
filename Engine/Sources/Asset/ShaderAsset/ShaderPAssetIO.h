#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>

namespace PigeonEngine
{

	/*
	* ShaderAsset - PAsset binary IO.
	*
	* The shader PAssets follow the same shape as the other assets: a flat little
	* endian byte buffer with an asset type header, written and read by the same
	* build, no alignment padding and no version negotiation. A string is a UINT32
	* length followed by that many raw characters, a blob is a UINT32 size followed by
	* that many bytes.
	*
	* These helpers exist so the stage asset and the shader asset do not have to walk
	* raw pointers by hand; the format itself stays trivially inspectable.
	*/

	// What kind of shader asset a file holds: the compiled binary of one (variant,
	// stage), or the aggregate that references those binaries.
	enum EShaderPAssetKindType : UINT32
	{
		SHADER_PASSET_KIND_STAGE	= 0,
		SHADER_PASSET_KIND_SHADER	= 1,
		SHADER_PASSET_KIND_COUNT,
	};

	class EShaderPAssetWriter
	{	public:
		EShaderPAssetWriter();
		~EShaderPAssetWriter();
	public:
		void	WriteUInt32(const UINT32 InValue);
		void	WriteInt32(const INT32 InValue);
		void	WriteString(const EString& InText);
		void	WriteBytes(const void* InData, const UINT32 InSize);
		PE_NODISCARD const TArray<BYTE>&	GetBytes() const { return Bytes; }
		PE_NODISCARD UINT32					GetByteSize() const { return Bytes.Num<UINT32>(); }
		BOOL32	SaveToFile(const EString& InFullPath) const;
	private:
		void	AppendRaw(const void* InData, const UINT32 InSize);
	private:
		TArray<BYTE> Bytes;

	public:
		CLASS_REMOVE_COPY_BODY(EShaderPAssetWriter)

	};

	class EShaderPAssetReader
	{
	public:
		EShaderPAssetReader();
		~EShaderPAssetReader();
	public:
		BOOL32	OpenFromFile(const EString& InFullPath);
		BOOL32	ReadUInt32(UINT32& OutValue);
		BOOL32	ReadInt32(INT32& OutValue);
		BOOL32	ReadString(EString& OutText);
		// Copies the blob out, which is what the cold data wants: the reader's buffer
		// must not outlive the load call.
		BOOL32	ReadBytesCopy(TArray<BYTE>& OutBytes);
		PE_NODISCARD UINT32	GetReadPosition() const { return Cursor; }
		PE_NODISCARD UINT32	GetByteSize() const { return Bytes.Num<UINT32>(); }
		PE_NODISCARD BOOL32	IsAtEnd() const;
	private:
		TArray<BYTE>	Bytes;
		UINT32			Cursor;

	public:
		CLASS_REMOVE_COPY_BODY(EShaderPAssetReader)

	};

	// Header every shader PAsset starts with: the shared asset type followed by the
	// shader kind, so a stage PAsset and a shader PAsset are told apart on load.
	PE_NODISCARD extern BOOL32	WriteShaderPAssetHeader(EShaderPAssetWriter& InOutWriter, const UINT32 InKind);
	PE_NODISCARD extern BOOL32	ReadShaderPAssetHeader(EShaderPAssetReader& InOutReader, UINT32& OutKind);

} // namespace PigeonEngine
