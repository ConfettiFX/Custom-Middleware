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

#define NUM_THREADS_X 16
#define NUM_THREADS (NUM_THREADS_X * NUM_THREADS_X)

struct Compute_Shader
{
    texture2d<float> SrcTexture;
    texture2d<float, access::read_write> DstTexture;
	
    float CombineGroup(float a, float b)
    {
        return max(a, b);
    };
	
   void main(thread uint3& GTid, thread uint3& Gid, thread uint3& DTid, thread uint& GroupIndex, threadgroup float* GroupOutput)
    {
        GroupOutput[GTid.y * NUM_THREADS_X + GTid.x] = SrcTexture.read(DTid.xy * 2).r;
        threadgroup_barrier(mem_flags::mem_threadgroup);
		
        for (uint i = (NUM_THREADS >> 1); i > 0; i = (i >> 1))
        {
            if (GroupIndex < i)
            {
                GroupOutput[GroupIndex] = CombineGroup(GroupOutput[GroupIndex], GroupOutput[GroupIndex + i]);
            }
            threadgroup_barrier(mem_flags::mem_threadgroup);
        }
		
        if (GroupIndex == 0)
        {
            DstTexture.write(GroupOutput[0], Gid.xy);
        }
    };

    Compute_Shader( texture2d<float> SrcTexture,
				   	texture2d<float, access::read_write> DstTexture) :
					SrcTexture(SrcTexture),
					DstTexture(DstTexture) {}
};

struct ArgsData
{
    texture2d<float> SrcTexture;
    texture2d<float, access::read_write> DstTexture;
};

//[numthreads(16, 16, 1)]
kernel void stageMain(
uint3 GTid [[thread_position_in_threadgroup]],
uint3 Gid [[threadgroup_position_in_grid]],
uint3 DTid [[thread_position_in_grid]],
uint GroupIndex [[thread_index_in_threadgroup]],
    constant ArgsData& argBufferStatic [[buffer(UPDATE_FREQ_NONE)]]
)
{
	threadgroup float GroupOutput[NUM_THREADS];
	
    uint3 GTid0;
    GTid0 = GTid;
    uint3 Gid0;
    Gid0 = Gid;
    uint3 DTid0;
    DTid0 = DTid;
    uint GroupIndex0;
    GroupIndex0 = GroupIndex;
    Compute_Shader main(
    argBufferStatic.SrcTexture,
    argBufferStatic.DstTexture);
    return main.main(GTid0, Gid0, DTid0, GroupIndex0, GroupOutput);
}
