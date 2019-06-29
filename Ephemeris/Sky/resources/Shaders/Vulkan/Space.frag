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
}SpaceUniform;

layout(set = 0, binding = 8) uniform texture2D depthTexture;
struct VSOutput
{
    vec4 Position;
    vec4 Normal;
    vec4 Color;
    vec2 ScreenCoord;
};
float hash(float n)
{
    return fract(((sin(n) * cos(((SpaceUniform.LightDirection).w * float (0.000010000000)))) * 10000.0));
}
float hash(vec2 p)
{
    return fract(((10000.0 * sin(((float (17.0) * (p).x) + ((p).y * float (0.1))))) * (float (0.1) + abs(sin((((p).y * float (13.0)) + (p).x))))));
}
float noise(float x)
{
    float i = floor(x);
    float f = fract(x);
    float u = ((f * f) * (float (3.0) - (float (2.0) * f)));
    return mix(hash(i), hash((i + float (1.0))), u);
}
float noise(vec2 x)
{
    vec2 i = floor(x);
    vec2 f = fract(x);
    float a = hash(i);
    float b = hash((i + vec2(1.0, 0.0)));
    float c = hash((i + vec2(0.0, 1.0)));
    float d = hash((i + vec2(1.0, 1.0)));
    vec2 u = ((f * f) * (vec2 (3.0) - (vec2 (2.0) * f)));
    return ((mix(a, b, (u).x) + (((c - a) * (u).y) * (float (1.0) - (u).x))) + (((d - b) * (u).x) * (u).y));
}
float noise(vec3 x)
{
    vec3 p = floor(x);
    vec3 f = fract(x);
    (f = ((f * f) * (vec3 (3.0) - (vec3 (2.0) * f))));
    float n = (((p).x + ((p).y * float (57.0))) + (float (113.0) * (p).z));
    return mix(mix(mix(hash((n + float (0.0))), hash((n + float (1.0))), (f).x), mix(hash((n + float (57.0))), hash((n + float (58.0))), (f).x), (f).y), mix(mix(hash((n + float (113.0))), hash((n + float (114.0))), (f).x), mix(hash((n + float (170.0))), hash((n + float (171.0))), (f).x), (f).y), (f).z);
}
float rand(vec2 co)
{
    return fract((sin(dot((co).xy, vec2(12.9898005, 78.23300170))) * float (43758.546875)));
}
vec3 addStars(vec2 screenSize, vec3 fs_UV)
{
    float time = (SpaceUniform.LightDirection).w;
    float galaxyClump = (((pow(noise(((fs_UV).xyz * vec3 ((float (30.0) * (screenSize).x)))),float(3.0)) * float (0.5)) + pow(noise((vec3 (100.0) + ((fs_UV).xyz * vec3 ((float (15.0) * (screenSize).x))))),float(5.0))) / float (3.5));
    float color = ((galaxyClump * pow(hash((fs_UV).xy),float(1500.0))) * float (80.0));
    vec3 starColor = vec3(color, color, color);
    ((starColor).x *= sqrt((noise((fs_UV).xyz) * float (1.20000004))));
    ((starColor).y *= sqrt(noise(((fs_UV).xyz * vec3 (4.0)))));
    vec2 delta = ((((fs_UV).xy - ((screenSize).xy * vec2 (0.5))) * vec2 ((screenSize).y)) * vec2 (1.20000004));
    float radialNoise = mix(float (1.0), noise((normalize(delta) * vec2 (20.0))), float (0.12000000));
    float att = (float (0.057) * pow(max(float (0.0), (float (1.0) - ((length(delta) - float (0.9)) / float (0.9)))),float(8.0)));
    (starColor += ((vec3 (radialNoise) * vec3(0.2, 0.3, 0.5)) * vec3 (min(float (1.0), att))));
    float randSeed = rand((fs_UV).xy);
    return (starColor * vec3 ((((sin((randSeed + ((randSeed * time) * float (0.05)))) + float (1.0)) * float (0.4)) + float (0.2))));
}
vec4 HLSLmain(VSOutput In)
{
    ivec3 LoadCoord = ivec3(ivec2 ((In).Position), 0);
    float depth = float ((texelFetch(depthTexture, ivec2(LoadCoord).xy, (LoadCoord).z)).x);
    if((depth < 1.0))
    {
        discard;
    }
    return vec4(addStars((SpaceUniform.ScreenSize).xy, ((In).Position).xyz), clamp((clamp(((-(SpaceUniform.LightDirection).y) + 0.2), 0.0, 1.0) * 2.0), 0.0, 1.0));  
}
void main()
{
    VSOutput In;
    In.Position = vec4(gl_FragCoord.xyz, 1.0 / gl_FragCoord.w);
    In.Normal = fragInput_NORMAL;
    In.Color = fragInput_COLOR;
    In.ScreenCoord = fragInput_TexCoord;
    vec4 result = HLSLmain(In);
    rast_FragData0 = result;
}
