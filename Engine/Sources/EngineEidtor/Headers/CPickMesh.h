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
		CustomType::Vector2 ScreenMin;
		CustomType::Vector2 ScreenMax;

		CPickedMesh2D(CustomType::Vector2 ScreenMin, CustomType::Vector2 ScreenMax):
			ScreenMax(ScreenMax),
			ScreenMin(ScreenMin)
		{
		}
	public:
		inline BOOL	IsInside(const CustomType::Vector2& v) const
		{
			return v.X() > ScreenMin.X() && v.Y() > ScreenMin.Y()
				&& v.X() < ScreenMax.X() && v.Y() < ScreenMax.Y();
		}
	};

	struct CPickMeshSphere2D
	{
	public:
		CustomType::Vector2 ScreenCenter;
		FLOAT Raidus;

		CPickMeshSphere2D(CustomType::Vector2 ScreenCenter, FLOAT Raidus) :
			ScreenCenter(ScreenCenter),
			Raidus(Raidus)
		{
		}
	public:
		inline BOOL	IsInside(const CustomType::Vector2& v)
		{
			CustomType::Vector2 direction(ScreenCenter - v);
			return CustomType::Vector2::Dot(direction, direction);
		}
	};


	static CGameObject* GetPickedGameObjectsByBox()
	{
		if (CInput::Controller.IsLeftMouseButtonDown())
		{
			//is this really the mouse position?
			//CInput::Controller.ReadRawDelta();
			std::pair<INT, INT> mousePos = CInput::Controller.GetMousePosition();
			CustomType::Vector2 mousePosVec(mousePos.first, mousePos.second);
			std::vector<CGameObject*> listObject;
			CGameObject* PickedGameObject = NULL;
			listObject = GameObjectUtility::GetGameObjectsHasBoundingBox();

			std::vector<std::pair<CPickedMesh2D, CGameObject*>> pickedMesh2Des;
			CCamera* camera = CManager::GetScene()->GetMainCamera<CCamera>();

			for (const auto& obj : listObject)
			{
				if (obj != NULL)
				{
					CustomType::Vector3 boundMin;
					CustomType::Vector3 boundMax;
					obj->GetRenderWorldAABBBoundingBox(boundMin, boundMax);

					//Convert to Screen
					CustomType::Vector4 posMin(boundMin.X(), boundMin.Y(), boundMin.Z(), 1.f);
					CustomType::Vector4 posMax(boundMax.X(), boundMax.Y(), boundMax.Z(), 1.f);
					{
						CustomType::Matrix4x4 MatrixVP = camera->GetViewProjectionMatrix();
						posMin = MatrixVP.MultiplyVector(posMin);
						posMax = MatrixVP.MultiplyVector(posMax);
						posMin /= posMin.W(); posMax /= posMax.W();
						boundMin = CustomType::Vector3(posMin.GetXMVECTOR());
						boundMax = CustomType::Vector3(posMax.GetXMVECTOR());
						CustomStruct::CRenderViewport vp = camera->GetViewport();
						boundMin = (boundMin * CustomType::Vector3(0.5f, -0.5f, 1.f) + CustomType::Vector3(0.5f, 0.5f, 0.f)) * CustomType::Vector3(vp.Width, vp.Height, 1.f) + CustomType::Vector3(vp.TopLeftX, vp.TopLeftY, 0.f);
						boundMax = (boundMax * CustomType::Vector3(0.5f, -0.5f, 1.f) + CustomType::Vector3(0.5f, 0.5f, 0.f)) * CustomType::Vector3(vp.Width, vp.Height, 1.f) + CustomType::Vector3(vp.TopLeftX, vp.TopLeftY, 0.f);
					}
					pickedMesh2Des.push_back(std::pair(CPickedMesh2D(CustomType::Vector2(boundMin.X(), boundMin.Y()), CustomType::Vector2(boundMax.X(), boundMax.Y())), obj));
				}
			}

			//handle mutily gameobject.
			FLOAT distance = ENGINE_FLOAT32_MAX;
			for (const auto& mesh : pickedMesh2Des)
			{
				if (mesh.first.IsInside(mousePosVec))
				{
					float disTemp = (mesh.second->GetWorldPosition() - camera->GetWorldPosition()).LengthSquare();
					if(distance > disTemp)
						PickedGameObject = mesh.second;
				}
			}

			return PickedGameObject;
		}
		return NULL;
	}

	static CGameObject* GetPickedGameObjectsBySphere()
	{
		if (CInput::Controller.IsLeftMouseButtonDown())
		{
			//is this really the mouse position?
			//CInput::Controller.ReadRawDelta();
			std::pair<INT, INT> mousePos = CInput::Controller.GetMousePosition();
			CustomType::Vector2 mousePosVec(mousePos.first, mousePos.second);
			std::vector<CGameObject*> listObject;
			CGameObject* PickedGameObject = NULL;
			listObject = GameObjectUtility::GetGameObjectsHasBoundingBox();

			std::vector<std::pair<CPickMeshSphere2D, CGameObject*>> CPickMeshSphere2Des;
			CCamera* camera = CManager::GetScene()->GetMainCamera<CCamera>();

			for (const auto& obj : listObject)
			{
				if (obj != NULL)
				{
					CustomType::Vector3 Center;
					FLOAT Raidus;
					obj->GetRenderWorldBoundingSphere(Center, Raidus);

					//Convert to Screen
					CustomType::Vector4 sphereCenter(Center.X(), Center.Y(), Center.Z(), 1.f);
					{
						CustomType::Matrix4x4 MatrixVP = camera->GetViewProjectionMatrix();
						sphereCenter = MatrixVP.MultiplyVector(sphereCenter);
						sphereCenter /= sphereCenter.W();
						Center = CustomType::Vector3(sphereCenter.GetXMVECTOR());
						CustomStruct::CRenderViewport vp = camera->GetViewport();
						Center = (Center * CustomType::Vector3(0.5f, -0.5f, 1.f) + CustomType::Vector3(0.5f, 0.5f, 0.f)) * CustomType::Vector3(vp.Width, vp.Height, 1.f) + CustomType::Vector3(vp.TopLeftX, vp.TopLeftY, 0.f);
					}
					CPickMeshSphere2Des.push_back(std::pair(CPickMeshSphere2D(CustomType::Vector2(Center.X(), Center.Y()), Raidus), obj));
				}
			}

			//handle mutily gameobject.
			FLOAT distance = ENGINE_FLOAT32_MAX;
			for (auto& mesh : CPickMeshSphere2Des)
			{
				if (mesh.first.IsInside(mousePosVec))
				{
					float disTemp = (mesh.second->GetWorldPosition() - camera->GetWorldPosition()).LengthSquare();
					if (distance > disTemp)
						PickedGameObject = mesh.second;
				}
			}

			return PickedGameObject;
		}
		return NULL;
	}
};