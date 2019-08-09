/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core

#extension GL_EXT_samplerless_texture_functions : enable

layout(location = 0) in vec4 fragInput_NORMAL;
layout(location = 1) in vec4 fragInput_COLOR;
layout(location = 2) in vec2 fragInput_TexCoord;
layout(location = 0) out vec4 rast_FragData0; 

layout(set = 0, binding = 7) uniform cbRootConstant_Block
{
    mat4 ViewProjMat;
    vec4 LightDirection;
    vec4 ScreenSize;
    vec4 NebulaHighColor;
    vec4 NebulaMidColor;
    vec4 NebulaLowColor;
}SpaceUniform;

layout(set = 0, binding = 8) uniform texture2D depthTexture;
struct VSOutput
{
    vec4 Position;
    vec4 Normal;
    vec4 Info;
    vec2 ScreenCoord;
};

vec4 HLSLmain(VSOutput In)
{
    ivec3 LoadCoord = ivec3(ivec2 ((In).Position), 0);
    float depth = float ((texelFetch(depthTexture, ivec2(LoadCoord).xy, (LoadCoord).z)).x);
    if((depth < 1.0))
    {
        discard;
    }

    //vec4 resultSpaceColor = In.Color.x > 0.5f ? mix(SpaceUniform.NebulaMidColor, SpaceUniform.NebulaHighColor, (In.Color.x - 0.5f) * 2.0f) : mix(SpaceUniform.NebulaLowColor, SpaceUniform.NebulaMidColor, In.Color.x * 2.0f);
    vec3 resultSpaceColor = (SpaceUniform.NebulaLowColor.rgb * SpaceUniform.NebulaLowColor.a * In.Info.x) + (SpaceUniform.NebulaMidColor.rgb * SpaceUniform.NebulaLowColor.a * In.Info.y) + (SpaceUniform.NebulaHighColor.rgb * SpaceUniform.NebulaHighColor.a * In.Info.z);
	float alpha = clamp(clamp(-SpaceUniform.LightDirection.y + 0.2f, 0.0, 1.0) * 2.0f, 0.0, 1.0);

    return vec4(resultSpaceColor.rgb * alpha, alpha);
}
void main()
{
    VSOutput In;
    In.Position = vec4(gl_FragCoord.xyz, 1.0 / gl_FragCoord.w);
    In.Normal = fragInput_NORMAL;
    In.Info = fragInput_COLOR;
    In.ScreenCoord = fragInput_TexCoord;
    vec4 result = HLSLmain(In);
    rast_FragData0 = result;
}
