/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core


layout(location = 0) in vec3 fragInput_TexCoord0;
layout(location = 1) in vec2 fragInput_TexCoord1;
layout(location = 0) out vec4 rast_FragData0; 

layout(set = 0, binding = 9) uniform SunUniform_Block
{
    mat4 ViewMat;
    mat4 ViewProjMat;
    vec4 LightDirection;
    vec4 Dx;
    vec4 Dy;
}SunUniform;

struct PsIn
{
    vec4 position;
    vec3 texCoord;
    vec2 screenCoord;
};
layout(set = 0, binding = 8) uniform texture2D depthTexture;
layout(set = 0, binding = 10) uniform texture2D moonAtlas;
layout(set = 0, binding = 12) uniform sampler g_LinearBorder;
vec4 HLSLmain(PsIn In)
{
    vec2 screenUV = (In).screenCoord;
    float sceneDepth = float ((textureLod(sampler2D( depthTexture, g_LinearBorder), vec2(screenUV), float (0))).r);
    if((sceneDepth < float (1.0)))
    {
        discard;
    }
    float ISun = 1.0;
    vec4 res;
    if(In.texCoord.z >= 0.0)
    {
        vec2 uv = In.texCoord.xy * 2.0 - vec2(0.5, 0.5);
        float param = (2.0 * sqrt((uv.x - 0.5) * (uv.x - 0.5) + (uv.y - 0.5) * (uv.y - 0.5)));
        float blendFactor = smoothstep(float (1), float (0.8), param);
        res = vec4((vec3(1.0, 0.95294, 0.91765) * vec3 (ISun)), blendFactor);
    }
    else
    {
        vec3 normal;
        ((normal).xy = ((((In).texCoord).xy - vec2 (0.5)) * vec2 (2)));
        ((normal).z = (float (1) - sqrt(clamp((((normal).x * (normal).x) + ((normal).y * (normal).y)), 0.0, 1.0))));
        res = texture(sampler2D( moonAtlas, g_LinearBorder), In.texCoord.xy * 2.0 - vec2(0.5, 0.5) );
    }

    vec2 glow = clamp(abs(In.texCoord.xy * 1.5 - vec2(0.75, 0.75)), 0.0, 1.0);
   
    float gl = clamp(1.0f - sqrt(dot(glow, glow)), 0.0, 1.0);
    gl = pow(gl, 2.1) * 1.5;
		res = mix(vec4(gl, gl, gl, res.a), res, res.a);
    res.a = clamp(gl + res.a, 0.0, 1.0);

		return res;
}
void main()
{
    PsIn In;
    In.position = vec4(gl_FragCoord.xyz, 1.0 / gl_FragCoord.w);
    In.texCoord = fragInput_TexCoord0;
    In.screenCoord = fragInput_TexCoord1;
    vec4 result = HLSLmain(In);
    rast_FragData0 = result;
}
