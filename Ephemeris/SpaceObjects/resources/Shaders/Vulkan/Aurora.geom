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
layout(location = 1) in vec3 geomInput_Color[];
layout(location = 2) in vec3 geomInput_TexCoord[];

layout(location = 0) out vec4 geomOutput_position;
layout(location = 1) out vec2 geomOutput_texCoord;
layout(location = 2) out vec3 geomOutput_color;

layout(UPDATE_FREQ_PER_FRAME, binding = 13) uniform AuroraUniformBuffer
{
    uint maxVertex;
    float heightOffset;
    float height;
    float deltaTime;
    mat4 ViewProjMat;
};

struct GsIn {
  vec3 Position;
  vec3 Color;
  vec3 NextPosition;
};

struct PsIn {
	vec4 position;
	vec2 texCoord;
    vec3 color;
};

void PushVertex(vec3 pos, vec3 color, vec2 UV)
{
	PsIn Out;

	// Output a billboarded quad  
	geomOutput_position = ViewProjMat * vec4(pos, 1.0);
	geomOutput_texCoord = UV;
	geomOutput_color = color;
  
	EmitVertex();
}

layout(triangle_strip, max_vertices = 4) out;
void main()
{
	GsIn In[1];
	In[0].Position = geomInput_Position[0].xyz;
    In[0].Color    = geomInput_Color[0];
    In[0].NextPosition = geomInput_TexCoord[0];

	PushVertex(In[0].Position.xyz + vec3(0.0, heightOffset, 0.0)              , In[0].Color, vec2(0.0, 1.0));
	PushVertex(In[0].Position.xyz + vec3(0.0, heightOffset + height, 0.0)     , In[0].Color, vec2(0.0, 0.0));
	PushVertex(In[0].NextPosition.xyz + vec3(0.0, heightOffset, 0.0)          , In[0].Color, vec2(1.0, 1.0));
	PushVertex(In[0].NextPosition.xyz + vec3(0.0, heightOffset + height, 0.0) , In[0].Color, vec2(1.0, 0.0));
	EndPrimitive();
	gl_PointSize = 1.0f;
}