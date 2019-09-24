/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include <metal_stdlib>
#include <metal_compute>
using namespace metal;

#include "volumetricCloud.h"

struct Compute_Shader
{
    texture2d<float> SrcTexture;
	texture2d<float, access::write> DstTexture;
	device uint4* DstBuffer;
	constant volumetricCloud::Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer;
	
	uint4 Float4ToUint4(half4 src)
	{
		return uint4( as_type<uint>((float)src.r) , as_type<uint>((float)src.g), as_type<uint>((float)src.b), as_type<uint>((float)src.a) );
	}
	
    void main(uint3 GTid, uint3 Gid, uint3 DTid)
    {
        DstTexture.write(SrcTexture.read(DTid.xy), DTid.xy);
		//half4 SrcInfo = SrcTexture.read(DTid.xy);
		//half4 SrcHalf = half4((half) SrcInfo.r, (half) SrcInfo.g, (half) SrcInfo.b, (half)SrcInfo.a);
		//DstBuffer[(uint)VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.z * DTid.y + DTid.x] = Float4ToUint4(SrcInfo);
		//half4 CheckReuslt = DstBuffer[(uint)VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.z * DTid.y + DTid.x];
    };

    Compute_Shader(
				   texture2d<float> SrcTexture,
				   texture2d<float, access::write> DstTexture,
				   device uint4* DstBuffer,
				   constant volumetricCloud::Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer) :
					SrcTexture(SrcTexture), DstTexture(DstTexture),
					DstBuffer(DstBuffer),
					VolumetricCloudsCBuffer(VolumetricCloudsCBuffer)
					{}
};

struct ArgsData
{
	texture2d<float> SrcTexture;
	texture2d<float, access::write> SavePrevTexture;
    device uint4* DstBuffer;
};

struct ArgsPerFrame
{
    constant volumetricCloud::Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer;
};

//[numthreads(16, 16, 1)]
kernel void stageMain(
uint3 GTid [[thread_position_in_threadgroup]],
uint3 Gid [[threadgroup_position_in_grid]],
uint3 DTid [[thread_position_in_grid]],
    constant ArgsData& argBufferStatic [[buffer(UPDATE_FREQ_NONE)]],
    constant ArgsPerFrame& argBufferPerFrame [[buffer(UPDATE_FREQ_PER_FRAME)]]
)
{
    uint3 GTid0;
    GTid0 = GTid;
    uint3 Gid0;
    Gid0 = Gid;
    uint3 DTid0;
    DTid0 = DTid;
    Compute_Shader main(
    argBufferStatic.SrcTexture,
	argBufferStatic.SavePrevTexture,
    argBufferStatic.DstBuffer,
	argBufferPerFrame.VolumetricCloudsCBuffer);
    return main.main(GTid0, Gid0, DTid0);
}
