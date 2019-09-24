/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core

layout(location = 0) in vec3 fragInput_PositionWS;
layout(location = 1) in vec2 fragInput_TexCoord;
layout(location = 2) in vec3 fragInput_Normal;
layout(location = 3) in vec3 fragInput_Tangent;
layout(location = 4) in vec3 fragInput_Bitangent;
layout(location = 0) out vec4 rast_FragData0; 
layout(location = 1) out vec4 rast_FragData1; 

layout(UPDATE_FREQ_PER_FRAME, binding = 0) uniform RenderTerrainUniformBuffer_Block
{
    mat4 projView;
    vec4 TerrainInfo;
    vec4 CameraInfo;
}RenderTerrainUniformBuffer;

layout(UPDATE_FREQ_NONE, binding = 0) uniform texture2D NormalMap;
layout(UPDATE_FREQ_NONE, binding = 1) uniform texture2D MaskMap;
layout(UPDATE_FREQ_NONE, binding = 2) uniform texture2D tileTextures[5];
layout(UPDATE_FREQ_NONE, binding = 7) uniform texture2D tileTexturesNrm[5];
layout(UPDATE_FREQ_NONE, binding = 12) uniform texture2D shadowMap;

layout(UPDATE_FREQ_NONE, binding = 13) uniform sampler g_LinearMirror;
layout(UPDATE_FREQ_NONE, binding = 14) uniform sampler g_LinearWrap;
layout(UPDATE_FREQ_NONE, binding = 15) uniform sampler g_LinearBorder;

struct PsIn
{
    vec4 position;
    vec3 positionWS;
    vec2 texcoord;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
};
struct PsOut
{
    vec4 albedo;
    vec4 normal;
};
float DepthLinearization(float depth, float near, float far)
{
    return ((float (2.0) * near) / ((far + near) - (depth * (far - near))));
}

vec3 ReconstructNormal(in vec4 sampleNormal, float intensity)
{
  vec3 tangentNormal;
  tangentNormal.xy = (sampleNormal.rg * 2 - 1) * intensity;
  tangentNormal.z = sqrt(1.0f - clamp(dot(tangentNormal.xy, tangentNormal.xy), 0.0f, 1.0f));
  return tangentNormal;
}

PsOut HLSLmain(PsIn In)
{
    PsOut Out;
    float linearDepth = DepthLinearization(((In).position).z, (RenderTerrainUniformBuffer.CameraInfo).x, (RenderTerrainUniformBuffer.CameraInfo).y);
    float dist = distance((RenderTerrainUniformBuffer.CameraInfo).xyz, (In).positionWS);
    (dist = (dist / (RenderTerrainUniformBuffer.CameraInfo).w));
    float lod = 4.0;
    vec4 maskVal = textureLod(sampler2D( MaskMap, g_LinearMirror), vec2((In).texcoord), int (0.0));
    float baseWeight = clamp((float (1) - dot(maskVal, vec4(1, 1, 1, 1))), 0.0, 1.0);
    float baseTileScale = float (70);
    vec4 tileScale = vec4(100, 120, 80, 80);
    vec3 result = vec3(0, 0, 0);
    vec3 surfaceNrm = vec3 (0);
    for (uint i = uint (0); (i < uint (4)); (++i))
    {
        (result += vec3 (((textureLod(sampler2D( tileTextures[i], g_LinearWrap), vec2(((In).texcoord * vec2 (tileScale[i]))), lod)).xyz * vec3 (maskVal[i]))));
        //(surfaceNrm += (vec3 ((((textureLod(sampler2D( tileTexturesNrm[i], g_LinearWrap), vec2(((In).texcoord * vec2 (tileScale[i]))), lod)).xyz * vec3 (2)) - vec3 (1))) * vec3 (maskVal[i])));

        surfaceNrm += ReconstructNormal(textureLod(sampler2D(tileTexturesNrm[i], g_LinearWrap), In.texcoord * tileScale[i], lod), 1.0f) * maskVal[i];
    }
    vec3 baseColor = vec3 (((textureLod(sampler2D( tileTextures[4], g_LinearWrap), vec2(((In).texcoord * vec2 (baseTileScale))), lod)).xyz * vec3 (baseWeight)));
    (result += baseColor);
    //(surfaceNrm += vec3 (((textureLod(sampler2D( tileTexturesNrm[4], g_LinearWrap), vec2(((In).texcoord * vec2 (baseTileScale))), lod)).xyz * vec3 (baseWeight))));

    surfaceNrm += ReconstructNormal(textureLod(sampler2D(tileTexturesNrm[4], g_LinearWrap), In.texcoord * baseTileScale, lod), 1.0f) * baseWeight;

    vec3 EarthNormal = normalize((In).normal);
    vec3 EarthTangent = normalize((In).tangent);
    vec3 EarthBitangent = normalize((In).bitangent);
    vec3 f3TerrainNormal;
    if(((RenderTerrainUniformBuffer.TerrainInfo).y > 0.5))
    {
        ((f3TerrainNormal).xzy = vec3 ((((textureLod(sampler2D( NormalMap, g_LinearMirror), vec2(((In).texcoord).xy), int (0.0))).xyz * vec3 (2)) - vec3 (1))));
        vec2 f2XZSign = sign((vec2 (0.5) - fract((((In).texcoord).xy / vec2 (2)))));
        ((f3TerrainNormal).xz *= (f2XZSign * vec2 (0.1)));
        ((f3TerrainNormal).y /= (RenderTerrainUniformBuffer.TerrainInfo).z);
    }
    else
    {
        ((f3TerrainNormal).xz = vec2 ((textureLod(sampler2D( NormalMap, g_LinearMirror), vec2(((In).texcoord).xy), int (0.0))).xy));
        vec2 f2XZSign = sign((vec2 (0.5) - fract((((In).texcoord).xy / vec2 (2)))));
        ((f3TerrainNormal).xz *= f2XZSign);
        ((f3TerrainNormal).y = (sqrt(clamp((float (1) - dot((f3TerrainNormal).xz, (f3TerrainNormal).xz)), 0.0, 1.0)) / (RenderTerrainUniformBuffer.TerrainInfo).z));
    }
    (f3TerrainNormal = normalize(((f3TerrainNormal)*(mat3(EarthTangent, EarthNormal, (-EarthBitangent))))));
    vec3 f3TerrainTangent, f3TerrainBitangent;
    (f3TerrainTangent = normalize(cross(f3TerrainNormal, vec3(0, 0, 1))));
    (f3TerrainBitangent = normalize(cross(f3TerrainTangent, f3TerrainNormal)));
    (f3TerrainNormal = normalize((((surfaceNrm).xzy)*(mat3(f3TerrainTangent, f3TerrainNormal, (-f3TerrainBitangent))))));
    ((Out).albedo = vec4(result, 1.0));
    ((Out).normal = vec4(f3TerrainNormal, 1));
    return Out;
}
void main()
{
    PsIn In;
    In.position = vec4(gl_FragCoord.xyz, 1.0 / gl_FragCoord.w);
    In.positionWS = fragInput_PositionWS;
    In.texcoord = fragInput_TexCoord;
    In.normal = fragInput_Normal;
    In.tangent = fragInput_Tangent;
    In.bitangent = fragInput_Bitangent;
    PsOut result = HLSLmain(In);
    rast_FragData0 = result.albedo;
    rast_FragData1 = result.normal;
}
