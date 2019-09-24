/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core

#extension GL_GOOGLE_include_directive : require
#include "RenderSky.h"

layout(location = 0) out vec3 vertOutput_TexCoord;
layout(location = 1) out vec2 vertOutput_TexCoord1;

layout(std430, UPDATE_FREQ_NONE, binding = 6) restrict buffer TransmittanceColor_Block {
 	 vec4 TransmittanceColor_Data[];
}TransmittanceColor;

struct PsIn
{
    vec4 position;
    vec3 texCoord;
    vec2 ScreenCoord;
};
PsIn HLSLmain(uint VertexID)
{
    PsIn Out;
    vec4 position;
    ((position).x = float ((((VertexID == uint (0)))?(float (3.0)):(float ((-1.0))))));
    ((position).y = float ((((VertexID == uint (2)))?(float (3.0)):(float ((-1.0))))));
    ((position).zw = vec2 (1.0));
    ((Out).position = position);

    ((position).z = float (0.0));
    ((position).w = (RenderSkyUniformBuffer.QNNear).z);
    ((position).xy *= vec2 ((position).w));

    ((Out).texCoord = (((RenderSkyUniformBuffer.invProj)*(position))).xyz);
    ((Out).texCoord /= vec3 (((Out).texCoord).z));

    (Out).texCoord = mat3(RenderSkyUniformBuffer.invView)*(Out).texCoord;

    ((Out).ScreenCoord = ((((Out).position).xy * vec2(0.5, (-0.5))) + vec2 (0.5)));

    vec3 ray = (RenderSkyUniformBuffer.LightDirection).xyz;
    vec3 x = (RenderSkyUniformBuffer.CameraPosition).xyz;
    vec3 v = normalize(ray);

    float r = length(x);
    float mu = (dot(x, v) / r);

    vec4 dayLight = vec4(transmittance(r, mu), 1.0f);
    vec4 nightLight = RenderSkyUniformBuffer.LightIntensity * 0.05f;

    TransmittanceColor.TransmittanceColor_Data[VertexID] = RenderSkyUniformBuffer.LightDirection.y >= 0.2f ? dayLight : mix(nightLight, dayLight, vec4(clamp(RenderSkyUniformBuffer.LightDirection.y / 0.2f, 0.0f, 1.0f)));

    return Out;
}
void main()
{
    uint VertexID;
    VertexID = gl_VertexIndex;
    PsIn result = HLSLmain(VertexID);
    gl_Position = result.position;
    vertOutput_TexCoord = result.texCoord;
    vertOutput_TexCoord1 = result.ScreenCoord;
}