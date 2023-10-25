#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class RFullScreenTriangle final
	{
	public:
		UINT32	GetIndexCount()const;
		void	BindPrimitiveBuffers();
	public:
		RFullScreenTriangle();
		RFullScreenTriangle(const RFullScreenTriangle& Other);
		~RFullScreenTriangle();
		RFullScreenTriangle& operator=(const RFullScreenTriangle& Other);
	};

#if _EDITOR_ONLY
	class RDebugCircleLineList
	{
	public:
		UINT32	GetIndexCount()const;
		void	BindPrimitiveBuffers();
	public:
		RDebugCircleLineList();
		RDebugCircleLineList(const RDebugCircleLineList& Other);
		~RDebugCircleLineList();
		RDebugCircleLineList& operator=(const RDebugCircleLineList& Other);
	};
#endif

};