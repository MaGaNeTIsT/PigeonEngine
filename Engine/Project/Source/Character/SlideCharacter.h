#pragma once

#include "CharacterStatus.h"
#include "PigeonBase/Object/Actor/LevelCharacter.h"
#include "PigeonBase/Object/Component/Primitive/LineComponent.h"

namespace PR_SlideHero
{
    using namespace PigeonEngine;
    struct ECharacterStatus;
    class PSlideCharacter : public PCharacter
    {
        CLASS_VIRTUAL_NOCOPY_BODY(PSlideCharacter)
    protected:
        // for users
        void UserBeginPlay() override;
        void UserTick(FLOAT deltaTime) override;
        void UserEndPlay() override;
        
    private:
        PSkeletalMeshComponent* SkeletalMeshComp = nullptr;
        PLineComponent*         IndicatorComp    = nullptr;

    private:
        PSlideCharacter* Target   = nullptr;
        PSlideCharacter* Support  = nullptr;
        
    public:
        const ECharacterStatus& GetStatus() const;
    private:
        ECharacterStatus Status;
        
    };
}
