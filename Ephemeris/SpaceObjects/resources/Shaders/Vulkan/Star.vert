/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core

layout(location = 0) in vec4 TEXCOORD0;
layout(location = 1) in vec4 TEXCOORD1;
layout(location = 2) in vec4 TEXCOORD2;

layout(location = 0) out vec3 vertOutput_Position;
layout(location = 1) out vec4 vertOutput_Color;
layout(location = 2) out vec4 vertOutput_StarInfo;

void main()
{
  vertOutput_Position = TEXCOORD0.xyz;
  vertOutput_Color    = TEXCOORD1;
  vertOutput_StarInfo = TEXCOORD2;
}