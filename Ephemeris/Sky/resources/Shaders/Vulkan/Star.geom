/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core


layout(points) in;
layout(location = 0) in vec3 geomInput_Position[];
layout(location = 1) in vec4 geomInput_Color[];
layout(location = 2) in vec4 geomInput_StarInfo[];

layout(location = 0) out vec3 geomOutput_TexCoord;
layout(location = 1) out vec2 geomOutput_ScreenCoord;
layout(location = 2) out vec3 geomOutput_Color;


layout(set = 0, binding = 11) uniform StarUniform_Block
{
    mat4 RotMat;
    mat4 ViewProjMat;
    vec4 LightDirection;
    vec4 Dx;
    vec4 Dy;
}StarUniform;

struct GsIn
{
    vec3 Position;
    vec4 Color;
    vec4 StarInfo;
};

struct PsIn
{
    vec4 position;
    vec3 texCoord;
    vec2 screenCoord;
    vec3 color;
};

void PushVertex(vec3 pos, vec3 color, vec3 dx, vec3 dy, vec2 vOffset, vec2 baseTC)
{
    PsIn Out;
    vec4 position = StarUniform.RotMat * vec4(pos.x, pos.y, pos.z, 1.0);
    gl_Position = StarUniform.ViewProjMat * vec4(position.xyz + (dx * vOffset.x) + (dy * vOffset.y), 1.0);

    geomOutput_TexCoord.z = gl_Position.z;

    ((gl_Position).z = (gl_Position).w);
    gl_Position /= vec4 ((gl_Position).w);

    geomOutput_ScreenCoord = vec2((((gl_Position).x + float (1.0)) * float (0.5)), ((float (1.0) - (gl_Position).y) * float (0.5)));

    //(vOffset += vec2(1, (-1)));
    //(vOffset *= vec2(0.5, (-0.5)));
    geomOutput_TexCoord.xy = (baseTC + vOffset);
    geomOutput_Color.xyz = color;
    EmitVertex();
}

layout(triangle_strip, max_vertices = 4) out;
void HLSLmain(GsIn In[1], uint VertexID)
{
    PsIn Out;
    vec2 baseTC = vec2(0, 0);

    vec4 starInfo         = In[0].StarInfo;
    float particleSize    = starInfo.y;
    vec3 Width            = StarUniform.Dx.xyz * particleSize;
    vec3 Height           = StarUniform.Dy.xyz * particleSize;

    vec3 Color = In[0].Color.rgb * In[0].Color.a;
    float time = StarUniform.LightDirection.a;
    float timeSeed = starInfo.z;
    float timeScale = starInfo.a;

    float blink = (sin(timeSeed + time * timeScale ) + 1.0f) * 0.5f;
    Color *= blink;

    PushVertex(((In[0]).Position).xyz, Color, Width, Height, vec2((-1),(-1)), baseTC);
    PushVertex(((In[0]).Position).xyz, Color, Width, Height, vec2(  1, (-1)), baseTC);
    PushVertex(((In[0]).Position).xyz, Color, Width, Height, vec2((-1),  1),  baseTC);
    PushVertex(((In[0]).Position).xyz, Color, Width, Height, vec2(  1,   1),  baseTC);
    EndPrimitive();
}
void main()
{
    GsIn In[1];
    In[0].Position = geomInput_Position[0].xyz;
    In[0].Color    = geomInput_Color[0];
    In[0].StarInfo = geomInput_StarInfo[0];
    uint VertexID;
    VertexID = gl_PrimitiveIDIn;
    PsIn Stream;
    HLSLmain(In, VertexID);
}
