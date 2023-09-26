#pragma once

#ifndef ENABLE_JOLTPHYSICS
#define ENABLE_JOLTPHYSICS 1
#endif

#ifdef ENABLE_JOLTPHYSICS
//Jolt define
#define JPH_PROFILE_ENABLED
//#define JPH_DEBUG_RENDERER
#define JPH_FLOATING_POINT_EXCEPTIONS_ENABLED
#define JPH_USE_AVX2
//#define JPH_USE_AVX
//#define JPH_USE_SSE4_1
//#define JPH_USE_SSE4_2
#define JPH_USE_LZCNT
#define JPH_USE_TZCNT
#define JPH_USE_F16C
#define JPH_USE_FMADD

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
//JPH_SUPPRESS_WARNINGS
#endif