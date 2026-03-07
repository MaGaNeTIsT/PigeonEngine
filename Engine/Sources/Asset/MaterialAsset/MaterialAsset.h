#pragma once

#include <CoreMinimal.h>
#include <BaseAsset.h>
#include <ShaderAsset/ShaderAsset.h>

namespace PigeonEngine
{

    // ---------------------------------------------------------------------------
    // Reflection data loaded from .refl.json
    // ---------------------------------------------------------------------------

    struct EMaterialCBField
    {
        EString Name;
        UINT32  Offset = 0u;
        UINT32  Size = 0u;
        EString Type;
    };

    struct EMaterialCBRefl
    {
        EString                    Name;
        UINT32                     Slot = 0u;
        UINT32                     SizeBytes = 0u;
        TArray<EMaterialCBField>   Fields;
    };

    struct EMaterialSRVRefl
    {
        EString Name;
        UINT32  Slot = 0u;
    };

    struct EMaterialSamplerRefl
    {
        EString Name;
        UINT32  Slot = 0u;
    };

    struct EMaterialInputLayoutRefl
    {
        EString Semantic;
        UINT32  Index = 0u;
        UINT32  Components = 4u;
    };

    struct EMaterialReflection
    {
        TArray<EMaterialCBRefl>          ConstantBuffers;
        TArray<EMaterialSRVRefl>         SRVs;
        TArray<EMaterialSamplerRefl>     Samplers;
        TArray<EMaterialInputLayoutRefl> InputLayout;
    };

    // ---------------------------------------------------------------------------
    // One compiled variant (one set of VS+PS+reflection)
    // ---------------------------------------------------------------------------

    struct EMaterialVariant
    {
        EString                     PassName;
        UINT32                      VariantIndex = 0u;
        EString                     VariantName;
        const EVertexShaderAsset*   VS = nullptr;
        const EPixelShaderAsset*    PS = nullptr;
        EMaterialReflection         Reflection;
    };

    // ---------------------------------------------------------------------------
    // Material asset — holds all passes/variants for one material
    // ---------------------------------------------------------------------------

    class EMaterialAsset
    {
    public:
        explicit EMaterialAsset(const EString& InName) : Name(InName) {}
        ~EMaterialAsset() = default;

        const EString& GetName() const { return Name; }

        void AddVariant(EMaterialVariant&& V) { Variants.Add(std::move(V)); }

        // Returns nullptr if not found.
        const EMaterialVariant* FindVariant(const EString& PassName, UINT32 VariantIndex) const;

    private:
        EString                    Name;
        TArray<EMaterialVariant>   Variants;

        EMaterialAsset() = delete;
        CLASS_REMOVE_COPY_BODY(EMaterialAsset)
    };

} // namespace PigeonEngine
