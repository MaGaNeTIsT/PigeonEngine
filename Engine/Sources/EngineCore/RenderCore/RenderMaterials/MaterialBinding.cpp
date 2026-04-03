#include "MaterialBinding.h"
#include <RenderCommon.h>

namespace PigeonEngine
{

    EMaterialTextureStanding::EMaterialTextureStanding()
        : Texture(nullptr)
    {
        PathBuf[0] = '\0';
    }

    TMap<EString, Microsoft::WRL::ComPtr<ID3D11SamplerState>> EMaterialBinding::SamplerCache;

    static RFilterType ParseFilter(const EString& Filter)
    {
        if (Filter == EString("POINT"))  return RFilterType::FILTER_POINT;
        if (Filter == EString("LINEAR")) return RFilterType::FILTER_LINEAR;
        if (Filter == EString("ANISO") || Filter == EString("ANISOTROPIC"))
            return RFilterType::FILTER_ANISOTROPIC;
        return RFilterType::FILTER_LINEAR;
    }

    static RTextureAddressModeType ParseAddress(const EString& Address)
    {
        if (Address == EString("WRAP"))   return RTextureAddressModeType::TEXTURE_ADDRESS_WRAP;
        if (Address == EString("CLAMP"))  return RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP;
        if (Address == EString("MIRROR")) return RTextureAddressModeType::TEXTURE_ADDRESS_MIRROR;
        if (Address == EString("BORDER")) return RTextureAddressModeType::TEXTURE_ADDRESS_BORDER;
        return RTextureAddressModeType::TEXTURE_ADDRESS_WRAP;
    }

    Microsoft::WRL::ComPtr<ID3D11SamplerState> EMaterialBinding::GetOrCreateSampler(
        RDeviceD3D11& Device, const EString& Filter, const EString& Address)
    {
        EString Key = Filter + EString("_") + Address;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> Existing;
        if (SamplerCache.FindValue(Key, Existing) && Existing)
            return Existing;

        RTextureAddressModeType AddrMode = ParseAddress(Address);
        RSamplerState Desc(ParseFilter(Filter), AddrMode, AddrMode, AddrMode);

        Microsoft::WRL::ComPtr<ID3D11SamplerState> NewSampler;
        Device.CreateSamplerState(NewSampler, Desc);
        SamplerCache.Add(Key, NewSampler);
        return NewSampler;
    }

    void EMaterialBinding::BindForDraw(
        RDeviceD3D11&              Device,
        const EMaterialVariant&    Variant,
        const EMaterialTextureMap& TextureMap)
    {
        // Bind VS shader + input layout
        if (Variant.VS && Variant.VS->GetRenderResource())
        {
            auto* VsRes = Variant.VS->GetRenderResource();
            Device.SetVSShader(VsRes->Shader);
            Device.SetInputLayout(VsRes->InputLayout);
        }

        // Bind PS shader
        if (Variant.PS && Variant.PS->GetRenderResource())
        {
            Device.SetPSShader(Variant.PS->GetRenderResource()->Shader);
        }

        // Bind SRVs to PS (material textures start at t1+)
        for (UINT32 i = 0u, n = Variant.Reflection.SRVs.Num<UINT32>(); i < n; ++i)
        {
            const EMaterialSRVRefl& Srv = Variant.Reflection.SRVs[i];
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SrvPtr;
            if (TextureMap.FindValue(Srv.Slot, SrvPtr) && SrvPtr)
            {
                Device.BindPSShaderResourceView(SrvPtr, Srv.Slot);
            }
        }

        // Bind custom sampler states (s4+). Engine-global samplers (s0-s3) are
        // already bound by SceneRenderer::Render(), so we skip those.
        for (UINT32 i = 0u, n = Variant.Reflection.Samplers.Num<UINT32>(); i < n; ++i)
        {
            const EMaterialSamplerRefl& Samp = Variant.Reflection.Samplers[i];
            if (Samp.bEngineGlobal)
                continue;

            if (Samp.Filter.Length() == 0u || Samp.Address.Length() == 0u)
                continue;

            Microsoft::WRL::ComPtr<ID3D11SamplerState> SampState =
                GetOrCreateSampler(Device, Samp.Filter, Samp.Address);
            if (SampState)
            {
                Device.BindPSSamplerState(SampState, Samp.Slot);
            }
        }
    }

} // namespace PigeonEngine
