#pragma once
#include "../../../../Entry/EngineMain.h"
#include "../../EngineBase/Headers/CInput.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshComponent.h"
#include "../../EngineGame/Headers/CGameObject.h"
#include "../../EngineGame/Headers/CScene.h"
#include "../../EngineGame/Headers/CGameObjectUtilty.h"
#include "../../EngineBase/Headers/CManager.h"
#include "../../EngineGame/Headers/CCamera.h"

class CPickMesh
{
public:
	struct CPickedMesh2D
	{
	public:
		CustomType::Vector2 ScreenMax;
		CustomType::Vector2 ScreenMin;

		CPickedMesh2D(CustomType::Vector2 ScreenMin, CustomType::Vector2 ScreenMax):
			ScreenMax(ScreenMax),
			ScreenMin(ScreenMin)
		{
		}
	public:
		inline BOOL	IsInside(const CustomType::Vector2& v) const
		{
			return v.X() < ScreenMax.X() && v.Y() < ScreenMax.Y()
				&& v.X() > ScreenMin.X() && v.Y() > ScreenMin.Y();
		}
	};

	std::vector<CGameObject*> GetPickedGameObjects()
	{
		if (CInput::Controller.IsLeftMouseButtonDown())
		{
			//is this really the mouse position?
			//CInput::Controller.ReadRawDelta();
			std::pair<INT, INT> mousePos = CInput::Controller.GetMousePosition();
			CustomType::Vector2 mousePosVec(mousePos.first, mousePos.second);
			std::vector<CGameObject*> listObject;
			std::vector<CGameObject*> PickedGameObjectIds;
			listObject = GameObjectUtility::GetGameObjectsByComponent<CMeshComponent>();

			for (const auto& obj : listObject)
			{
				std::vector<CPickedMesh2D> pickedMesh2Des;
				std::vector<const CMeshComponent*> meshComponents = obj->GetComponentListByType<CMeshComponent>();
				for (const auto& meshComponent : meshComponents)
				{
					CustomType::Vector3 boundMin;
					CustomType::Vector3 boundMax;
					meshComponent->GetMinMaxBounding(boundMin, boundMax);

					//Convert to Screen
					CustomType::Matrix4x4 MatrixVP = CManager::GetScene()->GetMainCamera<CCamera>()->GetViewProjectionMatrix();
					boundMin = MatrixVP.MultiplyVector(boundMin);
					boundMax = MatrixVP.MultiplyVector(boundMax);

					pickedMesh2Des.push_back(CPickedMesh2D(CustomType::Vector2(boundMin.X(), boundMin.Y()), CustomType::Vector2(boundMax.X(), boundMax.Y())));
				}

				for (const auto& mesh : pickedMesh2Des)
				{
					if (mesh.IsInside(mousePosVec))
					{
						PickedGameObjectIds.push_back(obj);
					}
				}
			}

			return PickedGameObjectIds;
		}
		return std::vector<CGameObject*>();
	}
};