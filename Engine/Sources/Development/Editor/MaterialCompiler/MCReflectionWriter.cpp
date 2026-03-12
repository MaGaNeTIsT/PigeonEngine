#include "MCReflectionWriter.h"
#include <document.h>
#include <prettywriter.h>
#include <stringbuffer.h>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace rapidjson;

namespace MC
{

static int TypeSizeBytes(const std::string& Type)
{
    if (Type == "float")  return 4;
    if (Type == "float2") return 8;
    if (Type == "float3") return 12;
    if (Type == "float4") return 16;
    if (Type == "int")    return 4;
    if (Type == "uint")   return 4;
    return 4;
}

static std::string VariantIndexStr(int Idx)
{
    std::ostringstream ss;
    ss << std::setw(4) << std::setfill('0') << Idx;
    return ss.str();
}

bool MCReflectionWriter::Write(const MCReflectionInput& In, const std::string& OutputDir, std::string& OutError)
{
    Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("pass",         Value(In.Pass->Name.c_str(), alloc), alloc);
    doc.AddMember("variant",      In.VariantIdx,                       alloc);
    doc.AddMember("variant_name", Value(In.Variant->Name.c_str(), alloc), alloc);
    doc.AddMember("stage",        Value(In.Pass->Stage.c_str(), alloc), alloc);

    // Constant buffers
    {
        Value cbArr(kArrayType);
        if (!In.Mat->CB.Fields.empty())
        {
            Value cb(kObjectType);
            cb.AddMember("name", Value(In.Mat->CB.Name.c_str(), alloc), alloc);
            cb.AddMember("slot", In.Slots.CBSlot, alloc);

            Value stageMask(kArrayType);
            stageMask.PushBack(Value("PS", alloc), alloc);
            cb.AddMember("stage_mask", stageMask, alloc);

            int offset = 0, total = 0;
            Value fields(kArrayType);
            for (auto& f : In.Mat->CB.Fields)
            {
                int sz = TypeSizeBytes(f.Type);
                Value field(kObjectType);
                field.AddMember("name",   Value(f.Name.c_str(), alloc), alloc);
                field.AddMember("offset", offset,                        alloc);
                field.AddMember("size",   sz,                            alloc);
                field.AddMember("type",   Value(f.Type.c_str(), alloc), alloc);
                fields.PushBack(field, alloc);
                offset += sz;
                total  += sz;
            }
            cb.AddMember("size_bytes", total,  alloc);
            cb.AddMember("fields",     fields, alloc);
            cbArr.PushBack(cb, alloc);
        }
        doc.AddMember("constant_buffers", cbArr, alloc);
    }

    // SRVs
    {
        Value srvArr(kArrayType);
        int slot = kMaterialSRVStartSlot;
        for (auto& tex : In.Mat->Textures)
        {
            Value srv(kObjectType);
            srv.AddMember("name", Value(tex.Name.c_str(), alloc), alloc);
            srv.AddMember("slot", slot++, alloc);
            srvArr.PushBack(srv, alloc);
        }
        doc.AddMember("srvs", srvArr, alloc);
    }

    // Samplers
    {
        Value sampArr(kArrayType);
        int slot = kMaterialSamplerStartSlot;
        for (auto& samp : In.Mat->Samplers)
        {
            Value s(kObjectType);
            s.AddMember("name", Value(samp.Name.c_str(), alloc), alloc);
            s.AddMember("slot", slot++, alloc);
            sampArr.PushBack(s, alloc);
        }
        doc.AddMember("samplers", sampArr, alloc);
    }

    doc.AddMember("uavs", Value(kArrayType), alloc);

    // Input layout
    {
        Value ilArr(kArrayType);
        for (auto& attr : In.VF->Attributes)
        {
            Value il(kObjectType);
            il.AddMember("semantic",   Value(attr.Semantic.c_str(), alloc), alloc);
            il.AddMember("index",      attr.Index,                          alloc);
            il.AddMember("format",     Value("FLOAT", alloc),               alloc);
            il.AddMember("components", attr.Num,                            alloc);
            ilArr.PushBack(il, alloc);
        }
        doc.AddMember("input_layout", ilArr, alloc);
    }

    // Serialize
    StringBuffer sb;
    PrettyWriter<StringBuffer> writer(sb);
    writer.SetIndent(' ', 4);
    doc.Accept(writer);

    std::string idxStr  = VariantIndexStr(In.VariantIdx);
    std::string outPath = OutputDir + "/" + In.Pass->Name + "_" + idxStr + ".refl.json";

    std::ofstream f(outPath);
    if (!f.is_open()) { OutError = "Cannot write: " + outPath; return false; }
    f << sb.GetString();
    return true;
}

} // namespace MC
