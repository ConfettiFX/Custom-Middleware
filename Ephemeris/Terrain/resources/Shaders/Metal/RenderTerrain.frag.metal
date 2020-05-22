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

#include "terrain_argument_buffers.h"

inline float3x3 matrix_ctor(float4x4 m)
{
        return float3x3(m[0].xyz, m[1].xyz, m[2].xyz);
}
struct Fragment_Shader
{
    constant Uniforms_RenderTerrainUniformBuffer & RenderTerrainUniformBuffer;
    texture2d<float> NormalMap;
    texture2d<float> MaskMap;
    const array<texture2d<float>, 5> tileTextures;
    const array<texture2d<float>, 5> tileTexturesNrm;
    texture2d<float> shadowMap;
    sampler g_LinearMirror;
    sampler g_LinearWrap;
    sampler g_LinearBorder;
    struct PsIn
    {
        float4 position [[position]];
        float3 positionWS [[sample_perspective]];
        float2 texcoord;
        float3 normal;
        float3 tangent;
        float3 bitangent;
    };
    struct PsOut
    {
        float4 albedo [[color(0)]];
        float4 normal [[color(1)]];
    };
    float DepthLinearization(float depth, float near, float far)
    {
        return (((float)(2.0) * near) / ((far + near) - (depth * (far - near))));
    };

    float3 ReconstructNormal(float4 sampleNormal, float intensity)
    {
        float3 tangentNormal;
        tangentNormal.xy = (sampleNormal.rg * 2 - 1) * intensity;
        tangentNormal.z = sqrt(1 - saturate(dot(tangentNormal.xy, tangentNormal.xy)));
        return tangentNormal;
    }

    float MipLevel( float2 uv )
    {
        float TextureSize = 2048.0f;

        float2 dx = dfdx( uv * TextureSize );
        float2 dy = dfdy( uv * TextureSize );
        float d = max( dot( dx, dx ), dot( dy, dy ) );

        uint MipCount = uint(log2(TextureSize)) + 1;
        
        // Clamp the value to the max mip level counts
        float rangeClamp = pow(2.0f, (float)MipCount - 1.0f);
        d = clamp(sqrt(d), 1.0, rangeClamp);
            
        float mipLevel = 0.75f * log2(d);
        return mipLevel;
    }

