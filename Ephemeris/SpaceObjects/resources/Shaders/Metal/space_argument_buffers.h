#ifndef space_argument_buffers_h
#define space_argument_buffers_h

struct AuroraParticle
{
	float4 PrevPosition;
	float4 Position;
	float4 Acceleration;
};

struct Uniforms_AuroraUniformBuffer
{
	uint maxVertex;
	float heightOffset;
	float height;
	float deltaTime;
	float4x4 ViewProjMat;
};

struct Uniforms_SpaceUniform
{
	float4x4 ViewProjMat;
	float4 LightDirection;
	float4 ScreenSize;
	float4 NebulaHighColor;
	float4 NebulaMidColor;
	float4 NebulaLowColor;
};

struct Uniforms_StarUniform
{
	float4x4 RotMat;
	float4x4 ViewProjMat;
	float4 LightDirection;
	float4 Dx;
	float4 Dy;
};

struct Uniforms_cbRootConstant
{
	float4x4 ViewMat;
	float4x4 ViewProjMat;
	float4 LightDirection;
	float4 Dx;
	float4 Dy;
};

struct ArgData
{
	device AuroraParticle* AuroraParticleBuffer;
	texture2d<float> depthTexture;
	texture2d<float> volumetricCloudsTexture;
	texture2d<float> moonAtlas;
    sampler g_LinearBorder;
};

struct ArgDataPerFrame
{
	constant Uniforms_AuroraUniformBuffer & AuroraUniformBuffer;
	constant Uniforms_SpaceUniform & SpaceUniform;
	constant Uniforms_StarUniform & StarUniform;
	constant Uniforms_cbRootConstant & SunUniform;
};

#endif /* space_argument_buffers_h */
