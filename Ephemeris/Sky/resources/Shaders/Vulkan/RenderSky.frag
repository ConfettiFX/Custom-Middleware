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
#extension GL_EXT_samplerless_texture_functions : enable

#include "RenderSky.h"

layout(location = 0) in vec3 fragInput_TexCoord;
layout(location = 1) in vec2 fragInput_TexCoord1;
layout(location = 0) out vec4 rast_FragData0;

struct PsIn
{
    vec4 position;
    vec3 texCoord;
    vec2 ScreenCoord;
};

float DepthLinearization(float depth)
{
    return ((float (2.0) * (RenderSkyUniformBuffer.QNNear).z) / (((RenderSkyUniformBuffer.QNNear).w + (RenderSkyUniformBuffer.QNNear).z) - (depth * ((RenderSkyUniformBuffer.QNNear).w - (RenderSkyUniformBuffer.QNNear).z))));
}

vec4 HLSLmain(PsIn In)
{
  //return vec4(In.texCoord, 0.0);

    ivec3 LoadCoord = ivec3(ivec2 ((In).position), 0);
    vec4 OriginalImage = texelFetch(SceneColorTexture, ivec2(LoadCoord).xy, (LoadCoord).z);
    float depth = float ((texelFetch(Depth, ivec2(LoadCoord).xy, (LoadCoord).z)).x);
    vec3 ReduceOriginalImage = ((OriginalImage).rgb * vec3 (0.3));
    float fixedDepth = (((-(RenderSkyUniformBuffer.QNNear).y) * (RenderSkyUniformBuffer.QNNear).x) / (depth - (RenderSkyUniformBuffer.QNNear).x));
    vec3 ray = (In).texCoord;
    vec3 x = (RenderSkyUniformBuffer.CameraPosition).xyz;
    vec3 v = normalize(ray);
    float r = length(x);
    float mu = (dot(x, v) / r);
    float VoRay = dot(v, ray);
    float t = (((depth < 1.0))?((VoRay * fixedDepth)):(0.0));
    vec3 attenuation;
    vec3 inscatterColor = inscatter(x, t, v, (RenderSkyUniformBuffer.LightDirection).xyz, r, mu, attenuation);
  
   

    vec3 _groundColor = vec3(0, 0, 0);
    vec3 _sunColor = sunColor(x, t, v, (RenderSkyUniformBuffer.LightDirection).xyz, r, mu);
    vec3 _spaceColor = vec3(0, 0, 0);
    if((t <= float (0.0)))
    {
        vec3 _transmittance = (((r <= Rt))?(transmittance(r, mu)):(vec3 ((1.0).xxx)));
        (_spaceColor = ((ReduceOriginalImage).xyz * _transmittance));
    }
    else
    {
        (_groundColor = (((ReduceOriginalImage).xyz * vec3 (ISun)) / vec3 (M_PI)));
    }
    (inscatterColor *= vec3 (((RenderSkyUniformBuffer.InScatterParams).x + (RenderSkyUniformBuffer.InScatterParams).y)));
    return vec4(((HDR_NORM(_groundColor) + HDR((_spaceColor + inscatterColor))) + (OriginalImage).rgb), 0.0);
}
void main()
{
    PsIn In;
    In.position = vec4(gl_FragCoord.xyz, 1.0 / gl_FragCoord.w);
    In.texCoord = fragInput_TexCoord;
    In.ScreenCoord = fragInput_TexCoord1;
    vec4 result = HLSLmain(In);
    rast_FragData0 = result;
}