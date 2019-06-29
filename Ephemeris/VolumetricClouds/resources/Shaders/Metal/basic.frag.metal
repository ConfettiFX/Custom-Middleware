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

struct Fragment_Shader
{
    texture2d<float> highResCloudTexture;
    sampler BilinearClampSampler;
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
    struct PSIn
    {
        float4 Position [[position]];
        float2 TexCoord;
        float2 VSray;
    };
    float4 main(PSIn input)
    {
        float4 result = highResCloudTexture.sample(BilinearClampSampler, (input).TexCoord);
        return result;
    };

    Fragment_Shader(
texture2d<float> highResCloudTexture,sampler BilinearClampSampler,constant Uniforms_uniformGlobalInfoRootConstant & uniformGlobalInfoRootConstant) :
highResCloudTexture(highResCloudTexture),BilinearClampSampler(BilinearClampSampler),uniformGlobalInfoRootConstant(uniformGlobalInfoRootConstant) {}
};


fragment float4 stageMain(
    Fragment_Shader::PSIn input [[stage_in]],
    texture2d<float> highResCloudTexture [[texture(0)]],
    sampler BilinearClampSampler [[sampler(0)]],
    constant Fragment_Shader::Uniforms_uniformGlobalInfoRootConstant & uniformGlobalInfoRootConstant [[buffer(1)]])
{
    Fragment_Shader::PSIn input0;
    input0.Position = float4(input.Position.xyz, 1.0 / input.Position.w);
    input0.TexCoord = input.TexCoord;
    input0.VSray = input.VSray;
    Fragment_Shader main(
    highResCloudTexture,
    BilinearClampSampler,
    uniformGlobalInfoRootConstant);
    return main.main(input0);
}
