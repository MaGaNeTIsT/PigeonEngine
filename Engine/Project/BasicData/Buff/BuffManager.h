#pragma once
#include "Buff.h"
#include "Base/DataStructure/Pointer/SharedPtr.h"
namespace PR_SlideHero
{
    using namespace PigeonEngine;
    class BuffManager
    {
    public:
        TSharedPtr<EBuff> GetBuffById(const EString& BuffId);
    };
}
