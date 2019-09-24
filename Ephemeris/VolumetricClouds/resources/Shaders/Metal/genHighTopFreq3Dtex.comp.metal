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

#define THREAD_SIZE 16

struct SliceNumInfo
{
	uint SliceNum;
};

struct Compute_Shader
{
	texture2d<float> SrcTexture;
    texture3d<float, access::read_write> DstTexture;
	constant SliceNumInfo & sliceNumInfo;
	
    void main(uint3 GTid, uint3 Gid)
    {
		float4 result = SrcTexture.read(uint2(GTid.x + Gid.x * THREAD_SIZE, GTid.y + Gid.y * THREAD_SIZE));
		DstTexture.write(result, uint3(GTid.x + Gid.x * THREAD_SIZE, GTid.y + Gid.y * THREAD_SIZE, sliceNumInfo.SliceNum));
    };

    Compute_Shader(texture2d<float> SrcTexture,
				   texture3d<float, access::read_write> DstTexture,
				   constant SliceNumInfo & sliceNumInfo) :
					SrcTexture(SrcTexture),
					DstTexture(DstTexture),
					sliceNumInfo(sliceNumInfo) {}
};

struct ArgsData
{
    texture2d<float> SrcTexture;
    texture3d<float, access::read_write> DstTexture;
};

//[numthreads(16, 16, 1)]
kernel void stageMain(
uint3 GTid [[thread_position_in_threadgroup]],
uint3 Gid [[threadgroup_position_in_grid]],
    constant ArgsData& argBufferStatic [[buffer(UPDATE_FREQ_NONE)]],
	constant SliceNumInfo & sliceRootConstant [[buffer(UPDATE_FREQ_USER + 2)]]
)
{
    uint3 GTid0;
    GTid0 = GTid;
    uint3 Gid0;
    Gid0 = Gid;
    Compute_Shader main(
    argBufferStatic.SrcTexture,
    argBufferStatic.DstTexture,
	sliceRootConstant);
    return main.main(GTid0, Gid0);
}
