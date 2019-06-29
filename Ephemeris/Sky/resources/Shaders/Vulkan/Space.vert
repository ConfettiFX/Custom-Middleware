/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core

layout(location = 0) in vec4 POSITION;
layout(location = 1) in vec4 NORMAL;
layout(location = 0) out vec4 vertOutput_NORMAL;
layout(location = 1) out vec4 vertOutput_COLOR;
layout(location = 2) out vec2 vertOutput_TexCoord;

layout(set = 0, binding = 7) uniform cbRootConstant_Block
{
  mat4 ViewProjMat;
  vec4 LightDirection;
  vec4 ScreenSize;
}SpaceUniform;

struct VSInput
{
  vec4 Position;
  vec4 Normal;
};
struct VSOutput
{
  vec4 Position;
  vec4 Normal;
  vec4 Color;
  vec2 ScreenCoord;
};
VSOutput HLSLmain(VSInput Input)
{
  VSOutput result;
  (((Input).Position).xyz *= vec3(1.000000e+08));
  ((result).Position = ((SpaceUniform.ViewProjMat)*((Input).Position)));
  ((result).Normal = (Input).Normal);
  ((result).ScreenCoord = ((((result).Position).xy * vec2(0.5, (-0.5))) + vec2(0.5)));
  return result;
}
void main()
{
  VSInput Input;
  Input.Position = POSITION;
  Input.Normal = NORMAL;
  VSOutput result = HLSLmain(Input);
  gl_Position = result.Position;
  vertOutput_NORMAL = result.Normal;
  vertOutput_COLOR = result.Color;
  vertOutput_TexCoord = result.ScreenCoord;
}