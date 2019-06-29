/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

cbuffer SpaceUniform : register(b1)
{
	float4x4 ViewProjMat;
	float4 LightDirection;
	float4 ScreenSize;
};

Texture2D depthTexture : register(t5);

struct VSOutput {
	float4 Position : SV_POSITION;
	float4 Normal : NORMAL;
    float4 Color : COLOR;
	float2 ScreenCoord : TexCoord;
};

float hash(float n)
{
	return frac(sin(n) *cos(LightDirection.w * 0.00001) * 1e4);

}

float hash(float2 p) { return frac(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x)))); }
float noise(float x) { float i = floor(x); float f = frac(x); float u = f * f * (3.0 - 2.0 * f); return lerp(hash(i), hash(i + 1.0), u); }
float noise(float2 x) { float2 i = floor(x); float2 f = frac(x); float a = hash(i); float b = hash(i + float2(1.0, 0.0)); float c = hash(i + float2(0.0, 1.0)); float d = hash(i + float2(1.0, 1.0)); float2 u = f * f * (3.0 - 2.0 * f); return lerp(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y; }

float noise( float3 x )
			{
			    // The noise function returns a value in the range -1.0f -> 1.0f

			    float3 p = floor(x);
			    float3 f = frac(x);

			    f       = f*f*(3.0-2.0*f);
			    float n = p.x + p.y*57.0 + 113.0*p.z;

			    return lerp(lerp(lerp( hash(n+0.0), hash(n+1.0),f.x),
			                   lerp( hash(n+57.0), hash(n+58.0),f.x),f.y),
			               lerp(lerp( hash(n+113.0), hash(n+114.0),f.x),
			                   lerp( hash(n+170.0), hash(n+171.0),f.x),f.y),f.z);
			}

float rand(float2 co) {
	return frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453);
}

//refer to Morgan McGuire's Earth-like Tiny Planet
float3 addStars(float2 screenSize, float3 fs_UV)
{
	float time = LightDirection.w;

	// Background starfield
	float galaxyClump = (pow(noise(fs_UV.xyz * (30.0 * screenSize.x)), 3.0) * 0.5 + pow(noise(100.0 + fs_UV.xyz * (15.0 * screenSize.x)), 5.0)) / 3.5;

	float color = galaxyClump * pow(hash(fs_UV.xy), 1500.0) * 80.0;
	float3 starColor = float3(color, color, color);

	starColor.x *= sqrt(noise(fs_UV.xyz) * 1.2);
	starColor.y *= sqrt(noise(fs_UV.xyz * 4.0));

	float2 delta = (fs_UV.xy - screenSize.xy * 0.5) * screenSize.y * 1.2;
	float radialNoise = lerp(1.0, noise(normalize(delta) * 20.0), 0.12);

	float att = 0.057 * pow(max(0.0, 1.0 - (length(delta) - 0.9) / 0.9), 8.0);

	starColor += radialNoise * float3(0.2, 0.3, 0.5) * min(1.0, att); /*u_AtmosphereColor.xyz */ 

	float randSeed = rand(fs_UV);

	return starColor * ((sin(randSeed + randSeed * time* 0.05) + 1.0)* 0.4 + 0.2);
}

float4 main(VSOutput In): SV_Target
{	
	int3 LoadCoord = int3((int2) In.Position, 0);

	float depth = depthTexture.Load(LoadCoord).x;

	if (depth < 1.0f)
		discard;


	// Background stars
	return float4(addStars(ScreenSize.xy, In.Position.xyz), saturate(saturate(-LightDirection.y + 0.2f) * 2.0f) );
}
//#endif