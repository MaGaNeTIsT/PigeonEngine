#include "MaterialAsset.h"
#include <RapidJson/Headers/JsonObject.h>
#include <IO/FileHelper.h>
#if _EDITOR_ONLY
#include <RenderCommon.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Editor/EditorLogManager.h>
#endif

namespace PigeonEngine
{
    EMaterialAsset::EMaterialAsset(const EString& InName)
        : Name(InName)
    {
    }
    EMaterialAsset::~EMaterialAsset()
    {
    }
    const EString& EMaterialAsset::GetName() const
    {
        return Name;
    }
    void EMaterialAsset::AddVariant(EMaterialVariant&& V)
    {
        Variants.Add(std::move(V));
    }
    const EMaterialVariant* EMaterialAsset::GetFirstVariant() const
    {
        return Variants.Num<UINT32>() > 0u ? &Variants[0] : nullptr;
    }
    const EMaterialVariant* EMaterialAsset::FindVariant(const EString& PassName, UINT32 VariantIndex) const
    {
        for (UINT32 VarIdx = 0u, NumVars = Variants.Num<UINT32>(); VarIdx < NumVars; VarIdx++)
        {
            const EMaterialVariant& Var = Variants[VarIdx];
            if ((Var.PassName == PassName) && (Var.VariantIndex == VariantIndex))
            {
                return (&Var);
            }
        }
        return nullptr;
    }

#if _EDITOR_ONLY
    // Translate HLSL semantic name string to engine enum value.
    static RShaderSemanticType SemanticTypeFromName(const EString& Name)
    {
        if (Name == EString("POSITION"))     return RShaderSemanticType::SHADER_SEMANTIC_POSITION;
        if (Name == EString("TEXCOORD"))     return RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD;
        if (Name == EString("NORMAL"))       return RShaderSemanticType::SHADER_SEMANTIC_NORMAL;
        if (Name == EString("TANGENT"))      return RShaderSemanticType::SHADER_SEMANTIC_TANGENT;
        if (Name == EString("COLOR"))        return RShaderSemanticType::SHADER_SEMANTIC_COLOR;
        if (Name == EString("BINORMAL"))     return RShaderSemanticType::SHADER_SEMANTIC_BINORMAL;
        if (Name == EString("BLENDWEIGHT"))  return RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT;
        if (Name == EString("BLENDINDICES")) return RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES;
        return RShaderSemanticType::SHADER_SEMANTIC_NONE;
    }
#endif

    EMaterialAssetManager::EMaterialAssetManager()
    {
    }

    EMaterialAssetManager::~EMaterialAssetManager()
    {
    }

    void EMaterialAssetManager::Initialize()
    {
    }

    void EMaterialAssetManager::ShutDown()
    {
        MaterialManager.Clear();
    }

