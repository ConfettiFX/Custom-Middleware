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

struct Compute_Shader
{
    struct Data
    {
        uint mip;
    };
    struct Uniforms_RootConstant
    {
        uint mip;
    };
    constant Uniforms_RootConstant & RootConstant;
    texture2d<float> SrcTexture;
    texture2d<float, access::read_write> DstTexture;
	
    void main(uint3 GTid, uint3 Gid, uint3 DTid)
    {
        DstTexture.write(SrcTexture.read(DTid.xy), DTid.xy, 0 /*RootConstant.mip*/);
    };

    Compute_Shader(
constant Uniforms_RootConstant & RootConstant,texture2d<float> SrcTexture,texture2d<float, access::read_write> DstTexture) :
RootConstant(RootConstant),SrcTexture(SrcTexture),DstTexture(DstTexture) {}
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
    constant ArgsData& argBufferStatic [[buffer(UPDATE_FREQ_NONE)]],
    constant Compute_Shader::Uniforms_RootConstant & RootConstant [[buffer(UPDATE_FREQ_USER + 4)]]
)
{
    uint3 GTid0;
    GTid0 = GTid;
    uint3 Gid0;
    Gid0 = Gid;
    uint3 DTid0;
    DTid0 = DTid;
    Compute_Shader main(
    RootConstant,
    argBufferStatic.SrcTexture,
    argBufferStatic.DstTexture);
    return main.main(GTid0, Gid0, DTid0);
}
