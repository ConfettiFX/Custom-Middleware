/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#pragma once

#include "../../../The-Forge/Common_3/OS/Math/MathTypes.h"
#include <stdint.h>

enum PerlinInterpolate { LINEAR, COSINE, CUBIC };

#define INTERP_METHOD PerlinInterpolate::CUBIC
#define OCTAVES 16
#define PERSISTANCE 0.5
#define SCALE2D 3.0f
#define SCALE3D 5.0f
#define OVERALLSCALE 100.0f

class Perlin
{
public:
  Perlin() {};
  ~Perlin() {};

  static float noise2D(const int32_t x, const int32_t y);
  static float noise3D(const int32_t x, const int32_t y, const int32_t z);

  static float interpolate(const float, const float, const float, const float, const float);
  static float linearInterpolate(const float, const float, const float);
  static float cosineInterpolate(const float, const float, const float);
  static float cubicInterpolate(const float, const float, const float, const float, const float);

  static float smoothedNoise2D(const int32_t, const int32_t);
  static float smoothedNoise3D(const int32_t x, const int32_t y, const int32_t z);
  static float interpolatedNoise2D(const float, const float);
  static float interpolatedNoise3D(const float x, const float y, const float z);
  static float perlinNoise2D(const float, const float);
  static float perlinNoise3D(const float x, const float y, const float z);
  static float clamp(const float, const float, const float);
};
