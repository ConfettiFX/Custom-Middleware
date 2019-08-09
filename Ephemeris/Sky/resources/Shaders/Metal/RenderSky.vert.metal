/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include "RenderSky.h"

inline float3x3 matrix_ctor(float4x4 m)
{
        return float3x3(m[0].xyz, m[1].xyz, m[2].xyz);
}
struct Vertex_Shader
{
	constant RenderSky::Uniforms_RenderSkyUniformBuffer & RenderSkyUniformBuffer;
	texture2d<float> SceneColorTexture;
	texture2d<float> Depth;
	texture2d<float> TransmittanceTexture;
	//texture2d<float> IrradianceTexture;
	texture3d<float> InscatterTexture;
	sampler g_LinearClamp;
    device float4* TransmittanceColor;
	
	float3x3 float4x4Tofloat3x3(float4x4 param)
	{
		float3x3 temp;
		temp[0] = param[0].xyz;
		temp[1] = param[1].xyz;
		temp[2] = param[2].xyz;
		return temp;
	}
	
	float2 getTransmittanceUV(float r, float mu)
	{
		float uR, uMu;
#ifdef TRANSMITTANCE_NON_LINEAR
		(uR = sqrt(((r - Rg) / (Rt - Rg))));
		(uMu = (atan((((mu + (float)(0.15)) / (float)((1.0 + 0.15))) * (float)(tan(1.5)))) / (float)(1.5)));
#else
		(uR = ((r - Rg) / (Rt - Rg)));
		(uMu = ((mu + (float)(0.15)) / (float)((1.0 + 0.15))));
#endif
		return float2(uMu, uR);
	};
	
	float3 transmittance(float r, float mu)
	{
		float2 uv = getTransmittanceUV(r, mu);
#ifdef USE_SAMPLELEVEL
		return TransmittanceTexture.sample(g_LinearClamp, uv, level(0)).rgb;
#else
		return TransmittanceTexture.sample(g_LinearClamp, uv).rgb;
#endif
	};
	
    struct PsIn
    {
        float4 position [[position]];
        float3 texCoord;
        float2 ScreenCoord;
    };
    PsIn main(uint VertexID)
    {
        PsIn Out;
        float4 position;
        ((position).x = (float)((((VertexID == (uint)(0)))?(3.0):((-1.0)))));
        ((position).y = (float)((((VertexID == (uint)(2)))?(3.0):((-1.0)))));
        ((position).zw = (float2)(1.0));
        ((Out).position = position);
        ((position).z = (float)(0.0));
        ((position).w = (RenderSkyUniformBuffer.QNNear).z);
        ((position).xy *= (float2)((position).w));
        ((Out).texCoord = (((RenderSkyUniformBuffer.invProj)*(position))).xyz);
        ((Out).texCoord /= (float3)(((Out).texCoord).z));
        Out.texCoord = matrix_ctor(RenderSkyUniformBuffer.invView) * Out.texCoord;
        ((Out).ScreenCoord = ((((Out).position).xy * float2(0.5, (-0.5))) + (float2)(0.5)));
        float3 ray = (RenderSkyUniformBuffer.LightDirection).xyz;
        float3 x = (RenderSkyUniformBuffer.CameraPosition).xyz;
        float3 v = normalize(ray);
        float r = length(x);
        float mu = (dot(x, v) / r);

        float4 dayLight = float4(transmittance(r, mu), 1.0f);
        float4 nightLight = RenderSkyUniformBuffer.LightIntensity * 0.05f;

		TransmittanceColor[VertexID] = RenderSkyUniformBuffer.LightDirection.y >= 0.2f ? dayLight : mix(nightLight , dayLight, saturate(RenderSkyUniformBuffer.LightDirection.y / 0.2f));
        return Out;
    };

    Vertex_Shader(
constant RenderSky::Uniforms_RenderSkyUniformBuffer & RenderSkyUniformBuffer,
				  texture2d<float> SceneColorTexture,
				  texture2d<float> Depth,
				  texture2d<float> TransmittanceTexture,
				  //texture2d<float> IrradianceTexture,
				  texture3d<float> InscatterTexture,
				  sampler g_LinearClamp,
				  device float4* TransmittanceColor) :
	RenderSkyUniformBuffer(RenderSkyUniformBuffer),
	SceneColorTexture(SceneColorTexture),
	Depth(Depth),
	TransmittanceTexture(TransmittanceTexture),
	//IrradianceTexture(IrradianceTexture),
	InscatterTexture(InscatterTexture),
	g_LinearClamp(g_LinearClamp),
	TransmittanceColor(TransmittanceColor) {}
};


vertex Vertex_Shader::PsIn stageMain(
uint VertexID [[vertex_id]],
    constant RenderSky::Uniforms_RenderSkyUniformBuffer & RenderSkyUniformBuffer [[buffer(1)]],
    texture2d<float> SceneColorTexture [[texture(0)]],
    texture2d<float> Depth [[texture(1)]],
    texture2d<float> TransmittanceTexture [[texture(2)]],
    //texture2d<float> IrradianceTexture [[texture(3)]],
    texture3d<float> InscatterTexture [[texture(3)]],
    sampler g_LinearClamp [[sampler(0)]],
    device float4* TransmittanceColor [[buffer(2)]])
{
    uint VertexID0;
    VertexID0 = VertexID;
    Vertex_Shader main(
    RenderSkyUniformBuffer,
    SceneColorTexture,
    Depth,
    TransmittanceTexture,
    //IrradianceTexture,
    InscatterTexture,
    g_LinearClamp,
    TransmittanceColor);
    return main.main(VertexID0);
}
