/*
* Copyright (c) 2017-2022 The Forge Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#include "Perlin.h"


float Perlin::noise2D(const int32_t x, const int32_t z) {
  //returns floats between -1 and 1
  //TODO: generate these dynamically based on seed, maybe move them to class static members
  const uint8_t prime0 = 13;
  const int32_t prime1 = 15731;
  const int32_t prime2 = 789221;
  const int32_t prime3 = 1376312589;
  const float prime4 = 1073741824.0;
  const int32_t primez = 59;

  int32_t n = x + z * primez;
  n = (n << prime0) ^ n;
  return (1.f - (float)((n * (n * n * prime1 + prime2) + prime3) & 0x7fffffff) / prime4);
}

float Perlin::noise3D(const int32_t x, const int32_t y, const int32_t z) {
  //returns floats between -1 and 1
  //TODO: generate these dynamically based on seed, maybe move them to class static members
  const uint8_t prime0 = 13;
  const int32_t prime1 = 15731;
  const int32_t prime2 = 789221;
  const int32_t prime3 = 1376312589;
  const float prime4 = 1073741824.0;
  const float primez = 59;
  const float primey = 101;

  int32_t n = x + y * (int32_t)primey + z * (int32_t)primez;
  n = (n << prime0) ^ n;
  return (1.f - (float)((n * (n * n * prime1 + prime2) + prime3) & 0x7fffffff) / prime4);
}

float Perlin::linearInterpolate(const float a, const float b, const float u) {
  return  a * (1.f - u) + b * u;
}

float Perlin::cosineInterpolate(const float a, const float b, const float u) {
  float newu = (1.f - cos(u*3.1415927f)) * .5f;
  return  a * (1.f - newu) + b * newu;
}

float Perlin::cubicInterpolate(const float v0, const float v1, const float v2, const float v3, const float u) {
  float P = (v3 - v2) - (v0 - v1);
  float Q = (v0 - v1) - P;
  float R = v2 - v0;
  float S = v1;

  return P * (u*u*u) + Q * (u*u) + R * u + S;
}

float Perlin::smoothedNoise2D(const int32_t x, const int32_t z) {
  float corners = (noise2D(x - 1, z - 1) + noise2D(x + 1, z - 1) + noise2D(x - 1, z + 1) + noise2D(x + 1, z + 1)) / 16.f;
  float sides = (noise2D(x - 1, z) + noise2D(x + 1, z) + noise2D(x, z - 1) + noise2D(x, z + 1)) / 8.f;
  float center = noise2D(x, z) / 4.f;
  return corners + sides + center;
}

float Perlin::smoothedNoise3D(const int32_t x, const int32_t y, const int32_t z) {
  float diags = (noise3D(x - 1, y + 1, z - 1) + noise3D(x + 1, y + 1, z - 1) + noise3D(x + 1, y + 1, z + 1) + noise3D(x - 1, y + 1, z + 1) +
    noise3D(x - 1, y - 1, z - 1) + noise3D(x + 1, y - 1, z - 1) + noise3D(x + 1, y - 1, z + 1) + noise3D(x - 1, y - 1, z + 1)) / 64.f;

  float corners = (noise3D(x - 1, y - 1, z) + noise3D(x + 1, y - 1, z) + noise3D(x - 1, y + 1, z) + noise3D(x + 1, y + 1, z) +
    noise3D(x - 1, y, z - 1) + noise3D(x + 1, y, z - 1) + noise3D(x - 1, y, z + 1) + noise3D(x + 1, y, z + 1) +
    noise3D(x, y - 1, z - 1) + noise3D(x, y + 1, z - 1) + noise3D(x, y - 1, z + 1) + noise3D(x, y + 1, z + 1)) / 32.f;

  float sides = (noise3D(x - 1, y, z) + noise3D(x + 1, y, z) + noise3D(x, y - 1, z) + noise3D(x, y + 1, z) +
    noise3D(x, y, z + 1) + noise3D(x, y, z - 1)) / 16.f;

  float center = noise3D(x, y, z) / 8.f;

  return corners + sides + center + diags;
}

float Perlin::interpolate(const float v0, const float v1, const float v2, const float v3, const float u) {
  float result = 0.f;
  switch (INTERP_METHOD) {
  case LINEAR:
    result = linearInterpolate(v1, v2, u);
    break;
  case COSINE:
    result = cosineInterpolate(v1, v2, u);
    break;
  case CUBIC:
    result = cubicInterpolate(v0, v1, v2, v3, u);
    break;
  default:
    break;
  }
  return result;
}

float Perlin::interpolatedNoise2D(const float x, const float z) {
  int32_t integer_X = (int32_t)x;
  float fractional_X = fabs(x - integer_X);

  int32_t integer_Z = (int32_t)z;
  float fractional_Z = fabs(z - integer_Z);

  float v0_botprev = smoothedNoise2D(integer_X - 1, integer_Z - 1);
  float v1_botprev = smoothedNoise2D(integer_X, integer_Z - 1);

  float v2_botprev = smoothedNoise2D(integer_X + 1, integer_Z - 1);
  float v3_botprev = smoothedNoise2D(integer_X + 2, integer_Z - 1);

  float v0_bot = smoothedNoise2D(integer_X - 1, integer_Z);
  float v1_bot = smoothedNoise2D(integer_X, integer_Z);
  float v2_bot = smoothedNoise2D(integer_X + 1, integer_Z);
  float v3_bot = smoothedNoise2D(integer_X + 2, integer_Z);

  float v0_top = smoothedNoise2D(integer_X - 1, integer_Z + 1);
  float v1_top = smoothedNoise2D(integer_X, integer_Z + 1);
  float v2_top = smoothedNoise2D(integer_X + 1, integer_Z + 1);
  float v3_top = smoothedNoise2D(integer_X + 2, integer_Z + 1);

  float v0_topafter = smoothedNoise2D(integer_X - 1, integer_Z + 2);
  float v1_topafter = smoothedNoise2D(integer_X, integer_Z + 2);
  float v2_topafter = smoothedNoise2D(integer_X + 1, integer_Z + 2);
  float v3_topafter = smoothedNoise2D(integer_X + 2, integer_Z + 2);

  float ix_botprev = interpolate(v0_botprev, v1_botprev, v2_botprev, v3_botprev, fractional_X);
  float ix_bot = interpolate(v0_bot, v1_bot, v2_bot, v3_bot, fractional_X);
  float ix_top = interpolate(v0_top, v1_top, v2_top, v3_top, fractional_X);
  float ix_topafter = interpolate(v0_topafter, v1_topafter, v2_topafter, v3_topafter, fractional_X);

  return interpolate(ix_botprev, ix_bot, ix_top, ix_topafter, fractional_Z);
}

float Perlin::interpolatedNoise3D(const float x, const float y, const float z)
{
  int32_t integer_X = (int32_t)x;
  float fractional_X = fabs(x - integer_X);

  int32_t integer_Y = (int32_t)y;
  float fractional_Y = fabs(y - integer_Y);

  int32_t integer_Z = (int32_t)z;
  float fractional_Z = fabs(z - integer_Z);

  //oh dear god...
  //down1
  float v0_d1_botprev = smoothedNoise3D(integer_X - 1, integer_Y - 1, integer_Z - 1);
  float v1_d1_botprev = smoothedNoise3D(integer_X, integer_Y - 1, integer_Z - 1);
  float v2_d1_botprev = smoothedNoise3D(integer_X + 1, integer_Y - 1, integer_Z - 1);
  float v3_d1_botprev = smoothedNoise3D(integer_X + 2, integer_Y - 1, integer_Z - 1);

  float v0_d1_bot = smoothedNoise3D(integer_X - 1, integer_Y - 1, integer_Z);
  float v1_d1_bot = smoothedNoise3D(integer_X, integer_Y - 1, integer_Z);
  float v2_d1_bot = smoothedNoise3D(integer_X + 1, integer_Y - 1, integer_Z);
  float v3_d1_bot = smoothedNoise3D(integer_X + 2, integer_Y - 1, integer_Z);

  float v0_d1_top = smoothedNoise3D(integer_X - 1, integer_Y - 1, integer_Z + 1);
  float v1_d1_top = smoothedNoise3D(integer_X, integer_Y - 1, integer_Z + 1);
  float v2_d1_top = smoothedNoise3D(integer_X + 1, integer_Y - 1, integer_Z + 1);
  float v3_d1_top = smoothedNoise3D(integer_X + 2, integer_Y - 1, integer_Z + 1);

  float v0_d1_topafter = smoothedNoise3D(integer_X - 1, integer_Y - 1, integer_Z + 2);
  float v1_d1_topafter = smoothedNoise3D(integer_X, integer_Y - 1, integer_Z + 2);
  float v2_d1_topafter = smoothedNoise3D(integer_X + 1, integer_Y - 1, integer_Z + 2);
  float v3_d1_topafter = smoothedNoise3D(integer_X + 2, integer_Y - 1, integer_Z + 2);

  //down0
  float v0_d0_botprev = smoothedNoise3D(integer_X - 1, integer_Y, integer_Z - 1);
  float v1_d0_botprev = smoothedNoise3D(integer_X, integer_Y, integer_Z - 1);
  float v2_d0_botprev = smoothedNoise3D(integer_X + 1, integer_Y, integer_Z - 1);
  float v3_d0_botprev = smoothedNoise3D(integer_X + 2, integer_Y, integer_Z - 1);

  float v0_d0_bot = smoothedNoise3D(integer_X - 1, integer_Y, integer_Z);
  float v1_d0_bot = smoothedNoise3D(integer_X, integer_Y, integer_Z);
  float v2_d0_bot = smoothedNoise3D(integer_X + 1, integer_Y, integer_Z);
  float v3_d0_bot = smoothedNoise3D(integer_X + 2, integer_Y, integer_Z);

  float v0_d0_top = smoothedNoise3D(integer_X - 1, integer_Y, integer_Z + 1);
  float v1_d0_top = smoothedNoise3D(integer_X, integer_Y, integer_Z + 1);
  float v2_d0_top = smoothedNoise3D(integer_X + 1, integer_Y, integer_Z + 1);
  float v3_d0_top = smoothedNoise3D(integer_X + 2, integer_Y, integer_Z + 1);

  float v0_d0_topafter = smoothedNoise3D(integer_X - 1, integer_Y, integer_Z + 2);
  float v1_d0_topafter = smoothedNoise3D(integer_X, integer_Y, integer_Z + 2);
  float v2_d0_topafter = smoothedNoise3D(integer_X + 1, integer_Y, integer_Z + 2);
  float v3_d0_topafter = smoothedNoise3D(integer_X + 2, integer_Y, integer_Z + 2);

  //up1
  float v0_u1_botprev = smoothedNoise3D(integer_X - 1, integer_Y + 1, integer_Z - 1);
  float v1_u1_botprev = smoothedNoise3D(integer_X, integer_Y + 1, integer_Z - 1);
  float v2_u1_botprev = smoothedNoise3D(integer_X + 1, integer_Y + 1, integer_Z - 1);
  float v3_u1_botprev = smoothedNoise3D(integer_X + 2, integer_Y + 1, integer_Z - 1);

  float v0_u1_bot = smoothedNoise3D(integer_X - 1, integer_Y + 1, integer_Z);
  float v1_u1_bot = smoothedNoise3D(integer_X, integer_Y + 1, integer_Z);
  float v2_u1_bot = smoothedNoise3D(integer_X + 1, integer_Y + 1, integer_Z);
  float v3_u1_bot = smoothedNoise3D(integer_X + 2, integer_Y + 1, integer_Z);

  float v0_u1_top = smoothedNoise3D(integer_X - 1, integer_Y + 1, integer_Z + 1);
  float v1_u1_top = smoothedNoise3D(integer_X, integer_Y + 1, integer_Z + 1);
  float v2_u1_top = smoothedNoise3D(integer_X + 1, integer_Y + 1, integer_Z + 1);
  float v3_u1_top = smoothedNoise3D(integer_X + 2, integer_Y + 1, integer_Z + 1);

  float v0_u1_topafter = smoothedNoise3D(integer_X - 1, integer_Y + 1, integer_Z + 2);
  float v1_u1_topafter = smoothedNoise3D(integer_X, integer_Y + 1, integer_Z + 2);
  float v2_u1_topafter = smoothedNoise3D(integer_X + 1, integer_Y + 1, integer_Z + 2);
  float v3_u1_topafter = smoothedNoise3D(integer_X + 2, integer_Y + 1, integer_Z + 2);

  //up2
  float v0_u2_botprev = smoothedNoise3D(integer_X - 1, integer_Y + 2, integer_Z - 1);
  float v1_u2_botprev = smoothedNoise3D(integer_X, integer_Y + 2, integer_Z - 1);
  float v2_u2_botprev = smoothedNoise3D(integer_X + 1, integer_Y + 2, integer_Z - 1);
  float v3_u2_botprev = smoothedNoise3D(integer_X + 2, integer_Y + 2, integer_Z - 1);

  float v0_u2_bot = smoothedNoise3D(integer_X - 1, integer_Y + 2, integer_Z);
  float v1_u2_bot = smoothedNoise3D(integer_X, integer_Y + 2, integer_Z);
  float v2_u2_bot = smoothedNoise3D(integer_X + 1, integer_Y + 2, integer_Z);
  float v3_u2_bot = smoothedNoise3D(integer_X + 2, integer_Y + 2, integer_Z);

  float v0_u2_top = smoothedNoise3D(integer_X - 1, integer_Y + 2, integer_Z + 1);
  float v1_u2_top = smoothedNoise3D(integer_X, integer_Y + 2, integer_Z + 1);
  float v2_u2_top = smoothedNoise3D(integer_X + 1, integer_Y + 2, integer_Z + 1);
  float v3_u2_top = smoothedNoise3D(integer_X + 2, integer_Y + 2, integer_Z + 1);

  float v0_u2_topafter = smoothedNoise3D(integer_X - 1, integer_Y + 2, integer_Z + 2);
  float v1_u2_topafter = smoothedNoise3D(integer_X, integer_Y + 2, integer_Z + 2);
  float v2_u2_topafter = smoothedNoise3D(integer_X + 1, integer_Y + 2, integer_Z + 2);
  float v3_u2_topafter = smoothedNoise3D(integer_X + 2, integer_Y + 2, integer_Z + 2);


  float ix_d1_botprev = interpolate(v0_d1_botprev, v1_d1_botprev, v2_d1_botprev, v3_d1_botprev, fractional_X);
  float ix_d1_bot = interpolate(v0_d1_bot, v1_d1_bot, v2_d1_bot, v3_d1_bot, fractional_X);
  float ix_d1_top = interpolate(v0_d1_top, v1_d1_top, v2_d1_top, v3_d1_top, fractional_X);
  float ix_d1_topafter = interpolate(v0_d1_topafter, v1_d1_topafter, v2_d1_topafter, v3_d1_topafter, fractional_X);

  float ix_d0_botprev = interpolate(v0_d0_botprev, v1_d0_botprev, v2_d0_botprev, v3_d0_botprev, fractional_X);
  float ix_d0_bot = interpolate(v0_d0_bot, v1_d0_bot, v2_d0_bot, v3_d0_bot, fractional_X);
  float ix_d0_top = interpolate(v0_d0_top, v1_d0_top, v2_d0_top, v3_d0_top, fractional_X);
  float ix_d0_topafter = interpolate(v0_d0_topafter, v1_d0_topafter, v2_d0_topafter, v3_d0_topafter, fractional_X);

  float ix_u1_botprev = interpolate(v0_u1_botprev, v1_u1_botprev, v2_u1_botprev, v3_u1_botprev, fractional_X);
  float ix_u1_bot = interpolate(v0_u1_bot, v1_u1_bot, v2_u1_bot, v3_u1_bot, fractional_X);
  float ix_u1_top = interpolate(v0_u1_top, v1_u1_top, v2_u1_top, v3_u1_top, fractional_X);
  float ix_u1_topafter = interpolate(v0_u1_topafter, v1_u1_topafter, v2_u1_topafter, v3_u1_topafter, fractional_X);

  float ix_u2_botprev = interpolate(v0_u2_botprev, v1_u2_botprev, v2_u2_botprev, v3_u2_botprev, fractional_X);
  float ix_u2_bot = interpolate(v0_u2_bot, v1_u2_bot, v2_u2_bot, v3_u2_bot, fractional_X);
  float ix_u2_top = interpolate(v0_u2_top, v1_u2_top, v2_u2_top, v3_u2_top, fractional_X);
  float ix_u2_topafter = interpolate(v0_u2_topafter, v1_u2_topafter, v2_u2_topafter, v3_u2_topafter, fractional_X);

  float iz_d1 = interpolate(ix_d1_botprev, ix_d1_bot, ix_d1_top, ix_d1_topafter, fractional_Z);
  float iz_d0 = interpolate(ix_d0_botprev, ix_d0_bot, ix_d0_top, ix_d0_topafter, fractional_Z);
  float iz_u1 = interpolate(ix_u1_botprev, ix_u1_bot, ix_u1_top, ix_u1_topafter, fractional_Z);
  float iz_u2 = interpolate(ix_u2_botprev, ix_u2_bot, ix_u2_top, ix_u2_topafter, fractional_Z);

  return interpolate(iz_d1, iz_d0, iz_u1, iz_u2, fractional_Y);
}

float Perlin::perlinNoise2D(const float x, const float z) {
  float total = 0.f;
  for (uint32_t i = 0; i < OCTAVES; i++) {
    const uint32_t bit = 1;
    const uint32_t frequency = bit << i; // 2 to the ith power, 1,2,4,8,16,etc.
    const double amplitude = pow(PERSISTANCE, (double)(i));
    total += interpolatedNoise2D(x * frequency, z * frequency) * (float)amplitude;
  }
  total *= SCALE2D;//additional dimension reduces likelyhood that this is 1
  return total;//clamp(total, -1, 1);
}


float Perlin::perlinNoise3D(const float x, const float y, const float z) {
  float total = 0.f;
  for (uint32_t i = 0; i < OCTAVES; i++) {
    const uint32_t bit = 1;
    const uint32_t frequency = bit << i; // 2 to the ith power, 1,2,4,8,16,etc.
    const double amplitude = pow(PERSISTANCE, (double)(i + 1));
    total += interpolatedNoise3D(x * frequency, y * frequency, z * frequency) * (float)amplitude;
  }
  total *= SCALE3D;//additional dimension reduces likelyhood that this is 1
  return clamp(total, -1, 1);
}

float Perlin::clamp(const float value, const float min, const float max) {
  if (value < min) {
    return min;
  }
  else if (value > max) {
    return max;
  }
  else {
    return value;
  }
}
