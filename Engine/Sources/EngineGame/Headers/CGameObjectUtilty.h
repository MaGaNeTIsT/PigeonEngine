#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineBase/Headers/CInput.h"
#include "../../EngineGame/Headers/CGameObject.h"
#include "../../EngineGame/Headers/CScene.h"
#include "../../EngineBase/Headers/CManager.h"

namespace GameObjectUtility
{

	template <class T>
	std::vector<CGameObject*> GetGameObjectsByComponent(const UINT& layout)
	{
		if (layout >= CScene::LAYOUT_COUNT)
		{
			return NULL;
		}
		std::vector<CGameObject*> listObj;
		if (CManager::GetScene()->m_GameObjects[layout].size() < 1)
		{
			return listObj;
		}
		for (const auto& obj : (CManager::GetScene()->m_GameObjects[layout]))
		{
			if ((obj.second) != NULL)
			{
				if (obj.second->GetFirstComponentByType<T>())
				{
					listObj.push_back(obj.second));
				}
			}
		}
		return listObj;
	}

	template <class T>
	std::vector<CGameObject*> GetGameObjectsByComponent()
	{
		std::vector<CGameObject*> listObj;
		for (UINT i = 0; i < CScene::LAYOUT_COUNT; i++)
		{
			std::vector<CGameObject*> listObjTemp;
			listObjTemp = GetGameObjectsByComponent<T>(i);
			listObj.resize(listObj.size() + listObjTemp.size());
			listObj.insert(listObj.end(), listObjTemp.begin(), listObjTemp.end());
		}
		return listObj;
	}

};