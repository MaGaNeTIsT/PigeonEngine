#include "SlideCharacter.h"
namespace PR_SlideHero
{
    static void RegisterClassTypes()
    {
        RegisterClassType<PSlideCharacter, PCharacter>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

    PSlideCharacter::PSlideCharacter()
    {
        POBJ_DEBUGNAME_SET(this, "SlideCharacter");
    }
    PSlideCharacter::~PSlideCharacter()
    {
    }

    void PSlideCharacter::UserBeginPlay()
    {
        PCharacter::UserBeginPlay();
    }

    void PSlideCharacter::UserTick(FLOAT deltaTime)
    {
        PCharacter::UserTick(deltaTime);
    }

    void PSlideCharacter::UserEndPlay()
    {
        PCharacter::UserEndPlay();
    }

    const ECharacterStatus& PSlideCharacter::GetStatus() const
    {
        return this->Status;
    }
}