    PsOut main(PsIn In)
    {
        PsOut Out;
        
        float dist = distance((RenderTerrainUniformBuffer.CameraInfo).xyz, (In).positionWS);
        (dist = (dist / (RenderTerrainUniformBuffer.CameraInfo).w));
        
        float4 maskVal = MaskMap.sample(g_LinearMirror, (In).texcoord, level(0.0));
        float baseWeight = saturate(((float)(1) - dot(maskVal, float4(1, 1, 1, 1))));
        float baseTileScale = 70;
        float4 tileScale = float4(100, 120, 80, 80);
        float3 result = float3(0, 0, 0);
        float3 surfaceNrm = (float3)(0);
        for (uint i = 0; (i < (uint)(4)); (++i))
        {
            float2 uv = In.texcoord * tileScale[i];
            float lod = MipLevel(uv);
            (result += (float3)(tileTextures[i].sample(g_LinearWrap, uv, level(lod)).xyz * (float3)(maskVal[i])));
            //(surfaceNrm += ((float3)(tileTexturesNrm[i].sample(g_LinearWrap, uv, level(lod)).xyz * (float3)(2) - (float3)(1)) * (float3)(maskVal[i])));
            surfaceNrm += (float3)ReconstructNormal(tileTexturesNrm[i].sample(g_LinearWrap, uv, level(lod)), 1.0f) * (float3)maskVal[i];
        }
        
        float2 uv = In.texcoord * baseTileScale;
        float lod = MipLevel(uv);
        float3 baseColor = tileTextures[4].sample(g_LinearWrap, uv, level(lod)).xyz * (float3)(baseWeight);
        (result += baseColor);
        //(surfaceNrm += (float3)(tileTexturesNrm[4].sample(g_LinearWrap, uv, level(lod)).xyz * (float3)(baseWeight)));
        surfaceNrm += (float3)ReconstructNormal(tileTexturesNrm[4].sample(g_LinearWrap, uv,  level(lod)), 1.0f) * (float3)baseWeight;
        float3 EarthNormal = normalize((In).normal);
        float3 EarthTangent = normalize((In).tangent);
        float3 EarthBitangent = normalize((In).bitangent);
        float3 f3TerrainNormal;
        if (((RenderTerrainUniformBuffer.TerrainInfo).y > 0.5))
        {
            ((f3TerrainNormal).xzy = (float3)(NormalMap.sample(g_LinearMirror, ((In).texcoord).xy, level(0.0)).xyz * (float3)(2) - (float3)(1)));
            float2 f2XZSign = sign(((float2)(0.5) - fract((((In).texcoord).xy / (float2)(2)))));
            ((f3TerrainNormal).xz *= (f2XZSign * (float2)(0.1)));
            ((f3TerrainNormal).y /= (RenderTerrainUniformBuffer.TerrainInfo).z);
        }
        else
        {
            ((f3TerrainNormal).xz = (float2)(NormalMap.sample(g_LinearMirror, ((In).texcoord).xy, level(0.0)).xy));
            float2 f2XZSign = sign(((float2)(0.5) - fract((((In).texcoord).xy / (float2)(2)))));
            ((f3TerrainNormal).xz *= f2XZSign);
            ((f3TerrainNormal).y = (sqrt(saturate(((float)(1) - dot((f3TerrainNormal).xz, (f3TerrainNormal).xz)))) / (RenderTerrainUniformBuffer.TerrainInfo).z));
        }
        (f3TerrainNormal = normalize(((f3TerrainNormal)*(float3x3(EarthTangent, EarthNormal, (-EarthBitangent))))));
        float3 f3TerrainTangent, f3TerrainBitangent;
        (f3TerrainTangent = normalize(cross(f3TerrainNormal, float3(0, 0, 1))));
        (f3TerrainBitangent = normalize(cross(f3TerrainTangent, f3TerrainNormal)));
        (f3TerrainNormal = normalize((((surfaceNrm).xzy)*(float3x3(f3TerrainTangent, f3TerrainNormal, (-f3TerrainBitangent))))));
        ((Out).albedo = float4(result, 1.0));
        ((Out).normal = float4(f3TerrainNormal, 1));
        return Out;
    };

    Fragment_Shader(constant Uniforms_RenderTerrainUniformBuffer & RenderTerrainUniformBuffer,
					texture2d<float> NormalMap,
					texture2d<float> MaskMap,
					const array<texture2d<float>, 5> tileTextures,
					const array<texture2d<float>, 5> tileTexturesNrm,
					texture2d<float> shadowMap,
					sampler g_LinearMirror,
					sampler g_LinearWrap,
					sampler g_LinearBorder) :
RenderTerrainUniformBuffer(RenderTerrainUniformBuffer),NormalMap(NormalMap),MaskMap(MaskMap),tileTextures(tileTextures),tileTexturesNrm(tileTexturesNrm),shadowMap(shadowMap),g_LinearMirror(g_LinearMirror),g_LinearWrap(g_LinearWrap),g_LinearBorder(g_LinearBorder) {}
};

fragment Fragment_Shader::PsOut stageMain(
    Fragment_Shader::PsIn In [[stage_in]],
    constant ArgData& argBufferStatic [[buffer(UPDATE_FREQ_NONE)]],
    constant ArgDataPerFrame& argBufferPerFrame [[buffer(UPDATE_FREQ_PER_FRAME)]]
)
{
    Fragment_Shader::PsIn In0;
    In0.position = float4(In.position.xyz, 1.0 / In.position.w);
    In0.positionWS = In.positionWS;
    In0.texcoord = In.texcoord;
    In0.normal = In.normal;
    In0.tangent = In.tangent;
    In0.bitangent = In.bitangent;
	
    Fragment_Shader main(
    argBufferPerFrame.RenderTerrainUniformBuffer,
    argBufferStatic.NormalMap,
    argBufferStatic.MaskMap,
    argBufferStatic.tileTextures,
    argBufferStatic.tileTexturesNrm,
    argBufferStatic.shadowMap,
    argBufferStatic.g_LinearMirror,
    argBufferStatic.g_LinearWrap,
    argBufferStatic.g_LinearBorder);
	
    return main.main(In0);
}
