/*
 * Copyright © 2018-2021 Confetti Interactive Inc.
 *
 * This is a part of Aura.
 * 
 * This file(code) is licensed under a 
 * Creative Commons Attribution-NonCommercial 4.0 International License 
 *
 *   (https://creativecommons.org/licenses/by-nc/4.0/legalcode) 
 *
 * Based on a work at https://github.com/ConfettiFX/The-Forge.
 * You may not use the material for commercial purposes.
 *
*/

#ifndef __AURARENDERER_H_9E2034BB_D65C_4EAE_9621_057ACCF12866_INCLUDED__
#define __AURARENDERER_H_9E2034BB_D65C_4EAE_9621_057ACCF12866_INCLUDED__

#include <stdint.h>
#include "../Config/AuraConfig.h"
// For memcpy
#include <string.h>
// For sprintf: contains vsnprintf
#include <stdio.h>
// For sprintf: contains va_start, etc.
#include <stdarg.h>
#include <ctype.h>
// atoi itoa
#include <stdlib.h>

#include "../Interfaces/IAuraMemoryManager.h"

typedef unsigned int uint;

#ifdef __cplusplus
#ifndef MAKE_ENUM_FLAG

#define MAKE_ENUM_FLAG(TYPE,ENUM_TYPE) \
static inline ENUM_TYPE operator|(ENUM_TYPE a, ENUM_TYPE b) \
{ \
	return (ENUM_TYPE)((TYPE)(a) | (TYPE)(b)); \
} \
static inline ENUM_TYPE operator&(ENUM_TYPE a, ENUM_TYPE b) \
{ \
	return (ENUM_TYPE)((TYPE)(a) & (TYPE)(b)); \
} \
static inline ENUM_TYPE operator|=(ENUM_TYPE& a, ENUM_TYPE b) \
{ \
	return a = (a | b); \
} \
static inline ENUM_TYPE operator&=(ENUM_TYPE& a, ENUM_TYPE b) \
{ \
	return a = (a & b); \
} \

#endif
#else
#define MAKE_ENUM_FLAG(TYPE,ENUM_TYPE)
#endif

// Client must implement all these structs and functions to be able to run the Aura middleware

typedef struct Renderer Renderer;
typedef struct Cmd Cmd;
typedef struct VertexLayout VertexLayout;
typedef struct RenderTarget RenderTarget;
typedef struct Shader Shader;
typedef struct Sampler Sampler;
typedef struct RootSignature RootSignature;
typedef struct Pipeline Pipeline;
typedef struct Buffer Buffer;
typedef struct Texture Texture;
typedef struct RenderTarget RenderTarget;
typedef struct LoadActionsDesc LoadActionsDesc;
typedef struct BufferBarrier BufferBarrier;
typedef struct TextureBarrier TextureBarrier;
typedef struct DescriptorSet DescriptorSet;
typedef struct PipelineCache PipelineCache;

namespace aura
{
	enum
	{
		MAX_RENDER_TARGET_ATTACHMENTS = 8,
		MAX_VERTEX_ATTRIBS = 15,
		MAX_SEMANTIC_NAME_LENGTH = 128,
	};

