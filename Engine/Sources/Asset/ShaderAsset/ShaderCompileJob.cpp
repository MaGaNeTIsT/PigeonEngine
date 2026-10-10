#include "ShaderCompileJob.h"

#include <ShaderAsset/ShaderAsset.h>
#include <IO/FileHelper.h>
#include <Editor/EditorLogManager.h>
#include <Config/EngineConfig.h>

#if _EDITOR_ONLY
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif	// _EDITOR_ONLY

namespace PigeonEngine
{

	static EString GetDefaultShaderSourceDirectory()
	{
		return EString(EBaseSettings::ENGINE_SHADER_SOURCE_DIRECTORY);
	}

	static EString EnsureTrailingSlash(const EString& InDirectory)
	{
		if (InDirectory.Length() == 0u)
		{
			return InDirectory;
		}
		const CHAR* DirectoryData = *InDirectory;
		if (DirectoryData[InDirectory.Length() - 1u] == '/')
		{
			return InDirectory;
		}
		return (InDirectory + EString("/"));
	}

	// "M_DefaultLit/M_DefaultLit.json" -> "M_DefaultLit". The artifacts are named after
	// the entry json, so the engine never has to parse the shader source for it.
	static EString GetShaderSourceNameFromEntryPath(const EString& InEntryJsonPath)
	{
		const INT32 SlashPos = InEntryJsonPath.RightFind(EString("/"));
		const EString LeafName = ((SlashPos >= 0)
			? InEntryJsonPath.Right(InEntryJsonPath.Length() - static_cast<UINT32>(SlashPos) - 1u)
			: InEntryJsonPath);
		const INT32 DotPos = LeafName.RightFind(EString("."));
		if (DotPos > 0)
		{
			return LeafName.Left(static_cast<UINT32>(DotPos));
		}
		return LeafName;
	}

	// ---------------------------------------------------------------------------
	// Paths
	// ---------------------------------------------------------------------------

	BOOL32 GetShaderBuildConfigName(EString& OutName)
	{
#if _SHIPPING_MODE
		OutName = EString("Shipping");
#elif _DEVELOP_MODE
		OutName = (_EDITOR_ONLY ? EString("DevelopEditor") : EString("Develop"));
#else
		OutName = (_EDITOR_ONLY ? EString("DebugEditor") : EString("Debug"));
#endif
		return TRUE;
	}

	BOOL32 GetShaderOutputDirectory(const EShaderCompileRequest& InRequest, EString& OutDirectory)
	{
		EString OutputRoot = InRequest.OutputDir;
		if (OutputRoot.Length() == 0u)
		{
			OutputRoot = EString(EBaseSettings::ENGINE_SHADER_BUILD_DIRECTORY);
		}
		OutputRoot = EnsureTrailingSlash(OutputRoot);

		EString BuildConfigName;
		if (!GetShaderBuildConfigName(BuildConfigName))
		{
			return FALSE;
		}
		EString ShaderModelDirectoryName;
		if (!GetShaderModelDirectoryName(InRequest.ShaderModel, ShaderModelDirectoryName))
		{
			return FALSE;
		}

		// Two targets must never share one directory: the reflection and the manifest
		// file names do not carry the shader model, so they would overwrite each other.
		OutDirectory = OutputRoot + BuildConfigName + EString("/") + ShaderModelDirectoryName + EString("/");
		return TRUE;
	}

	BOOL32 GetShaderManifestPath(const EShaderCompileRequest& InRequest, EString& OutManifestPath)
	{
		const EString SourceName = GetShaderSourceNameFromEntryPath(InRequest.EntryJsonPath);
		if (SourceName.Length() == 0u)
		{
			PE_LOG_ERROR(EString("[ShaderCompileJob] The compile request carries no entry json name."));
			return FALSE;
		}
		EString OutputDirectory;
		if (!GetShaderOutputDirectory(InRequest, OutputDirectory))
		{
			return FALSE;
		}
		EString StageSuffix;
		if (!GetShaderStageSuffix(InRequest.Stage, StageSuffix))
		{
			PE_LOG_ERROR(EString("[ShaderCompileJob] Unsupported stage in the compile request."));
			return FALSE;
		}
		OutManifestPath = OutputDirectory + SourceName + EString("_") + StageSuffix + EString(".Manifest.json");
		return TRUE;
	}

#if _EDITOR_ONLY