    BOOL32 EMaterialAssetManager::ParseReflection(const EString& ReflJsonPath, EMaterialReflection& Out)
    {
        EString JsonStr;
        if (!EFileHelper::ReadFileAsString(ReflJsonPath, JsonStr))
        {
            PE_FAILED((ENGINE_ASSET_ERROR), ("MaterialAssetManager: cannot read refl json"));
            return FALSE;
        }

        CJsonObject Json(JsonStr);

        // Constant buffers
        TArray<CJsonObject*> CbArr;
        if (Json.GetObjectArrayField("constant_buffers", CbArr))
        {
            for (CJsonObject* Cb : CbArr)
            {
                EMaterialCBRefl CbRefl;
                Cb->GetStringField("name", CbRefl.Name);
                Cb->GetUIntField("slot", CbRefl.Slot);
                Cb->GetUIntField("size_bytes", CbRefl.SizeBytes);

                TArray<CJsonObject*> Fields;
                if (Cb->GetObjectArrayField("fields", Fields))
                {
                    for (CJsonObject* F : Fields)
                    {
                        EMaterialCBField Field;
                        F->GetStringField("name", Field.Name);
                        F->GetStringField("type", Field.Type);
                        F->GetUIntField("offset", Field.Offset);
                        F->GetUIntField("size", Field.Size);
                        CbRefl.Fields.Add(std::move(Field));
                        delete F;
                    }
                }
                Out.ConstantBuffers.Add(std::move(CbRefl));
                delete Cb;
            }
        }

        // SRVs
        TArray<CJsonObject*> SrvArr;
        if (Json.GetObjectArrayField("srvs", SrvArr))
        {
            for (CJsonObject* S : SrvArr)
            {
                EMaterialSRVRefl Srv;
                S->GetStringField("name", Srv.Name);
                S->GetUIntField("slot", Srv.Slot);
                Out.SRVs.Add(std::move(Srv));
                delete S;
            }
        }

        // Samplers
        TArray<CJsonObject*> SampArr;
        if (Json.GetObjectArrayField("samplers", SampArr))
        {
            for (CJsonObject* S : SampArr)
            {
                EMaterialSamplerRefl Samp;
                S->GetStringField("name", Samp.Name);
                S->GetUIntField("slot", Samp.Slot);
                Out.Samplers.Add(std::move(Samp));
                delete S;
            }
        }

        // Input layout
        TArray<CJsonObject*> IlArr;
        if (Json.GetObjectArrayField("input_layout", IlArr))
        {
            for (CJsonObject* Il : IlArr)
            {
                EMaterialInputLayoutRefl Entry;
                Il->GetStringField("semantic", Entry.Semantic);
                Il->GetUIntField("index", Entry.Index);
                Il->GetUIntField("components", Entry.Components);
                Out.InputLayout.Add(std::move(Entry));
                delete Il;
            }
        }

        return TRUE;
    }

