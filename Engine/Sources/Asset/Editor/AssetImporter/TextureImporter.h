#pragma once
#include "IImporter.h"
#include <imgui.h>
#if ENGINE_EDITOR_ONLY
namespace PigeonEngine
{
	class ETextureImporter : public IImporter
	{
	public:
		virtual ~ETextureImporter()
		{

		}
		virtual void CreateImportEditor(TArray<EString> InPaths) override;
		virtual EString GetFileFilterList()
		{
			return PE_TEXT("PNG;TGA;JPG;JPEG");
		}
	protected:
		virtual void UpdateImportEditor() override;
	private:
		TArray<EString> Paths;
		TArray<INT32> CubeMapIndex;
		BOOL8 bInitializeEditor = FALSE;
	};
}   // namespace PigeonEngine
#endif
