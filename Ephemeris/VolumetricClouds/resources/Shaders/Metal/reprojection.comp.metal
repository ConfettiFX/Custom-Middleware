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
    texture2d<float> LowResCloudTexture;
    texture2d<float> PrevFrameTexture;
    sampler BilinearClampSampler;
    sampler PointClampSampler;
    texture2d<float, access::read_write> DstTexture;
    struct Uniforms_uniformGlobalInfoRootConstant
    {
        float2 _Time;
        float2 screenSize;
        float4 lightDirection;
        float4 lightColorAndIntensity;
        float4 cameraPosition;
        float4x4 VP;
        float4x4 InvVP;
        float4x4 InvWorldToCamera;
        float4x4 prevVP;
        float4x4 LP;
        float near;
        float far;
        float correctU;
        float correctV;
        float4 ProjectionExtents;
        uint lowResFrameIndex;
        uint JitterX;
        uint JitterY;
        float exposure;
        float decay;
        float density;
        float weight;
        uint NUM_SAMPLES;
        float4 skyBetaR;
        float4 skyBetaV;
        float turbidity;
        float rayleigh;
        float mieCoefficient;
        float mieDirectionalG;
    };
    constant Uniforms_uniformGlobalInfoRootConstant & uniformGlobalInfoRootConstant;
    float4 SamplePrev(float4 worldPos, thread float(& outOfBound))
    {
        float4 prevUV = ((uniformGlobalInfoRootConstant.prevVP)*(worldPos));
        (prevUV /= (float4)((prevUV).w));
        ((prevUV).xy = float2((((prevUV).x + (float)(1.0)) * (float)(0.5)), (((float)(1.0) - (prevUV).y) * (float)(0.5))));
        float oobmax = max((-(prevUV).x), (-(prevUV).y));
        float oobmin = (max((prevUV).x, (prevUV).y) - (float)(1.0));
        (outOfBound = step(0.0, max(oobmin, oobmax)));
        return PrevFrameTexture.sample(BilinearClampSampler, (prevUV).xy, level(0));
    };
    float4 SampleCurrent(float2 uv, float2 _Jitter)
    {
        (uv = (uv - ((_Jitter - (float2)(1.5)) * ((float2)(1.0) / (uniformGlobalInfoRootConstant.screenSize).xy))));
        return LowResCloudTexture.sample(PointClampSampler, uv, level(0));
    };
    float CurrentCorrect(float2 uv, float2 jitter)
    {
        float2 texelRelativePos = fmod((uv * (uniformGlobalInfoRootConstant.screenSize).xy), 4.0);
        (texelRelativePos -= jitter);
        float2 valid = saturate(((float2)(2.0) * (float2(0.5, 0.5) - abs((texelRelativePos - float2(0.5, 0.5))))));
        return ((valid).x * (valid).y);
    };
#define THREAD_SIZE 16
    void main(uint3 GTid, uint3 Gid, uint3 DTid)
    {
        float2 ScreenUV;
        ((ScreenUV).x = ((float)((float)(DTid.x) + 0.5) / (uniformGlobalInfoRootConstant.screenSize).x));
        ((ScreenUV).y = ((float)((float)(DTid.y) + 0.5) / (uniformGlobalInfoRootConstant.screenSize).y));
        float outOfBound;
        float2 _Jitter = float2(float(uniformGlobalInfoRootConstant.JitterX), float(uniformGlobalInfoRootConstant.JitterY));
        float4 currSample = SampleCurrent(ScreenUV, _Jitter);
        float depth = (currSample).b;
        float4 result = currSample;
        float3 ScreenNDC;
        ((ScreenNDC).x = (((ScreenUV).x * (float)(2.0)) - (float)(1.0)));
        ((ScreenNDC).y = ((((float)(1.0) - (ScreenUV).y) * (float)(2.0)) - (float)(1.0)));
        float3 vspos = (float3((((ScreenNDC).xy * (uniformGlobalInfoRootConstant.ProjectionExtents).xy) + (uniformGlobalInfoRootConstant.ProjectionExtents).zw), 1.0) * (float3)(depth));
        float4 worldPos = ((uniformGlobalInfoRootConstant.InvWorldToCamera)*(float4(vspos, 1.0)));
        float4 prevSample = SamplePrev(worldPos, outOfBound);
        float correct = max((CurrentCorrect(ScreenUV, _Jitter) * 0.5), outOfBound);
        (DstTexture.write(mix(prevSample, result, correct), uint2(int2((DTid).xy))));
    };

    Compute_Shader(
texture2d<float> LowResCloudTexture,texture2d<float> PrevFrameTexture,sampler BilinearClampSampler,sampler PointClampSampler,texture2d<float, access::read_write> DstTexture,constant Uniforms_uniformGlobalInfoRootConstant & uniformGlobalInfoRootConstant) :
LowResCloudTexture(LowResCloudTexture),PrevFrameTexture(PrevFrameTexture),BilinearClampSampler(BilinearClampSampler),PointClampSampler(PointClampSampler),DstTexture(DstTexture),uniformGlobalInfoRootConstant(uniformGlobalInfoRootConstant) {}
};

//[numthreads(16, 16, 1)]
kernel void stageMain(
uint3 GTid [[thread_position_in_threadgroup]],
uint3 Gid [[threadgroup_position_in_grid]],
uint3 DTid [[thread_position_in_grid]],
    texture2d<float> LowResCloudTexture [[texture(0)]],
    texture2d<float> PrevFrameTexture [[texture(1)]],
    sampler BilinearClampSampler [[sampler(0)]],
    sampler PointClampSampler [[sampler(1)]],
    texture2d<float, access::read_write> DstTexture [[texture(2)]],
    constant Compute_Shader::Uniforms_uniformGlobalInfoRootConstant & uniformGlobalInfoRootConstant [[buffer(1)]])
{
    uint3 GTid0;
    GTid0 = GTid;
    uint3 Gid0;
    Gid0 = Gid;
    uint3 DTid0;
    DTid0 = DTid;
    Compute_Shader main(
    LowResCloudTexture,
    PrevFrameTexture,
    BilinearClampSampler,
    PointClampSampler,
    DstTexture,
    uniformGlobalInfoRootConstant);
    return main.main(GTid0, Gid0, DTid0);
}
