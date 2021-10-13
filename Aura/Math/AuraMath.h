/*
 * Copyright © 2018-2021 Confetti Interactive Inc.
 *
 * This is a part of Aura.
 * 
 * This file(code) is licensed under a 
 * Creative Commons Attribution-NonCommercial 4.0 International License 
 *
 *   (https://creativecommons.org/licenses/by-nc/4.0/legalcode) 
 *
 * Based on a work at https://github.com/ConfettiFX/The-Forge.
 * You may not use the material for commercial purposes.
 *
*/

#ifndef __AURAMATH_H_A375A29C_6EE6_40FD_AC17_612483470A76_INCLUDED__
#define __AURAMATH_H_A375A29C_6EE6_40FD_AC17_612483470A76_INCLUDED__

#include <math.h>
#include <cmath>

//	Igor: doing this to be able to instantiate vector min/max later
#ifdef min
#undef min
#undef max

// Tim: building with VS2012 for XBOX using our Renderer produces a re-naming error with this
#if !defined(XBOX) && !defined(_WINDOWS)
template <class T>
T min(const T &x, const T &y) { return (x < y) ? x : y; }
template <class T>
T max(const T &x, const T &y) { return (x > y) ? x : y; }
#endif  // XBOX

#endif

namespace aura
{

	//	TODO: replace this define with static const int???
#define PI 3.14159265358979323846f

	//namespace{
	//const float PI = 3.14159265358979323846f;
	//};


	inline float rsqrtf(const float v) {
		union {
			float vh;
			int i0;
		};

		union {
			float vr;
			int i1;
		};

		vh = v * 0.5f;
		i1 = 0x5f3759df - (i0 >> 1);
		return vr * (1.5f - vh * vr * vr);
	}


	inline float sqrf(const float x) {
		return x * x;
	}

	inline float sincf(const float x) {
		return (x == 0) ? 1 : sinf(x) / x;
	}

	inline float roundf(float x) { return floorf((x)+0.5f); }

	//	Igor: use reference as argument since this function will be inlined anyway
	template <class T>
	T min(const T &x, const T &y) { return (x < y) ? x : y; }
	template <class T>
	T max(const T &x, const T &y) { return (x > y) ? x : y; }

	inline float intAdjustf(const float x, const float diff = 0.01f) {
		float f = roundf(x);

		return (fabsf(f - x) < diff) ? f : x;
	}

	inline unsigned int getClosestPowerOfTwo(const unsigned int x) {
		unsigned int i = 1;
		while (i < x) i += i;

		if (4 * x < 3 * i) i >>= 1;
		return i;
	}

	inline unsigned int getUpperPowerOfTwo(const unsigned int x) {
		unsigned int i = 1;
		while (i < x) i += i;

		return i;
	}

	inline unsigned int getLowerPowerOfTwo(const unsigned int x) {
		unsigned int i = 1;
		while (i <= x) i += i;

		return i >> 1;
	}

	inline int round(float x) {
		if (x > 0) {
			return int(x + 0.5f);
		}
		else {
			return int(x - 0.5f);
		}
	}

}

#endif //__AURAMATH_H_A375A29C_6EE6_40FD_AC17_612483470A76_INCLUDED__
