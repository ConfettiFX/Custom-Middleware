/*
 * Copyright (c) 2018 Kostas Anagnostou (https://twitter.com/KostasAAA).
 * 
 * This file is part of The-Forge
 * (see https://github.com/ConfettiFX/The-Forge).
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
*/

#define ITERATIONS			12
#define SUBPIXEL_QUALITY	0.75
#define EDGE_THRESHOLD_MIN	0.0312
#define EDGE_THRESHOLD_MAX	0.125

Texture2D SrcTexture : register(t0);

SamplerState g_LinearClamp : register(s0);

cbuffer FXAARootConstant : register(b21) 
{
	float2 ScreenSize;
	float Use;
	float Time;
}

float rgb2luma(float3 rgb){
    return sqrt(dot(rgb, float3(0.299, 0.587, 0.114)));
}

float3 FXAA( float2 UV, int2 Pixel )
{
	float QUALITY[ITERATIONS] = {0.0, 0.0, 0.0, 0.0, 0.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0};

	float3 colorCenter = SrcTexture.Load( int3(Pixel.x, Pixel.y, 0), int2(0, 0) ).rgb;

	// Luma at the current fragment
	float lumaCenter = rgb2luma(colorCenter);

	// Luma at the four direct neighbours of the current fragment.
	float lumaD = rgb2luma( SrcTexture.Load( int3(Pixel.x, Pixel.y, 0), int2(0, -1) ).rgb);
	float lumaU = rgb2luma( SrcTexture.Load( int3(Pixel.x, Pixel.y, 0), int2(0, 1) ).rgb);
	float lumaL = rgb2luma( SrcTexture.Load( int3(Pixel.x, Pixel.y, 0), int2(-1, 0) ).rgb);
	float lumaR = rgb2luma( SrcTexture.Load( int3(Pixel.x, Pixel.y, 0), int2(1, 0) ).rgb);

	// Find the maximum and minimum luma around the current fragment.
	float lumaMin = min(lumaCenter,min(min(lumaD,lumaU),min(lumaL,lumaR)));
	float lumaMax = max(lumaCenter,max(max(lumaD,lumaU),max(lumaL,lumaR)));

	// Compute the delta.
	float lumaRange = lumaMax - lumaMin;

	if(lumaRange < max(EDGE_THRESHOLD_MIN, lumaMax * EDGE_THRESHOLD_MAX))
		return SrcTexture.Sample(g_LinearClamp, UV).rgb;

	// Query the 4 remaining corners lumas.
	float lumaDL = rgb2luma( SrcTexture.Load( int3(Pixel.x, Pixel.y, 0), int2(-1, -1) ).rgb);
	float lumaUR = rgb2luma( SrcTexture.Load( int3(Pixel.x, Pixel.y, 0), int2(1, 1) ).rgb);
	float lumaUL = rgb2luma( SrcTexture.Load( int3(Pixel.x, Pixel.y, 0), int2(-1, 1) ).rgb);
	float lumaDR = rgb2luma( SrcTexture.Load( int3(Pixel.x, Pixel.y, 0), int2(1, -1) ).rgb);

	// Combine the four edges lumas (using intermediary variables for future computations with the same values).
	float lumaDownUp = lumaD + lumaU;
	float lumaLeftRight = lumaL + lumaR;

	// Same for corners
	float lumaLeftCorners = lumaDL + lumaUL;
	float lumaDownCorners = lumaDL + lumaDR;
	float lumaRightCorners = lumaDR + lumaUR;
	float lumaUpCorners = lumaUR + lumaUL;

	// Compute an estimation of the gradient along the horizontal and vertical axis.
	float edgeHorizontal =  abs(-2.0 * lumaL + lumaLeftCorners)  + abs(-2.0 * lumaCenter + lumaDownUp ) * 2.0    + abs(-2.0 * lumaR + lumaRightCorners);
	float edgeVertical =    abs(-2.0 * lumaU + lumaUpCorners)      + abs(-2.0 * lumaCenter + lumaLeftRight) * 2.0  + abs(-2.0 * lumaD + lumaDownCorners);

	// Is the local edge horizontal or vertical ?
	float isHorizontal = (edgeHorizontal >= edgeVertical) ? 0.0f : 1.0f;

	// Select the two neighboring texels lumas in the opposite direction to the local edge.
	float luma1 = lerp(lumaD, lumaL, isHorizontal);
	float luma2 = lerp(lumaU, lumaR, isHorizontal);

	// Compute gradients in this direction.
	float gradient1 = luma1 - lumaCenter;
	float gradient2 = luma2 - lumaCenter;

	// Which direction is the steepest ?
	bool is1Steepest = abs(gradient1) >= abs(gradient2);

	// Gradient in the corresponding direction, normalized.
	float gradientScaled = 0.25*max(abs(gradient1),abs(gradient2));

	// Choose the step size (one pixel) according to the edge direction.

	float2 inverseScreenSize = float2((1.0 / ScreenSize.x), (1.0 / ScreenSize.y)); 

	float stepLength = lerp( inverseScreenSize.y , inverseScreenSize.x, isHorizontal);

	// Average luma in the correct direction.
	float lumaLocalAverage = 0.0;

	if(is1Steepest)
	{
		// Switch the direction
		stepLength = - stepLength;
		lumaLocalAverage = 0.5*(luma1 + lumaCenter);
	}
	else
	{
		lumaLocalAverage = 0.5*(luma2 + lumaCenter);
	}

	// Shift UV in the correct direction by half a pixel.
	float2 currentUv = UV;
	if(isHorizontal < 0.5f)
	{
		currentUv.y += stepLength * 0.5;
	}
	else
	{
		currentUv.x += stepLength * 0.5;
	}

	// Compute offset (for each iteration step) in the right direction.
	float2 offset = lerp( float2(inverseScreenSize.x,0.0) , float2(0.0,inverseScreenSize.y), isHorizontal);
	// Compute UVs to explore on each side of the edge, orthogonally. The QUALITY allows us to step faster.
	float2 uv1 = currentUv - offset;
	float2 uv2 = currentUv + offset;

	// Read the lumas at both current extremities of the exploration segment, and compute the delta wrt to the local average luma.
	float lumaEnd1 = rgb2luma( SrcTexture.Sample(g_LinearClamp, uv1).rgb );
	float lumaEnd2 = rgb2luma( SrcTexture.Sample(g_LinearClamp, uv2).rgb );
	lumaEnd1 -= lumaLocalAverage;
	lumaEnd2 -= lumaLocalAverage;

	// If the luma deltas at the current extremities are larger than the local gradient, we have reached the side of the edge.
	bool reached1 = abs(lumaEnd1) >= gradientScaled;
	bool reached2 = abs(lumaEnd2) >= gradientScaled;
	bool reachedBoth = reached1 && reached2;

	// If the side is not reached, we continue to explore in this direction.
	if(!reached1)
	{
		uv1 -= offset;
	}
	if(!reached2)
	{
		uv2 += offset;
	}  
	

	if(!reachedBoth)
	{
		[unroll]
		for(int i = 2; i < ITERATIONS; ++i)
		{
			// If needed, read luma in 1st direction, compute delta.
			if(!reached1)
			{
				lumaEnd1 = rgb2luma( SrcTexture.Sample(g_LinearClamp, uv1).rgb );
				lumaEnd1 = lumaEnd1 - lumaLocalAverage;
			}

			// If needed, read luma in opposite direction, compute delta.
			if(!reached2)
			{
				lumaEnd2 = rgb2luma( SrcTexture.Sample(g_LinearClamp, uv2).rgb );
				lumaEnd2 = lumaEnd2 - lumaLocalAverage;
			}

			// If the luma deltas at the current extremities is larger than the local gradient, we have reached the side of the edge.
			reached1 = abs(lumaEnd1) >= gradientScaled;
			reached2 = abs(lumaEnd2) >= gradientScaled;
			reachedBoth = reached1 && reached2;

			// If the side is not reached, we continue to explore in this direction, with a variable quality.
			if(!reached1){
				uv1 -= offset * QUALITY[i];
			}
			if(!reached2){
				uv2 += offset * QUALITY[i];
			}

			// If both sides have been reached, stop the exploration.
			if(reachedBoth){ break;}
		}
	}


	// Compute the distances to each extremity of the edge.
	float distance1 = lerp( (UV.x - uv1.x) , (UV.y - uv1.y), isHorizontal);
	float distance2 = lerp( (uv2.x - UV.x) , (uv2.y - UV.y), isHorizontal);

	// In which direction is the extremity of the edge closer ?
	bool isDirection1 = distance1 < distance2;
	float distanceFinal = min(distance1, distance2);

	// Length of the edge.
	float edgeThickness = (distance1 + distance2);

	// UV offset: read in the direction of the closest side of the edge.
	float pixelOffset = - distanceFinal / edgeThickness + 0.5;

	// Is the luma at center smaller than the local average ?
	bool isLumaCenterSmaller = lumaCenter < lumaLocalAverage;

	// If the luma at center is smaller than at its neighbour, the delta luma at each end should be positive (same variation).
	// (in the direction of the closer side of the edge.)
	bool correctVariation = ((isDirection1 ? lumaEnd1 : lumaEnd2) < 0.0) != isLumaCenterSmaller;

	// If the luma variation is incorrect, do not offset.
	float finalOffset = correctVariation ? pixelOffset : 0.0;

	// Sub-pixel shifting
	// Full weighted average of the luma over the 3x3 neighborhood.
	float lumaAverage = (1.0/12.0) * (2.0 * (lumaDownUp + lumaLeftRight) + lumaLeftCorners + lumaRightCorners);
	// Ratio of the delta between the global average and the center luma, over the luma range in the 3x3 neighborhood.
	float subPixelOffset1 = clamp(abs(lumaAverage - lumaCenter)/lumaRange,0.0,1.0);
	float subPixelOffset2 = (-2.0 * subPixelOffset1 + 3.0) * subPixelOffset1 * subPixelOffset1;
	// Compute a sub-pixel offset based on this delta.
	float subPixelOffsetFinal = subPixelOffset2 * subPixelOffset2 * SUBPIXEL_QUALITY;

	// Pick the biggest of the two offsets.
	finalOffset = max(finalOffset,subPixelOffsetFinal);

	// Compute the final UV coordinates.
	float2 finalUv = UV;
	if(isHorizontal < 0.5f)
	{
		finalUv.y += finalOffset * stepLength;
	}
	else
	{
		finalUv.x += finalOffset * stepLength;
	}

	// Read the color at the new UV coordinates, and use it.
	return SrcTexture.Sample(g_LinearClamp, finalUv).rgb;
}

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
    return fragColor + random(float3(fragCoord, Time));
}

struct PSIn {
	float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

float4 main(PSIn input) : SV_TARGET
{	
	float3 result = float3(0.0, 0.0, 0.0);
	if(Use > 0.5f)
		result = FXAA(input.TexCoord, int2(input.TexCoord * ScreenSize));
	else
		result = SrcTexture.Sample(g_LinearClamp, input.TexCoord).rgb;

  result.xyz = quantise(result.xyz, input.TexCoord);

	return float4(result.r, result.g, result.b, 1.0);	
}