	typedef enum TinyImageFormat
	{
		TinyImageFormat_UNDEFINED = 0,
		TinyImageFormat_R1_UNORM = 1,
		TinyImageFormat_R2_UNORM = 2,
		TinyImageFormat_R4_UNORM = 3,
		TinyImageFormat_R4G4_UNORM = 4,
		TinyImageFormat_G4R4_UNORM = 5,
		TinyImageFormat_A8_UNORM = 6,
		TinyImageFormat_R8_UNORM = 7,
		TinyImageFormat_R8_SNORM = 8,
		TinyImageFormat_R8_UINT = 9,
		TinyImageFormat_R8_SINT = 10,
		TinyImageFormat_R8_SRGB = 11,
		TinyImageFormat_B2G3R3_UNORM = 12,
		TinyImageFormat_R4G4B4A4_UNORM = 13,
		TinyImageFormat_R4G4B4X4_UNORM = 14,
		TinyImageFormat_B4G4R4A4_UNORM = 15,
		TinyImageFormat_B4G4R4X4_UNORM = 16,
		TinyImageFormat_A4R4G4B4_UNORM = 17,
		TinyImageFormat_X4R4G4B4_UNORM = 18,
		TinyImageFormat_A4B4G4R4_UNORM = 19,
		TinyImageFormat_X4B4G4R4_UNORM = 20,
		TinyImageFormat_R5G6B5_UNORM = 21,
		TinyImageFormat_B5G6R5_UNORM = 22,
		TinyImageFormat_R5G5B5A1_UNORM = 23,
		TinyImageFormat_B5G5R5A1_UNORM = 24,
		TinyImageFormat_A1B5G5R5_UNORM = 25,
		TinyImageFormat_A1R5G5B5_UNORM = 26,
		TinyImageFormat_R5G5B5X1_UNORM = 27,
		TinyImageFormat_B5G5R5X1_UNORM = 28,
		TinyImageFormat_X1R5G5B5_UNORM = 29,
		TinyImageFormat_X1B5G5R5_UNORM = 30,
		TinyImageFormat_B2G3R3A8_UNORM = 31,
		TinyImageFormat_R8G8_UNORM = 32,
		TinyImageFormat_R8G8_SNORM = 33,
		TinyImageFormat_G8R8_UNORM = 34,
		TinyImageFormat_G8R8_SNORM = 35,
		TinyImageFormat_R8G8_UINT = 36,
		TinyImageFormat_R8G8_SINT = 37,
		TinyImageFormat_R8G8_SRGB = 38,
		TinyImageFormat_R16_UNORM = 39,
		TinyImageFormat_R16_SNORM = 40,
		TinyImageFormat_R16_UINT = 41,
		TinyImageFormat_R16_SINT = 42,
		TinyImageFormat_R16_SFLOAT = 43,
		TinyImageFormat_R16_SBFLOAT = 44,
		TinyImageFormat_R8G8B8_UNORM = 45,
		TinyImageFormat_R8G8B8_SNORM = 46,
		TinyImageFormat_R8G8B8_UINT = 47,
		TinyImageFormat_R8G8B8_SINT = 48,
		TinyImageFormat_R8G8B8_SRGB = 49,
		TinyImageFormat_B8G8R8_UNORM = 50,
		TinyImageFormat_B8G8R8_SNORM = 51,
		TinyImageFormat_B8G8R8_UINT = 52,
		TinyImageFormat_B8G8R8_SINT = 53,
		TinyImageFormat_B8G8R8_SRGB = 54,
		TinyImageFormat_R8G8B8A8_UNORM = 55,
		TinyImageFormat_R8G8B8A8_SNORM = 56,
		TinyImageFormat_R8G8B8A8_UINT = 57,
		TinyImageFormat_R8G8B8A8_SINT = 58,
		TinyImageFormat_R8G8B8A8_SRGB = 59,
		TinyImageFormat_B8G8R8A8_UNORM = 60,
		TinyImageFormat_B8G8R8A8_SNORM = 61,
		TinyImageFormat_B8G8R8A8_UINT = 62,
		TinyImageFormat_B8G8R8A8_SINT = 63,
		TinyImageFormat_B8G8R8A8_SRGB = 64,
		TinyImageFormat_R8G8B8X8_UNORM = 65,
		TinyImageFormat_B8G8R8X8_UNORM = 66,
		TinyImageFormat_R16G16_UNORM = 67,
		TinyImageFormat_G16R16_UNORM = 68,
		TinyImageFormat_R16G16_SNORM = 69,
		TinyImageFormat_G16R16_SNORM = 70,
		TinyImageFormat_R16G16_UINT = 71,
		TinyImageFormat_R16G16_SINT = 72,
		TinyImageFormat_R16G16_SFLOAT = 73,
		TinyImageFormat_R16G16_SBFLOAT = 74,
		TinyImageFormat_R32_UINT = 75,
		TinyImageFormat_R32_SINT = 76,
		TinyImageFormat_R32_SFLOAT = 77,
		TinyImageFormat_A2R10G10B10_UNORM = 78,
		TinyImageFormat_A2R10G10B10_UINT = 79,
		TinyImageFormat_A2R10G10B10_SNORM = 80,
		TinyImageFormat_A2R10G10B10_SINT = 81,
		TinyImageFormat_A2B10G10R10_UNORM = 82,
		TinyImageFormat_A2B10G10R10_UINT = 83,
		TinyImageFormat_A2B10G10R10_SNORM = 84,
		TinyImageFormat_A2B10G10R10_SINT = 85,
		TinyImageFormat_R10G10B10A2_UNORM = 86,
		TinyImageFormat_R10G10B10A2_UINT = 87,
		TinyImageFormat_R10G10B10A2_SNORM = 88,
		TinyImageFormat_R10G10B10A2_SINT = 89,
		TinyImageFormat_B10G10R10A2_UNORM = 90,
		TinyImageFormat_B10G10R10A2_UINT = 91,
		TinyImageFormat_B10G10R10A2_SNORM = 92,
		TinyImageFormat_B10G10R10A2_SINT = 93,
		TinyImageFormat_B10G11R11_UFLOAT = 94,
		TinyImageFormat_E5B9G9R9_UFLOAT = 95,
		TinyImageFormat_R16G16B16_UNORM = 96,
		TinyImageFormat_R16G16B16_SNORM = 97,
		TinyImageFormat_R16G16B16_UINT = 98,
		TinyImageFormat_R16G16B16_SINT = 99,
		TinyImageFormat_R16G16B16_SFLOAT = 100,
		TinyImageFormat_R16G16B16_SBFLOAT = 101,
		TinyImageFormat_R16G16B16A16_UNORM = 102,
		TinyImageFormat_R16G16B16A16_SNORM = 103,
		TinyImageFormat_R16G16B16A16_UINT = 104,
		TinyImageFormat_R16G16B16A16_SINT = 105,
		TinyImageFormat_R16G16B16A16_SFLOAT = 106,
		TinyImageFormat_R16G16B16A16_SBFLOAT = 107,
		TinyImageFormat_R32G32_UINT = 108,
		TinyImageFormat_R32G32_SINT = 109,
		TinyImageFormat_R32G32_SFLOAT = 110,
		TinyImageFormat_R32G32B32_UINT = 111,
		TinyImageFormat_R32G32B32_SINT = 112,
		TinyImageFormat_R32G32B32_SFLOAT = 113,
		TinyImageFormat_R32G32B32A32_UINT = 114,
		TinyImageFormat_R32G32B32A32_SINT = 115,
		TinyImageFormat_R32G32B32A32_SFLOAT = 116,
		TinyImageFormat_R64_UINT = 117,
		TinyImageFormat_R64_SINT = 118,
		TinyImageFormat_R64_SFLOAT = 119,
		TinyImageFormat_R64G64_UINT = 120,
		TinyImageFormat_R64G64_SINT = 121,
		TinyImageFormat_R64G64_SFLOAT = 122,
		TinyImageFormat_R64G64B64_UINT = 123,
		TinyImageFormat_R64G64B64_SINT = 124,
		TinyImageFormat_R64G64B64_SFLOAT = 125,
		TinyImageFormat_R64G64B64A64_UINT = 126,
		TinyImageFormat_R64G64B64A64_SINT = 127,
		TinyImageFormat_R64G64B64A64_SFLOAT = 128,
		TinyImageFormat_D16_UNORM = 129,
		TinyImageFormat_X8_D24_UNORM = 130,
		TinyImageFormat_D32_SFLOAT = 131,
		TinyImageFormat_S8_UINT = 132,
		TinyImageFormat_D16_UNORM_S8_UINT = 133,
		TinyImageFormat_D24_UNORM_S8_UINT = 134,
		TinyImageFormat_D32_SFLOAT_S8_UINT = 135,
		TinyImageFormat_DXBC1_RGB_UNORM = 136,
		TinyImageFormat_DXBC1_RGB_SRGB = 137,
		TinyImageFormat_DXBC1_RGBA_UNORM = 138,
		TinyImageFormat_DXBC1_RGBA_SRGB = 139,
		TinyImageFormat_DXBC2_UNORM = 140,
		TinyImageFormat_DXBC2_SRGB = 141,
		TinyImageFormat_DXBC3_UNORM = 142,
		TinyImageFormat_DXBC3_SRGB = 143,
		TinyImageFormat_DXBC4_UNORM = 144,
		TinyImageFormat_DXBC4_SNORM = 145,
		TinyImageFormat_DXBC5_UNORM = 146,
		TinyImageFormat_DXBC5_SNORM = 147,
		TinyImageFormat_DXBC6H_UFLOAT = 148,
		TinyImageFormat_DXBC6H_SFLOAT = 149,
		TinyImageFormat_DXBC7_UNORM = 150,
		TinyImageFormat_DXBC7_SRGB = 151,
		TinyImageFormat_PVRTC1_2BPP_UNORM = 152,
		TinyImageFormat_PVRTC1_4BPP_UNORM = 153,
		TinyImageFormat_PVRTC2_2BPP_UNORM = 154,
		TinyImageFormat_PVRTC2_4BPP_UNORM = 155,
		TinyImageFormat_PVRTC1_2BPP_SRGB = 156,
		TinyImageFormat_PVRTC1_4BPP_SRGB = 157,
		TinyImageFormat_PVRTC2_2BPP_SRGB = 158,
		TinyImageFormat_PVRTC2_4BPP_SRGB = 159,
		TinyImageFormat_ETC2_R8G8B8_UNORM = 160,
		TinyImageFormat_ETC2_R8G8B8_SRGB = 161,
		TinyImageFormat_ETC2_R8G8B8A1_UNORM = 162,
		TinyImageFormat_ETC2_R8G8B8A1_SRGB = 163,
		TinyImageFormat_ETC2_R8G8B8A8_UNORM = 164,
		TinyImageFormat_ETC2_R8G8B8A8_SRGB = 165,
		TinyImageFormat_ETC2_EAC_R11_UNORM = 166,
		TinyImageFormat_ETC2_EAC_R11_SNORM = 167,
		TinyImageFormat_ETC2_EAC_R11G11_UNORM = 168,
		TinyImageFormat_ETC2_EAC_R11G11_SNORM = 169,
		TinyImageFormat_ASTC_4x4_UNORM = 170,
		TinyImageFormat_ASTC_4x4_SRGB = 171,
		TinyImageFormat_ASTC_5x4_UNORM = 172,
		TinyImageFormat_ASTC_5x4_SRGB = 173,
		TinyImageFormat_ASTC_5x5_UNORM = 174,
		TinyImageFormat_ASTC_5x5_SRGB = 175,
		TinyImageFormat_ASTC_6x5_UNORM = 176,
		TinyImageFormat_ASTC_6x5_SRGB = 177,
		TinyImageFormat_ASTC_6x6_UNORM = 178,
		TinyImageFormat_ASTC_6x6_SRGB = 179,
		TinyImageFormat_ASTC_8x5_UNORM = 180,
		TinyImageFormat_ASTC_8x5_SRGB = 181,
		TinyImageFormat_ASTC_8x6_UNORM = 182,
		TinyImageFormat_ASTC_8x6_SRGB = 183,
		TinyImageFormat_ASTC_8x8_UNORM = 184,
		TinyImageFormat_ASTC_8x8_SRGB = 185,
		TinyImageFormat_ASTC_10x5_UNORM = 186,
		TinyImageFormat_ASTC_10x5_SRGB = 187,
		TinyImageFormat_ASTC_10x6_UNORM = 188,
		TinyImageFormat_ASTC_10x6_SRGB = 189,
		TinyImageFormat_ASTC_10x8_UNORM = 190,
		TinyImageFormat_ASTC_10x8_SRGB = 191,
		TinyImageFormat_ASTC_10x10_UNORM = 192,
		TinyImageFormat_ASTC_10x10_SRGB = 193,
		TinyImageFormat_ASTC_12x10_UNORM = 194,
		TinyImageFormat_ASTC_12x10_SRGB = 195,
		TinyImageFormat_ASTC_12x12_UNORM = 196,
		TinyImageFormat_ASTC_12x12_SRGB = 197,
		TinyImageFormat_CLUT_P4 = 198,
		TinyImageFormat_CLUT_P4A4 = 199,
		TinyImageFormat_CLUT_P8 = 200,
		TinyImageFormat_CLUT_P8A8 = 201,
	} TinyImageFormat;

