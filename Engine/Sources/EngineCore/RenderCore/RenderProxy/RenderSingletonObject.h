#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class RFullScreenTriangle final
	{
	public:
		void	BindPrimitiveBuffers();
	public:
		RFullScreenTriangle();
		RFullScreenTriangle(const RFullScreenTriangle& Other);
		~RFullScreenTriangle();
		RFullScreenTriangle& operator=(const RFullScreenTriangle& Other);
	};

};