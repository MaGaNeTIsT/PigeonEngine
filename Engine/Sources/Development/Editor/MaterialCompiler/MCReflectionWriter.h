#pragma once
#include "MCJsonParser.h"
#include "MCTemplateAssembler.h"
#include <string>

namespace MC
{

struct MCReflectionInput
{
    const MCShaderPass*    Pass        = nullptr;
    const MCMaterial*      Mat         = nullptr;
    const MCVertexFactory* VF          = nullptr;
    const MCVariant*       Variant     = nullptr;
    int                    VariantIdx  = 0;
    MCSlotMap              Slots;
};

class MCReflectionWriter
{
public:
    // Write BasePass_NNNN.refl.json to OutputDir.
    bool Write(const MCReflectionInput& Input, const std::string& OutputDir, std::string& OutError);
};

} // namespace MC
