#include "MCJsonParser.h"
#include "MCTemplateAssembler.h"
#include "MCDxcCompiler.h"
#include "MCReflectionWriter.h"
#include "MCFileWatcher.h"
#include "MCLogger.h"

#include <Config/EngineShaderNaming.h>

#include <document.h>
#include <prettywriter.h>
#include <stringbuffer.h>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <chrono>

namespace fs = std::filesystem;
using namespace rapidjson;

// ---------------------------------------------------------------------------
// CLI args
// ---------------------------------------------------------------------------
struct MCArgs
{
    std::string SourceDir;
    std::string OutputDir;
    std::string ShaderIncludeDir;
    std::string FilterMaterial;
    std::string LogPath = "MaterialCompiler.log";
    bool        Debug = false;
    bool        Watch = false;
    bool        Dx11  = false;
    bool        DumpHlsl = false;
};

static void PrintUsage()
{
    std::cout <<
        "Usage: MaterialCompiler.exe\n"
        "  --shader-include <dir>   Engine shader include root\n"
        "  [--debug]                Emit debug symbols (PDB/DXIL)\n"
        "  [--dump-hlsl]            Dump assembled HLSL to .hlsl files\n"
        "  [--watch]                Stay alive, recompile on JSON change\n"
        "  [--material <name>]      Filter to one material\n"
        "  [--dx11]                 Target DX11 (DXBC/SM5.0); default is DX12 (DXIL/SM6.0)\n"
        "  [--log <path>]           Log file path (default: MaterialCompiler.log)\n"
        "  <source_dir>             Assets/MaterialSources/\n"
        "  <output_dir>             Build/Temp/MaterialAssets/\n";
}

