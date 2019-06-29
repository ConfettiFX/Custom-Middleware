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
layout(location = 0) in vec4 geomInput_Position[];
//layout(location = 1) in flat uint geomInput_SV_PrimitiveID[];
layout(location = 0) out vec3 geomOutput_TexCoord0;
layout(location = 1) out vec2 geomOutput_TexCoord1;

layout(set = 0, binding = 9) uniform SunUniform_Block
{
    mat4 ViewMat;
    mat4 ViewProjMat;
    vec4 LightDirection;
    vec4 Dx;
    vec4 Dy;
}SunUniform;

struct GsIn
{
    vec4 OffsetScale;
};
struct PsIn
{
    vec4 position;
    vec3 texCoord;
    vec2 screenCoord;
};
void PushVertex(vec3 pos, vec3 dx, vec3 dy, vec2 vOffset, vec2 baseTC)
{
    PsIn Out;
    (gl_Position = ((SunUniform.ViewProjMat)*(vec4(((pos + (dx * vec3 ((vOffset).x))) + (dy * vec3 ((vOffset).y))), 1.0))));

    geomOutput_TexCoord0.z = gl_Position.z;

    ((gl_Position).z = (gl_Position).w);
    (gl_Position /= vec4 ((gl_Position).w));
    (geomOutput_TexCoord1 = vec2((((gl_Position).x + float (1.0)) * float (0.5)), ((float (1.0) - (gl_Position).y) * float (0.5))));
    (vOffset += vec2(1, (-1)));
    (vOffset *= vec2(0.5, (-0.5)));
    ((geomOutput_TexCoord0).xy = (baseTC + vOffset));
    EmitVertex();
}

layout(triangle_strip, max_vertices = 4) out;
void HLSLmain(GsIn In[1], uint VertexID)
{
    PsIn Out;
    vec2 baseTC = vec2(0, 0);
    PushVertex(((In[0]).OffsetScale).xyz, (SunUniform.Dx).xyz, (SunUniform.Dy).xyz, vec2((-1), (-1)), baseTC);
    PushVertex(((In[0]).OffsetScale).xyz, (SunUniform.Dx).xyz, (SunUniform.Dy).xyz, vec2(1, (-1)), baseTC);
    PushVertex(((In[0]).OffsetScale).xyz, (SunUniform.Dx).xyz, (SunUniform.Dy).xyz, vec2((-1), 1), baseTC);
    PushVertex(((In[0]).OffsetScale).xyz, (SunUniform.Dx).xyz, (SunUniform.Dy).xyz, vec2(1, 1), baseTC);
    EndPrimitive();
}
void main()
{
    GsIn In[1];
    In[0].OffsetScale = geomInput_Position[0];
    uint VertexID;
    VertexID = gl_PrimitiveIDIn;
    PsIn Stream;
    HLSLmain(In, VertexID);
}