	typedef enum BlendConstant
	{
		BC_ZERO = 0,
		BC_ONE,
		BC_SRC_COLOR,
		BC_ONE_MINUS_SRC_COLOR,
		BC_DST_COLOR,
		BC_ONE_MINUS_DST_COLOR,
		BC_SRC_ALPHA,
		BC_ONE_MINUS_SRC_ALPHA,
		BC_DST_ALPHA,
		BC_ONE_MINUS_DST_ALPHA,
		BC_SRC_ALPHA_SATURATE,
		BC_BLEND_FACTOR,
		BC_INV_BLEND_FACTOR,
		MAX_BLEND_CONSTANTS
	} BlendConstant;

	typedef enum BlendMode
	{
		BM_ADD,
		BM_SUBTRACT,
		BM_REVERSE_SUBTRACT,
		BM_MIN,
		BM_MAX,
		MAX_BLEND_MODES,
	} BlendMode;

	typedef enum CompareMode
	{
		CMP_NEVER,
		CMP_LESS,
		CMP_EQUAL,
		CMP_LEQUAL,
		CMP_GREATER,
		CMP_NOTEQUAL,
		CMP_GEQUAL,
		CMP_ALWAYS,
		MAX_COMPARE_MODES,
	} CompareMode;

	typedef enum StencilOp
	{
		STENCIL_OP_KEEP,
		STENCIL_OP_SET_ZERO,
		STENCIL_OP_REPLACE,
		STENCIL_OP_INVERT,
		STENCIL_OP_INCR,
		STENCIL_OP_DECR,
		STENCIL_OP_INCR_SAT,
		STENCIL_OP_DECR_SAT,
		MAX_STENCIL_OPS,
	} StencilOp;

	static const int RED = 0x1;
	static const int GREEN = 0x2;
	static const int BLUE = 0x4;
	static const int ALPHA = 0x8;
	static const int ALL = (RED | GREEN | BLUE | ALPHA);
	static const int NONE = 0;

	static const int BS_NONE = -1;
	static const int DS_NONE = -1;
	static const int RS_NONE = -1;

	// Blend states are always attached to one of the eight or more render targets that
	// are in a MRT
	// Mask constants
	typedef enum BlendStateTargets
	{
		BLEND_STATE_TARGET_0 = 0x1,
		BLEND_STATE_TARGET_1 = 0x2,
		BLEND_STATE_TARGET_2 = 0x4,
		BLEND_STATE_TARGET_3 = 0x8,
		BLEND_STATE_TARGET_4 = 0x10,
		BLEND_STATE_TARGET_5 = 0x20,
		BLEND_STATE_TARGET_6 = 0x40,
		BLEND_STATE_TARGET_7 = 0x80,
		BLEND_STATE_TARGET_ALL = 0xFF,
	} BlendStateTargets;
	MAKE_ENUM_FLAG(uint32_t, BlendStateTargets)

	typedef enum CullMode {
		CULL_MODE_NONE = 0,
		CULL_MODE_BACK,
		CULL_MODE_FRONT,
		CULL_MODE_BOTH,
		MAX_CULL_MODES
	} CullMode;

	typedef enum FrontFace {
		FRONT_FACE_CCW = 0,
		FRONT_FACE_CW
	} FrontFace;

	typedef enum FillMode
	{
		FILL_MODE_SOLID,
		FILL_MODE_WIREFRAME,
		MAX_FILL_MODES
	} FillMode;

	typedef enum FilterType {
		FILTER_NEAREST = 0,
		FILTER_LINEAR,
	} FilterType;

