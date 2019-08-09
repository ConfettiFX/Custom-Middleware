/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core

vec4 MulMat(mat4 lhs, vec4 rhs)
{
    vec4 dst;
	dst[0] = lhs[0][0]*rhs[0] + lhs[0][1]*rhs[1] + lhs[0][2]*rhs[2] + lhs[0][3]*rhs[3];
	dst[1] = lhs[1][0]*rhs[0] + lhs[1][1]*rhs[1] + lhs[1][2]*rhs[2] + lhs[1][3]*rhs[3];
	dst[2] = lhs[2][0]*rhs[0] + lhs[2][1]*rhs[1] + lhs[2][2]*rhs[2] + lhs[2][3]*rhs[3];
	dst[3] = lhs[3][0]*rhs[0] + lhs[3][1]*rhs[1] + lhs[3][2]*rhs[2] + lhs[3][3]*rhs[3];
    return dst;
}


layout(location = 0) in vec3 POSITION;
layout(location = 1) in vec3 NORMAL;
layout(location = 2) in vec3 TEXCOORD;
layout(location = 0) out vec4 vertOutput_NORMAL;
layout(location = 1) out vec4 vertOutput_COLOR;
layout(location = 2) out vec2 vertOutput_TexCoord;

layout(row_major, set = 0, binding = 7) uniform SpaceUniform
{
    mat4 ViewProjMat;
    vec4 LightDirection;
    vec4 ScreenSize;
    vec4 NebulaHighColor;
    vec4 NebulaMidColor;
    vec4 NebulaLowColor;
};

struct VSInput
{
    vec3 Position;
    vec3 Normal;
    vec3 StarInfo;
};
struct VSOutput
{
    vec4 Position;
    vec4 Normal;
    vec4 Info;
    vec2 ScreenCoord;
};
VSOutput HLSLmain(VSInput Input)
{
    VSOutput result;
    ((result).Position = MulMat(ViewProjMat,vec4((Input).Position, 1.0)));
    ((result).Normal = vec4((Input).Normal, 0.0));
    ((result).ScreenCoord = ((((result).Position).xy * vec2(0.5, (-0.5))) + vec2 (0.5)));
    ((result).Info = vec4(((Input).StarInfo).x, ((Input).StarInfo).y, ((Input).StarInfo).z, 1.0));
    return result;
}
void main()
{
    VSInput Input;
    Input.Position = POSITION;
    Input.Normal = NORMAL;
    Input.StarInfo = TEXCOORD;
    VSOutput result = HLSLmain(Input);
    gl_Position = result.Position;
    vertOutput_NORMAL = result.Normal;
    vertOutput_COLOR = result.Info;
    vertOutput_TexCoord = result.ScreenCoord;
}
