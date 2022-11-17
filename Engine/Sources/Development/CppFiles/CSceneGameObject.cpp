#include "../../../../Entry/EngineMain.h"
#include "../Headers/CSceneGameObject.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRendererComponent.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshComponent.h"

CSceneGameObject::CSceneGameObject()
{
	this->AddNewTransform();
	{
		std::shared_ptr<CMeshRendererComponent> meshRendererComponent(std::shared_ptr<CMeshRendererComponent>(new CMeshRendererComponent()));
		std::shared_ptr<CMeshComponent> meshComponent(std::shared_ptr<CMeshComponent>(new CMeshComponent()));
		this->AddComponent(meshRendererComponent);
		this->AddComponent(meshComponent);
	}
}
CSceneGameObject::CSceneGameObject(const CSceneGameObject& sceneGameObject)
{
	if (sceneGameObject.m_Transform)
	{
		this->AddNewTransformWithValue(sceneGameObject.m_Transform->GetWorldPosition(), sceneGameObject.m_Transform->GetWorldRotation(), sceneGameObject.m_Transform->GetWorldScale());
	}
	else
	{
		this->AddNewTransform();
	}
}
CSceneGameObject::~CSceneGameObject()
{
}
void CSceneGameObject::Init();
void CSceneGameObject::Uninit();
void CSceneGameObject::Update();
void CSceneGameObject::FixedUpdate();