	typedef enum AddressMode {
		ADDRESS_MODE_MIRROR,
		ADDRESS_MODE_REPEAT,
		ADDRESS_MODE_CLAMP_TO_EDGE,
		ADDRESS_MODE_CLAMP_TO_BORDER
	} AddressMode;

	typedef enum MipMapMode
	{
		MIPMAP_MODE_NEAREST = 0,
		MIPMAP_MODE_LINEAR
	} MipMapMode;

#if defined(VULKAN)
	typedef enum SamplerRange
	{
		SAMPLER_RANGE_FULL = 0,
		SAMPLER_RANGE_NARROW = 1,
	} SamplerRange;

	typedef enum SamplerModelConversion
	{
		SAMPLER_MODEL_CONVERSION_RGB_IDENTITY = 0,
		SAMPLER_MODEL_CONVERSION_YCBCR_IDENTITY = 1,
		SAMPLER_MODEL_CONVERSION_YCBCR_709 = 2,
		SAMPLER_MODEL_CONVERSION_YCBCR_601 = 3,
		SAMPLER_MODEL_CONVERSION_YCBCR_2020 = 4,
	} SamplerModelConversion;

	typedef enum SampleLocation
	{
		SAMPLE_LOCATION_COSITED = 0,
		SAMPLE_LOCATION_MIDPOINT = 1,
	} SampleLocation;
#endif

	typedef enum LoadActionType
	{
		LOAD_ACTION_DONTCARE,
		LOAD_ACTION_LOAD,
		LOAD_ACTION_CLEAR
	} LoadActionType;

	typedef enum DescriptorType
	{
		DESCRIPTOR_TYPE_UNDEFINED = 0,
		DESCRIPTOR_TYPE_SAMPLER = 0x01,
		// SRV Read only texture
		DESCRIPTOR_TYPE_TEXTURE = (DESCRIPTOR_TYPE_SAMPLER << 1),
		/// UAV Texture
		DESCRIPTOR_TYPE_RW_TEXTURE = (DESCRIPTOR_TYPE_TEXTURE << 1),
		// SRV Read only buffer
		DESCRIPTOR_TYPE_BUFFER = (DESCRIPTOR_TYPE_RW_TEXTURE << 1),
		DESCRIPTOR_TYPE_BUFFER_RAW = (DESCRIPTOR_TYPE_BUFFER | (DESCRIPTOR_TYPE_BUFFER << 1)),
		/// UAV Buffer
		DESCRIPTOR_TYPE_RW_BUFFER = (DESCRIPTOR_TYPE_BUFFER << 2),
		DESCRIPTOR_TYPE_RW_BUFFER_RAW = (DESCRIPTOR_TYPE_RW_BUFFER | (DESCRIPTOR_TYPE_RW_BUFFER << 1)),
		/// Uniform buffer
		DESCRIPTOR_TYPE_UNIFORM_BUFFER = (DESCRIPTOR_TYPE_RW_BUFFER << 2),
		DESCRIPTOR_TYPE_VERTEX_BUFFER = (DESCRIPTOR_TYPE_UNIFORM_BUFFER << 1),
		DESCRIPTOR_TYPE_INDEX_BUFFER = (DESCRIPTOR_TYPE_VERTEX_BUFFER << 1),
		DESCRIPTOR_TYPE_INDIRECT_BUFFER = (DESCRIPTOR_TYPE_INDEX_BUFFER << 1),
		/// Push constant / Root constant
		DESCRIPTOR_TYPE_ROOT_CONSTANT = (DESCRIPTOR_TYPE_INDIRECT_BUFFER << 1),
		/// Cubemap SRV
		DESCRIPTOR_TYPE_TEXTURE_CUBE = (DESCRIPTOR_TYPE_TEXTURE | (DESCRIPTOR_TYPE_ROOT_CONSTANT << 1)),
		/// RTV / DSV per array slice
		DESCRIPTOR_TYPE_RENDER_TARGET_ARRAY_SLICES = (DESCRIPTOR_TYPE_ROOT_CONSTANT << 2),
		/// RTV / DSV per depth slice
		DESCRIPTOR_TYPE_RENDER_TARGET_DEPTH_SLICES = (DESCRIPTOR_TYPE_RENDER_TARGET_ARRAY_SLICES << 1),
#if defined(VULKAN)
		/// Subpass input (descriptor type only available in Vulkan)
		DESCRIPTOR_TYPE_INPUT_ATTACHMENT = (DESCRIPTOR_TYPE_RENDER_TARGET_DEPTH_SLICES << 1),
		DESCRIPTOR_TYPE_TEXEL_BUFFER = (DESCRIPTOR_TYPE_INPUT_ATTACHMENT << 1),
		DESCRIPTOR_TYPE_RW_TEXEL_BUFFER = (DESCRIPTOR_TYPE_TEXEL_BUFFER << 1),
#endif
	} DescriptorType;
	MAKE_ENUM_FLAG(uint32_t, DescriptorType)

	typedef enum SampleCount {
		SAMPLE_COUNT_1 = 1,
		SAMPLE_COUNT_2 = 2,
		SAMPLE_COUNT_4 = 4,
		SAMPLE_COUNT_8 = 8,
		SAMPLE_COUNT_16 = 16,
	} SampleCount;

#ifdef METAL
	typedef enum ShaderStage {
		SHADER_STAGE_NONE = 0,
		SHADER_STAGE_VERT = 0X00000001,
		SHADER_STAGE_FRAG = 0X00000002,
		SHADER_STAGE_COMP = 0X00000004,
		SHADER_STAGE_ALL_GRAPHICS = 0X00000003,
		SHADER_STAGE_COUNT = 3,
	} ShaderStage;
#else
	typedef enum ShaderStage {
		SHADER_STAGE_NONE = 0,
		SHADER_STAGE_VERT = 0X00000001,
		SHADER_STAGE_TESC = 0X00000002,
		SHADER_STAGE_TESE = 0X00000004,
		SHADER_STAGE_GEOM = 0X00000008,
		SHADER_STAGE_FRAG = 0X00000010,
		SHADER_STAGE_COMP = 0X00000020,
		SHADER_STAGE_RAYTRACING = 0X00000040,
		SHADER_STAGE_ALL_GRAPHICS = 0X0000001F,
		SHADER_STAGE_HULL = SHADER_STAGE_TESC,
		SHADER_STAGE_DOMN = SHADER_STAGE_TESE,
		SHADER_STAGE_COUNT = 7,
	} ShaderStage;
	MAKE_ENUM_FLAG(uint32_t, ShaderStage)
#endif

	typedef enum PrimitiveTopology {
		PRIMITIVE_TOPO_POINT_LIST = 0,
		PRIMITIVE_TOPO_LINE_LIST,
		PRIMITIVE_TOPO_LINE_STRIP,
		PRIMITIVE_TOPO_TRI_LIST,
		PRIMITIVE_TOPO_TRI_STRIP,
		PRIMITIVE_TOPO_PATCH_LIST,
		PRIMITIVE_TOPO_COUNT,
	} PrimitiveTopology;

