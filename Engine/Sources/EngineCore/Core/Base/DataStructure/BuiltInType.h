#pragma once

namespace PigeonEngine
{

	typedef	char				BOOL8;
	typedef	char				INT8;
	typedef	char				CHAR;
	typedef	unsigned char		UINT8;
	typedef	unsigned char		BYTE;
	typedef	short				BOOL16;
	typedef	short				SHORT;
	typedef	unsigned short		USHORT;
	typedef	unsigned short		WORD;
	typedef	int					BOOL;
	typedef	int					BOOL32;
	typedef	int					INT;
	typedef	int					INT32;
	typedef	unsigned int		UINT;
	typedef	unsigned int		UINT32;
	typedef	long				BOOL64;
	typedef	long				LONG;
	typedef	long				INT64;
	typedef	unsigned long		ULONG;
	typedef	unsigned long		UINT64;
	typedef	unsigned long		DWORD;
	typedef	unsigned long long	size_t;
	typedef	unsigned long long	SIZE_T;
	typedef	float				FLOAT;
	typedef	double				DOUBLE;

#define TRUE						(1)
#define FALSE						(0)

#define ENGINE_INLINE				inline
#define ENGINE_NODISCARD			[[nodiscard]]
#define ENGINE_FLOAT32_MAX			(3.402823466e+38f)

};