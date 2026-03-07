#include "MaterialBinding.h"

namespace PigeonEngine
{

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

        // Note: material sampler states (s4+) must be created by the caller and bound
        // separately using Device.BindPSSamplerState(samplerState, slot) with slot info
        // from Variant.Reflection.Samplers[i].Slot.
    }

} // namespace PigeonEngine
