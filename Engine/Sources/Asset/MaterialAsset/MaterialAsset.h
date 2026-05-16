#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <BaseAsset.h>
#include <ShaderAsset/ShaderAsset.h>

namespace PigeonEngine
{

    // ---------------------------------------------------------------------------
    // Reflection data loaded from .refl.json
    // ---------------------------------------------------------------------------

    struct EMaterialCBField
    {
        EString     Name;
        UINT32      Offset      = 0u;
        UINT32      Size        = 0u;
        EString     Type;
        BOOL32      bIsColor    = FALSE;
    };

    struct EMaterialCBRefl
    {
        EString                     Name;
        UINT32                      Slot            = 0u;
        UINT32                      SizeBytes       = 0u;
        TArray<EMaterialCBField>    Fields;
    };

    struct EMaterialSRVRefl
    {
        EString     Name;
        UINT32      Slot        = 0u;
    };

    struct EMaterialSamplerRefl
    {
        EString     Name;
        UINT32      Slot                = 0u;
        EString     Filter;                         // "POINT" / "LINEAR" / "ANISO" etc.
        EString     Address;                        // "WRAP" / "CLAMP" etc.
        BOOL32      bEngineGlobal       = FALSE;    // TRUE if mapped to engine global sampler (s0-s3)
    };

    struct EMaterialInputLayoutRefl
    {
        EString     Semantic;           // e.g. "POSITION", "NORMAL"
        UINT32      Index       = 0u;   // semantic index
        EString     Format;             // e.g. "FLOAT4", "FLOAT2", "UINT4" — encodes type and component count
        UINT32      Slot        = 0u;   // vertex buffer input slot
    };

    struct EMaterialReflection
    {
        TArray<EMaterialCBRefl>                 ConstantBuffers;
        TArray<EMaterialSRVRefl>                SRVs;
        TArray<EMaterialSamplerRefl>            Samplers;
        TArray<EMaterialInputLayoutRefl>        InputLayout;
    };

    // ---------------------------------------------------------------------------
    // One compiled variant (one set of VS+PS+reflection)
    // ---------------------------------------------------------------------------

    struct EMaterialVariant
    {
        EString                         PassName;
        UINT32                          VariantIndex        = 0u;
        EString                         VariantName;
        const EVertexShaderAsset*       VS                  = nullptr;
        const EPixelShaderAsset*        PS                  = nullptr;
        const EComputeShaderAsset*      CS                  = nullptr;
        EMaterialReflection             Reflection;
    };

    // ---------------------------------------------------------------------------
    // Material asset — holds all passes/variants for one material
    // ---------------------------------------------------------------------------

    class EMaterialAsset
    {
    public:
        const EString&          GetName() const;
        void                    AddVariant(EMaterialVariant&& V);
        const EMaterialVariant* FindVariant(const EString& PassName, UINT32 VariantIndex) const;
        const EMaterialVariant* GetFirstVariant() const;

    private:
        EString                     Name;
        TArray<EMaterialVariant>    Variants;

    public:
        EMaterialAsset(const EString& InName);
        ~EMaterialAsset();
        EMaterialAsset() = delete;

        CLASS_REMOVE_COPY_BODY(EMaterialAsset)

    };

    class EMaterialAssetManager : public EManagerBase
    {
    public:
        virtual void Initialize() override;
        virtual void ShutDown()   override;

    public:
        // Load a material from its compiled output directory.
        // InLoadPath  : e.g. "Assets/Materials/M_Rock/"
        // InLoadName  : e.g. "M_Rock"
        // OutMaterial : set on success
        BOOL32 LoadMaterialAsset(
            const EString& InLoadPath,
            const EString& InLoadName,
            const EMaterialAsset*& OutMaterial);

    private:
        BOOL32 ParseReflection(const EString& ReflJsonPath, EMaterialReflection& Out);

    private:
        typedef TAssetManager<EString, EMaterialAsset> EMaterialManager;
        EMaterialManager MaterialManager;

        CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EMaterialAssetManager)

    };

    // Try to load a material; in editor mode, compiles it first if the manifest is missing.
    extern BOOL32 TryLoadMaterialAsset(const EString& InOutputDir, const EString& InLoadName,
        const EString& InSourceDir, const EString& InShaderIncludeDir, BOOL32 bEmitDebugInfo,
        const EMaterialAsset*& OutMaterial
    );

} // namespace PigeonEngine
