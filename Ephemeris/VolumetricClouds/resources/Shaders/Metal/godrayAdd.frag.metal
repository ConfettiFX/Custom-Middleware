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
        
        float3 sunWorldPos = VolumetricCloudsCBuffer.g_VolumetricClouds.lightDirection.xyz * 63600000.0;
        float4 sunPos;
        (sunPos = (((VolumetricCloudsCBuffer.g_VolumetricClouds).m_DataPerEye[0].m_WorldToProjMat)*(float4(sunWorldPos, 1.0))));
        ((sunPos).xy /= (float2)((sunPos).w));
        float2 ScreenNDC = float2(((((input).TexCoord).x * (float)(2.0)) - (float)(1.0)), ((((float)(1.0) - ((input).TexCoord).y) * (float)(2.0)) - (float)(1.0)));
        float3 projectedPosition = float3((ScreenNDC).xy, 0.0);
        float4 worldPos = (((VolumetricCloudsCBuffer.g_VolumetricClouds).m_DataPerEye[0].m_ProjToWorldMat)*(float4(projectedPosition, 1.0)));
        (worldPos /= (float4)((worldPos).w));
        float4 CameraPosition = (VolumetricCloudsCBuffer.g_VolumetricClouds).m_DataPerEye[0].cameraPosition;
        float3 viewDir = normalize(((worldPos).xyz - (CameraPosition).xyz));
        float cosTheta = saturate(dot(viewDir, ((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).xyz));
        if (((cosTheta <= 0.0) || ((sunPos).z < (float)(0.0))))
        {
            return float4(0.0, 0.0, 0.0, 0.0);
        }
        
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
    constant volumetricCloud::GraphicsArgData& argBufferStatic [[buffer(UPDATE_FREQ_NONE)]],
    constant volumetricCloud::GraphicsArgDataPerFrame& argBufferPerFrame [[buffer(UPDATE_FREQ_PER_FRAME)]]
)
{
    Fragment_Shader::PSIn input0;
    input0.Position = float4(input.Position.xyz, 1.0 / input.Position.w);
    input0.TexCoord = input.TexCoord;
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
    argBufferStatic.g_GodrayTexture);
    return main.main(input0);
}
