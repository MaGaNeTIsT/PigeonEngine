#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineBase/Headers/CInput.h"
#include "../../EngineGame/Headers/CGameObject.h"
#include "../../EngineGame/Headers/CScene.h"
#include "../../EngineBase/Headers/CManager.h"

namespace GameObjectUtility
{

	std::vector<CGameObject*> GetGameObjectsHasBoundingBox(const UINT& layout)
	{
		if (layout >= CScene::LAYOUT_COUNT)
		{
			return std::vector<CGameObject*>();
		}
		std::vector<CGameObject*> listObj;
		std::vector<CGameObject*> LayerObj = CManager::GetScene()->GetGameObjectAll(layout);
		for (const auto& obj : LayerObj)
		{
			if ((obj) != NULL)
			{
				if (obj->HasRenderBoundingBox())
				{
					listObj.push_back(obj);
				}
			}
		}
		return listObj;
	}

	std::vector<CGameObject*> GetGameObjectsHasBoundingBox()
	{
		std::vector<CGameObject*> listObj;
		for (UINT i = 0; i < CScene::LAYOUT_COUNT; i++)
		{
			std::vector<CGameObject*> listObjTemp;
			listObjTemp = GetGameObjectsHasBoundingBox(i);
			listObj.resize(listObj.size() + listObjTemp.size());
			listObj.insert(listObj.end(), listObjTemp.begin(), listObjTemp.end());
		}
		return listObj;
	}

};