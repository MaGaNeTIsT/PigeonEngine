#pragma once

namespace PigeonEngine
{

#define CLASS_SINGLETON_BODY(_ClassName) \
	private:\
		_ClassName();\
		_ClassName(const _ClassName&) {}\
		~_ClassName();\
		_ClassName& operator=(const _ClassName&) {}\
	public:\
		static _ClassName* GetSingleton()\
		{\
			static _ClassName classSingletonObject;\
			return (&classSingletonObject);\
		}\

};