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

#include "space_argument_buffers.h"

struct Vertex_Shader
{
    constant Uniforms_cbRootConstant & cbRootConstant;
    struct PsIn
    {
		float4 position [[position]];
		float3 texCoord;
		float2 screenCoord;
    };
	
	PsIn PushVertex(float3 pos, float3 dx, float3 dy, float2 vOffset)
	{
		PsIn output;
		output.position = cbRootConstant.ViewProjMat * float4(pos + dx*vOffset.x + dy*vOffset.y, 1.0);

		output.texCoord.z = output.position.z;

		output.position.z = output.position.w;
		output.position /= output.position.w;
		
		output.screenCoord = float2( (output.position.x + 1.0) * 0.5, (1.0 - output.position.y) * 0.5);
		
		vOffset += float2(1.0, -1.0);
		vOffset *= float2(0.5, -0.5);
		
		output.texCoord.xy = vOffset;
		return output;
	}
	
    PsIn main(uint VertexID)
    {
        PsIn Out;
		
		if(VertexID == 0)
		{
			Out = PushVertex(cbRootConstant.LightDirection.xyz, cbRootConstant.Dx.xyz, cbRootConstant.Dy.xyz, float2(-1.0, -1.0));
		}
		else if(VertexID == 1)
		{
			Out = PushVertex(cbRootConstant.LightDirection.xyz, cbRootConstant.Dx.xyz, cbRootConstant.Dy.xyz, float2(1.0, -1.0));
		}
		else if(VertexID == 2)
		{
			Out = PushVertex(cbRootConstant.LightDirection.xyz, cbRootConstant.Dx.xyz, cbRootConstant.Dy.xyz, float2(-1.0, 1.0));
		}
		else
		{
			Out = PushVertex(cbRootConstant.LightDirection.xyz, cbRootConstant.Dx.xyz, cbRootConstant.Dy.xyz, float2(1.0, 1.0));
		}
		
		
        return Out;
    };

    Vertex_Shader(
constant Uniforms_cbRootConstant & SunUniform) :
cbRootConstant(SunUniform) {}
};

vertex Vertex_Shader::PsIn stageMain(
uint VertexID [[vertex_id]],
    constant ArgDataPerFrame& argBufferPerFrame [[buffer(UPDATE_FREQ_PER_FRAME)]]
)
{
    uint VertexID0;
    VertexID0 = VertexID;
    Vertex_Shader main(
    argBufferPerFrame.SunUniform);
    return main.main(VertexID0);
}
