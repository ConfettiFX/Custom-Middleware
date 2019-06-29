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
#include "volumetricCloud.h"

struct Fragment_Shader
{
	texture3d<float> highFreqNoiseTexture;
	texture3d<float> lowFreqNoiseTexture;
	texture2d<float> curlNoiseTexture;
	texture2d<float> weatherTexture;
	texture2d<float> depthTexture;
	texture2d<float> LowResCloudTexture;
	texture2d<float> g_PrevFrameTexture;
	sampler g_LinearClampSampler;
	sampler g_LinearWrapSampler;
	sampler g_PointClampSampler;
	sampler g_LinearBorderSampler;
	constant volumetricCloud::Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer;
	texture2d<float> g_GodrayTexture;
	
    struct PSIn
    {
        float4 Position [[position]];
        float2 TexCoord;
    };
	
    struct PSOut
    {
        float4 VolumetricClouds;
        float4 ResultColor;
    };
	
    float4 main(PSIn input)
    {
        float2 db_uvs = (input).TexCoord;
        float3 sunWorldPos = VolumetricCloudsCBuffer.g_VolumetricClouds.lightDirection.xyz * 1000000.0;
        float4 sunPos;
        (sunPos = (((VolumetricCloudsCBuffer.g_VolumetricClouds).m_WorldToProjMat_1st)*(float4(sunWorldPos, 1.0))));
        ((sunPos).xy /= (float2)((sunPos).w));
        float2 ScreenNDC = float2(((((input).TexCoord).x * (float)(2.0)) - (float)(1.0)), ((((float)(1.0) - ((input).TexCoord).y) * (float)(2.0)) - (float)(1.0)));
        float3 projectedPosition = float3((ScreenNDC).xy, 0.0);
        float4 worldPos = (((VolumetricCloudsCBuffer.g_VolumetricClouds).m_ProjToWorldMat_1st)*(float4(projectedPosition, 1.0)));
        (worldPos /= (float4)((worldPos).w));
        float4 CameraPosition = (VolumetricCloudsCBuffer.g_VolumetricClouds).cameraPosition_1st;
        float3 viewDir = normalize(((worldPos).xyz - (CameraPosition).xyz));
        float cosTheta = saturate(dot(viewDir, ((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).xyz));
        if (((cosTheta <= 0.0) || ((sunPos).z < (float)(0.0))))
        {
            return float4(0.0, 0.0, 0.0, 0.0);
        }
        float sceneDepth = depthTexture.sample(g_LinearClampSampler, (input).TexCoord, level(0)).r;
        float4 GodrayColor = saturate(g_GodrayTexture.sample(g_LinearClampSampler, (input).TexCoord));
        return pow(GodrayColor, 1.0);
    };

    Fragment_Shader(
					texture3d<float> highFreqNoiseTexture,
					texture3d<float> lowFreqNoiseTexture,
					texture2d<float> curlNoiseTexture,
					texture2d<float> weatherTexture,
					texture2d<float> depthTexture,
					texture2d<float> LowResCloudTexture,
					texture2d<float> g_PrevFrameTexture,
					sampler g_LinearClampSampler,
					sampler g_LinearWrapSampler,
					sampler g_PointClampSampler,
					sampler g_LinearBorderSampler,
					constant volumetricCloud::Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer,
					texture2d<float> g_GodrayTexture) :
					highFreqNoiseTexture(highFreqNoiseTexture),
					lowFreqNoiseTexture(lowFreqNoiseTexture),
					curlNoiseTexture(curlNoiseTexture),
					weatherTexture(weatherTexture),
					depthTexture(depthTexture),
					LowResCloudTexture(LowResCloudTexture),
					g_PrevFrameTexture(g_PrevFrameTexture),
					g_LinearClampSampler(g_LinearClampSampler),
					g_LinearWrapSampler(g_LinearWrapSampler),
					g_PointClampSampler(g_PointClampSampler),
					g_LinearBorderSampler(g_LinearBorderSampler),
					VolumetricCloudsCBuffer(VolumetricCloudsCBuffer),
					g_GodrayTexture(g_GodrayTexture) {}
};


fragment float4 stageMain(
    Fragment_Shader::PSIn input [[stage_in]],
    texture3d<float> highFreqNoiseTexture [[texture(0)]],
    texture3d<float> lowFreqNoiseTexture [[texture(1)]],
    texture2d<float> curlNoiseTexture [[texture(2)]],
    texture2d<float> weatherTexture [[texture(3)]],
    texture2d<float> depthTexture [[texture(4)]],
    texture2d<float> LowResCloudTexture [[texture(5)]],
    texture2d<float> g_PrevFrameTexture [[texture(6)]],
    sampler g_LinearClampSampler [[sampler(0)]],
    sampler g_LinearWrapSampler [[sampler(1)]],
    sampler g_PointClampSampler [[sampler(2)]],
    sampler g_LinearBorderSampler [[sampler(3)]],
    constant volumetricCloud::Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer [[buffer(1)]],
    texture2d<float> g_GodrayTexture [[texture(10)]])
{
    Fragment_Shader::PSIn input0;
    input0.Position = float4(input.Position.xyz, 1.0 / input.Position.w);
    input0.TexCoord = input.TexCoord;
    Fragment_Shader main(
    highFreqNoiseTexture,
    lowFreqNoiseTexture,
    curlNoiseTexture,
    weatherTexture,
    depthTexture,
    LowResCloudTexture,
    g_PrevFrameTexture,
    g_LinearClampSampler,
    g_LinearWrapSampler,
    g_PointClampSampler,
    g_LinearBorderSampler,
    VolumetricCloudsCBuffer,
    g_GodrayTexture);
    return main.main(input0);
}
