#pragma once

namespace PigeonEngine
{

#define CLASS_REMOVE_COPY_BODY(_ClassName) \
	public:\
		_ClassName(const _ClassName&) = delete;\
		_ClassName& operator=(const _ClassName&) = delete;\

#define CLASS_NOCOPY_BODY(_ClassName) \
	public:\
		_ClassName();\
		~_ClassName();\
	public:\
		_ClassName(const _ClassName&) = delete;\
		_ClassName& operator=(const _ClassName&) = delete;\

#define CLASS_VIRTUAL_NOCOPY_BODY(_ClassName) \
	public:\
		_ClassName();\
		virtual ~_ClassName();\
	public:\
		_ClassName(const _ClassName&) = delete;\
		_ClassName& operator=(const _ClassName&) = delete;\

#define CLASS_COPY_BODY(_ClassName) \
	public:\
		_ClassName();\
		~_ClassName();\
	public:\
		_ClassName(const _ClassName& Other);\
		_ClassName& operator=(const _ClassName& Other);\

#define CLASS_VIRTUAL_COPY_BODY(_ClassName) \
	public:\
		_ClassName();\
		virtual ~_ClassName();\
	public:\
		_ClassName(const _ClassName& Other);\
		_ClassName& operator=(const _ClassName& Other);\

#define RENDER_PROXY_CLASS_BODY(_ClassName) \
	public:\
		_ClassName();\
		_ClassName(const _ClassName& Other);\
		virtual ~_ClassName();\
		_ClassName& operator=(const _ClassName&) = delete;\

#define CLASS_SINGLETON_BODY(_ClassName) \
	private:\
		_ClassName();\
		_ClassName(const _ClassName&) {}\
		~_ClassName();\
		_ClassName& operator=(const _ClassName&) {}\
	public:\
		static _ClassName* GetSingleton()\
		{\
			static _ClassName _StaticSingletonObject;\
			return (&_StaticSingletonObject);\
		}\

#define CLASS_VIRTUAL_SINGLETON_BODY(_ClassName) \
	private:\
		_ClassName();\
		_ClassName(const _ClassName&) {}\
		virtual ~_ClassName();\
		_ClassName& operator=(const _ClassName&) {}\
	public:\
		static _ClassName* GetSingleton()\
		{\
			static _ClassName _StaticSingletonObject;\
			return (&_StaticSingletonObject);\
		}\

#define CLASS_MANAGER_SINGLETON_BODY(_ClassName) \
	public:\
		_ClassName(const _ClassName&) = delete;\
		_ClassName& operator=(const _ClassName&) = delete;\
	private:\
		_ClassName();\
		~_ClassName();\
	public:\
		static _ClassName* GetManagerSingleton()\
		{\
			static _ClassName _StaticSingletonObject;\
			return (&_StaticSingletonObject);\
		}\

#define CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(_ClassName) \
	public:\
		_ClassName(const _ClassName&) = delete;\
		_ClassName& operator=(const _ClassName&) = delete;\
	private:\
		_ClassName();\
		virtual ~_ClassName();\
	public:\
		static _ClassName* GetManagerSingleton()\
		{\
			static _ClassName _StaticSingletonObject;\
			return (&_StaticSingletonObject);\
		}\

};