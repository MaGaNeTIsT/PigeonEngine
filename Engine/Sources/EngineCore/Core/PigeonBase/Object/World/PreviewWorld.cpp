#include "PreviewWorld.h"

#if _EDITOR_ONLY

#include <PigeonBase/Object/Component/CameraAndLight/DirectionalLightComponent.h>

namespace PigeonEngine
{
    namespace
    {
        Vector3 NormalizeOrDefaultPreviewVector(const Vector3& InVector, const Vector3& InDefaultVector)
        {
            if (InVector.LengthSquare() <= PE_SMALL_NUMBER)
            {
                return InDefaultVector;
            }

            return Vector3::Normalize(InVector);
        }

        Quaternion MakePreviewLightRotation(const Vector3& InWorldLightDirection)
        {
            const Vector3 WorldLightDirection = NormalizeOrDefaultPreviewVector(InWorldLightDirection, Vector3(0.0f, 0.0f, 1.0f));
            const Vector3 ForwardVector = -WorldLightDirection;
            const Vector3 UpVector = (EMath::Abs(Vector3::Dot(ForwardVector, Vector3::YVector())) > 0.95f) ? Vector3::ZVector() : Vector3::YVector();
            return LookAtTargetQuaternionRotation(ForwardVector, Vector3::Zero(), UpVector);
        }

        void AddPreviewDirectionalLight(PWorld* InWorld, const CHAR* InDebugName, const Color3& InColor, FLOAT InIntensity, const Vector3& InWorldLightDirection)
        {
            if (!InWorld)
            {
                return;
            }

            PActor* LightActor = new PActor();
            LightActor->SetIsTickable(FALSE);
            POBJ_DEBUGNAME_SET(LightActor, InDebugName);

            PDirectionalLightComponent* LightComponent = new PDirectionalLightComponent();
            LightComponent->SetLightColor(InColor);
            LightComponent->SetLightIntensity(InIntensity);
            LightComponent->SetShadowMapSize(FALSE, Vector2Int(2, 2));
            LightActor->SetRootComponent(LightComponent);

            ETransform LightTransform;
            LightTransform.SetRotation_Local(MakePreviewLightRotation(InWorldLightDirection));
            InWorld->AddActor(LightActor, LightTransform);
        }
    }

    static void RegisterPreviewWorldClassTypes()
    {
        RegisterClassType<PPreviewWorld, PWorld>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterPreviewWorldClassTypes);

    PPreviewWorld::PPreviewWorld()
    {
    }

    PPreviewWorld::~PPreviewWorld()
    {
    }

    void PPreviewWorld::Init()
    {
        PWorld::Init();

        AddPreviewDirectionalLight(this, "PreviewKeyLightActor", Color3(1.0f, 0.97f, 0.93f), 0.55f, Vector3(0.45f, 0.70f, 0.55f));
        AddPreviewDirectionalLight(this, "PreviewBackFillLightActor", Color3(0.92f, 0.95f, 1.0f), 0.18f, Vector3(-0.30f, 0.35f, -0.88f));
        AddPreviewDirectionalLight(this, "PreviewLeftFillLightActor", Color3(0.84f, 0.90f, 1.0f), 0.12f, Vector3(-0.92f, 0.22f, 0.10f));
        AddPreviewDirectionalLight(this, "PreviewRightFillLightActor", Color3(1.0f, 0.95f, 0.90f), 0.10f, Vector3(0.92f, 0.18f, -0.12f));
        AddPreviewDirectionalLight(this, "PreviewTopFillLightActor", Color3(0.98f, 0.99f, 1.0f), 0.08f, Vector3(0.0f, 1.0f, 0.0f));
    }

    PActor* PPreviewWorld::CreateRootActor()
    {
        return new PActor();
    }

    void PPreviewWorld::CreateDefaultController()
    {
    }

    BOOL32 PPreviewWorld::ShouldAffectGlobalPhysics() const
    {
        return FALSE;
    }
}

#endif