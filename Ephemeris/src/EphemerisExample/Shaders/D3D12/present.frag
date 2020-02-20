/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creatifloatommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

Texture2D SrcTexture : register(t0);
//Texture2D shadowTexture : register(t2);
SamplerState g_LinearClamp : register(s0);

cbuffer PresentRootConstant : register(b22) 
{
	float time;
  float pad0;
  float pad1;
  float pad2;
}

// Dithering refered to https://www.shadertoy.com/view/MdVfz3
// A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint hash( uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}

// Compound versions of the hashing algorithm I whipped together.
uint hash( uint2 v ) { return hash( v.x ^ hash(v.y)                         ); }
uint hash( uint3 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z)             ); }
uint hash( uint4 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w) ); }

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct( uint m ) {
    // const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeMantissa = 0x00007FFFu;
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = asfloat( m );               // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}

// Pseudo-random value in half-open range [0:1].
// NL because of >>8 mantissa returns in range [0:1/256] which is perfect for quantising
float random( float x ) { return floatConstruct(hash(asuint(x))); }
float random( float2  v ) { return floatConstruct(hash(asuint(v))); }
float random( float3  v ) { return floatConstruct(hash(asuint(v))); }
float random( float4  v ) { return floatConstruct(hash(asuint(v))); }

/* stuff by nomadic lizard */

float3 quantise(in float3 fragColor, in float2 fragCoord)
{
    return fragColor + random(float3(fragCoord, time));
}

struct PSIn {
	float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

float4 main(PSIn input) : SV_TARGET
{
  float4 sceneColor = SrcTexture.Sample(g_LinearClamp, input.TexCoord);
  sceneColor.xyz = quantise(sceneColor.xyz, input.TexCoord);

	return sceneColor;
}