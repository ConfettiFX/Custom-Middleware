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

inline float rcp(float x) {
    return 1.0f / x;
}
struct Fragment_Shader
{
    struct Uniforms_cbRootConstant
    {
        float heightScale;
    };
    constant Uniforms_cbRootConstant & cbRootConstant;
    texture2d<float> Heightmap;
    sampler g_LinearMirror;
    struct PsIn
    {
        float4 position [[position]];
        float2 screenPos [[sample_perspective]];
    };
    float GET_ELEV(float2 f2ElevMapUV, float fMIPLevel, int2 Offset)
    {
        return Heightmap.sample(g_LinearMirror, f2ElevMapUV, level(fMIPLevel)).r;
    };
    float3 ComputeNormal(float2 f2ElevMapUV, float fSampleSpacingInterval, float fMIPLevel)
    {
        float Height00 = GET_ELEV(f2ElevMapUV, fMIPLevel, int2((-1), (-1)));
        float Height10 = GET_ELEV(f2ElevMapUV, fMIPLevel, int2(0, (-1)));
        float Height20 = GET_ELEV(f2ElevMapUV, fMIPLevel, int2((+1), (-1)));
        float Height01 = GET_ELEV(f2ElevMapUV, fMIPLevel, int2((-1), 0));
        float Height21 = GET_ELEV(f2ElevMapUV, fMIPLevel, int2((+1), 0));
        float Height02 = GET_ELEV(f2ElevMapUV, fMIPLevel, int2((-1), (+1)));
        float Height12 = GET_ELEV(f2ElevMapUV, fMIPLevel, int2(0, (+1)));
        float Height22 = GET_ELEV(f2ElevMapUV, fMIPLevel, int2((+1), (+1)));
        float3 Grad;
        ((Grad).x = (((Height00 + Height01) + Height02) - ((Height20 + Height21) + Height22)));
        ((Grad).y = (((Height00 + Height10) + Height20) - ((Height02 + Height12) + Height22)));
        ((Grad).z = ((fSampleSpacingInterval * 6.0) * rcp(cbRootConstant.heightScale)));
        ((Grad).xy *= ((const float2)(((const float)(65536) * 0.015000000)) * float2(1.0, (-1.0))));
        float3 Normal = normalize(Grad);
        return Normal;
    };
    float2 main(PsIn In)
    {
        float2 f2UV = (float2(0.5, 0.5) + (float2(0.5, (-0.5)) * ((In).screenPos).xy));
        float3 Normal = ComputeNormal(f2UV, ((float)(0.015000000) * exp2(9.0)), 9.0);
        return (Normal).xy;
    };

    Fragment_Shader(
constant Uniforms_cbRootConstant & cbRootConstant,texture2d<float> Heightmap,sampler g_LinearMirror) :
cbRootConstant(cbRootConstant),Heightmap(Heightmap),g_LinearMirror(g_LinearMirror) {}
};

struct ArgsData
{
    texture2d<float> Heightmap;
    sampler g_LinearMirror;
};

fragment float4 stageMain(
    Fragment_Shader::PsIn In [[stage_in]],
    constant ArgsData& argBufferStatic [[buffer(UPDATE_FREQ_NONE)]],
    constant Fragment_Shader::Uniforms_cbRootConstant & cbRootConstant [[buffer(UPDATE_FREQ_USER)]]
)
{
    Fragment_Shader::PsIn In0;
    In0.position = float4(In.position.xyz, 1.0 / In.position.w);
    In0.screenPos = In.screenPos;
    Fragment_Shader main(
    cbRootConstant,
    argBufferStatic.Heightmap,
    argBufferStatic.g_LinearMirror);
    return float4(main.main(In0), 0.0, 0.0);
}