	BOOL32 EShaderCompileJob::GetCompileDescriptionJsonPath(const EShaderCompileRequest& InRequest, EString& OutCompileJsonPath)
	{
		const EString SourceName = GetShaderSourceNameFromEntryPath(InRequest.EntryJsonPath);
		if (SourceName.Length() == 0u)
		{
			return FALSE;
		}
		EString OutputDirectory;
		if (!GetShaderOutputDirectory(InRequest, OutputDirectory))
		{
			return FALSE;
		}
		EString StageSuffix;
		if (!GetShaderStageSuffix(InRequest.Stage, StageSuffix))
		{
			return FALSE;
		}
		OutCompileJsonPath = OutputDirectory + SourceName + EString("_") + StageSuffix + EString(".Compile.json");
		return TRUE;
	}

	BOOL32 EShaderCompileJob::IsUpToDate(const EShaderCompileRequest& InRequest)
	{
		// The cooked asset is the thing a load needs, so that is what is checked: a
		// manifest without its PAsset still has to be imported. The dependency graph
		// (entry json chain, includes, compiler binary) is a follow up; callers that know
		// better can always set bForceRecompile.
		EString OutputDirectory;
		if (!GetShaderOutputDirectory(InRequest, OutputDirectory))
		{
			return FALSE;
		}
		const EString SourceName = GetShaderSourceNameFromEntryPath(InRequest.EntryJsonPath);
		if (SourceName.Length() == 0u)
		{
			return FALSE;
		}
		return EFileHelper::IsFileExists(OutputDirectory + SourceName + EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE));
	}

	// ---------------------------------------------------------------------------
	// Compile description json
	// ---------------------------------------------------------------------------

	static EString EscapeJsonString(const EString& InText)
	{
		EString Result;
		const CHAR* TextData = *InText;
		while (*TextData)
		{
			const CHAR CurrentChar = *TextData++;
			if ((CurrentChar == '\\') || (CurrentChar == '"'))
			{
				Result += EString("\\");
			}
			Result += EString::FromChar(CurrentChar);
		}
		return Result;
	}

	static EString MakeJsonStringField(const CHAR* InFieldName, const EString& InValue)
	{
		return (EString("\"") + EString(InFieldName) + EString("\": \"") + EscapeJsonString(InValue) + EString("\""));
	}

	BOOL32 EShaderCompileJob::WriteCompileDescriptionJson(const EShaderCompileRequest& InRequest, EString& OutCompileJsonPath)
	{
		EString OutputDirectory;
		if (!GetShaderOutputDirectory(InRequest, OutputDirectory))
		{
			return FALSE;
		}
		if (!EFileHelper::MakeDirectory(OutputDirectory))
		{
			PE_LOG_ERROR(EString("[ShaderCompileJob] Cannot create the output directory: ") + OutputDirectory);
			return FALSE;
		}

		EString RootDirectory = InRequest.RootDir;
		if (RootDirectory.Length() == 0u)
		{
			RootDirectory = GetDefaultShaderSourceDirectory();
		}
		EString IncludeDirectory = InRequest.ShaderIncludeDir;
		if (IncludeDirectory.Length() == 0u)
		{
			IncludeDirectory = RootDirectory;
		}

		EString ManifestPath;
		if (!GetShaderManifestPath(InRequest, ManifestPath))
		{
			return FALSE;
		}
		if (!GetCompileDescriptionJsonPath(InRequest, OutCompileJsonPath))
		{
			return FALSE;
		}

		EString StageStr;
		if (!TranslateShaderStageToString(InRequest.Stage, StageStr))
		{
			return FALSE;
		}

		// Target flags. Dx11 wins when both are set, matching the compiler.
		const BOOL32 bDx11 = ((InRequest.ShaderModel == EShaderModelType::SHADER_MODEL_5_0) ? TRUE : FALSE);
		const BOOL32 bSm66 = ((InRequest.ShaderModel == EShaderModelType::SHADER_MODEL_6_6) ? TRUE : FALSE);

		EString VariantName = InRequest.Variant.Name.ToString();
		if (VariantName.Length() == 0u)
		{
			VariantName = EString("Base");
		}

		EString CompileJson;
		CompileJson += EString("{\n");
		CompileJson += EString("    ") + MakeJsonStringField("RootDir", RootDirectory) + EString(",\n");
		CompileJson += EString("    ") + MakeJsonStringField("JsonFilePath", InRequest.EntryJsonPath) + EString(",\n");
		CompileJson += EString("    ") + MakeJsonStringField("Stage", StageStr) + EString(",\n");
		CompileJson += EString("    ") + MakeJsonStringField("OutputDir", OutputDirectory) + EString(",\n");
		CompileJson += EString("    ") + MakeJsonStringField("ShaderIncludeDir", IncludeDirectory) + EString(",\n");
		CompileJson += EString("    ") + MakeJsonStringField("ManifestFilePath", ManifestPath) + EString(",\n");
		CompileJson += EString("    \"Dx11\": ") + (bDx11 ? EString("true") : EString("false")) + EString(",\n");
		CompileJson += EString("    \"Sm66\": ") + (bSm66 ? EString("true") : EString("false")) + EString(",\n");
		CompileJson += EString("    \"Debug\": ") + (InRequest.bDebug ? EString("true") : EString("false")) + EString(",\n");
		CompileJson += EString("    \"DumpHlsl\": ") + (InRequest.bDumpHlsl ? EString("true") : EString("false")) + EString(",\n");

		CompileJson += EString("    \"Variants\": [\n");
		CompileJson += EString("        {\n");
		CompileJson += EString("            ") + MakeJsonStringField("Name", VariantName) + EString(",\n");
		CompileJson += EString("            \"IsVariant\": ") + (InRequest.Variant.bIsVariant ? EString("true") : EString("false")) + EString(",\n");

		CompileJson += EString("            \"Defines\": [");
		BOOL32 bHasPreviousDefine = FALSE;
		for (UINT32 DefIdx = 0u, DefNum = InRequest.Variant.Defines.Num<UINT32>(); DefIdx < DefNum; DefIdx++)
		{
			const EShaderVariantDefine& Define = InRequest.Variant.Defines[DefIdx];
			if (bHasPreviousDefine)
			{
				CompileJson += EString(",");
			}
			CompileJson += EString("\n                { \"Name\": \"") + EscapeJsonString(Define.Name.ToString())
				+ EString("\", \"Value\": ") + ToString(Define.Value) + EString(" }");
			bHasPreviousDefine = TRUE;
		}
		// The bindless switch is an engine macro, so it travels with the variant and
		// shows up in the manifest: the runtime reads the binding mode from data
		// instead of guessing it from the shader model.
		if ((InRequest.bBindlessPath) && (InRequest.ShaderModel != EShaderModelType::SHADER_MODEL_5_0))
		{
			if (bHasPreviousDefine)
			{
				CompileJson += EString(",");
			}
			CompileJson += EString("\n                { \"Name\": \"") + EString(SHADER_VARIANT_DEFINE_BINDLESS)
				+ EString("\", \"Value\": 1 }");
			bHasPreviousDefine = TRUE;
		}
		CompileJson += (bHasPreviousDefine ? EString("\n            ]") : EString("]"));

		CompileJson += EString(",\n            \"Replaces\": [");
		BOOL32 bHasPreviousReplace = FALSE;
		for (UINT32 RplIdx = 0u, RplNum = InRequest.Variant.Replaces.Num<UINT32>(); RplIdx < RplNum; RplIdx++)
		{
			const EShaderVariantReplace& Replace = InRequest.Variant.Replaces[RplIdx];
			if (bHasPreviousReplace)
			{
				CompileJson += EString(",");
			}
			CompileJson += EString("\n                { \"Name\": \"") + EscapeJsonString(Replace.Name.ToString())
				+ EString("\", \"Element\": \"") + EscapeJsonString(Replace.Element) + EString("\" }");
			bHasPreviousReplace = TRUE;
		}
		CompileJson += (bHasPreviousReplace ? EString("\n            ]") : EString("]"));
		CompileJson += EString("\n        }\n    ]\n}\n");

		if (!EFileHelper::SaveStringToFile(OutCompileJsonPath, CompileJson, TRUE, FALSE))
		{
			PE_LOG_ERROR(EString("[ShaderCompileJob] Cannot write the compile description: ") + OutCompileJsonPath);
			return FALSE;
		}
		return TRUE;
	}

	// ---------------------------------------------------------------------------
	// Compiler invocation
	// ---------------------------------------------------------------------------

	static void LogCompilerLine(const EString& InLine)
	{
		if ((InLine.Find(EString("error")) >= 0) || (InLine.Find(EString("Error")) >= 0))
		{
			PE_LOG_ERROR(InLine);
			return;
		}
		if ((InLine.Find(EString("warning")) >= 0) || (InLine.Find(EString("Warning")) >= 0))
		{
			PE_LOG_WARN(InLine);
			return;
		}
		PE_LOG_LOG(InLine);
	}

	BOOL32 EShaderCompileJob::RunShaderCompiler(const EString& InCompileJsonPath)
	{
		const EString CompilerExe(EBaseSettings::ENGINE_SHADER_COMPILER_EXE);
		if (!EFileHelper::IsFileExists(CompilerExe))
		{
			PE_LOG_ERROR(EString("[ShaderCompileJob] The shader compiler was not found: ") + CompilerExe);
			PE_FAILED((ENGINE_ASSET_ERROR), (PE_TEXT("[ShaderCompileJob] The shader compiler was not found.")));
			return FALSE;
		}

		// Capture stdout and stderr so compiler diagnostics end up in the editor log
		// instead of a console nobody watches.
		HANDLE ReadPipe = nullptr;
		HANDLE WritePipe = nullptr;
		SECURITY_ATTRIBUTES PipeAttributes = {};
		PipeAttributes.nLength = sizeof(PipeAttributes);
		PipeAttributes.bInheritHandle = TRUE;

		const BOOL8 bPipeValid = ::CreatePipe(&ReadPipe, &WritePipe, &PipeAttributes, 0);
		if (bPipeValid)
		{
			::SetHandleInformation(ReadPipe, HANDLE_FLAG_INHERIT, 0);
		}
		else
		{
			PE_LOG_WARN(EString("[ShaderCompileJob] CreatePipe failed, the compiler output will not be captured."));
		}

		STARTUPINFOA StartUpInfo = {};
		PROCESS_INFORMATION ProcessInfo = {};
		StartUpInfo.cb = sizeof(StartUpInfo);
		if (bPipeValid)
		{
			StartUpInfo.hStdOutput = WritePipe;
			StartUpInfo.hStdError = WritePipe;
			StartUpInfo.dwFlags = STARTF_USESTDHANDLES;
		}

		const EString CommandLine = EString("\"") + CompilerExe + EString("\" \"") + InCompileJsonPath + EString("\"");
		const UINT32 CommandLength = CommandLine.Length() + 1u;
		CHAR* CommandBuffer = new CHAR[CommandLength];
		::memcpy_s(CommandBuffer, CommandLength, *CommandLine, CommandLength);

		PE_LOG_LOG(EString("[ShaderCompileJob] Running: ") + CommandLine);

		const BOOL8 bStarted = ::CreateProcessA(
			nullptr,
			CommandBuffer,
			nullptr, nullptr,
			(bPipeValid ? TRUE : FALSE),
			0,
			nullptr, nullptr,
			&StartUpInfo, &ProcessInfo);

		delete[] CommandBuffer;
		CommandBuffer = nullptr;

		if (bPipeValid)
		{
			// The child owns the write end now; closing it here is what makes the read
			// side report end of file when the compiler exits.
			::CloseHandle(WritePipe);
			WritePipe = nullptr;
		}

		if (!bStarted)
		{
			if (bPipeValid)
			{
				::CloseHandle(ReadPipe);
				ReadPipe = nullptr;
			}
			PE_LOG_ERROR(EString("[ShaderCompileJob] CreateProcess failed for: ") + CompilerExe);
			PE_FAILED((ENGINE_ASSET_ERROR), (PE_TEXT("[ShaderCompileJob] CreateProcess failed.")));
			return FALSE;
		}

		if (bPipeValid)
		{
			CHAR ReadBuffer[1024];
			DWORD ReadBytes = 0u;
			EString LineBuffer;
			while ((::ReadFile(ReadPipe, ReadBuffer, sizeof(ReadBuffer) - 1u, &ReadBytes, nullptr)) && (ReadBytes > 0u))
			{
				ReadBuffer[ReadBytes] = '\0';
				for (DWORD CharIdx = 0u; CharIdx < ReadBytes; CharIdx++)
				{
					const CHAR ReadedChar = ReadBuffer[CharIdx];
					if (ReadedChar == '\n')
					{
						if (LineBuffer.Length() > 0u)
						{
							LogCompilerLine(LineBuffer);
						}
						LineBuffer = EString();
					}
					else if (ReadedChar != '\r')
					{
						LineBuffer += EString::FromChar(ReadedChar);
					}
				}
			}
			if (LineBuffer.Length() > 0u)
			{
				LogCompilerLine(LineBuffer);
			}
			::CloseHandle(ReadPipe);
			ReadPipe = nullptr;
		}

		// The read above only returns once the child closed the pipe, so it exited and
		// waiting now cannot deadlock.
		::WaitForSingleObject(ProcessInfo.hProcess, INFINITE);

		DWORD ExitCode = 0u;
		::GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode);
		::CloseHandle(ProcessInfo.hProcess);
		::CloseHandle(ProcessInfo.hThread);

		if (ExitCode != 0u)
		{
			PE_LOG_ERROR(EString("[ShaderCompileJob] The shader compiler exited with code ") + ToString(static_cast<UINT32>(ExitCode)));
			return FALSE;
		}
		return TRUE;
	}

	// What a compile attempt is identified by: which stage source, into which target, for
	// which variant. Two requests that share a variant name are the same variant by
	// contract - the builder rejects an axis the name does not match - so the name is the
	// whole of the variant half.
	static EString MakeShaderCompileKey(const EString& InCompileJsonPath, const EShaderCompileRequest& InRequest)
	{
		EString VariantName = InRequest.Variant.Name.ToString();
		if (VariantName.Length() == 0u)
		{
			VariantName = EString(PE_TEXT("Base"));
		}
		return (InCompileJsonPath + EString(PE_TEXT("|")) + VariantName);
	}

	// The attempts the compiler has already rejected in this process, so that none of them
	// is run twice. Its verdict on one source and one variant does not change while the
	// process lives, so asking again only spawns the compiler once more - and the request
	// that fails is made from the render thread, inside the draw, once per frame for every
	// primitive that wants the shader. An engine that never stops compiling shaders is
	// exactly what that looks like, with the render thread blocked for the length of a
	// compiler run each time.
	//
	// The latch is per process and never written to disk: restarting clears it, so a fixed
	// shader compiles on the next run, and a broken one costs one compiler run per session
	// instead of one per frame. The table only ever grows and holds a handful of entries.
	static TArray<EString>& GetFailedShaderCompiles()
	{
		static TArray<EString> FailedShaderCompiles;
		return FailedShaderCompiles;
	}

	static BOOL32 IsShaderCompileLatched(const EString& InCompileKey)
	{
		return (GetFailedShaderCompiles().Contains(InCompileKey) ? TRUE : FALSE);
	}

	static void LatchShaderCompileFailure(const EString& InCompileKey)
	{
		TArray<EString>& FailedShaderCompiles = GetFailedShaderCompiles();
		if (!FailedShaderCompiles.Contains(InCompileKey))
		{
			FailedShaderCompiles.Add(InCompileKey);
		}
	}

	BOOL32 EShaderCompileJob::CompileShader(const EShaderCompileRequest& InRequest, EString& OutManifestPath)
	{
		OutManifestPath = EString();
		if (InRequest.EntryJsonPath.Length() == 0u)
		{
			PE_LOG_ERROR(EString("[ShaderCompileJob] The compile request carries no entry json."));
			return FALSE;
		}

		EString RootDirectory = InRequest.RootDir;
		if (RootDirectory.Length() == 0u)
		{
			RootDirectory = GetDefaultShaderSourceDirectory();
		}
		const EString EntryJsonFullPath = RootDirectory + InRequest.EntryJsonPath;
		if (!EFileHelper::IsFileExists(EntryJsonFullPath))
		{
			PE_LOG_ERROR(EString("[ShaderCompileJob] The entry json was not found: ") + EntryJsonFullPath);
			PE_FAILED((ENGINE_ASSET_ERROR), (PE_TEXT("[ShaderCompileJob] The entry json was not found.")));
			return FALSE;
		}

		EString CompileJsonPath;
		if (!WriteCompileDescriptionJson(InRequest, CompileJsonPath))
		{
			return FALSE;
		}
		const EString CompileKey = MakeShaderCompileKey(CompileJsonPath, InRequest);
		if (IsShaderCompileLatched(CompileKey))
		{
			// The first attempt logged the compiler's own diagnostics; the verdict is not
			// going to change under this process, so the answer is no without asking again.
			return FALSE;
		}
		if (!RunShaderCompiler(CompileJsonPath))
		{
			LatchShaderCompileFailure(CompileKey);
			// One line for a failure whose reason does not change until the process restarts, and
			// the line says what the failure looks like from outside: a mesh whose material this
			// stage belongs to is drawn with the error marker, while a pass that has no substitute -
			// a compute dispatch, an engine pass - has only this line to show for it.
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderCompileJob] Compilation failed and is not retried until the engine restarts. A mesh whose material this stage belongs to is drawn with the M_EngineError marker; a pass with no substitute has only this line. Compile target: ")) + CompileKey);
			return FALSE;
		}
		if (!GetShaderManifestPath(InRequest, OutManifestPath))
		{
			return FALSE;
		}
		if (!EFileHelper::IsFileExists(OutManifestPath))
		{
			PE_LOG_ERROR(EString("[ShaderCompileJob] The compiler reported success but wrote no manifest: ") + OutManifestPath);
			LatchShaderCompileFailure(CompileKey);
			return FALSE;
		}
		return TRUE;
	}

