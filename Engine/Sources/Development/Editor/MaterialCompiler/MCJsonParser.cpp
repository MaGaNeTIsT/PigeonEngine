#include "MCJsonParser.h"
#include <document.h>
#include <error/en.h>
#include <fstream>
#include <sstream>

namespace MC
{

static bool ReadFile(const std::string& Path, std::string& Out, std::string& Err)
{
    std::ifstream f(Path);
    if (!f.is_open()) { Err = "Cannot open: " + Path; return false; }
    std::ostringstream ss; ss << f.rdbuf();
    Out = ss.str();
    return true;
}

static bool LoadHlslField(const rapidjson::Value& obj, const char* key,
    const std::string& Dir, std::string& Out, std::string& Err)
{
    if (!obj.HasMember(key)) return true;
    std::string val = obj[key].GetString();
    if (val.size() > 5 && val.substr(val.size() - 5) == ".hlsl")
        return ReadFile(Dir + val, Out, Err);
    Out = std::move(val);
    return true;
}

static bool ParseDoc(const std::string& Raw, rapidjson::Document& Doc, std::string& Err)
{
    Doc.Parse(Raw.c_str());
    if (Doc.HasParseError())
    {
        Err = std::string("JSON parse error: ") + rapidjson::GetParseError_En(Doc.GetParseError());
        return false;
    }
    return true;
}

bool ParseVertexFactory(const std::string& JsonPath, MCVertexFactory& Out, std::string& OutError)
{
    std::string raw;
    if (!ReadFile(JsonPath, raw, OutError)) return false;
    rapidjson::Document doc;
    if (!ParseDoc(raw, doc, OutError)) return false;

    Out.Name = doc["name"].GetString();
    for (auto& a : doc["attributes"].GetArray())
    {
        MCVFAttribute attr;
        attr.Semantic = a["semantic"].GetString();
        attr.Index    = a["index"].GetInt();
        attr.Define   = a["define"].GetString();
        attr.Num      = a["num"].GetInt();
        Out.Attributes.push_back(std::move(attr));
    }
    std::string dir = JsonPath.substr(0, JsonPath.find_last_of("/\\") + 1);
    if (doc.HasMember("defines"))
        for (auto& d : doc["defines"].GetArray())
        {
            MCDefine def;
            def.Name  = d["name"].GetString();
            def.Value = d["value"].GetInt();
            Out.Defines.push_back(std::move(def));
        }
    if (doc.HasMember("vs"))
    {
        auto& vs = doc["vs"];
        if (!LoadHlslField(vs, "hlsl_functions", dir, Out.HlslVSFunctions, OutError)) return false;
        if (!LoadHlslField(vs, "hlsl_body",      dir, Out.HlslVS,          OutError)) return false;
        if (!LoadHlslField(vs, "varying_struct",  dir, Out.VaryingStruct,   OutError)) return false;
    }
    return true;
}

bool ParseMaterial(const std::string& JsonPath, MCMaterial& Out, std::string& OutError)
{
    std::string raw;
    if (!ReadFile(JsonPath, raw, OutError)) return false;
    rapidjson::Document doc;
    if (!ParseDoc(raw, doc, OutError)) return false;

    Out.Name = doc["name"].GetString();

    if (doc.HasMember("constant_buffer"))
    {
        auto& cb = doc["constant_buffer"];
        Out.CB.Name = cb["name"].GetString();
        for (auto& f : cb["fields"].GetArray())
        {
            MCCBField field;
            field.Name = f["name"].GetString();
            field.Type = f["type"].GetString();
            if (f.HasMember("default"))
            {
                auto& def = f["default"];
                if (def.IsArray())
                    for (auto& v : def.GetArray()) field.Default.push_back(v.GetFloat());
                else
                    field.Default.push_back(def.GetFloat());
            }
            Out.CB.Fields.push_back(std::move(field));
        }
    }

    if (doc.HasMember("textures"))
        for (auto& t : doc["textures"].GetArray())
        {
            MCTexture tex;
            tex.Name    = t["name"].GetString();
            tex.Type    = t["type"].GetString();
            tex.Sampler = t["sampler"].GetString();
            Out.Textures.push_back(std::move(tex));
        }

    if (doc.HasMember("samplers"))
        for (auto& s : doc["samplers"].GetArray())
        {
            MCSampler samp;
            samp.Name    = s["name"].GetString();
            samp.Filter  = s["filter"].GetString();
            samp.Address = s["address"].GetString();
            Out.Samplers.push_back(std::move(samp));
        }

    if (doc.HasMember("defines"))
        for (auto& d : doc["defines"].GetArray())
        {
            MCDefine def;
            def.Name  = d["name"].GetString();
            def.Value = d["value"].GetInt();
            Out.Defines.push_back(std::move(def));
        }

    std::string dir = JsonPath.substr(0, JsonPath.find_last_of("/\\") + 1);

    if (doc.HasMember("ps"))
    {
        auto& ps = doc["ps"];
        if (!LoadHlslField(ps, "hlsl_functions", dir, Out.HlslPSFunctions, OutError)) return false;
        if (!LoadHlslField(ps, "hlsl_surface",   dir, Out.HlslSurface,     OutError)) return false;
    }
    if (doc.HasMember("cs"))
    {
        auto& cs = doc["cs"];
        if (!LoadHlslField(cs, "hlsl_functions", dir, Out.HlslCSFunctions, OutError)) return false;
        if (!LoadHlslField(cs, "hlsl_surface",   dir, Out.HlslSurface,     OutError)) return false;
    }
    return true;
}

bool ParseShaderPass(const std::string& JsonPath, MCShaderPass& Out, std::string& OutError)
{
    std::string raw;
    if (!ReadFile(JsonPath, raw, OutError)) return false;
    rapidjson::Document doc;
    if (!ParseDoc(raw, doc, OutError)) return false;

    Out.Name          = doc["name"].GetString();
    Out.Stage         = doc["stage"].GetString();
    Out.VertexFactory = doc.HasMember("vertex_factory") ? doc["vertex_factory"].GetString() : "";
    Out.Material      = doc["material"].GetString();
    Out.TemplateVS    = doc.HasMember("template_vs") ? doc["template_vs"].GetString() : "";
    Out.TemplatePS    = doc.HasMember("template_ps") ? doc["template_ps"].GetString() : "";
    Out.TemplateCS    = doc.HasMember("template_cs") ? doc["template_cs"].GetString() : "";

    if (doc.HasMember("defines"))
        for (auto& d : doc["defines"].GetArray())
        {
            MCDefine def;
            def.Name  = d["name"].GetString();
            def.Value = d["value"].GetInt();
            Out.Defines.push_back(std::move(def));
        }

    if (doc.HasMember("variants"))
        for (auto& v : doc["variants"].GetArray())
        {
            MCVariant var;
            var.Name = v["name"].GetString();
            if (v.HasMember("defines"))
                for (auto& d : v["defines"].GetArray())
                {
                    MCDefine def;
                    def.Name  = d["name"].GetString();
                    def.Value = d["value"].GetInt();
                    var.Defines.push_back(std::move(def));
                }
            Out.Variants.push_back(std::move(var));
        }

    return true;
}

} // namespace MC
