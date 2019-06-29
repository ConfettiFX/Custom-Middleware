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
    texture2d<float> SrcTexture;
    texture2d<float, access::read_write> DstTexture;
    struct Uniforms_CameraInfoRootConstant
    {
        float nearPlane;
        float farPlane;
        float padding00;
        float padding01;
    };
    constant Uniforms_CameraInfoRootConstant & CameraInfoRootConstant;
    float DepthLinearization(float depth)
    {
        return (((float)(2.0) * CameraInfoRootConstant.nearPlane) / ((CameraInfoRootConstant.farPlane + CameraInfoRootConstant.nearPlane) - (depth * (CameraInfoRootConstant.farPlane - CameraInfoRootConstant.nearPlane))));
    };
    void main(uint3 GTid, uint3 Gid, uint3 DTid)
    {
        (DstTexture.write((DepthLinearization(SrcTexture.read(DTid.xy).x) * CameraInfoRootConstant.farPlane), uint2(int2((DTid).xy))));
    };

    Compute_Shader(
texture2d<float> SrcTexture,texture2d<float, access::read_write> DstTexture,constant Uniforms_CameraInfoRootConstant & CameraInfoRootConstant) :
SrcTexture(SrcTexture),DstTexture(DstTexture),CameraInfoRootConstant(CameraInfoRootConstant) {}
};

//[numthreads(16, 16, 1)]
kernel void stageMain(
uint3 GTid [[thread_position_in_threadgroup]],
uint3 Gid [[threadgroup_position_in_grid]],
uint3 DTid [[thread_position_in_grid]],
    texture2d<float> SrcTexture [[texture(0)]],
    texture2d<float, access::read_write> DstTexture [[texture(1)]],
    constant Compute_Shader::Uniforms_CameraInfoRootConstant & CameraInfoRootConstant [[buffer(1)]])
{
    uint3 GTid0;
    GTid0 = GTid;
    uint3 Gid0;
    Gid0 = Gid;
    uint3 DTid0;
    DTid0 = DTid;
    Compute_Shader main(
    SrcTexture,
    DstTexture,
    CameraInfoRootConstant);
    return main.main(GTid0, Gid0, DTid0);
}
