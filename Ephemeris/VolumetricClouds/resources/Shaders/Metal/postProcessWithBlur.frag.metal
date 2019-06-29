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
	
    struct PSIn
    {
        float4 Position [[position]];
        float2 TexCoord;
        float2 VSray;
    };
    struct PSOut
    {
        float4 VolumetricClouds;
        float4 ResultColor;
    };
    texture2d<float> g_SrcTexture2D;
    texture2d<float> g_SkyBackgroudTexture;
    device float4* TransmittanceColor;
    float4 main(PSIn input)
    {
        float4 volumetricCloudsResult = g_SrcTexture2D.sample(g_LinearClampSampler, (input).TexCoord);
        float intensity = (volumetricCloudsResult).r;
        float density = (volumetricCloudsResult).a;
        float3 BackgroudColor = g_SkyBackgroudTexture.sample(g_LinearClampSampler, (input).TexCoord).rgb;
        float3 TransmittanceRGB = TransmittanceColor[0].rgb;
        float4 PostProcessedResult;
        ((PostProcessedResult).a = density);
        PostProcessedResult.rgb = (float3)(intensity) * mix(TransmittanceRGB, mix(VolumetricCloudsCBuffer.g_VolumetricClouds.lightColorAndIntensity.rgb, TransmittanceRGB, pow(saturate(1.0 - VolumetricCloudsCBuffer.g_VolumetricClouds.lightDirection.y), 0.5)), VolumetricCloudsCBuffer.g_VolumetricClouds.Test00) * (float3)(VolumetricCloudsCBuffer.g_VolumetricClouds.lightColorAndIntensity.a) * (float3)(VolumetricCloudsCBuffer.g_VolumetricClouds.CloudBrightness);
        ((PostProcessedResult).rgb = (float3)(mix((saturate(BackgroudColor) + (PostProcessedResult).rgb), (PostProcessedResult).rgb, min(((PostProcessedResult).a * 1.0), (VolumetricCloudsCBuffer.g_VolumetricClouds).BackgroundBlendFactor))));
        ((PostProcessedResult).rgb = (float3)(mix(BackgroudColor, (PostProcessedResult).rgb, (VolumetricCloudsCBuffer.g_VolumetricClouds).BackgroundBlendFactor)));
        return float4((PostProcessedResult).rgb, 1.0);
    };

    Fragment_Shader(
texture3d<float> highFreqNoiseTexture,texture3d<float> lowFreqNoiseTexture,texture2d<float> curlNoiseTexture,texture2d<float> weatherTexture,texture2d<float> depthTexture,texture2d<float> LowResCloudTexture,texture2d<float> g_PrevFrameTexture,sampler g_LinearClampSampler,sampler g_LinearWrapSampler,sampler g_PointClampSampler,sampler g_LinearBorderSampler,constant volumetricCloud::Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer,texture2d<float> g_SrcTexture2D,texture2d<float> g_SkyBackgroudTexture,device float4* TransmittanceColor) :
highFreqNoiseTexture(highFreqNoiseTexture),lowFreqNoiseTexture(lowFreqNoiseTexture),curlNoiseTexture(curlNoiseTexture),weatherTexture(weatherTexture),depthTexture(depthTexture),LowResCloudTexture(LowResCloudTexture),g_PrevFrameTexture(g_PrevFrameTexture),g_LinearClampSampler(g_LinearClampSampler),g_LinearWrapSampler(g_LinearWrapSampler),g_PointClampSampler(g_PointClampSampler),g_LinearBorderSampler(g_LinearBorderSampler),VolumetricCloudsCBuffer(VolumetricCloudsCBuffer),g_SrcTexture2D(g_SrcTexture2D),g_SkyBackgroudTexture(g_SkyBackgroudTexture),TransmittanceColor(TransmittanceColor) {}
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
    texture2d<float> g_SrcTexture2D [[texture(7)]],
    texture2d<float> g_SkyBackgroudTexture [[texture(8)]],
    device float4* TransmittanceColor [[buffer(2)]])
{
    Fragment_Shader::PSIn input0;
    input0.Position = float4(input.Position.xyz, 1.0 / input.Position.w);
    input0.TexCoord = input.TexCoord;
    input0.VSray = input.VSray;
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
    g_SrcTexture2D,
    g_SkyBackgroudTexture,
    TransmittanceColor);
    return main.main(input0);
}