	typedef enum ShaderSemantic {
		SEMANTIC_UNDEFINED = 0,
		SEMANTIC_POSITION,
		SEMANTIC_NORMAL,
		SEMANTIC_COLOR,
		SEMANTIC_TANGENT,
		SEMANTIC_BITANGENT,
		SEMANTIC_TEXCOORD0,
		SEMANTIC_TEXCOORD1,
		SEMANTIC_TEXCOORD2,
		SEMANTIC_TEXCOORD3,
		SEMANTIC_TEXCOORD4,
		SEMANTIC_TEXCOORD5,
		SEMANTIC_TEXCOORD6,
		SEMANTIC_TEXCOORD7,
		SEMANTIC_TEXCOORD8,
		SEMANTIC_TEXCOORD9,
	} ShaderSemantic;

	typedef enum DepthStencilClearFlags
	{
		ClEAR_DEPTH = 0x01,
		CLEAR_STENCIL = 0x02
	} DepthStencilClearFlags;

	typedef struct ClearValue {
		union {
			struct {
				float r;
				float g;
				float b;
				float a;
			};
			struct {
				float    depth;
				uint32_t stencil;
			};
		};
	} ClearValue;

	typedef enum DescriptorUpdateFrequency
	{
		DESCRIPTOR_UPDATE_FREQ_NONE = 0,
		DESCRIPTOR_UPDATE_FREQ_PER_FRAME,
		DESCRIPTOR_UPDATE_FREQ_PER_BATCH,
		DESCRIPTOR_UPDATE_FREQ_PER_DRAW,
		DESCRIPTOR_UPDATE_FREQ_COUNT,
	} DescriptorUpdateFrequency;

	typedef struct DescriptorSetDesc
	{
		RootSignature*             pRootSignature;
		DescriptorUpdateFrequency  mUpdateFrequency;
		uint32_t                   mMaxSets;
		uint32_t                   mNodeIndex;
	} DescriptorSetDesc;

	typedef struct LoadActionsDesc {
		ClearValue		mClearColorValues[MAX_RENDER_TARGET_ATTACHMENTS];
		LoadActionType	mLoadActionsColor[MAX_RENDER_TARGET_ATTACHMENTS] = {};
		ClearValue		mClearDepth;
		LoadActionType	mLoadActionDepth = {};
		LoadActionType	mLoadActionStencil = {};
		uint32_t*		pColorRenderTargetViewIndices = NULL;
		uint32_t		mDepthStencilViewIndex = (uint32_t)(-1);
	} LoadActionsDesc;

	typedef enum RenderTargetType {
		RENDER_TARGET_TYPE_UNKNOWN = 0,
		RENDER_TARGET_TYPE_1D,
		RENDER_TARGET_TYPE_2D,
		RENDER_TARGET_TYPE_3D,
		RENDER_TARGET_TYPE_BUFFER,
	} RenderTargetType;

	typedef enum RenderTargetUsage {
		RENDER_TARGET_USAGE_UNDEFINED = 0x00,
		RENDER_TARGET_USAGE_UNORDERED_ACCESS = 0x04,
		RENDER_TARGET_USAGE_1D_MIPPED = 0x08,
		RENDER_TARGET_USAGE_2D_MIPPED = 0x10,
		RENDER_TARGET_USAGE_3D_MIPPED = 0x20,
		RENDER_TARGET_USAGE_1D_SLICES = 0x40,
		RENDER_TARGET_USAGE_2D_SLICES = 0x80,
		RENDER_TARGET_USAGE_1D_MIPPED_SLICES = 0x200,
		RENDER_TARGET_USAGE_2D_MIPPED_SLICES = 0x400,
	} RenderTargetUsage;
	MAKE_ENUM_FLAG(uint32_t, RenderTargetUsage)

	typedef enum TextureCreationFlags
	{
		/// Default flag (Texture will use default allocation strategy decided by the api specific allocator)
		TEXTURE_CREATION_FLAG_NONE = 0,
		/// Texture will allocate its own memory (COMMITTED resource)
		TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT = 0x01,
		/// Texture will be allocated in memory which can be shared among multiple processes
		TEXTURE_CREATION_FLAG_EXPORT_BIT = 0x02,
		/// Texture will be allocated in memory which can be shared among multiple gpus
		TEXTURE_CREATION_FLAG_EXPORT_ADAPTER_BIT = 0x04,
		/// Texture will be imported from a handle created in another process
		TEXTURE_CREATION_FLAG_IMPORT_BIT = 0x08,
		/// Use ESRAM to store this texture
		TEXTURE_CREATION_FLAG_ESRAM = 0x10,
	} TextureCreationFlags;
	MAKE_ENUM_FLAG(uint32_t, TextureCreationFlags)
	
	typedef enum ResourceState
	{
		RESOURCE_STATE_UNDEFINED = 0,
		RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER = 0x1,
		RESOURCE_STATE_INDEX_BUFFER = 0x2,
		RESOURCE_STATE_RENDER_TARGET = 0x4,
		RESOURCE_STATE_UNORDERED_ACCESS = 0x8,
		RESOURCE_STATE_DEPTH_WRITE = 0x10,
		RESOURCE_STATE_DEPTH_READ = 0x20,
		RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE = 0x40,
		RESOURCE_STATE_PIXEL_SHADER_RESOURCE = 0x80,
		RESOURCE_STATE_SHADER_RESOURCE = 0x40 | 0x80,
		RESOURCE_STATE_STREAM_OUT = 0x100,
		RESOURCE_STATE_INDIRECT_ARGUMENT = 0x200,
		RESOURCE_STATE_COPY_DEST = 0x400,
		RESOURCE_STATE_COPY_SOURCE = 0x800,
		RESOURCE_STATE_GENERIC_READ = (((((0x1 | 0x2) | 0x40) | 0x80) | 0x200) | 0x800),
		RESOURCE_STATE_PRESENT = 0x4000,
		RESOURCE_STATE_COMMON = 0x8000,
	} ResourceState;
	MAKE_ENUM_FLAG(uint32_t, ResourceState)

