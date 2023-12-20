#pragma once
#include <CoreMinimal.h>

#include "../Character/SlideCharacter.h"

namespace PR_SlideHero
{
    using namespace PigeonEngine;
    class Party final
    {
        
    public:
        BOOL8 AddMember   (PSlideCharacter* NewMember);
        BOOL8 RemoveMember(PSlideCharacter* NewMember);
    private:
        TArray<PSlideCharacter*> Members;
    };
}
