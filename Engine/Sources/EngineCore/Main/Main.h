#pragma once

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <wrl/client.h>
#include <stdio.h>
#include <ctime>
#include <assert.h>
#include <io.h>
#include <tchar.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <memory>
#include <functional>

#include <d3d11.h>
#include <DirectXMath.h>
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "xaudio2.lib")

#ifdef _DEVELOPMENT_EDITOR
#include "../../../ThirdParty/imgui/imgui.h"
#endif