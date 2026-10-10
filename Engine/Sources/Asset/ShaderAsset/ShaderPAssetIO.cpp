#include "ShaderPAssetIO.h"

#include <BaseAsset.h>
#include <IO/FileHelper.h>
#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	// ---------------------------------------------------------------------------
	// Writer
	// ---------------------------------------------------------------------------

	EShaderPAssetWriter::EShaderPAssetWriter()
		: Bytes{}
	{
	}

	EShaderPAssetWriter::~EShaderPAssetWriter()
	{
	}

	// TArray has no raw pointer append, so grow first and copy into the tail; SetNum
	// keeps the existing elements and default initializes the new ones.
	void EShaderPAssetWriter::AppendRaw(const void* InData, const UINT32 InSize)
	{
		if ((!InData) || (InSize == 0u))
		{
			return;
		}
		const UINT32 OldByteNum = Bytes.Num<UINT32>();
		Bytes.SetNum(OldByteNum + InSize);
		::memcpy_s((Bytes.GetData() + OldByteNum), static_cast<rsize_t>(InSize), InData, static_cast<rsize_t>(InSize));
	}

	void EShaderPAssetWriter::WriteUInt32(const UINT32 InValue)
	{
		AppendRaw(&InValue, sizeof(UINT32));
	}

	void EShaderPAssetWriter::WriteInt32(const INT32 InValue)
	{
		AppendRaw(&InValue, sizeof(INT32));
	}

	void EShaderPAssetWriter::WriteString(const EString& InText)
	{
		const UINT32 TextLength = InText.Length();
		WriteUInt32(TextLength);
		if (TextLength > 0u)
		{
			AppendRaw(*InText, TextLength);
		}
	}

	void EShaderPAssetWriter::WriteBytes(const void* InData, const UINT32 InSize)
	{
		WriteUInt32(InSize);
		if ((InData) && (InSize > 0u))
		{
			AppendRaw(InData, InSize);
		}
	}

	BOOL32 EShaderPAssetWriter::SaveToFile(const EString& InFullPath) const
	{
		if (Bytes.Num<UINT32>() == 0u)
		{
			return FALSE;
		}
		return EFileHelper::SaveBytesToFile(InFullPath, Bytes.GetData(), Bytes.Num<ULONG>(), TRUE);
	}

	// ---------------------------------------------------------------------------
	// Reader
	// ---------------------------------------------------------------------------

	EShaderPAssetReader::EShaderPAssetReader()
		: Bytes{}
		, Cursor(0u)
	{
	}

	EShaderPAssetReader::~EShaderPAssetReader()
	{
	}

	BOOL32 EShaderPAssetReader::OpenFromFile(const EString& InFullPath)
	{
		void* ReadFileMem = nullptr;
		ULONG ReadFileSize = 0u;
		if (!EFileHelper::ReadFileAsBinary(InFullPath, ReadFileMem, ReadFileSize))
		{
			if (ReadFileMem)
			{
				delete[] static_cast<BYTE*>(ReadFileMem);
				ReadFileMem = nullptr;
			}
			return FALSE;
		}
		if ((!ReadFileMem) || (ReadFileSize == 0u))
		{
			if (ReadFileMem)
			{
				delete[] static_cast<BYTE*>(ReadFileMem);
				ReadFileMem = nullptr;
			}
			return FALSE;
		}
		if (Bytes.Num<UINT32>() > 0u)
		{
			Bytes.Empty();
		}
		Bytes.SetNum(static_cast<UINT32>(ReadFileSize));
		::memcpy_s(Bytes.GetData(), static_cast<rsize_t>(ReadFileSize), ReadFileMem, static_cast<rsize_t>(ReadFileSize));
		delete[] static_cast<BYTE*>(ReadFileMem);
		ReadFileMem = nullptr;
		Cursor = 0u;
		return TRUE;
	}

	BOOL32 EShaderPAssetReader::ReadUInt32(UINT32& OutValue)
	{
		if ((Cursor + sizeof(UINT32)) > Bytes.Num<UINT32>())
		{
			return FALSE;
		}
		::memcpy_s(&OutValue, sizeof(UINT32), (Bytes.GetData() + Cursor), sizeof(UINT32));
		Cursor += sizeof(UINT32);
		return TRUE;
	}

	BOOL32 EShaderPAssetReader::ReadInt32(INT32& OutValue)
	{
		if ((Cursor + sizeof(INT32)) > Bytes.Num<UINT32>())
		{
			return FALSE;
		}
		::memcpy_s(&OutValue, sizeof(INT32), (Bytes.GetData() + Cursor), sizeof(INT32));
		Cursor += sizeof(INT32);
		return TRUE;
	}

	BOOL32 EShaderPAssetReader::ReadString(EString& OutText)
	{
		UINT32 TextLength = 0u;
		if (!ReadUInt32(TextLength))
		{
			return FALSE;
		}
		if ((Cursor + TextLength) > Bytes.Num<UINT32>())
		{
			return FALSE;
		}
		if (TextLength == 0u)
		{
			OutText = EString();
			return TRUE;
		}
		// EString wants a terminated character range, so build it through std::string.
		const std::string TempStr(reinterpret_cast<const char*>(Bytes.GetData() + Cursor), static_cast<size_t>(TextLength));
		OutText = EString(TempStr);
		Cursor += TextLength;
		return TRUE;
	}

	BOOL32 EShaderPAssetReader::ReadBytesCopy(TArray<BYTE>& OutBytes)
	{
		UINT32 BlobSize = 0u;
		if (!ReadUInt32(BlobSize))
		{
			return FALSE;
		}
		if ((Cursor + BlobSize) > Bytes.Num<UINT32>())
		{
			return FALSE;
		}
		if (OutBytes.Num<UINT32>() > 0u)
		{
			OutBytes.Empty();
		}
		if (BlobSize > 0u)
		{
			OutBytes.SetNum(BlobSize);
			::memcpy_s(OutBytes.GetData(), static_cast<rsize_t>(BlobSize), (Bytes.GetData() + Cursor), static_cast<rsize_t>(BlobSize));
		}
		Cursor += BlobSize;
		return TRUE;
	}

	BOOL32 EShaderPAssetReader::IsAtEnd() const
	{
		return ((Cursor >= Bytes.Num<UINT32>()) ? TRUE : FALSE);
	}

	// ---------------------------------------------------------------------------
	// Header
	// ---------------------------------------------------------------------------

	BOOL32 WriteShaderPAssetHeader(EShaderPAssetWriter& InOutWriter, const UINT32 InKind)
	{
		InOutWriter.WriteUInt32(static_cast<UINT32>(EAssetType::ASSET_TYPE_SHADER));
		InOutWriter.WriteUInt32(InKind);
		return TRUE;
	}

	BOOL32 ReadShaderPAssetHeader(EShaderPAssetReader& InOutReader, UINT32& OutKind)
	{
		UINT32 AssetTypeValue = 0u;
		if (!InOutReader.ReadUInt32(AssetTypeValue))
		{
			return FALSE;
		}
		if (AssetTypeValue != static_cast<UINT32>(EAssetType::ASSET_TYPE_SHADER))
		{
			PE_LOG_ERROR(EString("[ShaderPAsset] The file is not a shader asset."));
			return FALSE;
		}
		if (!InOutReader.ReadUInt32(OutKind))
		{
			return FALSE;
		}
		return TRUE;
	}

} // namespace PigeonEngine
