#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include "MaterialAsset.h"
#include <BaseAsset.h>

namespace PigeonEngine
{

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

} // namespace PigeonEngine
