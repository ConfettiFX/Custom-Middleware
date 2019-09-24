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
	texture2d<float> g_PostProcessedTexture;
	
    struct PSIn
    {
        float4 Position [[position]];
        float2 TexCoord;
        float2 VSray;
    };
	
    float4 main(PSIn input)
    {
        float4 volumetricCloudsResult = g_PostProcessedTexture.sample(g_LinearClampSampler, (input).TexCoord);
        return float4(volumetricCloudsResult.r, volumetricCloudsResult.g, volumetricCloudsResult.b, 1.0);
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
					texture2d<float> g_PostProcessedTexture) :
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
	g_PostProcessedTexture(g_PostProcessedTexture) {}
};

struct ArgsData
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
    texture2d<float> g_PostProcessedTexture;
};

struct ArgsPerFrame
{
    constant volumetricCloud::Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer;
};

fragment float4 stageMain(
    Fragment_Shader::PSIn input [[stage_in]],
    constant ArgsData& argBufferStatic [[buffer(UPDATE_FREQ_NONE)]],
    constant ArgsPerFrame& argBufferPerFrame [[buffer(UPDATE_FREQ_PER_FRAME)]]
)
{
    Fragment_Shader::PSIn input0;
    input0.Position = float4(input.Position.xyz, 1.0 / input.Position.w);
    input0.TexCoord = input.TexCoord;
    input0.VSray = input.VSray;
    Fragment_Shader main(
    argBufferStatic.highFreqNoiseTexture,
    argBufferStatic.lowFreqNoiseTexture,
    argBufferStatic.curlNoiseTexture,
    argBufferStatic.weatherTexture,
    argBufferStatic.depthTexture,
    argBufferStatic.LowResCloudTexture,
    argBufferStatic.g_PrevFrameTexture,
    argBufferStatic.g_LinearClampSampler,
    argBufferStatic.g_LinearWrapSampler,
    argBufferStatic.g_PointClampSampler,
    argBufferStatic.g_LinearBorderSampler,
    argBufferPerFrame.VolumetricCloudsCBuffer,
    argBufferStatic.g_PostProcessedTexture);
    return main.main(input0);
}
