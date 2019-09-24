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
//#include <metal_math>

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
    texture3d<float> SrcTexture;
    texture3d<float, access::read_write> DstTexture;
    void main(uint3 GTid, uint3 Gid)
    {
        float4 result = float4(0.0, 0.0, 0.0, 0.0);
        int iteration = (int)pow(2.0, (float)RootConstant.mip);
        for (int x = 0; (x < iteration); (x++))
        {
            for (int y = 0; (y < iteration); (y++))
            {
                for (int z = 0; (z < iteration); (z++))
                {
                    (result += (float4)(SrcTexture.read(((Gid * (uint3)(iteration)) + uint3(x, y, z)))));
                }
            }
        }
        (result /= (float4)(((iteration * iteration) * iteration)));
        (DstTexture.write(result, uint3(Gid)));
    };

    Compute_Shader(
constant Uniforms_RootConstant & RootConstant,texture3d<float> SrcTexture,texture3d<float, access::read_write> DstTexture) :
RootConstant(RootConstant),SrcTexture(SrcTexture),DstTexture(DstTexture) {}
};

struct ArgsData
{
    texture3d<float> SrcTexture;
    texture3d<float, access::read_write> DstTexture;
};

//[numthreads(1, 1, 1)]
kernel void stageMain(
uint3 GTid [[thread_position_in_threadgroup]],
uint3 Gid [[threadgroup_position_in_grid]],
    constant ArgsData& argBufferStatic [[buffer(UPDATE_FREQ_NONE)]],
    constant Compute_Shader::Uniforms_RootConstant & RootConstant [[buffer(UPDATE_FREQ_USER + 3)]]
)
{
    uint3 GTid0;
    GTid0 = GTid;
    uint3 Gid0;
    Gid0 = Gid;
    Compute_Shader main(
    RootConstant,
    argBufferStatic.SrcTexture,
    argBufferStatic.DstTexture);
    return main.main(GTid0, Gid0);
}
