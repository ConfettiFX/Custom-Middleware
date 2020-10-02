/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include <metal_stdlib>
#include <metal_relational>

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
    texture2d<float> g_LinearDepthTexture;

    sampler g_LinearClampSampler;
    sampler g_LinearWrapSampler;
    sampler g_PointClampSampler;
    sampler g_LinearBorderSampler;

    constant volumetricCloud::Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer;
	
	// Check whether current texture coordinates should be updated or not
    float ShouldbeUpdated(float2 uv, float2 jitter)
    {
		float2 texelRelativePos = fmod(uv * VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.zw, 4.0);
        texelRelativePos -= jitter;
        float2 valid = saturate(2.0 * (float2(0.5, 0.5) - abs(texelRelativePos - float2(0.5, 0.5))));
        return valid.x * valid.y;
    };
	
    struct PSIn
    {
        float4 Position [[position]];
        float2 TexCoord;
        float2 VSray;
    };
	
    float4 main(PSIn input)
    {
        float outOfBound;
        float2 _Jitter = float2(float((VolumetricCloudsCBuffer.g_VolumetricClouds).m_JitterX), float((VolumetricCloudsCBuffer.g_VolumetricClouds).m_JitterY));
        float2 onePixeloffset = ((float2)(1.0) / ((VolumetricCloudsCBuffer.g_VolumetricClouds).TimeAndScreenSize).zw);
        float2 uv = ((input).TexCoord - ((_Jitter - (float2)(1.5)) * onePixeloffset));
        float4 currSample = LowResCloudTexture.sample(g_PointClampSampler, uv);
        float depth = volumetricCloud::UnPackFloat16((currSample).z);
        float4 prevUV;
        float2 NDC = float2(((((input).TexCoord).x * (float)(2.0)) - (float)(1.0)), ((((float)(1.0) - ((input).TexCoord).y) * (float)(2.0)) - (float)(1.0)));
		
		float4 worldPos = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerEye[0].m_ProjToWorldMat * float4(NDC, 0.0, 1.0);
		worldPos /= worldPos.w;
	    float4 CameraPosition = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerEye[0].cameraPosition;
		
        float3 viewDir = normalize(((worldPos).xyz - CameraPosition.xyz));
        float3 firstHitRayPos = ((viewDir * (float3)(depth)) + (CameraPosition).xyz);
		
        (prevUV = (((VolumetricCloudsCBuffer.g_VolumetricClouds).m_DataPerEye[0].m_PrevWorldToProjMat)*(float4(firstHitRayPos, 1.0))));
		
        (prevUV /= (float4)((prevUV).w));
        ((prevUV).xy = float2((((prevUV).x + (float)(1.0)) * (float)(0.5)), (((float)(1.0) - (prevUV).y) * (float)(0.5))));
        (outOfBound = step(0.0, max(max((-(prevUV).x), (-(prevUV).y)), (max((prevUV).x, (prevUV).y) - (float)(1.0)))));
		
		float4 prevSample = g_PrevFrameTexture.sample(g_LinearClampSampler, (prevUV).xy);
		
		float blend = max(ShouldbeUpdated((input).TexCoord, _Jitter), outOfBound);
		
		float4 result = mix(prevSample, currSample, blend);
		
		if (any(isnan(result)))
			result = float4(0.0f, 0.0f, 0.0f, 0.0f);
		
		return result;
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
					constant volumetricCloud::Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer
					) :
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
	VolumetricCloudsCBuffer(VolumetricCloudsCBuffer) {}
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
    argBufferPerFrame.VolumetricCloudsCBuffer);
    return main.main(input0);
}
