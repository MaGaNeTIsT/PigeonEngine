#include "AssetManager.h"
namespace PigeonEngine
{
	static void RegisterClassTypes()
	{
		RegisterClassType<EAssetManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	EAssetManager::EAssetManager()
	{
	}

	EAssetManager::~EAssetManager()
	{

	}
	void EAssetManager::Initialize()
	{

	}

	void EAssetManager::ShutDown()
	{

	}

	void EAssetManager::Init()
	{
		
	}

	void EAssetManager::Uninit()
	{

	}

	void EAssetManager::EditorUpdate()
	{
		GenerateContentBrowser();
	}

	void EAssetManager::GenerateContentBrowser()
	{

	}

}