	/// Data structure holding necessary info to create a Texture
	typedef struct TextureDesc
	{
		/// Optimized clear value (recommended to use this same value when clearing the rendertarget)
		ClearValue				mClearValue;
		/// Pointer to native texture handle if the texture does not own underlying resource
		const void*				pNativeHandle;
		/// Debug name used in gpu profile
		const char*				pName;
		/// GPU indices to share this texture
		uint32_t*				pSharedNodeIndices;
#if defined(VULKAN)
		/// VkSamplerYcbcrConversionInfo*
		void* pVkSamplerYcbcrConversionInfo;
#endif
		/// Texture creation flags (decides memory allocation strategy, sharing access,...)
		TextureCreationFlags	mFlags;
		/// Width
		uint32_t				mWidth;
		/// Height
		uint32_t				mHeight;
		/// Depth (Should be 1 if not a mType is not TEXTURE_TYPE_3D)
		uint32_t				mDepth;
		/// Texture array size (Should be 1 if texture is not a texture array or cubemap)
		uint32_t				mArraySize;
		/// Number of mip levels
		uint32_t				mMipLevels;
		/// Number of multisamples per pixel (currently Textures created with mUsage TEXTURE_USAGE_SAMPLED_IMAGE only support SAMPLE_COUNT_1)
		SampleCount				mSampleCount;
		/// The image quality level. The higher the quality, the lower the performance. The valid range is between zero and the value appropriate for mSampleCount
		uint32_t				mSampleQuality;
		/// Internal image format
		TinyImageFormat			mFormat;
		/// What state will the texture get created in
		ResourceState			mStartState;
		/// Descriptor creation
		DescriptorType			mDescriptors;
		/// Number of GPUs to share this texture
		uint32_t				mSharedNodeIndexCount;
		/// GPU which will own this texture
		uint32_t				mNodeIndex;
	} TextureDesc;

	typedef struct SamplerDesc
	{
		FilterType  mMinFilter;
		FilterType  mMagFilter;
		MipMapMode  mMipMapMode;
		AddressMode mAddressU;
		AddressMode mAddressV;
		AddressMode mAddressW;
		float       mMipLodBias;
		float       mMaxAnisotropy;
		CompareMode mCompareFunc;

#if defined(VULKAN)
		struct
		{
			TinyImageFormat        mFormat;
			SamplerModelConversion mModel;
			SamplerRange           mRange;
			SampleLocation         mChromaOffsetX;
			SampleLocation         mChromaOffsetY;
			FilterType             mChromaFilter;
			bool                   mForceExplicitReconstruction;
		} mSamplerConversionDesc;
#endif
	} SamplerDesc;

	typedef struct RenderTargetDesc
	{
		/// Texture creation flags (decides memory allocation strategy, sharing access,...)
		TextureCreationFlags	mFlags;
		/// Width
		uint32_t				mWidth;
		/// Height
		uint32_t				mHeight;
		/// Depth (Should be 1 if not a mType is not TEXTURE_TYPE_3D)
		uint32_t				mDepth;
		/// Texture array size (Should be 1 if texture is not a texture array or cubemap)
		uint32_t				mArraySize;
		/// Number of mip levels
		uint32_t				mMipLevels;
		/// MSAA
		SampleCount				mSampleCount;
		/// Internal image format
		TinyImageFormat			mFormat;
		/// What state will the texture get created in
		ResourceState			mStartState;
		/// Optimized clear value (recommended to use this same value when clearing the rendertarget)
		ClearValue				mClearValue;
		/// The image quality level. The higher the quality, the lower the performance. The valid range is between zero and the value appropriate for mSampleCount
		uint32_t				mSampleQuality;
		/// Descriptor creation
		DescriptorType			mDescriptors;
		const void*				pNativeHandle;
		/// Debug name used in gpu profile
		const char*				pName;
		/// GPU indices to share this texture
		uint32_t*				pSharedNodeIndices;
		/// Number of GPUs to share this texture
		uint32_t				mSharedNodeIndexCount;
		/// GPU which will own this texture
		uint32_t				mNodeIndex;
	} RenderTargetDesc;

	typedef enum RootSignatureFlags
	{
		/// Default flag
		ROOT_SIGNATURE_FLAG_NONE = 0,
		/// Local root signature used mainly in raytracing shaders
		ROOT_SIGNATURE_FLAG_LOCAL_BIT = 0x1,
	} RootSignatureFlags;
	MAKE_ENUM_FLAG(uint32_t, RootSignatureFlags)

	typedef struct RootSignatureDesc
	{
		Shader**               ppShaders;
		uint32_t               mShaderCount;
		uint32_t               mMaxBindlessTextures;
		const char**           ppStaticSamplerNames;
		Sampler**              ppStaticSamplers;
		uint32_t               mStaticSamplerCount;
		RootSignatureFlags     mFlags;
	} RootSignatureDesc;

	typedef struct ShaderMacro
	{
		const char* definition;
		const char* value;
	} ShaderMacro;

	typedef enum ShaderTarget
	{
		shader_target_5_1,
		shader_target_6_0,
	} ShaderTarget;

	typedef enum ShaderStageLoadFlags
	{
		SHADER_STAGE_LOAD_FLAG_NONE = 0x0,
		SHADER_STAGE_LOAD_FLAG_ENABLE_PS_PRIMITIVEID = 0x1,
	} ShaderStageLoadFlags;
	MAKE_ENUM_FLAG(uint32_t, ShaderStageLoadFlags);

	/// ShaderConstant only supported by Vulkan and Metal APIs
	typedef struct ShaderConstant
	{
		const void* pValue;
		uint32_t       mIndex;
		uint32_t       mSize;
	} ShaderConstant;

	typedef struct ShaderStageLoadDesc
	{ //-V802 : Very user-facing struct, and order is highly important to convenience
		const char*          pFileName;
		ShaderMacro*         pMacros;
		uint32_t             mMacroCount;
		const char*          pEntryPointName;
		ShaderStageLoadFlags mFlags;
	} ShaderStageLoadDesc;

	typedef struct ShaderLoadDesc
	{
		ShaderStageLoadDesc   mStages[SHADER_STAGE_COUNT];
		ShaderTarget          mTarget;
		const ShaderConstant* pConstants;
		uint32_t              mConstantCount;
	} ShaderLoadDesc;

	typedef struct VertexAttrib
	{
		ShaderSemantic		mSemantic;
		uint32_t			mSemanticNameLength;
		char				mSemanticName[MAX_SEMANTIC_NAME_LENGTH];
		TinyImageFormat		mFormat;
		uint32_t			mBinding;
		uint32_t			mLocation;
		uint32_t			mOffset;
	} VertexAttrib;

