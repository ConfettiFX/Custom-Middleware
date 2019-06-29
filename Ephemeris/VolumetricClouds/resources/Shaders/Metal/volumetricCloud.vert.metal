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

struct Vertex_Shader
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
	
    struct VSInput
    {
        float4 m_ProjectionExtents [[attribute(0)]];
    };
    struct VSOutput
    {
        float4 Position [[position]];
        float2 TexCoord;
        float2 VSray;
    };
	
    VSOutput main(VSInput input, uint VertexID)
    {
        VSOutput Out;
        float4 position;
        ((position).x = (float)((((VertexID == (uint)(2)))?(3.0):((-1.0)))));
        ((position).y = (float)((((VertexID == (uint)(0)))?((-3.0)):(1.0))));
        ((position).zw = (float2)(1.0));
        ((Out).Position = position);
        ((Out).TexCoord = (((position).xy * float2(0.5, (-0.5))) + (float2)(0.5)));
        ((Out).VSray = (((position).xy * ((input).m_ProjectionExtents).xy) + ((input).m_ProjectionExtents).zw));
        return Out;
    };

    Vertex_Shader(texture3d<float> highFreqNoiseTexture,
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
				  constant volumetricCloud::Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer) :
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


vertex Vertex_Shader::VSOutput stageMain(
    Vertex_Shader::VSInput input [[stage_in]],
	uint VertexID [[vertex_id]],
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
    constant volumetricCloud::Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer [[buffer(1)]])
{
    Vertex_Shader::VSInput input0;
    input0.m_ProjectionExtents = input.m_ProjectionExtents;
    Vertex_Shader main(
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
    VolumetricCloudsCBuffer);
    return main.main(input0, VertexID);
}
