/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#ifndef	SKY_COMMON_H
#define	SKY_COMMON_H

#if !defined(VULKAN) || defined(__linux__) 
static const int TRANSMITTANCE_INTEGRAL_SAMPLES = 500;
static const int INSCATTER_INTEGRAL_SAMPLES = 50;
static const int IRRADIANCE_INTEGRAL_SAMPLES = 32;
static const int INSCATTER_SPHERICAL_INTEGRAL_SAMPLES = 16;

#if defined(METAL) || defined(__linux__) 
static const float m_PI = 3.141592657f;
#else
static const float M_PI = 3.141592657f;
#endif

static const float Rg = 6360.0f;
static const float Rt = 6420.0f;
static const float RL = 6421.0f;

static const int TRANSMITTANCE_W = 256;
static const int TRANSMITTANCE_H = 64;

static const int SKY_W = 64;
static const int SKY_H = 16;

static const int RES_R = 32;
static const int RES_MU = 128;
static const int RES_MU_S = 32;
static const int RES_NU = 8;

static const float ISun = 100.0f;
#else
const int TRANSMITTANCE_INTEGRAL_SAMPLES = 500;
const int INSCATTER_INTEGRAL_SAMPLES = 50;
const int IRRADIANCE_INTEGRAL_SAMPLES = 32;
const int INSCATTER_SPHERICAL_INTEGRAL_SAMPLES = 16;

const float M_PI = 3.141592657f;

const float Rg = 6360.0f;
const float Rt = 6420.0f;
const float RL = 6421.0f;

const int TRANSMITTANCE_W = 256;
const int TRANSMITTANCE_H = 64;

const int SKY_W = 64;
const int SKY_H = 16;

const int RES_R = 32;
const int RES_MU = 128;
const int RES_MU_S = 32;
const int RES_NU = 8;

const float ISun = 100.0f;
#endif

#endif