	typedef struct BlendStateDesc
	{
		/// Source blend factor per render target.
		BlendConstant		mSrcFactors[MAX_RENDER_TARGET_ATTACHMENTS];
		/// Destination blend factor per render target.
		BlendConstant		mDstFactors[MAX_RENDER_TARGET_ATTACHMENTS];
		/// Source alpha blend factor per render target.
		BlendConstant		mSrcAlphaFactors[MAX_RENDER_TARGET_ATTACHMENTS];
		/// Destination alpha blend factor per render target.
		BlendConstant		mDstAlphaFactors[MAX_RENDER_TARGET_ATTACHMENTS];
		/// Blend mode per render target.
		BlendMode			mBlendModes[MAX_RENDER_TARGET_ATTACHMENTS];
		/// Alpha blend mode per render target.
		BlendMode			mBlendAlphaModes[MAX_RENDER_TARGET_ATTACHMENTS];
		/// Write mask per render target.
		int32_t				mMasks[MAX_RENDER_TARGET_ATTACHMENTS];
		/// Mask that identifies the render targets affected by the blend state.
		BlendStateTargets	mRenderTargetMask;
		/// Set whether alpha to coverage should be enabled.
		bool				mAlphaToCoverage;
		/// Set whether each render target has an unique blend function. When false the blend function in slot 0 will be used for all render targets.
		bool				mIndependentBlend;
	} BlendStateDesc;

	typedef struct DepthStateDesc
	{
		bool		mDepthTest;
		bool		mDepthWrite;
		CompareMode	mDepthFunc;
		bool		mStencilTest;
		uint8_t		mStencilReadMask;
		uint8_t		mStencilWriteMask;
		CompareMode	mStencilFrontFunc;
		StencilOp	mStencilFrontFail;
		StencilOp	mDepthFrontFail;
		StencilOp	mStencilFrontPass;
		CompareMode	mStencilBackFunc;
		StencilOp	mStencilBackFail;
		StencilOp	mDepthBackFail;
		StencilOp	mStencilBackPass;
	} DepthStateDesc;

	typedef struct RasterizerStateDesc
	{
		CullMode	mCullMode;
		int32_t		mDepthBias;
		float		mSlopeScaledDepthBias;
		FillMode	mFillMode;
		bool		mMultiSample;
		bool		mScissor;
		FrontFace	mFrontFace;
	} RasterizerStateDesc;

	typedef struct VertexLayout
	{
		uint32_t			mAttribCount;
		VertexAttrib		mAttribs[MAX_VERTEX_ATTRIBS];
	} VertexLayout;

	typedef enum PipelineType
	{
		PIPELINE_TYPE_UNDEFINED = 0,
		PIPELINE_TYPE_COMPUTE,
		PIPELINE_TYPE_GRAPHICS,
		PIPELINE_TYPE_RAYTRACING,
		PIPELINE_TYPE_COUNT,
	} PipelineType;

	typedef struct RaytracingPipelineDesc
	{
		struct Raytracing*	pRaytracing;
		RootSignature*		pGlobalRootSignature;
		Shader*             pRayGenShader;
		RootSignature*		pRayGenRootSignature;
		Shader**            ppMissShaders;
		RootSignature**		ppMissRootSignatures;
		void*				pHitGroups;
		RootSignature*		pEmptyRootSignature;
		unsigned			mMissShaderCount;
		unsigned			mHitGroupCount;
		// #TODO : Remove this after adding shader reflection for raytracing shaders
		unsigned			mPayloadSize;
		// #TODO : Remove this after adding shader reflection for raytracing shaders
		unsigned			mAttributeSize;
		unsigned			mMaxTraceRecursionDepth;
		unsigned            mMaxRaysCount;
	} RaytracingPipelineDesc;

	typedef struct GraphicsPipelineDesc
	{
		Shader*					pShaderProgram;
		RootSignature*			pRootSignature;
		VertexLayout*			pVertexLayout;
		BlendStateDesc*			pBlendState;
		DepthStateDesc*			pDepthState;
		RasterizerStateDesc*	pRasterizerState;
		TinyImageFormat*		pColorFormats;
		uint32_t				mRenderTargetCount;
		SampleCount				mSampleCount;
		uint32_t				mSampleQuality;
		TinyImageFormat			mDepthStencilFormat;
		PrimitiveTopology		mPrimitiveTopo;
		bool					mSupportIndirectCommandBuffer;
	} GraphicsPipelineDesc;

	typedef struct ComputePipelineDesc
	{
		Shader*        pShaderProgram;
		RootSignature* pRootSignature;
	} ComputePipelineDesc;

	typedef struct PipelineDesc
	{
		union
		{
			ComputePipelineDesc     mComputeDesc;
			GraphicsPipelineDesc    mGraphicsDesc;
			RaytracingPipelineDesc  mRaytracingDesc;
		};
		PipelineCache*              pCache;
		void*                       pPipelineExtensions;
		const char*                 pName;
		PipelineType                mType;
		uint32_t                    mExtensionCount;
	} PipelineDesc;

	typedef struct DescriptorData
	{
		/// User can either set name of descriptor or index (index in pRootSignature->pDescriptors array)
		/// Name of descriptor
		const char* pName = NULL;
		union
		{
			struct
			{
				/// Offset to bind the buffer descriptor
				uint64_t* pOffsets;
				uint64_t* pSizes;
			};

			// Descriptor set buffer extraction options
			struct
			{
				Shader*     mDescriptorSetShader;
				uint32_t    mDescriptorSetBufferIndex;
				ShaderStage mDescriptorSetShaderStage;
			};

			uint32_t mUAVMipSlice;
			bool mBindStencilResource;
		};
		/// Array of resources containing descriptor handles or constant to be used in ring buffer memory - DescriptorRange can hold only one resource type array
		union
		{
			/// Array of texture descriptors (srv and uav textures)
			Texture** ppTextures;
			/// Array of sampler descriptors
			Sampler** ppSamplers;
			/// Array of buffer descriptors (srv, uav and cbv buffers)
			Buffer** ppBuffers;
			/// Array of pipline descriptors
			Pipeline** ppPipelines;
			/// DescriptorSet buffer extraction
			DescriptorSet** ppDescriptorSet;
		};
		/// Number of resources in the descriptor(applies to array of textures, buffers,...)
		uint32_t mCount = 0;
		uint32_t mIndex = (uint32_t)-1;
		bool     mExtractBuffer = false;
	} DescriptorData;

	typedef struct BufferBarrier
	{
		Buffer*        pBuffer;
		ResourceState  mCurrentState;
		ResourceState  mNewState;
		uint8_t        mBeginOnly : 1;
		uint8_t        mEndOnly : 1;
		uint8_t        mAcquire : 1;
		uint8_t        mRelease : 1;
		uint8_t        mQueueType : 5;
	} BufferBarrier;

	typedef struct TextureBarrier
	{
		Texture*       pTexture;
		ResourceState  mCurrentState;
		ResourceState  mNewState;
		uint8_t        mBeginOnly : 1;
		uint8_t        mEndOnly : 1;
		uint8_t        mAcquire : 1;
		uint8_t        mRelease : 1;
		uint8_t        mQueueType : 5;
		/// Specifiy whether following barrier targets particular subresource
		uint8_t        mSubresourceBarrier : 1;
		/// Following values are ignored if mSubresourceBarrier is false
		uint8_t        mMipLevel : 7;
		uint16_t       mArrayLayer;
	} TextureBarrier;

