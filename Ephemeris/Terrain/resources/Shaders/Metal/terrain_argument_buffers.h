#ifndef terrain_argument_buffers_h
#define terrain_argument_buffers_h

struct Uniforms_RenderTerrainUniformBuffer
{
	float4x4 projView;
	float4 TerrainInfo;
	float4 CameraInfo;
};

struct Uniforms_LightingTerrainUniformBuffer
{
	float4x4 InvViewProjMat;
	float4x4 ShadowViewProjMat;
	float4 ShadowSpheres;
	float4 LightDirection;
	float4 SunColor;
	float4 LightColor;
};

struct VolumetricCloudsShadowCB
{
	float4 SettingInfo00;
	float4 StandardPosition;
	float4 ShadowInfo;
};
struct Uniforms_VolumetricCloudsShadowCB
{
	VolumetricCloudsShadowCB g_VolumetricCloudsShadow;
};

struct ArgData
{
    texture2d<float> NormalMap;
    texture2d<float> MaskMap;
    const array<texture2d<float>, 5> tileTextures;
    const array<texture2d<float>, 5> tileTexturesNrm;
    texture2d<float> shadowMap;
    sampler g_LinearMirror;
    sampler g_LinearWrap;
    sampler g_LinearBorder;
	
    texture2d<float> BasicTexture;
    texture2d<float> NormalTexture;
    texture2d<float> weatherTexture;
    texture2d<float> depthTexture;
};

struct ArgDataPerFrame
{
    constant Uniforms_RenderTerrainUniformBuffer & RenderTerrainUniformBuffer;
	
    constant Uniforms_LightingTerrainUniformBuffer & LightingTerrainUniformBuffer;
    constant Uniforms_VolumetricCloudsShadowCB & VolumetricCloudsShadowCB;
};


#endif /* terrain_argument_buffers_h */
