#include "MaterialAssetManager.h"
#include <ShaderAsset/ShaderAsset.h>
#include <RapidJson/Headers/JsonObject.h>
#include <IO/FileHelper.h>

namespace PigeonEngine
{

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
                Cb->GetStringField("name",       CbRefl.Name);
                Cb->GetUIntField("slot",         CbRefl.Slot);
                Cb->GetUIntField("size_bytes",   CbRefl.SizeBytes);

                TArray<CJsonObject*> Fields;
                if (Cb->GetObjectArrayField("fields", Fields))
                {
                    for (CJsonObject* F : Fields)
                    {
                        EMaterialCBField Field;
                        F->GetStringField("name",   Field.Name);
                        F->GetStringField("type",   Field.Type);
                        F->GetUIntField("offset",   Field.Offset);
                        F->GetUIntField("size",     Field.Size);
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
                S->GetUIntField("slot",   Srv.Slot);
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
                S->GetUIntField("slot",   Samp.Slot);
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
                Il->GetStringField("semantic",   Entry.Semantic);
                Il->GetUIntField("index",        Entry.Index);
                Il->GetUIntField("components",   Entry.Components);
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
        if (Existing) { OutMaterial = Existing; return TRUE; }

        // Read manifest: <path>/<name>.mat.json
        EString ManifestPath = InLoadPath + InLoadName + ".mat.json";
        EString ManifestStr;
        if (!EFileHelper::ReadFileAsString(ManifestPath, ManifestStr))
        {
            PE_FAILED((ENGINE_ASSET_ERROR), ("MaterialAssetManager: cannot read manifest"));
            return FALSE;
        }

        CJsonObject Manifest(ManifestStr);

        TArray<CJsonObject*> Passes;
        if (!Manifest.GetObjectArrayField("passes", Passes))
        {
            PE_FAILED((ENGINE_ASSET_ERROR), ("MaterialAssetManager: no passes in manifest"));
            return FALSE;
        }

        EMaterialAsset* Asset = new EMaterialAsset(InLoadName);

        for (CJsonObject* PassObj : Passes)
        {
            EString PassName;
            PassObj->GetStringField("pass", PassName);

            TArray<CJsonObject*> Variants;
            if (!PassObj->GetObjectArrayField("variants", Variants))
            {
                delete PassObj; continue;
            }

            for (CJsonObject* VarObj : Variants)
            {
                UINT32  VarIdx = 0u;
                EString VarName, VsFile, PsFile, ReflFile;
                VarObj->GetUIntField("index", VarIdx);
                VarObj->GetStringField("name", VarName);
                VarObj->GetStringField("vs",   VsFile);
                VarObj->GetStringField("ps",   PsFile);
                VarObj->GetStringField("refl", ReflFile);

                EMaterialVariant Variant;
                Variant.PassName     = PassName;
                Variant.VariantIndex = VarIdx;
                Variant.VariantName  = VarName;

                // Derive asset names from file names (strip .dxil extension)
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

                EShaderAssetManager::GetManagerSingleton()->LoadVertexShaderAsset(InLoadPath, VsName, Variant.VS);
                EShaderAssetManager::GetManagerSingleton()->LoadPixelShaderAsset(InLoadPath, PsName, Variant.PS);

                // Parse reflection
                EString ReflPath = InLoadPath + ReflFile;
                ParseReflection(ReflPath, Variant.Reflection);

                Asset->AddVariant(std::move(Variant));
                delete VarObj;
            }

            delete PassObj;
        }

        MaterialManager.Add(InLoadName, Asset);
        OutMaterial = Asset;
        return TRUE;
    }

} // namespace PigeonEngine
