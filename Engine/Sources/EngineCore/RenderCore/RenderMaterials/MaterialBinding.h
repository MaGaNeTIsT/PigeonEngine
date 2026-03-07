#pragma once

#include <CoreMinimal.h>
#include <RenderDevice/DeviceD3D11.h>
#include <TextureAsset/TextureAsset.h>
#include <MaterialAsset/MaterialAsset.h>

namespace PigeonEngine
{

    // Maps SRV slot -> texture SRV to bind at draw time.
    // Key = slot number, Value = SRV ComPtr
    using EMaterialTextureMap = TMap<UINT32, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>;

    class EMaterialBinding
    {
    public:
        // Stateless helper — binds shaders, input layout, CBs, SRVs, and samplers
        // for the given variant. Caller is responsible for uploading CB data separately.
        static void BindForDraw(
            RDeviceD3D11&              Device,
            const EMaterialVariant&    Variant,
            const EMaterialTextureMap& TextureMap);

    private:
        EMaterialBinding() = delete;
    };

} // namespace PigeonEngine
