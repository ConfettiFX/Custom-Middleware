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
    };
	
    float4 main(PSIn input)
    {
        float2 db_uvs = (input).TexCoord;
        float3 sunWorldPos = (((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).xyz * (float3)(1000000.0));
        float4 sunPos = (((VolumetricCloudsCBuffer.g_VolumetricClouds).m_WorldToProjMat_1st)*(float4(sunWorldPos, 1.0)));
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
        float cosTheta2 = (cosTheta * cosTheta);
        float cosTheta4 = (cosTheta2 * cosTheta2);
        float cosTheta16 = (cosTheta4 * cosTheta4);
        float cosTheta32 = (cosTheta16 * cosTheta2);
        float cosTheta64 = (cosTheta16 * cosTheta4);
        float2 deltaTexCoord = float2((ScreenNDC - (sunPos).xy));
        if ((length(deltaTexCoord) > (float)(1.0)))
        {
            (deltaTexCoord = normalize(deltaTexCoord));
        }
        ((deltaTexCoord).y = (-(deltaTexCoord).y));
        float2 texCoord = db_uvs;
        float2 delta = float2(1.0, 1.0);
        (deltaTexCoord *= (delta / (float2)((800.0 * (VolumetricCloudsCBuffer.g_VolumetricClouds).GodrayDensity))));
        float illuminationDecay = 1.0;
        float finalIntensity = 0.0;
        for (uint i = 0; (i < (uint)(80)); (i++))
        {
            (texCoord -= deltaTexCoord);
            float localDensity = ((float)(1.0) - (float)(g_PrevFrameTexture.sample(g_LinearBorderSampler, texCoord).a));
            (localDensity *= (illuminationDecay * (VolumetricCloudsCBuffer.g_VolumetricClouds).GodrayWeight));
            (finalIntensity += localDensity);
            (illuminationDecay *= (VolumetricCloudsCBuffer.g_VolumetricClouds).GodrayDecay);
        }
        (finalIntensity *= (VolumetricCloudsCBuffer.g_VolumetricClouds).GodrayExposure);
        (finalIntensity += min(((VolumetricCloudsCBuffer.g_VolumetricClouds).CloudCoverage * (float)(5.0)), 0.0));
        (finalIntensity = saturate(finalIntensity));
		
		float3 LightColor = VolumetricCloudsCBuffer.g_VolumetricClouds.lightColorAndIntensity.rgb * VolumetricCloudsCBuffer.g_VolumetricClouds.lightColorAndIntensity.a;
		
		return float4(LightColor * finalIntensity * cosTheta32, 0.0f);
    };

    Fragment_Shader(
texture3d<float> highFreqNoiseTexture,texture3d<float> lowFreqNoiseTexture,texture2d<float> curlNoiseTexture,texture2d<float> weatherTexture,texture2d<float> depthTexture,texture2d<float> LowResCloudTexture,texture2d<float> g_PrevFrameTexture,sampler g_LinearClampSampler,sampler g_LinearWrapSampler,sampler g_PointClampSampler,sampler g_LinearBorderSampler,constant volumetricCloud::Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer) :
highFreqNoiseTexture(highFreqNoiseTexture),lowFreqNoiseTexture(lowFreqNoiseTexture),curlNoiseTexture(curlNoiseTexture),weatherTexture(weatherTexture),depthTexture(depthTexture),LowResCloudTexture(LowResCloudTexture),g_PrevFrameTexture(g_PrevFrameTexture),g_LinearClampSampler(g_LinearClampSampler),g_LinearWrapSampler(g_LinearWrapSampler),g_PointClampSampler(g_PointClampSampler),g_LinearBorderSampler(g_LinearBorderSampler),VolumetricCloudsCBuffer(VolumetricCloudsCBuffer) {}
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
    argBufferPerFrame.VolumetricCloudsCBuffer);
    return main.main(input0);
}
