#pragma once

#include "../../../../Entry/EngineMain.h"
#include "./CComponent.h"
#include "./CGameBoundComponent.h"
#include "./CGameObject.h"
#include "./CScene.h"

namespace GameObjectUtility
{
	BOOL GetObjectHasBoundingBoxInView(CScene* scene, const UINT& layout, std::vector<std::pair<CGameObject*, CGameBoundBaseComponent*>>& list)
	{
		if (layout >= CScene::SceneLayout::LAYOUT_COUNT)
		{
			return FALSE;
		}
		if (list.size() > 0)
		{
			list.clear();
		}
		std::vector<CGameObject*> objects = scene->GetGameObjectAll(layout);
		for (const auto& obj : objects)
		{
			if ((obj) != NULL)
			{
				if (obj->HasGameBoundComponent())
				{
					CGameBoundBaseComponent* boundComponent = obj->GetGameBoundComponent<CGameBoundBaseComponent>();
					std::pair<CGameObject*, CGameBoundBaseComponent*> tempObj(obj, boundComponent);
					list.push_back(tempObj);
				}
			}
		}
		return (list.size() > 0);
	}

	std::vector<std::pair<CGameObject*, CGameBoundBaseComponent*>> GetGameObjectsHasBoundingBoxInView(CScene* scene)
	{
		std::vector<std::pair<CGameObject*, CGameBoundBaseComponent*>> listObj;
		for (UINT i = 0; i < CScene::LAYOUT_COUNT; i++)
		{
			std::vector<std::pair<CGameObject*, CGameBoundBaseComponent*>> listObjTemp;
			if (GetObjectHasBoundingBoxInView(scene, i, listObjTemp))
			{
				auto iterStart = listObj.end();
				listObj.resize(listObj.size() + listObjTemp.size());
				listObj.insert(iterStart, listObjTemp.begin(), listObjTemp.end());
			}
		}
		return listObj;
	}

};