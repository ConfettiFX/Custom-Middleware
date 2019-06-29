/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include <metal_stdlib>
using namespace metal;

struct Fragment_Shader
{
    struct spaceUniform
    {
        float4x4 ViewProjMat;
        float4 LightDirection;
        float4 ScreenSize;
    };
    constant spaceUniform & SpaceUniform;
    texture2d<float> Depth;
    struct VSOutput
    {
        float4 Position [[position]];
        float4 Normal;
        float4 Color;
        float2 ScreenCoord;
    };
    float hash(float n)
    {
        return fract(((sin(n) * cos(((SpaceUniform.LightDirection).w * (float)(0.000010000000)))) * 10000.0));
    };
    float hash(float2 p)
    {
        return fract(((10000.0 * sin((((float)(17.0) * (p).x) + ((p).y * (float)(0.1))))) * ((float)(0.1) + abs(sin((((p).y * (float)(13.0)) + (p).x))))));
    };
    float noise(float x)
    {
        float i = floor(x);
        float f = fract(x);
        float u = ((f * f) * ((float)(3.0) - ((float)(2.0) * f)));
        return mix(hash(i), hash((i + (float)(1.0))), u);
    };
    float noise(float2 x)
    {
        float2 i = floor(x);
        float2 f = fract(x);
        float a = hash(i);
        float b = hash((i + float2(1.0, 0.0)));
        float c = hash((i + float2(0.0, 1.0)));
        float d = hash((i + float2(1.0, 1.0)));
        float2 u = ((f * f) * ((float2)(3.0) - ((float2)(2.0) * f)));
        return ((mix(a, b, (u).x) + (((c - a) * (u).y) * ((float)(1.0) - (u).x))) + (((d - b) * (u).x) * (u).y));
    };
    float noise(float3 x)
    {
        float3 p = floor(x);
        float3 f = fract(x);
        f = (f * f) * (float3(3.0, 3.0, 3.0) - f * 2.0);
		
        float n = (p.x + (p.y * 57.1246)) + (113.1231 * p.z);
		
        return mix(mix(mix(hash((n + (float)(0.0))), hash((n + (float)(1.0))), (f).x), mix(hash((n + (float)(57.0))), hash((n + (float)(58.0))), (f).x), (f).y), mix(mix(hash((n + (float)(113.0))), hash((n + (float)(114.0))), (f).x), mix(hash((n + (float)(170.0))), hash((n + (float)(171.0))), (f).x), (f).y), (f).z);
    };
    float rand(float2 co)
    {
        return fract((sin(dot((co).xy, float2(12.9898005, 78.23300170))) * (float)(43758.546875)));
    };
    float3 addStars(float2 screenSize, float3 fs_UV)
    {
        float time = (SpaceUniform.LightDirection).w;
		
		float a = pow(noise(fs_UV.xyz * 27.385627 * screenSize.x), 3.0) * 0.5;
		float b = pow(noise((float3(100.0, 100.0, 100.0) + (fs_UV.xyz * 15.0 * screenSize.x))), 5.0);
		
        float galaxyClump = (a + b) / 3.5;
        float color = ((galaxyClump * pow(hash((fs_UV).xy), 1500.0)) * (float)(80.0));
        float3 starColor = float3(color, color, color);
        ((starColor).x *= sqrt((noise((fs_UV).xyz) * (float)(1.20000004))));
        ((starColor).y *= sqrt(noise(((fs_UV).xyz * (float3)(4.0)))));
        float2 delta = ((((fs_UV).xy - ((screenSize).xy * (float2)(0.5))) * (float2)((screenSize).y)) * (float2)(1.20000004));
        float radialNoise = mix(1.0, noise((normalize(delta) * (float2)(20.0))), 0.12000000);
        float att = ((float)(0.057) * pow(max(0.0, ((float)(1.0) - ((length(delta) - (float)(0.9)) / (float)(0.9)))), 8.0));
        (starColor += (((float3)(radialNoise) * float3(0.2, 0.3, 0.5)) * (float3)(min(1.0, att))));
        float randSeed = rand((fs_UV).xy);
        return (starColor * (float3)((((sin((randSeed + ((randSeed * time) * (float)(0.05)))) + (float)(1.0)) * (float)(0.4)) + (float)(0.2))));
    };
    float4 main(VSOutput In)
    {
        uint3 LoadCoord = uint3(In.Position.x, In.Position.y, 0);
        float depth = Depth.read(LoadCoord.xy).x;
		
		if ((depth < 1.0))
        {
            discard_fragment();
        }
		
		float3 resultColor = addStars((SpaceUniform.ScreenSize).xy, float3(noise(In.Position.x), noise(In.Position.y), noise(In.Position.x*13.981 + In.Position.y*2.574621  )));
		float alpha = saturate((saturate(((-(SpaceUniform.LightDirection).y) + 0.2)) * 2.0));
        return float4(resultColor, alpha);		
    };

    Fragment_Shader(
					constant spaceUniform & SpaceUniform,
					texture2d<float> Depth) :
					SpaceUniform(SpaceUniform),
					Depth(Depth) {}
};


fragment float4 stageMain(
    Fragment_Shader::VSOutput In [[stage_in]],
    constant Fragment_Shader::spaceUniform & SpaceUniform [[buffer(3)]],
    texture2d<float> depthTexture [[texture(4)]])
{
    Fragment_Shader::VSOutput In0;
    In0.Position = float4(In.Position.xyz, 1.0 / In.Position.w);
    In0.Normal = In.Normal;
    In0.Color = In.Color;
    In0.ScreenCoord = In.ScreenCoord;
    Fragment_Shader main(
    SpaceUniform,
    depthTexture);
    return main.main(In0);
}