static bool ParseArgs(int argc, char** argv, MCArgs& Out)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a == "--shader-include" && i + 1 < argc) { Out.ShaderIncludeDir = argv[++i]; }
        else if (a == "--debug")    { Out.Debug = true; }
        else if (a == "--watch")    { Out.Watch = true; }
        else if (a == "--dx11")     { Out.Dx11  = true; }
        else if (a == "--dump-hlsl") { Out.DumpHlsl = true; }
        else if (a == "--log" && i + 1 < argc) { Out.LogPath = argv[++i]; }
        else if (a == "--material" && i + 1 < argc) { Out.FilterMaterial = argv[++i]; }
        else if (Out.SourceDir.empty()) { Out.SourceDir = a; }
        else if (Out.OutputDir.empty()) { Out.OutputDir = a; }
    }
    if (Out.SourceDir.empty() || Out.OutputDir.empty() || Out.ShaderIncludeDir.empty())
    {
        PrintUsage(); return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Incremental cache  (rapidjson-based)
// ---------------------------------------------------------------------------
static std::string CachePath(const std::string& OutputDir)
{
    return OutputDir + "/.mc_cache.json";
}

static Document LoadCache(const std::string& OutputDir)
{
    Document d;
    d.SetObject();
    std::ifstream f(CachePath(OutputDir));
    if (!f.is_open()) return d;
    std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    d.Parse(s.c_str());
    if (d.HasParseError()) d.SetObject();
    return d;
}

static void SaveCache(const std::string& OutputDir, const Document& Cache)
{
    StringBuffer sb;
    PrettyWriter<StringBuffer> w(sb);
    w.SetIndent(' ', 2);
    Cache.Accept(w);
    std::ofstream f(CachePath(OutputDir));
    if (f.is_open()) f << sb.GetString();
}

static int64_t FileMtime(const std::string& Path)
{
    std::error_code ec;
    auto t = fs::last_write_time(Path, ec);
    if (ec) return 0;
    return t.time_since_epoch().count();
}

static std::string VariantKey(const std::string& MatName, const std::string& PassName, int VariantIdx)
{
    std::ostringstream ss;
    ss << MatName << "::" << PassName << "::" << std::setw(4) << std::setfill('0') << VariantIdx;
    return ss.str();
}

static bool IsUpToDate(const Document& Cache, const std::string& Key,
    const std::vector<std::string>& InputFiles,
    const std::vector<std::string>& OutputFiles)
{
    // Output files must all exist
    for (auto& f : OutputFiles)
    {
        if (!fs::exists(f)) return false;
    }
    if (!Cache.HasMember(Key.c_str())) return false;
    const Value& entry = Cache[Key.c_str()];
    for (auto& f : InputFiles)
    {
        if (!entry.HasMember(f.c_str())) return false;
        if (entry[f.c_str()].GetInt64() != FileMtime(f)) return false;
    }
    return true;
}

static void UpdateCache(Document& Cache, const std::string& Key,
    const std::vector<std::string>& InputFiles)
{
    auto& alloc = Cache.GetAllocator();
    Value entry(kObjectType);
    for (auto& f : InputFiles)
        entry.AddMember(Value(f.c_str(), alloc), Value(FileMtime(f)), alloc);
    Value k(Key.c_str(), alloc);
    if (Cache.HasMember(Key.c_str()))
        Cache[Key.c_str()] = entry;
    else
        Cache.AddMember(k, entry, alloc);
}

// ---------------------------------------------------------------------------
// Write DXIL to disk
// ---------------------------------------------------------------------------
static bool WriteDxil(const std::string& Path, const std::vector<uint8_t>& Bytes, std::string& Err)
{
    std::ofstream f(Path, std::ios::binary);
    if (!f.is_open()) { Err = "Cannot write: " + Path; return false; }
    f.write(reinterpret_cast<const char*>(Bytes.data()), Bytes.size());
    return true;
}

// ---------------------------------------------------------------------------
// Derive engine root from --shader-include path
// Walks up from ShaderIncludeDir until it finds a directory that directly
// contains a subdirectory named "Engine".
// ---------------------------------------------------------------------------
static std::string DeriveEngineRoot(const std::string& ShaderIncludeDir)
{
    fs::path p = fs::absolute(ShaderIncludeDir);
    if (p.filename().empty()) p = p.parent_path();
    while (p.has_parent_path())
    {
        if (fs::is_directory(p / "Engine")) return p.string() + "/";
        fs::path up = p.parent_path();
        if (up == p) break;
        p = up;
    }
    return {};
}

// ---------------------------------------------------------------------------
// Compile one material (now using unified ShaderPass header)
// ---------------------------------------------------------------------------
static bool CompileMaterial(
    const std::string& PassJsonPath,
    const std::string& MatOutputDir,
    const MCArgs&      Args,
    const std::string& EngineRoot,
    MC::MCDxcCompiler& Dxc,
    Document&          Cache)
{
    auto& log = MC::MCLogger::Get();
    std::string err;

    log.Stage("ParseShaderPass", PassJsonPath);
    MC::MCShaderPass pass;
    if (!MC::ParseShaderPass(PassJsonPath, pass, err))
    { log.Error("Pass parse failed: " + err); return false; }

    std::string passDir = PassJsonPath.substr(0, PassJsonPath.find_last_of("/\\") + 1);

    // Resolve paths relative to pass json
    auto ResolvePath = [&](const std::string& p) -> std::string {
        if (p.empty() || fs::path(p).is_absolute()) return p;
        return passDir + p;
    };

    std::string vfPath = ResolvePath(pass.VertexFactoryPath);
    std::string matPath = ResolvePath(pass.MaterialPath);

    MC::MCVertexFactory vf;
    MC::MCMaterial mat;

    if (!vfPath.empty())
    {
        log.Stage("ParseVertexFactory", vfPath);
        if (!MC::ParseVertexFactory(vfPath, vf, err))
        { log.Error("VF parse failed: " + err); return false; }
    }

    log.Stage("ParseMaterial", matPath);
    if (!MC::ParseMaterial(matPath, mat, err))
    { log.Error("Material parse failed: " + err); return false; }

    fs::create_directories(MatOutputDir);

    const std::string kVSProfile = Args.Dx11 ? "vs_5_0" : "vs_6_0";
    const std::string kPSProfile = Args.Dx11 ? "ps_5_0" : "ps_6_0";
    const std::string kCSProfile = Args.Dx11 ? "cs_5_0" : "cs_6_0";
    const std::string kShaderExt = Args.Dx11 ? ".dxbc" : ".dxil";
    const std::string kShaderModel = Args.Dx11 ? "5_0" : "6_0";
    const std::string kVSSuffix = MC_SHADER_VS_SUFFIX;
    const std::string kPSSuffix = MC_SHADER_PS_SUFFIX;
    const std::string kCSSuffix = MC_SHADER_CS_SUFFIX;

    Document manifest;
    manifest.SetObject();
    auto& alloc = manifest.GetAllocator();
    manifest.AddMember("name", Value(mat.Name.c_str(), alloc), alloc);
    manifest.AddMember("shader_model", Value(kShaderModel.c_str(), alloc), alloc);
    manifest.AddMember("passes", Value(kArrayType), alloc);

    Value passEntry(kObjectType);
    passEntry.AddMember("pass", Value(pass.Name.c_str(), alloc), alloc);
    passEntry.AddMember("variants", Value(kArrayType), alloc);

    for (int vi = 0; vi < static_cast<int>(pass.Variants.size()); ++vi)
    {
        auto& variant = pass.Variants[vi];
        std::ostringstream idxStream;
        idxStream << std::setw(4) << std::setfill('0') << vi;
        std::string idxStr = idxStream.str();

        auto ResolveTemplate = [&](const std::string& p) -> std::string {
            if (p.empty() || fs::path(p).is_absolute()) return p;
            return EngineRoot + p;
        };

        std::string tmplVSPath = ResolveTemplate(pass.TemplateVS);
        std::string tmplPSPath = ResolveTemplate(pass.TemplatePS);
        std::string tmplCSPath = ResolveTemplate(pass.TemplateCS);

        std::vector<std::string> inputs = { matPath, PassJsonPath };
        if (!tmplVSPath.empty()) inputs.push_back(tmplVSPath);
        if (!tmplPSPath.empty()) inputs.push_back(tmplPSPath);
        if (!tmplCSPath.empty()) inputs.push_back(tmplCSPath);
        if (!vfPath.empty()) inputs.push_back(vfPath);

        std::string cacheKey = VariantKey(mat.Name, pass.Name, vi);
        std::vector<std::string> outputs;
        if (!pass.TemplateVS.empty()) outputs.push_back(MatOutputDir + "/" + pass.Name + "_" + idxStr + kVSSuffix + kShaderExt);
        if (!pass.TemplatePS.empty()) outputs.push_back(MatOutputDir + "/" + pass.Name + "_" + idxStr + kPSSuffix + kShaderExt);
        if (!pass.TemplateCS.empty()) outputs.push_back(MatOutputDir + "/" + pass.Name + "_" + idxStr + kCSSuffix + kShaderExt);
        outputs.push_back(MatOutputDir + "/" + pass.Name + "_" + idxStr + ".refl.json");

        if (IsUpToDate(Cache, cacheKey, inputs, outputs))
        {
            log.Info("Up-to-date: " + mat.Name + " " + pass.Name + " variant " + std::to_string(vi));
        }
        else
        {
            log.Stage("CompileVariant", mat.Name + " " + pass.Name + " #" + std::to_string(vi) + " (" + variant.Name + ")");

            bool ok = true;

            if (!pass.TemplateVS.empty())
            {
                log.Stage("AssembleVS", pass.Name + "_" + idxStr);
                MC::MCTemplateAssembler asmVS;
                if (!asmVS.LoadTemplate(tmplVSPath, err))
                { log.Error(err); ok = false; }
                else
                {
                    MC::MCAssembledShader assembled;
                    if (Args.DumpHlsl)
                        assembled.DumpPath = MatOutputDir + "/" + pass.Name + "_" + idxStr + kVSSuffix + ".hlsl";
                    if (!asmVS.Assemble(vf, mat, pass, variant, assembled, err, Args.DumpHlsl))
                    { log.Error("Assemble VS: " + err); ok = false; }
                    else
                    {
                        log.Stage("CompileVS", pass.Name + "_" + idxStr);
                        MC::MCDxcResult result;
                        std::string tmplVSDir = fs::path(tmplVSPath).parent_path().string();
                        Dxc.Compile(assembled.HlslSource, pass.Name + "_VS", kVSProfile,
                            Args.ShaderIncludeDir, tmplVSDir, passDir, Args.Debug, Args.Dx11, result);
                        if (!result.Errors.empty())
                            log.Error("VS compile:\n" + result.Errors);
                        if (result.Success)
                        {
                            std::string outPath = MatOutputDir + "/" + pass.Name + "_" + idxStr + kVSSuffix + kShaderExt;
                            WriteDxil(outPath, result.DxilBytes, err);
                            if (Args.Debug && !result.PdbBytes.empty())
                                WriteDxil(MatOutputDir + "/" + pass.Name + "_" + idxStr + kVSSuffix + ".pdb", result.PdbBytes, err);
                        }
                        else ok = false;
                    }
                }
            }

                if (ok && !pass.TemplatePS.empty())
                {
                    log.Stage("AssemblePS", pass.Name + "_" + idxStr);
                    MC::MCTemplateAssembler asmPS;
                    if (!asmPS.LoadTemplate(tmplPSPath, err))
                    { log.Error(err); ok = false; }
                    else
                    {
                        MC::MCAssembledShader assembled;
                        if (Args.DumpHlsl)
                            assembled.DumpPath = MatOutputDir + "/" + pass.Name + "_" + idxStr + kPSSuffix + ".hlsl";
                        if (!asmPS.Assemble(vf, mat, pass, variant, assembled, err, Args.DumpHlsl))
                        { log.Error("Assemble PS: " + err); ok = false; }
                        else
                        {
                            log.Stage("CompilePS", pass.Name + "_" + idxStr);
                            MC::MCDxcResult result;
                            std::string tmplPSDir = fs::path(tmplPSPath).parent_path().string();
                            Dxc.Compile(assembled.HlslSource, pass.Name + "_PS", kPSProfile,
                                Args.ShaderIncludeDir, tmplPSDir, passDir, Args.Debug, Args.Dx11, result);
                            if (!result.Errors.empty())
                                log.Error("PS compile:\n" + result.Errors);
                            if (result.Success)
                            {
                                std::string outPath = MatOutputDir + "/" + pass.Name + "_" + idxStr + kPSSuffix + kShaderExt;
                                WriteDxil(outPath, result.DxilBytes, err);
                                if (Args.Debug && !result.PdbBytes.empty())
                                    WriteDxil(MatOutputDir + "/" + pass.Name + "_" + idxStr + kPSSuffix + ".pdb", result.PdbBytes, err);
                            }
                            else ok = false;
                        }
                    }
                }

                if (ok && !pass.TemplateCS.empty())
                {
                    log.Stage("AssembleCS", pass.Name + "_" + idxStr);
                    MC::MCTemplateAssembler asmCS;
                    if (!asmCS.LoadTemplate(tmplCSPath, err))
                    { log.Error(err); ok = false; }
                    else
                    {
                        MC::MCAssembledShader assembled;
                        if (Args.DumpHlsl)
                            assembled.DumpPath = MatOutputDir + "/" + pass.Name + "_" + idxStr + kCSSuffix + ".hlsl";
                        if (!asmCS.Assemble(vf, mat, pass, variant, assembled, err, Args.DumpHlsl))
                        { log.Error("Assemble CS: " + err); ok = false; }
                        else
                        {
                            log.Stage("CompileCS", pass.Name + "_" + idxStr);
                            MC::MCDxcResult result;
                            std::string tmplCSDir = fs::path(tmplCSPath).parent_path().string();
                            Dxc.Compile(assembled.HlslSource, pass.Name + "_CS", kCSProfile,
                                Args.ShaderIncludeDir, tmplCSDir, passDir, Args.Debug, Args.Dx11, result);
                            if (!result.Errors.empty())
                                log.Error("CS compile:\n" + result.Errors);
                            if (result.Success)
                            {
                                std::string outPath = MatOutputDir + "/" + pass.Name + "_" + idxStr + kCSSuffix + kShaderExt;
                                WriteDxil(outPath, result.DxilBytes, err);
                                if (Args.Debug && !result.PdbBytes.empty())
                                    WriteDxil(MatOutputDir + "/" + pass.Name + "_" + idxStr + kCSSuffix + ".pdb", result.PdbBytes, err);
                            }
                            else ok = false;
                        }
                    }
                }

                if (ok)
                {
                    log.Stage("WriteReflection", pass.Name + "_" + idxStr);
                    MC::MCReflectionInput reflIn;
                    reflIn.Pass = &pass;
                    reflIn.Mat = &mat;
                    reflIn.VF = &vf;
                    reflIn.Variant = &variant;
                    reflIn.VariantIdx = vi;
                    MC::MCReflectionWriter rw;
                    rw.Write(reflIn, MatOutputDir, err);
                    UpdateCache(Cache, cacheKey, inputs);
                }
            }

            std::string vsFile = pass.Name + "_" + idxStr + kVSSuffix + kShaderExt;
            std::string psFile = pass.Name + "_" + idxStr + kPSSuffix + kShaderExt;
            std::string csFile = pass.Name + "_" + idxStr + kCSSuffix + kShaderExt;
            std::string reflFile = pass.Name + "_" + idxStr + ".refl.json";

            Value varEntry(kObjectType);
            varEntry.AddMember("index", vi, alloc);
            varEntry.AddMember("name", Value(variant.Name.c_str(), alloc), alloc);
            if (!pass.TemplateVS.empty()) varEntry.AddMember("vs", Value(vsFile.c_str(), alloc), alloc);
            if (!pass.TemplatePS.empty()) varEntry.AddMember("ps", Value(psFile.c_str(), alloc), alloc);
            if (!pass.TemplateCS.empty()) varEntry.AddMember("cs", Value(csFile.c_str(), alloc), alloc);
            varEntry.AddMember("refl", Value(reflFile.c_str(), alloc), alloc);
            passEntry["variants"].PushBack(varEntry, alloc);
        }

        manifest["passes"].PushBack(passEntry, alloc);

        StringBuffer sb;
        PrettyWriter<StringBuffer> pw(sb);
        pw.SetIndent(' ', 4);
        manifest.Accept(pw);
        std::string manifestPath = MatOutputDir + "/" + mat.Name + ".mat.json";
        std::ofstream mf(manifestPath);
        if (mf.is_open()) mf << sb.GetString();

        return true;
    }

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int argc, char** argv)
{
    MCArgs args;
    if (!ParseArgs(argc, argv, args)) return 1;

    std::string logPath = args.OutputDir + "/" + args.LogPath;
    MC::MCLogger::Get().Init(logPath);
    auto& log = MC::MCLogger::Get();

    log.Info("MaterialCompiler started");
    log.Info("Target: " + std::string(args.Dx11 ? "DX11 (DXBC/SM5)" : "DX12 (DXIL/SM6)"));
    log.Info("Debug: " + std::string(args.Debug ? "ON" : "OFF"));
    log.Info("Dump HLSL: " + std::string(args.DumpHlsl ? "ON" : "OFF"));

    std::string err;
    MC::MCDxcCompiler dxc;
    if (!dxc.Init(err)) { log.Error("DXC init failed: " + err); return 1; }

    auto RunAll = [&]()
    {
        if (!fs::exists(args.SourceDir))
        {
            log.Error("Source directory does not exist: " + args.SourceDir);
            return;
        }

        Document cache = LoadCache(args.OutputDir);
        std::string engineRoot = DeriveEngineRoot(args.ShaderIncludeDir);
        if (engineRoot.empty())
        {
            log.Error("Could not derive engine root from: " + args.ShaderIncludeDir);
            return;
        }

        log.Info("Engine root: " + engineRoot);

        // New architecture: scan for .json files that are ShaderPass definitions
        for (auto& entry : fs::recursive_directory_iterator(args.SourceDir))
        {
            if (!entry.is_regular_file()) continue;
            std::string path = entry.path().string();
            if (path.size() < 5 || path.substr(path.size() - 5) != ".json") continue;

            // Try to parse as ShaderPass
            MC::MCShaderPass testPass;
            std::string testErr;
            if (!MC::ParseShaderPass(path, testPass, testErr)) continue;

            if (!args.FilterMaterial.empty() && testPass.Name != args.FilterMaterial) continue;

            std::string matOut = args.OutputDir + "/" + testPass.Name;
            CompileMaterial(path, matOut, args, engineRoot, dxc, cache);
        }

        SaveCache(args.OutputDir, cache);
    };

    RunAll();

    if (args.Watch)
    {
        log.Info("Watching " + args.SourceDir + " for changes...");
        MC::MCFileWatcher watcher;
        bool needsRebuild = false;

        watcher.Start(args.SourceDir, [&](const std::string& changed)
        {
            log.Info("Changed: " + changed);
            needsRebuild = true;
        }, err);

        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (needsRebuild)
            {
                needsRebuild = false;
                RunAll();
            }
        }
    }

    dxc.Shutdown();
    log.Info("MaterialCompiler finished");
    MC::MCLogger::Get().Shutdown();
    return 0;
}
