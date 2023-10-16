#pragma once
#include "Base/DataStructure/ObjectBase.h"
namespace PigeonEngine
{
	class EAssetManager : public EManagerBase
	{
		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EAssetManager)
	public:
		void Initialize() override;
		void ShutDown() override;

		void Init();
		void Uninit();
#if _EDITOR_ONLY
	public:
		void EditorUpdate();

	protected:
		void GenerateContentBrowser();

	private:
		EString CurrentSelectedFolder;
#endif

	};

}

