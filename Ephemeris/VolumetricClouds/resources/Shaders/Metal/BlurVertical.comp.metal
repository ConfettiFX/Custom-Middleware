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
#define MAX_HEIGHT 1024
    texture2d<float> InputTex;
    texture2d<float, access::read_write> OutputTex;
    struct Uniforms_RootConstantScreenSize
    {
        uint width;
        uint height;
    };
    constant Uniforms_RootConstantScreenSize & RootConstantScreenSize;
float2 SharedData[MAX_HEIGHT];
    void main(uint3 GroupId, uint3 GroupThreadId, uint GroupIndex)
    {
        int groupIndex = (GroupId).x;
        int localIndex = (GroupThreadId).y;
        float weight[3] = { 0.68269, 0.157305, 0.00135 };
        float4 CurrentPixelValue = InputTex.read(uint2(groupIndex, localIndex)).rgba;
        (SharedData[localIndex] = (float2)((CurrentPixelValue).r));
        simdgroup_barrier(mem_flags::mem_threadgroup);
        float2 resultColor = ((float)((CurrentPixelValue).r) * weight[0]);
        for (int i = 1; (i < 3); (i++))
        {
            float weihtValue = weight[i];
            (resultColor += (float2)(((float2)(SharedData[(localIndex + i)]) * (float2)(weihtValue))));
            (resultColor += (float2)(((float2)(SharedData[(localIndex - i)]) * (float2)(weihtValue))));
        }
        (OutputTex.write(float4((resultColor).r, (CurrentPixelValue).gb, (resultColor).g), uint2(uint2(groupIndex, localIndex))));
    };

    Compute_Shader(
texture2d<float> InputTex,texture2d<float, access::read_write> OutputTex,constant Uniforms_RootConstantScreenSize & RootConstantScreenSize) :
InputTex(InputTex),OutputTex(OutputTex),RootConstantScreenSize(RootConstantScreenSize) {}
};

//[numthreads(1, 1024, 1)]
kernel void stageMain(
uint3 GroupId [[threadgroup_position_in_grid]],
uint3 GroupThreadId [[thread_position_in_threadgroup]],
uint GroupIndex [[thread_index_in_threadgroup]],
    constant volumetricCloud::ComputeArgData& argBufferStatic [[buffer(UPDATE_FREQ_NONE)]],
    constant Compute_Shader::Uniforms_RootConstantScreenSize & RootConstantScreenSize [[buffer(UPDATE_FREQ_USER)]])
{
    uint3 GroupId0;
    GroupId0 = GroupId;
    uint3 GroupThreadId0;
    GroupThreadId0 = GroupThreadId;
    uint GroupIndex0;
    GroupIndex0 = GroupIndex;
    Compute_Shader main(
    argBufferStatic.InputTex,
    argBufferStatic.OutputTex,
    RootConstantScreenSize);
    return main.main(GroupId0, GroupThreadId0, GroupIndex0);
}
