#pragma once

#include <CoreMinimal.h>
#include <RenderDevice/DeviceD3D11.h>
#include <TextureAsset/TextureAsset.h>
#include <MaterialAsset/MaterialAsset.h>

namespace PigeonEngine
{

    struct EMaterialCBStanding
    {
        EString             Name;
        TArray<UINT8>       Data;
    };
    struct EMaterialTextureStanding
    {
        const ETexture2DAsset*      Texture;
        CHAR                        PathBuf[EEngineSettings::ENGINE_MATERIAL_NAME_LENGTH_MAX];

        EMaterialTextureStanding();
    };
    struct EMaterialSlotStanding
    {
        TArray<EMaterialCBStanding>             ConstantBufferSlots;
        TArray<EMaterialTextureStanding>        TextureSlots;
    };
    // Maps SRV slot -> texture SRV to bind at draw time.
    // Key = slot number, Value = SRV ComPtr
    using EMaterialTextureMap = TMap<UINT32, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>;

    struct RMaterialTextureSRV
    {
        UINT32                                          Slot;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV;
    };

    class EMaterialBinding
    {
    public:
        // Stateless helper — binds shaders, input layout, CBs, SRVs, and samplers
        // for the given variant. Caller is responsible for uploading CB data separately.
        static void BindForDraw(
            RDeviceD3D11& Device,
            const EMaterialVariant& Variant,
            const EMaterialTextureMap& TextureMap);

    private:
        EMaterialBinding() = delete;

        // Cached sampler states keyed by (Filter, Address) string pair.
        // Lazily created on first use. Static so they persist across frames.
        static TMap<EString, Microsoft::WRL::ComPtr<ID3D11SamplerState>> SamplerCache;

        static Microsoft::WRL::ComPtr<ID3D11SamplerState> GetOrCreateSampler(
            RDeviceD3D11& Device, const EString& Filter, const EString& Address);
    };

} // namespace PigeonEngine