    BOOL32 EMaterialAssetManager::LoadMaterialAsset(
        const EString& InLoadPath,
        const EString& InLoadName,
        const EMaterialAsset*& OutMaterial)
    {
        // Return cached asset if already loaded
        EMaterialAsset* Existing = MaterialManager.Find(InLoadName);
        if (Existing)
        {
#if _EDITOR_ONLY
            PE_LOG_LOG(EString("[MaterialAsset] '") + InLoadName + "' already loaded from cache, skipping disk load.");
#endif
            OutMaterial = Existing;
            return TRUE;
        }

#if _EDITOR_ONLY
        PE_LOG_LOG(EString("[MaterialAsset] Loading material '") + InLoadName + "' from: " + InLoadPath);
#endif

        // Read manifest: <path>/<name>.mat.json
        EString ManifestPath = InLoadPath + InLoadName + ".mat.json";
        EString ManifestStr;
        if (!EFileHelper::ReadFileAsString(ManifestPath, ManifestStr))
        {
#if _EDITOR_ONLY
            PE_LOG_ERROR(EString("[MaterialAsset] Cannot read manifest: ") + ManifestPath);
#endif
            PE_FAILED((ENGINE_ASSET_ERROR), ("MaterialAssetManager: cannot read manifest"));
            return FALSE;
        }
#if _EDITOR_ONLY
        PE_LOG_LOG(EString("[MaterialAsset] Manifest found: ") + ManifestPath);
#endif

        CJsonObject Manifest(ManifestStr);

        TArray<CJsonObject*> Passes;
        if (!Manifest.GetObjectArrayField("passes", Passes))
        {
#if _EDITOR_ONLY
            PE_LOG_ERROR(EString("[MaterialAsset] Manifest has no 'passes' field: ") + ManifestPath);
#endif
            PE_FAILED((ENGINE_ASSET_ERROR), ("MaterialAssetManager: no passes in manifest"));
            return FALSE;
        }

        EMaterialAsset* Asset = new EMaterialAsset(InLoadName);

        for (CJsonObject* PassObj : Passes)
        {
            EString PassName;
            PassObj->GetStringField("pass", PassName);
#if _EDITOR_ONLY
            PE_LOG_LOG(EString("[MaterialAsset] Loading pass: '") + PassName + "'");
#endif

            TArray<CJsonObject*> Variants;
            if (!PassObj->GetObjectArrayField("variants", Variants))
            {
#if _EDITOR_ONLY
                PE_LOG_WARN(EString("[MaterialAsset] Pass '") + PassName + "' has no variants, skipping.");
#endif
                delete PassObj; continue;
            }

            for (CJsonObject* VarObj : Variants)
            {
                UINT32  VarIdx = 0u;
                EString VarName, VsFile, PsFile, ReflFile;
                VarObj->GetUIntField("index", VarIdx);
                VarObj->GetStringField("name", VarName);
                VarObj->GetStringField("vs", VsFile);
                VarObj->GetStringField("ps", PsFile);
                VarObj->GetStringField("refl", ReflFile);
#if _EDITOR_ONLY
                PE_LOG_LOG(EString("[MaterialAsset]   Variant[") + ToString(VarIdx) + "] '" + VarName + "' VS='" + VsFile + "' PS='" + PsFile + "' Refl='" + ReflFile + "'");
#endif

                EMaterialVariant Variant;
                Variant.PassName = PassName;
                Variant.VariantIndex = VarIdx;
                Variant.VariantName = VarName;

                // Parse reflection first — needed for input layout when importing VS.
                EString ReflPath = InLoadPath + ReflFile;
                ParseReflection(ReflPath, Variant.Reflection);

                // Derive asset names from file names (strip extension).
                EString VsName = VsFile;
                EString PsName = PsFile;
                {
                    INT32 DotPos = VsName.RightFind(".");
                    if (DotPos >= 0) VsName = VsName.Left(static_cast<UINT32>(DotPos));
                }
                {
                    INT32 DotPos = PsName.RightFind(".");
                    if (DotPos >= 0) PsName = PsName.Left(static_cast<UINT32>(DotPos));
                }

#if _EDITOR_ONLY
                // Auto-import VS from raw bytecode when the .PAsset doesn't exist yet.
                if (VsFile.Length() > 0u)
                {
                    EString VsAssetPath = InLoadPath + VsName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
                    if (!EFileHelper::IsFileExists(VsAssetPath))
                    {
                        void* VsBytes = nullptr; ULONG VsBytesSize = 0u;
                        EString RawVsPath = InLoadPath + VsFile;
                        if (EFileHelper::ReadFileAsBinary(RawVsPath, VsBytes, VsBytesSize))
                        {
                            // Translate reflection input layout to RInputLayoutDesc.
                            TArray<RInputLayoutDesc> Layouts;
                            for (const EMaterialInputLayoutRefl& ILRefl : Variant.Reflection.InputLayout)
                            {
                                RInputLayoutDesc Desc;
                                Desc.SemanticName = SemanticTypeFromName(ILRefl.Semantic);
                                Desc.SemanticIndex = ILRefl.Index;
                                Desc.MemberNum = ILRefl.Components;
                                Desc.MemberFormat = RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT;
                                Desc.MemberStride = static_cast<UINT32>(sizeof(FLOAT));
                                Layouts.Add(Desc);
                            }
                            const UINT32 LayoutNum = Layouts.Num<UINT32>();
                            EShaderAssetManager::GetManagerSingleton()->ImportVertexShaderFromBytes(
                                VsName, InLoadPath, VsBytes, VsBytesSize, Layouts.GetData(), LayoutNum);
                            delete[] VsBytes;
                        }
                    }
                }

                // Auto-import PS from raw bytecode when the .PAsset doesn't exist yet.
                if (PsFile.Length() > 0u)
                {
                    EString PsAssetPath = InLoadPath + PsName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
                    if (!EFileHelper::IsFileExists(PsAssetPath))
                    {
                        void* PsBytes = nullptr; ULONG PsBytesSize = 0u;
                        EString RawPsPath = InLoadPath + PsFile;
                        if (EFileHelper::ReadFileAsBinary(RawPsPath, PsBytes, PsBytesSize))
                        {
                            EShaderAssetManager::GetManagerSingleton()->ImportPixelShaderFromBytes(
                                PsName, InLoadPath, PsBytes, PsBytesSize);
                            delete[] PsBytes;
                        }
                    }
                }
#endif

                EShaderAssetManager::GetManagerSingleton()->LoadVertexShaderAsset(InLoadPath, VsName, Variant.VS);
                EShaderAssetManager::GetManagerSingleton()->LoadPixelShaderAsset(InLoadPath, PsName, Variant.PS);
#if _EDITOR_ONLY
                if (Variant.VS) { PE_LOG_LOG(EString("[MaterialAsset]     VS loaded OK: '") + VsName + "'"); }
                else            { PE_LOG_ERROR(EString("[MaterialAsset]     VS load FAILED: '") + VsName + "'"); }
                if (Variant.PS) { PE_LOG_LOG(EString("[MaterialAsset]     PS loaded OK: '") + PsName + "'"); }
                else            { PE_LOG_ERROR(EString("[MaterialAsset]     PS load FAILED: '") + PsName + "'"); }
#endif

                Asset->AddVariant(std::move(Variant));
                delete VarObj;
            }

            delete PassObj;
        }

        MaterialManager.Add(InLoadName, Asset);
        OutMaterial = Asset;
#if _EDITOR_ONLY
        PE_LOG_LOG(EString("[MaterialAsset] Material '") + InLoadName + "' loaded successfully.");
#endif
        return TRUE;
    }

#if _EDITOR_ONLY
    BOOL32 EMaterialAssetManager::LoadOrCompileMaterialAsset(
        const EString& InOutputDir,
        const EString& InLoadName,
        const EString& InSourceDir,
        const EString& InShaderIncludeDir,
        const EMaterialAsset*& OutMaterial)
    {
        // Output path for this material's manifest: <OutputDir>/<Name>/<Name>.mat.json
        EString MatOutputDir = InOutputDir + InLoadName + "/";
        EString ManifestPath = MatOutputDir + InLoadName + ".mat.json";

        PE_LOG_LOG(EString("[MaterialCompiler] LoadOrCompile '") + InLoadName + "'");
        PE_LOG_LOG(EString("[MaterialCompiler]   OutputDir  : ") + MatOutputDir);
        PE_LOG_LOG(EString("[MaterialCompiler]   SourceDir  : ") + InSourceDir);
        PE_LOG_LOG(EString("[MaterialCompiler]   IncludeDir : ") + InShaderIncludeDir);
        PE_LOG_LOG(EString("[MaterialCompiler]   Manifest   : ") + ManifestPath);

        // Also check that at least the first compiled refl file exists alongside the manifest.
        // If the manifest was created but compilation failed (shader files missing), force recompile.
        BOOL32 bManifestExists = EFileHelper::IsFileExists(ManifestPath);
        if (bManifestExists)
        {
            PE_LOG_LOG(EString("[MaterialCompiler] Manifest exists, checking shader files..."));
            EString ManifestStr;
            if (EFileHelper::ReadFileAsString(ManifestPath, ManifestStr))
            {
                CJsonObject Manifest(ManifestStr);
                TArray<CJsonObject*> Passes;
                if (Manifest.GetObjectArrayField("passes", Passes) && Passes.Num() > 0)
                {
                    TArray<CJsonObject*> Variants;
                    if (Passes[0]->GetObjectArrayField("variants", Variants) && Variants.Num() > 0)
                    {
                        EString ReflFile;
                        Variants[0]->GetStringField("refl", ReflFile);
                        if (ReflFile.Length() > 0 && !EFileHelper::IsFileExists(MatOutputDir + ReflFile))
                        {
                            PE_LOG_WARN(EString("[MaterialCompiler] Refl file missing (") + ReflFile + "), forcing recompile.");
                            bManifestExists = FALSE; // shader files missing, force recompile
                        }
                        else
                        {
                            PE_LOG_LOG(EString("[MaterialCompiler] Shader files OK (refl: ") + ReflFile + "), skipping compile.");
                        }
                        for (INT32 i = 0; i < Variants.Num<INT32>(); i++) delete Variants[i];
                    }
                    for (INT32 i = 0; i < Passes.Num<INT32>(); i++) delete Passes[i];
                }
            }
        }
        else
        {
            PE_LOG_LOG(EString("[MaterialCompiler] Manifest not found, will compile."));
        }

        if (!bManifestExists)
        {
            // Pre-check: material source sub-directory must exist
            EString MatSourceDir = InSourceDir + InLoadName + "/";
            if (!EFileHelper::IsDirectoryExists(MatSourceDir))
            {
                PE_LOG_ERROR(EString("[MaterialCompiler] Source directory not found: ") + MatSourceDir);
                PE_FAILED((ENGINE_ASSET_ERROR), ("LoadOrCompileMaterialAsset: material source directory not found"));
                return FALSE;
            }
            PE_LOG_LOG(EString("[MaterialCompiler] Source directory OK: ") + MatSourceDir);

            // Pre-check: compiler exe must exist
            const EString CompilerExe(EBaseSettings::ENGINE_MATERIAL_COMPILER_EXE);
            if (!EFileHelper::IsFileExists(CompilerExe))
            {
                PE_LOG_ERROR(EString("[MaterialCompiler] MaterialCompiler.exe not found at: ") + CompilerExe);
                PE_FAILED((ENGINE_ASSET_ERROR), ("LoadOrCompileMaterialAsset: MaterialCompiler.exe not found"));
                return FALSE;
            }
            PE_LOG_LOG(EString("[MaterialCompiler] Compiler exe found: ") + CompilerExe);

            // Build command: MaterialCompiler.exe <SourceDir> <OutputDir> --shader-include <IncludeDir> --material <Name> --dx11
            // <SourceDir> must be the parent directory that contains per-material subdirectories,
            // e.g. "Assets/MaterialSources/", NOT "Assets/MaterialSources/M_Robot".
            EString CmdLine =
                EString("\"") + CompilerExe + "\" " +
                "\"" + *InSourceDir + "\" " +
                "\"" + *InOutputDir + "\" " +
                "--shader-include \"" + *InShaderIncludeDir + "\" " +
                "--material \"" + InLoadName + "\" " +
                "--dx11";

            PE_LOG_LOG(EString("[MaterialCompiler] Running: ") + CmdLine);

            // Create a pipe to capture stdout + stderr from MaterialCompiler
            HANDLE hReadPipe  = nullptr;
            HANDLE hWritePipe = nullptr;
            SECURITY_ATTRIBUTES PipeAttr = {};
            PipeAttr.nLength        = sizeof(PipeAttr);
            PipeAttr.bInheritHandle = TRUE; // write end must be inheritable by child

            BOOL bPipeOk = ::CreatePipe(&hReadPipe, &hWritePipe, &PipeAttr, 0);
            if (bPipeOk)
            {
                // Read end must NOT be inherited so the child doesn't hold it open
                ::SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);
            }
            else
            {
                PE_LOG_WARN(EString("[MaterialCompiler] CreatePipe failed, output will not be captured."));
            }

            STARTUPINFOA        StartUpInfo = {};
            PROCESS_INFORMATION ProcessInfo = {};
            StartUpInfo.cb = sizeof(StartUpInfo);
            if (bPipeOk)
            {
                StartUpInfo.hStdOutput = hWritePipe;
                StartUpInfo.hStdError  = hWritePipe;
                StartUpInfo.dwFlags    = STARTF_USESTDHANDLES;
            }

            // CreateProcessA needs a mutable buffer for the command line
            const UINT32 CmdLen = CmdLine.Length() + 1u;
            CHAR* CmdBuf = new CHAR[CmdLen];
            ::memcpy_s(CmdBuf, CmdLen, *CmdLine, CmdLen);

            BOOL bOk = ::CreateProcessA(
                nullptr,    // lpApplicationName — let Windows parse from CmdBuf
                CmdBuf,
                nullptr, nullptr,
                bPipeOk ? TRUE : FALSE, // bInheritHandles: only needed when using pipes
                0,
                nullptr, nullptr,
                &StartUpInfo, &ProcessInfo);

            delete[] CmdBuf;

            // Close write end in parent immediately — child owns it now.
            // This is required so ReadFile on the read end returns EOF when the child exits.
            if (bPipeOk) { ::CloseHandle(hWritePipe); hWritePipe = nullptr; }

            if (!bOk)
            {
                DWORD WinErr = ::GetLastError();
                if (bPipeOk) { ::CloseHandle(hReadPipe); }
                PE_LOG_ERROR(EString("[MaterialCompiler] CreateProcess FAILED. WinError=") + ToString(static_cast<UINT32>(WinErr)));
                PE_FAILED((ENGINE_ASSET_ERROR), ("LoadOrCompileMaterialAsset: CreateProcess failed for MaterialCompiler"));
                return FALSE;
            }

            PE_LOG_LOG(EString("[MaterialCompiler] Process launched (PID=") + ToString(static_cast<UINT32>(ProcessInfo.dwProcessId)) + "), waiting...");

            // Drain the pipe while the child runs.
            // ReadFile blocks until data arrives and returns FALSE when the child
            // closes its end of the pipe (i.e. on exit) — which naturally serves
            // as our wait. Doing WaitForSingleObject first would deadlock if the
            // child's output fills the pipe buffer.
            if (bPipeOk)
            {
                CHAR   ReadBuf[1024];
                DWORD  BytesRead = 0;
                EString LineBuf;
                while (::ReadFile(hReadPipe, ReadBuf, sizeof(ReadBuf) - 1u, &BytesRead, nullptr) && BytesRead > 0u)
                {
                    ReadBuf[BytesRead] = '\0';
                    for (DWORD CharIdx = 0u; CharIdx < BytesRead; ++CharIdx)
                    {
                        CHAR ReadedChar = ReadBuf[CharIdx];
                        if (ReadedChar == '\n')
                        {
                            if (LineBuf.Length() > 0u)
                            {
                                if (LineBuf.Find("error") >= 0 || LineBuf.Find("Error") >= 0 || LineBuf.Find("ERROR") >= 0)
                                {
                                    PE_LOG_ERROR(LineBuf);
                                }
                                else if (LineBuf.Find("warning") >= 0 || LineBuf.Find("Warning") >= 0 || LineBuf.Find("WARNING") >= 0)
                                {
                                    PE_LOG_WARN(LineBuf);
                                }
                                else
                                {
                                    PE_LOG_LOG(LineBuf);
                                }
                            }
                            LineBuf = EString();
                        }
                        else if (ReadedChar != '\r')
                        {
                            LineBuf += EString::FromChar(ReadedChar);
                        }
                    }
                }
                // Flush any remaining partial line
                if (LineBuf.Length() > 0u)
                    PE_LOG_LOG(LineBuf);

                ::CloseHandle(hReadPipe);
            }

            // Child has closed the pipe (exited); wait just to safely retrieve exit code.
            ::WaitForSingleObject(ProcessInfo.hProcess, INFINITE);

            DWORD ExitCode = 0;
            ::GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode);
            ::CloseHandle(ProcessInfo.hProcess);
            ::CloseHandle(ProcessInfo.hThread);

            if (ExitCode != 0)
            {
                PE_LOG_ERROR(EString("[MaterialCompiler] MaterialCompiler.exe exited with code ") + ToString(static_cast<UINT32>(ExitCode)));
                PE_FAILED((ENGINE_ASSET_ERROR), ("LoadOrCompileMaterialAsset: MaterialCompiler exited with error"));
                return FALSE;
            }

            PE_LOG_LOG(EString("[MaterialCompiler] Compilation succeeded (exit code 0)."));
        }

        PE_LOG_LOG(EString("[MaterialCompiler] Proceeding to load compiled asset from: ") + MatOutputDir);
        return LoadMaterialAsset(MatOutputDir, InLoadName, OutMaterial);
    }
#endif

} // namespace PigeonEngine
