/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

cbuffer cbRootConstant : register(b0)
{
	float heightScale;
}

Texture2D Heightmap;

SamplerState g_LinearMirror;

struct PsIn
{
    float4 position : SV_Position;
    sample float2 screenPos : ScreenPos;
};

float3 ComputeNormal(float2 f2ElevMapUV,
                     float fSampleSpacingInterval,
                     float fMIPLevel)
{
#define GET_ELEV(Offset) Heightmap.SampleLevel( g_LinearMirror, f2ElevMapUV, fMIPLevel, Offset)

#if 1
    float Height00 = GET_ELEV( int2( -1, -1) );
    float Height10 = GET_ELEV( int2(  0, -1) );
    float Height20 = GET_ELEV( int2( +1, -1) );

    float Height01 = GET_ELEV( int2( -1, 0) );
    //float Height11 = GET_ELEV( int2(  0, 0) );
    float Height21 = GET_ELEV( int2( +1, 0) );

    float Height02 = GET_ELEV( int2( -1, +1) );
    float Height12 = GET_ELEV( int2(  0, +1) );
    float Height22 = GET_ELEV( int2( +1, +1) );

    float3 Grad;
    Grad.x = (Height00+Height01+Height02) - (Height20+Height21+Height22);
    Grad.y = (Height00+Height10+Height20) - (Height02+Height12+Height22);
    Grad.z = fSampleSpacingInterval * 6.f * rcp(heightScale);
    //Grad.x = (3*Height00+10*Height01+3*Height02) - (3*Height20+10*Height21+3*Height22);
    //Grad.y = (3*Height00+10*Height10+3*Height20) - (3*Height02+10*Height12+3*Height22);
    //Grad.z = fSampleSpacingInterval * 32.f;
#else
    float Height1 = GET_ELEV( int2( 1, 0) );
    float Height2 = GET_ELEV( int2(-1, 0) );
    float Height3 = GET_ELEV( int2( 0, 1) );
    float Height4 = GET_ELEV( int2( 0,-1) );
       
    float3 Grad;
    Grad.x = Height2 - Height1;
    Grad.y = Height4 - Height3;
    Grad.z = fSampleSpacingInterval * 2.f;
#endif
    Grad.xy *= (65536) * 0.015f * float2(1.0f,-1.0f);
    float3 Normal = normalize( Grad );

    return Normal;
}

float2 main(PsIn In) : SV_TARGET
{
  float2 f2UV = float2(0.5,0.5) + float2(0.5,-0.5) * In.screenPos.xy;
  float3 Normal = ComputeNormal( f2UV, 0.015*exp2(9), 9 );

  // Only xy components are stored. z component is calculated in the shader
  return Normal.xy;
}
