#include "MaterialAsset.h"

namespace PigeonEngine
{

    const EMaterialVariant* EMaterialAsset::FindVariant(const EString& PassName, UINT32 VariantIndex) const
    {
        for (UINT32 VarIdx = 0u, NumVars = Variants.Num<UINT32>(); VarIdx < NumVars; VarIdx++)
        {
            const EMaterialVariant& Var = Variants[VarIdx];
            if ((Var.PassName == PassName) && (Var.VariantIndex == VariantIndex))
            {
                return (&Var);
            }
        }
        return nullptr;
    }

} // namespace PigeonEngine
