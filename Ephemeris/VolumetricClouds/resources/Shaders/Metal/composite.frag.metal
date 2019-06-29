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
		float atmosphericBlendFactor = volumetricCloud::UnPackFloat16(g_PrevVolumetricCloudTexture.sample(g_LinearClampSampler, (input).TexCoord, level(0)).g);
        if (((float)((VolumetricCloudsCBuffer.g_VolumetricClouds).EnabledDepthCulling) > (float)(0.5)))
        {
			return float4(volumetricCloudsResult.r, volumetricCloudsResult.g, volumetricCloudsResult.b, (((sceneDepth >= 1.0 /*VolumetricCloudsCBuffer.g_VolumetricClouds.CameraFarClip*/))?(((float)(1.0) - volumetricCloud::getAtmosphereBlendForComposite(atmosphericBlendFactor))):(0.0)));
        }
        else
        {
            return float4(volumetricCloudsResult.r, volumetricCloudsResult.g, volumetricCloudsResult.b, ((float)(1.0) - volumetricCloud::getAtmosphereBlendForComposite(atmosphericBlendFactor)));
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
    texture2d<float> g_PostProcessedTexture [[texture(11)]],
    texture2d<float> g_PrevVolumetricCloudTexture [[texture(12)]])
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
    g_PostProcessedTexture,
    g_PrevVolumetricCloudTexture);
    return main.main(input0);
}
