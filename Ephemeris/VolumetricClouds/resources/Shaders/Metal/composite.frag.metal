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
	texture2d<float> g_PrevVolumetricCloudTexture;
	
	float getAtmosphereBlendForComposite(float distance)
    {
        float rate = mix(0.75f, 0.4f, saturate(VolumetricCloudsCBuffer.g_VolumetricClouds.m_MaxSampleDistance / MAX_SAMPLE_STEP_DISTANCE));
        float Threshold = VolumetricCloudsCBuffer.g_VolumetricClouds.m_MaxSampleDistance * rate;
        float InvThreshold = VolumetricCloudsCBuffer.g_VolumetricClouds.m_MaxSampleDistance - Threshold;
        return saturate(max(distance * MAX_SAMPLE_STEP_DISTANCE - Threshold, 0.0f) / InvThreshold);
    }
	
    struct PSIn
    {
        float4 Position [[position]];
        float2 TexCoord;
        float2 VSray;
    };
	
    float4 main(PSIn input)
    {
        float4 volumetricCloudsResult = g_PostProcessedTexture.sample(g_LinearClampSampler, (input).TexCoord);
        float sceneDepth = depthTexture.sample(g_LinearClampSampler, (input).TexCoord, level(0)).r;
		float atmosphericBlendFactor = g_PrevVolumetricCloudTexture.sample(g_LinearClampSampler, (input).TexCoord, level(0)).g;
        if (((float)((VolumetricCloudsCBuffer.g_VolumetricClouds).EnabledDepthCulling) > (float)(0.5)))
        {
			return float4(volumetricCloudsResult.r, volumetricCloudsResult.g, volumetricCloudsResult.b, sceneDepth >= 1.0 ? 1.0 - getAtmosphereBlendForComposite(atmosphericBlendFactor) : 0.0);
        }
        else
        {
            return float4(volumetricCloudsResult.r, volumetricCloudsResult.g, volumetricCloudsResult.b, ((float)(1.0) - getAtmosphereBlendForComposite(atmosphericBlendFactor)));
        }
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
					texture2d<float> g_PostProcessedTexture,
					texture2d<float> g_PrevVolumetricCloudTexture) :
highFreqNoiseTexture(highFreqNoiseTexture),lowFreqNoiseTexture(lowFreqNoiseTexture),curlNoiseTexture(curlNoiseTexture),weatherTexture(weatherTexture),depthTexture(depthTexture),LowResCloudTexture(LowResCloudTexture),g_PrevFrameTexture(g_PrevFrameTexture),g_LinearClampSampler(g_LinearClampSampler),g_LinearWrapSampler(g_LinearWrapSampler),g_PointClampSampler(g_PointClampSampler),g_LinearBorderSampler(g_LinearBorderSampler),VolumetricCloudsCBuffer(VolumetricCloudsCBuffer),g_PostProcessedTexture(g_PostProcessedTexture),g_PrevVolumetricCloudTexture(g_PrevVolumetricCloudTexture) {}
};

struct ArgsData
{
	texture3d<float> highFreqNoiseTexture;
	texture3d<float> lowFreqNoiseTexture;
	texture2d<float> curlNoiseTexture;
	texture2d<float> weatherTexture;
	texture2d<float> depthTexture;
	texture2d<float> LowResCloudTexture;
	sampler g_LinearClampSampler;
	sampler g_LinearWrapSampler;
	sampler g_PointClampSampler;
    sampler g_LinearBorderSampler;
	texture2d<float> g_PrevFrameTexture;	
    texture2d<float> g_PostProcessedTexture;
    texture2d<float> g_PrevVolumetricCloudTexture;
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
    argBufferStatic.g_PostProcessedTexture,
    argBufferStatic.g_PrevVolumetricCloudTexture);
    return main.main(input0);
}
