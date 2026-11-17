#pragma once

namespace PigeonEngine
{

#define CLASS_REMOVE_COPY_BODY(__ClassName) \
	public:\
		__ClassName(const __ClassName&) = delete;\
		__ClassName(__ClassName&&) = delete;\
		__ClassName& operator=(const __ClassName&) = delete;\
		__ClassName& operator=(__ClassName&&) = delete;\

#define CLASS_NOCOPY_BODY(__ClassName) \
	public:\
		__ClassName();\
		~__ClassName();\
	public:\
		__ClassName(const __ClassName&) = delete;\
		__ClassName(__ClassName&&) = delete;\
		__ClassName& operator=(const __ClassName&) = delete;\
		__ClassName& operator=(__ClassName&&) = delete;\

#define CLASS_VIRTUAL_NOCOPY_BODY(__ClassName) \
	public:\
		__ClassName();\
		virtual ~__ClassName();\
	public:\
		__ClassName(const __ClassName&) = delete;\
		__ClassName(__ClassName&&) = delete;\
		__ClassName& operator=(const __ClassName&) = delete;\
		__ClassName& operator=(__ClassName&&) = delete;\

#define CLASS_COPY_BODY(__ClassName) \
	public:\
		__ClassName();\
		~__ClassName();\
	public:\
		__ClassName(const __ClassName& Other);\
		__ClassName(__ClassName&& Other);\
		__ClassName& operator=(const __ClassName& Other);\
		__ClassName& operator=(__ClassName&& Other);\

#define CLASS_VIRTUAL_COPY_BODY(__ClassName) \
	public:\
		__ClassName();\
		virtual ~__ClassName();\
	public:\
		__ClassName(const __ClassName& Other);\
		__ClassName(__ClassName&& Other);\
		__ClassName& operator=(const __ClassName& Other);\
		__ClassName& operator=(__ClassName&& Other);\

#define RENDER_PROXY_CLASS_BODY(__ClassName) \
	public:\
		__ClassName();\
		__ClassName(const __ClassName& Other);\
		virtual ~__ClassName();\
		__ClassName(__ClassName&& Other) = delete;\
		__ClassName& operator=(const __ClassName&) = delete;\
		__ClassName& operator=(__ClassName&&) = delete;\

#define CLASS_SINGLETON_BODY(__ClassName) \
	private:\
		__ClassName();\
		__ClassName(const __ClassName&) {}\
		__ClassName(__ClassName&&) {}\
		~__ClassName();\
		__ClassName& operator=(const __ClassName&) { return (*this); }\
		__ClassName& operator=(__ClassName&&) { return (*this); }\
	public:\
		static __ClassName* GetSingleton()\
		{\
			static __ClassName _StaticSingletonObject;\
			return (&_StaticSingletonObject);\
		}\

#define CLASS_VIRTUAL_SINGLETON_BODY(__ClassName) \
	private:\
		__ClassName();\
		__ClassName(const __ClassName&) {}\
		__ClassName(__ClassName&&) {}\
		virtual ~__ClassName();\
		__ClassName& operator=(const __ClassName&) { return (*this); }\
		__ClassName& operator=(__ClassName&&) { return (*this); }\
	public:\
		static __ClassName* GetSingleton()\
		{\
			static __ClassName _StaticSingletonObject;\
			return (&_StaticSingletonObject);\
		}\

#define CLASS_MANAGER_SINGLETON_BODY(__ClassName) \
	public:\
		__ClassName(const __ClassName&) = delete;\
		__ClassName(__ClassName&&) = delete;\
		__ClassName& operator=(const __ClassName&) = delete;\
		__ClassName& operator=(__ClassName&&) = delete;\
	private:\
		__ClassName();\
		~__ClassName();\
	public:\
		static __ClassName* GetManagerSingleton()\
		{\
			static __ClassName _StaticSingletonObject;\
			return (&_StaticSingletonObject);\
		}\

#define CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(__ClassName) \
	public:\
		__ClassName(const __ClassName&) = delete;\
		__ClassName(__ClassName&&) = delete;\
		__ClassName& operator=(const __ClassName&) = delete;\
		__ClassName& operator=(__ClassName&&) = delete;\
	private:\
		__ClassName();\
		virtual ~__ClassName();\
	public:\
		static __ClassName* GetManagerSingleton()\
		{\
			static __ClassName _StaticSingletonObject;\
			return (&_StaticSingletonObject);\
		}\

};
