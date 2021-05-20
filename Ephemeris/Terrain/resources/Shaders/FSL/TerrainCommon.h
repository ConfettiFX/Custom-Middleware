#ifndef TERRAIN_COMMON_H
#define TERRAIN_COMMON_H

CBUFFER(RenderTerrainUniformBuffer, UPDATE_FREQ_PER_FRAME, b0, binding = 0)
{
	DATA(f4x4,   projView,    None);
	DATA(float4, TerrainInfo, None); //x: y: usePregeneratedNormalmap
	DATA(float4, CameraInfo,  None);
};

CBUFFER(LightingTerrainUniformBuffer, UPDATE_FREQ_PER_FRAME, b1, binding = 1)
{
	DATA(f4x4,   InvViewProjMat,    None);
	DATA(f4x4,   ShadowViewProjMat, None);
	DATA(float4, ShadowSpheres,     None);
	DATA(float4, LightDirection,    None);
	DATA(float4, SunColor,          None);
	DATA(float4, LightColor,        None);
};

CBUFFER(VolumetricCloudsShadowCB, UPDATE_FREQ_PER_FRAME, b2, binding = 2)
{
	DATA(float4, SettingInfo00,    None); // x : EnableCastShadow, y : CloudCoverage, z : WeatherTextureTiling, w : Time
	DATA(float4, StandardPosition, None); // xyz : The current center location for applying wind, w : ShadowBrightness
	DATA(float4, ShadowInfo,       None);
};

RES(Tex2D(float4), NormalMap,          UPDATE_FREQ_NONE, t0,  binding = 0);
RES(Tex2D(float4), MaskMap,            UPDATE_FREQ_NONE, t1,  binding = 1);
RES(Tex2D(float4), tileTextures[5],    UPDATE_FREQ_NONE, t2,  binding = 2);
RES(Tex2D(float4), tileTexturesNrm[5], UPDATE_FREQ_NONE, t7,  binding = 7);
RES(Tex2D(float4), BasicTexture,       UPDATE_FREQ_NONE, t12, binding = 12);
RES(Tex2D(float4), NormalTexture,      UPDATE_FREQ_NONE, t13, binding = 13);
RES(Tex2D(float4), weatherTexture,     UPDATE_FREQ_NONE, t14, binding = 14);
RES(Tex2D(float4), depthTexture,       UPDATE_FREQ_NONE, t15, binding = 15);
RES(SamplerState,  g_LinearMirror,     UPDATE_FREQ_NONE, s0,  binding = 16);
RES(SamplerState,  g_LinearWrap,       UPDATE_FREQ_NONE, s1,  binding = 17);

#endif // TERRAIN_COMMON_H
