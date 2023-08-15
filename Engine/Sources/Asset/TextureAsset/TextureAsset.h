#pragma once

#include <CoreMinimal.h>
#include <RenderCommon.h>
#include <wincodec.h>
#include <wincodecsdk.h>
#pragma comment (lib, "windowscodecs.lib")

namespace PigeonEngine
{

	class ETexture : public EObjectBase
	{

		EClass(ETexture, EObjectBase)

	public:

		CLASS_VIRTUAL_COPY_BODY(ETexture)

		ETexture(const EString& InTexName);

	};

};