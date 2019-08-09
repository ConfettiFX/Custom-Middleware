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
        float4 NebulaHighColor;
        float4 NebulaMidColor;
        float4 NebulaLowColor;
    };
    constant spaceUniform & SpaceUniform;
    texture2d<float> Depth;
    struct VSOutput
    {
        float4 Position [[position]];
        float4 Normal;
        float4 Info;
        float2 ScreenCoord;
    };

    float4 main(VSOutput In)
    {
        uint3 LoadCoord = uint3(In.Position.x, In.Position.y, 0);
        float depth = Depth.read(LoadCoord.xy).x;
		
		    if ((depth < 1.0))
            {
                discard_fragment();
            }

        float3 resultSpaceColor = (SpaceUniform.NebulaLowColor.rgb * SpaceUniform.NebulaLowColor.a * In.Info.x) + (SpaceUniform.NebulaMidColor.rgb * SpaceUniform.NebulaLowColor.a * In.Info.y) + (SpaceUniform.NebulaHighColor.rgb * SpaceUniform.NebulaHighColor.a * In.Info.z);
        float alpha = clamp(clamp(-SpaceUniform.LightDirection.y + 0.2f, 0.0f, 1.0f) * 2.0f, 0.0f, 1.0f);
        return float4(resultSpaceColor.rgb * alpha, alpha);
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
    In0.Info = In.Info;
    In0.ScreenCoord = In.ScreenCoord;
    Fragment_Shader main(
    SpaceUniform,
    depthTexture);
    return main.main(In0);
}
