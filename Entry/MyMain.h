#pragma once

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <io.h>
#include <strstream>
#include <string>
#include <vector>
#include <list>
#include <map>
using namespace std;

#include <d3d11.h>
#include <DirectXMath.h>
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "xaudio2.lib")
using namespace DirectX;

//#include <fbxsdk.h>
//#pragma comment (lib, "libfbxsdk-mt.lib")
//#pragma comment (lib, "libxml2-mt.lib")
//#pragma comment (lib, "zlib-mt.lib")

#include "../Headers/Base/CTimer.h"


#define SCREEN_WIDTH		(1778)
#define SCREEN_HEIGHT		(1000)


HWND	GetWindow();
CTimer	GetTimer();