#endif	// _EDITOR_ONLY

	// ---------------------------------------------------------------------------
	// Entry point
	// ---------------------------------------------------------------------------

	BOOL32 TryLoadShaderAsset(const EShaderCompileRequest& InRequest, const EShaderAsset*& OutAsset)
	{
		OutAsset = nullptr;

		EString OutputDirectory;
		if (!GetShaderOutputDirectory(InRequest, OutputDirectory))
		{
			return FALSE;
		}
		const EString SourceName = GetShaderSourceNameFromEntryPath(InRequest.EntryJsonPath);
		if (SourceName.Length() == 0u)
		{
			PE_LOG_ERROR(EString("[ShaderCompileJob] The compile request carries no entry json name."));
			return FALSE;
		}

#if _EDITOR_ONLY
		// Editor: compile when needed, then import the artifacts into PAssets. Runtime
		// only loads: cooking already happened, the PAssets are shipped.
		if ((InRequest.bForceRecompile) || (!EShaderCompileJob::IsUpToDate(InRequest)))
		{
			EString CompiledManifestPath;
			if (!EShaderCompileJob::CompileShader(InRequest, CompiledManifestPath))
			{
				return FALSE;
			}
			if (!EShaderAssetManager::GetManagerSingleton()->ImportShaderAssetFromManifest(CompiledManifestPath, OutputDirectory, SourceName))
			{
				return FALSE;
			}
		}
#endif	// _EDITOR_ONLY

		return EShaderAssetManager::GetManagerSingleton()->LoadShaderAsset(OutputDirectory, SourceName, OutAsset, InRequest.bForceRecompile);
	}

} // namespace PigeonEngine