	typedef struct RenderTargetBarrier
	{
		RenderTarget*  pRenderTarget;
		ResourceState  mCurrentState;
		ResourceState  mNewState;
		uint8_t        mBeginOnly : 1;
		uint8_t        mEndOnly : 1;
		uint8_t        mAcquire : 1;
		uint8_t        mRelease : 1;
		uint8_t        mQueueType : 5;
		/// Specifiy whether following barrier targets particular subresource
		uint8_t        mSubresourceBarrier : 1;
		/// Following values are ignored if mSubresourceBarrier is false
		uint8_t        mMipLevel : 7;
		uint16_t       mArrayLayer;
	} RenderTargetBarrier;

	// API functions
	// allocates memory and initializes the renderer -> returns pRenderer
	//
	void addUniformBuffer(Renderer* pRenderer, uint32_t size, Buffer** ppUniformBuffer);
	void addReadbackBuffer(Renderer* pRenderer, uint32_t size, Buffer** ppReadbackBuffer);
	void addUploadBuffer(Renderer* pRenderer, uint32_t size, Buffer** ppUploadBuffer);
	void removeBuffer(Renderer* pRenderer, Buffer* pUniformBuffer);
	void updateUniformBuffer(Renderer* pRenderer, Buffer* pBuffer, uint32_t srcOffset, const void* pData, uint32_t dstOffset, uint32_t size);
	void addTexture(Renderer* pRenderer, const TextureDesc* pDesc, Texture** pp_texture);
	void removeTexture(Renderer* pRenderer, Texture*p_texture);
	void addRenderTarget(Renderer* pRenderer, const RenderTargetDesc* p_desc, RenderTarget** pp_render_target);
	void removeRenderTarget(Renderer* pRenderer, RenderTarget* p_render_target);
	void addSampler(Renderer* pRenderer, const SamplerDesc* p_desc, Sampler** pp_sample);
	void removeSampler(Renderer* pRenderer, Sampler* p_sampler);

	// Retrieve the underlying texture pointer from the render target
	void getTextureFromRenderTarget(RenderTarget* pRenderTarget, Texture** ppTexture);
	void getCpuMappedAddress(Buffer* pBuffer, void** pData);
	bool hasShaderResource(RootSignature* pRootSignature, const char* pResName);

	// shader functions
	void addShader(Renderer* pRenderer, const ShaderLoadDesc* p_desc, Shader** p_shader_program);
	void removeShader(Renderer* pRenderer, Shader* p_shader_program);

	// pipeline functions
	void addRootSignature(Renderer* pRenderer, const RootSignatureDesc* pRootDesc, RootSignature** pp_root_signature);
	void removeRootSignature(Renderer* pRenderer, RootSignature* pRootSignature);
	void addPipeline(Renderer* pRenderer, const PipelineDesc* p_pipeline_settings, Pipeline** pp_pipeline);
	void removePipeline(Renderer* pRenderer, Pipeline* p_pipeline);

	// Descriptor Set functions
	void addDescriptorSet(Renderer* pRenderer, const DescriptorSetDesc* pDesc, DescriptorSet** ppDescriptorSet);
	void removeDescriptorSet(Renderer* pRenderer, DescriptorSet* pDescriptorSet);
	void updateDescriptorSet(Renderer* pRenderer, uint32_t index, DescriptorSet* pDescriptorSet, uint32_t count, const DescriptorData* pParams);

	// command buffer functions
	void cmdBindRenderTargets(Cmd* p_cmd, uint32_t render_target_count, RenderTarget** pp_render_targets, RenderTarget* p_depth_stencil, const LoadActionsDesc* loadActions, uint32_t* pColorArraySlices, uint32_t* pColorMipSlices, uint32_t depthArraySlice, uint32_t depthMipSlice);
	void cmdSetViewport(Cmd* p_cmd, float x, float y, float width, float height, float min_depth, float max_depth);
	void cmdSetScissor(Cmd* p_cmd, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
	void cmdBindPipeline(Cmd* p_cmd, Pipeline* p_pipeline);
	void cmdBindDescriptorSet(Cmd* pCmd, uint32_t index, DescriptorSet* pDescriptorSet);
	void cmdBindPushConstants(Cmd* pCmd, RootSignature* pRootSignature, const char* pName, const void* pConstants);
	void cmdDraw(Cmd* p_cmd, uint32_t vertex_count, uint32_t first_vertex);
	void cmdDrawInstanced(Cmd* p_cmd, uint32_t vertex_count, uint32_t first_vertex, uint32_t instance_count, uint32_t first_instance);
	void cmdDispatch(Cmd* p_cmd, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);

	// Texture copy functions
	void cmdCopyTexture(Cmd* p_cmd, Texture* pSrc, Texture* pDst);
	void mapAsynchronousResources(Renderer* pRenderer);
	void removeAsynchronousResources();
	void unmapBuffer(Renderer* pRenderer, Buffer* pBuffer);

	void cmdCopyResource(Cmd* pCmd, const TextureDesc* pDesc, Texture* pSrc, Buffer* pDst);
	void cmdCopyResource(Cmd* p_cmd, Buffer* pSrc, Texture* pDst);

	//
	////Debug markers
	////Color is in XRGB format (X being padding) 0xff0000 -> red, 0x00ff00 -> green, 0x0000ff -> blue
	//void cmdPushDebugMarker(Cmd* pCmd, uint32_t color, const char* name, ...);
	//void cmdPopDebugMarker(Cmd* pCmd);

	// Transition Commands
	void cmdResourceBarrier(Cmd* p_cmd,
		uint32_t buffer_barrier_count, BufferBarrier* p_buffer_barriers,
		uint32_t texture_barrier_count, TextureBarrier* p_texture_barriers,
		uint32_t rt_barrier_count, RenderTargetBarrier* p_rt_barriers);

	////Queue debug markers
	////Color is in XRGB format (X being padding) 0xff0000 -> red, 0x00ff00 -> green, 0x0000ff -> blue
	//void queuePushDebugMarker(Queue* pQueue, uint32_t color, const char* name, ...);
	//void queuePopDebugMarker(Queue* pQueue);
	/************************************************************************/
	// Debug Marker Interface
	/************************************************************************/
	void cmdBeginDebugMarker(Cmd* pCmd, float r, float g, float b, const char* pName);
	void cmdEndDebugMarker(Cmd* pCmd);
	void cmdAddDebugMarker(Cmd* pCmd, float r, float g, float b, const char* pName);
	/************************************************************************/
}

#endif //__AURARENDERER_H_9E2034BB_D65C_4EAE_9621_057ACCF12866_INCLUDED__
