#pragma once

#include "../../Core/Headers/Main.h"

namespace PigeonEngine
{

	class EReadFile
	{
	public:
		static void _Check_Default(const std::string& inCaption, const std::string& inText, const BOOL& condition);
	public:
		EReadFile() = delete;
		EReadFile(const EReadFile&) = delete;
		~EReadFile() {}
	public:
		EReadFile& operator=(const EReadFile&) = delete;
	};

};