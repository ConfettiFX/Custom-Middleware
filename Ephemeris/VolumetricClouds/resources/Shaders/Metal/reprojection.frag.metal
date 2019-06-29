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
	constant uint4 & g_PrevFrameBuffer;
	
	float ShouldbeUpdated(float2 uv, float2 jitter)
	{
		float2 texelRelativePos = fmod((uv * ((VolumetricCloudsCBuffer.g_VolumetricClouds).TimeAndScreenSize).zw), 4.0);
		(texelRelativePos -= jitter);
		float2 valid = saturate(((float2)(2.0) * (float2(0.5, 0.5) - abs((texelRelativePos - float2(0.5, 0.5))))));
		return ((valid).x * (valid).y);
	};
	
    struct PSIn
    {
        float4 Position [[position]];
        float2 TexCoord;
        float2 VSray;
    };
	
    float4 main(PSIn input)
    {
        float2 db_uvs = (input).TexCoord;
        float outOfBound;
        float2 _Jitter = float2(float((VolumetricCloudsCBuffer.g_VolumetricClouds).m_JitterX), float((VolumetricCloudsCBuffer.g_VolumetricClouds).m_JitterY));
        float2 onePixeloffset = ((float2)(1.0) / ((VolumetricCloudsCBuffer.g_VolumetricClouds).TimeAndScreenSize).zw);
        float2 uv = ((input).TexCoord - ((_Jitter - (float2)(1.5)) * onePixeloffset));
        float4 currSample = LowResCloudTexture.sample(g_PointClampSampler, uv);
        float depth = volumetricCloud::UnPackFloat16((currSample).z);
        float4 prevUV;
        float4 worldPos;
        float2 NDC = float2(((((input).TexCoord).x * (float)(2.0)) - (float)(1.0)), ((((float)(1.0) - ((input).TexCoord).y) * (float)(2.0)) - (float)(1.0)));
        (worldPos = (((VolumetricCloudsCBuffer.g_VolumetricClouds).m_ProjToWorldMat_1st)*(float4(NDC, 0.0, 1.0))));
        (worldPos /= (float4)((worldPos).w));
        float3 viewDir = normalize(((worldPos).xyz - ((VolumetricCloudsCBuffer.g_VolumetricClouds).cameraPosition_1st).xyz));
        float3 firstHitRayPos = ((viewDir * (float3)(depth)) + ((VolumetricCloudsCBuffer.g_VolumetricClouds).cameraPosition_1st).xyz);
        (prevUV = (((VolumetricCloudsCBuffer.g_VolumetricClouds).m_PrevWorldToProjMat_1st)*(float4(firstHitRayPos, 1.0))));
        (prevUV /= (float4)((prevUV).w));
        ((prevUV).xy = float2((((prevUV).x + (float)(1.0)) * (float)(0.5)), (((float)(1.0) - (prevUV).y) * (float)(0.5))));
        (outOfBound = step(0.0, max(max((-(prevUV).x), (-(prevUV).y)), (max((prevUV).x, (prevUV).y) - (float)(1.0)))));
		
		float4 prevSample = g_PrevFrameTexture.sample(g_LinearClampSampler, (prevUV).xy);
		uint Y = (uint)VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.z * (uint)(VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.w * prevUV.y);
		uint X = (uint)(VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.z * prevUV.x);
		
		//half4 data = g_PrevFrameBuffer[ X + Y ];
		
		//float4 prevSample = float4((float)data.x, (float)data.y, (float)data.z, (float)data.w);
		
		
		
		//uint4 prevSampleUint = g_PrevFrameBuffer[ X + Y ];
		//float4 prevSample = float4( as_type<float>(prevSampleUint.x), as_type<float>(prevSampleUint.y), as_type<float>(prevSampleUint.z), as_type<float>(prevSampleUint.w) );
		
		
		//float4 prevSample = g_PrevFrameTexture[uint2(VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.z * prevUV.x, VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.w * prevUV.y)];
		
		float blend = max(ShouldbeUpdated((input).TexCoord, _Jitter), outOfBound);
		
		float4 result = mix(prevSample, currSample, blend);
		//result.a = currSample.a;
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
					constant volumetricCloud::Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer,
					constant uint4 & g_PrevFrameBuffer) :
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
	g_PrevFrameBuffer(g_PrevFrameBuffer) {}
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
	constant uint4 & g_PrevFrameBuffer [[buffer(2)]])
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
	g_PrevFrameBuffer);
    return main.main(input0);
}
