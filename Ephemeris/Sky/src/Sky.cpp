/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include "Sky.h"

#include "../../../../The-Forge/Common_3/Renderer/ResourceLoader.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IFileSystem.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/ILog.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IMemory.h"

#define SKY_NEAR 50.0f
#define SKY_FAR 100000000.0f

int				        gNumberOfSpherePoints;
int				        gNumberOfSphereIndices;

#if defined(_DURANGO)
const int				gSphereResolution = 4;    // Increase for higher resolution spheres
#else
const int				gSphereResolution = 5;    // Increase for higher resolution spheres
#endif
const float				gSphereDiameter = 1.0f;

#define USING_MILKYWAY	0
#define USING_AURORA	0

//Precomputed Atmospheric Sacttering
Shader*				    pPAS_Shader = NULL;
Pipeline*			    pPAS_Pipeline = NULL;

Shader*           pSpaceShader = NULL;
Pipeline*         pSpacePipeline = NULL;

BlendState*		  pBlendStateSpace;
BlendState*		  pBlendStateSun;
BlendState*       pBlendStateStar;

Shader*           pSunShader = NULL;
Pipeline*         pSunPipeline = NULL;

Shader*           pStarShader = NULL;
Pipeline*         pStarPipeline = NULL;

Shader*           pMoonShader = NULL;
Pipeline*         pMoonPipeline = NULL;

Shader*           pMilkyWayShader = NULL;
Pipeline*         pMilkyWayPipeline = NULL;

Shader*           pAuroraShader = NULL;
Pipeline*         pAuroraPipeline = NULL;

Shader*           pAuroraComputeShader = NULL;
Pipeline*         pAuroraComputePipeline = NULL;

DescriptorSet*    pSkyDescriptorSet[2] = { NULL };
RootSignature*    pSkyRootSignature = NULL;
RootSignature*    pSkyRootSignatureCompute = NULL;

Buffer*           pSphereVertexBuffer = NULL;
Buffer*           pSphereIndexBuffer = NULL;

#if USING_MILKYWAY
Buffer*           pMilkyWayVertexBuffer = NULL;
Buffer*           pMilkyWayIndexBuffer = NULL;
uint32_t          MilkyWayIndexCount = 0;
#endif

Buffer*           pRenderSkyUniformBuffer[3] = { NULL };
Buffer*           pSpaceUniformBuffer[3] = { NULL };
Buffer*           pStarUniformBuffer[3] = { NULL };
Buffer*           pSunUniformBuffer[3] = { NULL };

typedef struct ParticleData
{
	vec4 ParticlePositions;
	vec4 ParticleColors;
	vec4 ParticleInfo;        // x: temperature, y: particle size, z: blink time seed,
}ParticleData;

typedef struct ParticleDataSet
{
	eastl::vector<ParticleData> ParticleDataArray;
}ParticleDataSet;

typedef struct ParticleSystem
{
	Buffer* pParticleVertexBuffer;
	Buffer* pParticleInstanceBuffer;
	ParticleDataSet particleDataSet;
} ParticleSystem;

ParticleSystem gParticleSystem;

mat4	  ViewProjMat;

static float g_ElapsedTime = 0.0f;

Icosahedron           sphere;
float*                pSpherePoints;
//Aurora                gAurora;

typedef struct SkySettings
{
	float4		SkyInfo; // x: fExposure, y: fInscatterIntencity, z: fInscatterContrast, w: fUnitsToM
	float4		OriginLocation;
} SkySettings;

SkySettings	gSkySettings;

static float      SunSize = 30000.0f;

static float      SpaceScale = 10000000.0f;
static float      NebulaScale = 9.453f;
static float      StarIntensity = 1.5f;
static float      StarDensity = 15.0f;
static float      StarDistribution = 2000000.0f;
//static float ParticleScale = 100.0f;
static float ParticleSize = 20000.0f;

uint32_t NebulaHighColor = 0x5D3E2878;
uint32_t NebulaMidColor = 0x06272EFF;
uint32_t NebulaLowColor = 0x06041CFF;


struct RenderSkyUniformBuffer
{
	mat4	InvViewMat;
	mat4	InvProjMat;

	float4	LightDirection; // w:exposure
	float4	CameraPosition;
	float4	QNNear;
	float4	InScatterParams;

	float4	LightIntensity;
};

struct SpaceUniformBuffer
{
	mat4 ViewProjMat;
	float4 LightDirection;
	float4 ScreenSize;
};

struct SunUniformBuffer
{
	mat4 ViewMat;
	mat4 ViewProjMat;
	float4 LightDirection;
	float4 Dx;
	float4 Dy;
};

struct AuroraParticle
{
	float4 PrevPosition;			// PrePosition and movable flag
	float4 Position;				// Position and mass
	float4 Acceleration;
};

struct AuroraConstraint
{
	uint IndexP0;
	uint IndexP1;
	float RestDistance;
	float Padding00;
	float Padding01;
	float Padding02;
	float Padding03;
	float Padding04;
};

struct AuroraUniformStruct
{
	uint        maxVertex;
	float       heightOffset;
	float       height;
	float       deltaTime;

	mat4        ViewProjMat;
};

AuroraUniformStruct gAuroraUniformStruct;

Buffer*               pAuroraVertexBuffer;
Buffer*               pAuroraParticle;
Buffer*               pAuroraConstraint;
Buffer*               pAuroraUniformBuffer;

const float AuroraWidth = 100000.0f;
const float AuroraHeight = 4000.0f;
const uint32_t AuroraParticleNum = 64;

#if defined(VULKAN)
static void TransitionRenderTargets(RenderTarget *pRT, ResourceState state, Renderer* renderer, Cmd* cmd, Queue* queue, Fence* fence)
{
	// Transition render targets to desired state
	beginCmd(cmd);

	TextureBarrier barrier[] = {
		   { pRT->pTexture, state }
	};

	cmdResourceBarrier(cmd, 0, NULL, 1, barrier);
	endCmd(cmd);

	queueSubmit(queue, 1, &cmd, fence, 0, NULL, 0, NULL);
	waitForFences(renderer, 1, &fence);
}
#endif


static void ShaderPath(const eastl::string &shaderPath, char* pShaderName, eastl::string &result)
{
	result.resize(0);
	eastl::string shaderName(pShaderName);
	result = shaderPath + shaderName;
}

mat4 MakeRotationMatrix(float angle, vec3 axis)
{
	float s, c;
	//sincos(-angle, s, c);
	s = sin(-angle);
	c = cos(-angle);

	float x, y, z;
	x = axis.getX();
	y = axis.getY();
	z = axis.getZ();
	float xy, yz, zx;
	xy = axis.getX() * axis.getY();
	yz = axis.getY() * axis.getZ();
	zx = axis.getZ() * axis.getX();
	float oneMinusC = 1.0f - c;

	mat4 result;
	result.setCol0(vec4(x * x * oneMinusC + c, xy * oneMinusC + z * s, zx * oneMinusC - y * s, 0.0f));
	result.setCol1(vec4(xy * oneMinusC - z * s, y * y * oneMinusC + c, yz * oneMinusC + x * s, 0.0f));
	result.setCol2(vec4(zx * oneMinusC + y * s, yz * oneMinusC - x * s, z * z * oneMinusC + c, 0.0f));
	result.setCol3(vec4(0.0f, 0.0f, 0.0f, 1.0f));
	return result;
}

void Sky::CalculateLookupData()
{
	TextureLoadDesc SkyTransmittanceTextureDesc = {};
	SkyTransmittanceTextureDesc.mRoot = FSR_OtherFiles;
#if defined(_DURANGO)
	SkyTransmittanceTextureDesc.pFilename = "Textures/Transmittance.dds";
#else
	SkyTransmittanceTextureDesc.pFilename = "../../../../Ephemeris/Sky/resources/Textures/Transmittance.dds";
#endif
	SkyTransmittanceTextureDesc.ppTexture = &pTransmittanceTexture;
	addResource(&SkyTransmittanceTextureDesc, false);

	TextureLoadDesc SkyIrradianceTextureDesc = {};
	SkyIrradianceTextureDesc.mRoot = FSR_OtherFiles;
#if defined(_DURANGO)
	SkyIrradianceTextureDesc.pFilename = "Textures/Irradiance.dds";
#else
	SkyIrradianceTextureDesc.pFilename = "../../../../Ephemeris/Sky/resources/Textures/Irradiance.dds";
#endif
	SkyIrradianceTextureDesc.ppTexture = &pIrradianceTexture;
	addResource(&SkyIrradianceTextureDesc, false);

	TextureLoadDesc SkyInscatterTextureDesc = {};
	SkyInscatterTextureDesc.mRoot = FSR_OtherFiles;
#if defined(_DURANGO)
	SkyInscatterTextureDesc.pFilename = "Textures/Inscatter.dds";
#else
	SkyInscatterTextureDesc.pFilename = "../../../../Ephemeris/Sky/resources/Textures/Inscatter.dds";
#endif
	SkyInscatterTextureDesc.ppTexture = &pInscatterTexture;
	addResource(&SkyInscatterTextureDesc, false);

	TextureLoadDesc SkyMoonTextureDesc = {};
	SkyMoonTextureDesc.mRoot = FSR_OtherFiles;
#if defined(_DURANGO)
	SkyMoonTextureDesc.pFilename = "Textures/Moon.dds";
#else
	SkyMoonTextureDesc.pFilename = "../../../../Ephemeris/Sky/resources/Textures/Moon.dds";
#endif
	SkyMoonTextureDesc.ppTexture = &pMoonTexture;
	addResource(&SkyMoonTextureDesc, false);
}

//	<https://www.shadertoy.com/view/4dS3Wd>
//	By Morgan McGuire @morgan3d, http://graphicscodex.com
//

float hash(float n)
{
	return fmod(sin(n) * 10000.0f, 1.0f);
}

float hash(vec2 p) { return fmod(10000.0f * sin(17.0f * p.getX() + p.getY() * 0.1f) * (0.1f + abs(sin(p.getY() * 13.0f + p.getX()))), 1.0f); }
float noise(float x) { float i = floor(x); float f = fmod(x, 1.0f); float u = f * f * (3.0f - 2.0f * f); return lerp(hash(i), hash(i + 1.0f), u); }
float noise(vec2 x) { vec2 i = vec2(floor(x.getX()), floor(x.getY())); vec2 f = vec2(fmod(x.getX(), 1.0f), fmod(x.getY(), 1.0f)); float a = hash(i); float b = hash(i + vec2(1.0f, 0.0f)); float c = hash(i + vec2(0.0f, 1.0f)); float d = hash(i + vec2(1.0f, 1.0f)); vec2 u = vec2(f.getX() * f.getX() * (3.0f - 2.0f * f.getX()), f.getY() * f.getY() * (3.0f - 2.0f * f.getY())); return lerp(a, b, u.getX()) + (c - a) * u.getY() * (1.0f - u.getX()) + (d - b) * u.getX() * u.getY(); }

float noise(vec3 x)
{
	// The noise function returns a value in the range -1.0f -> 1.0f

	vec3 p = vec3(floor(x.getX()), floor(x.getY()), floor(x.getZ()));
	vec3 f = vec3(fmod(x.getX(), 1.0f), fmod(x.getY(), 1.0f), fmod(x.getZ(), 1.0f));

	//f = f * f*(3.0 - 2.0*f);
	f = vec3(f.getX() * f.getX() * (3.0f - 2.0f * f.getX()), f.getY() * f.getY() * (3.0f - 2.0f * f.getY()), f.getZ() * f.getZ() * (3.0f - 2.0f * f.getZ()));

	float n = p.getX() + p.getY() * 57.0f + 113.0f * p.getZ();

	return lerp(lerp(lerp(hash(n + 0.0f), hash(n + 1.0f), f.getX()), lerp(hash(n + 57.0f), hash(n + 58.0f), f.getX()), f.getY()), lerp(lerp(hash(n + 113.0f), hash(n + 114.0f), f.getX()), lerp(hash(n + 170.0f), hash(n + 171.0f), f.getX()), f.getY()), f.getZ());
}


float3 ColorTemperatureToRGB(float temperatureInKelvins)
{
	float3 retColor;

	temperatureInKelvins = clamp(temperatureInKelvins, 1000.0f, 40000.0f) / 100.0f;

	if (temperatureInKelvins <= 66.0f)
	{
		retColor.x = 1.0f;
		retColor.y = clamp(0.39008157876901960784f * log(temperatureInKelvins) - 0.63184144378862745098f, 0.0f, 1.0f);
	}
	else
	{
		float t = temperatureInKelvins - 60.0f;
		retColor.x = clamp(1.29293618606274509804f * pow(t, -0.1332047592f), 0.0f, 1.0f);
		retColor.y = clamp(1.12989086089529411765f * pow(t, -0.0755148492f), 0.0f, 1.0f);
	}

	if (temperatureInKelvins > 66.0f)
		retColor.z = 1.0f;
	else if (temperatureInKelvins <= 19.0f)
		retColor.z = 0.0f;
	else
		retColor.z = clamp(0.54320678911019607843f * log(temperatureInKelvins - 10.0f) - 1.19625408914f, 0.0f, 1.0f);

	return retColor;
}

void Sky::GenerateRing(eastl::vector<float> &vertices, eastl::vector<uint32_t> &indices, uint32_t WidthDividor, uint32_t HeightDividor, float radius, float height)
{

	float DegreeToRadian = PI / 180.0f;

	float angle = 360.0f / (float)WidthDividor * DegreeToRadian;

	float currentAngle;

	float heightGap = height / (float)HeightDividor;
	float y = height * 0.5f;

	for (uint32_t i = 0; i < HeightDividor; i++)
	{
		currentAngle = 0.0f;
		for (uint32_t w = 0; w <= WidthDividor; w++)
		{
			// POS
			vertices.push_back(cos(currentAngle) * radius);
			vertices.push_back(y);
			vertices.push_back(sin(currentAngle) * radius);

			vec3 normailzedVertex = normalize(vec3(cos(currentAngle), 0.0f, sin(currentAngle)));

			// NOR
			vertices.push_back(normailzedVertex.getX());
			vertices.push_back(0.0f);
			vertices.push_back(normailzedVertex.getZ());

			// Info
			vertices.push_back(0.0f);
			vertices.push_back(0.0f);
			vertices.push_back(0.0f);

			currentAngle += angle;
		}
		y -= heightGap;
	}

	uint WidthRow = (WidthDividor + 1);

	for (uint h = 0; h < HeightDividor - 1; h++)
	{
		for (uint w = 0; w < WidthDividor; w++)
		{
			indices.push_back(w + (h * WidthRow));
			indices.push_back(w + (WidthRow)+(h * WidthRow));
			indices.push_back(w + 1 + (h * WidthRow));

			indices.push_back(w + 1 + (h * WidthRow));
			indices.push_back(w + (WidthRow)+(h * WidthRow));
			indices.push_back(w + (WidthRow + 1) + (h * WidthRow));
		}
	}
}


// Generates an array of vertices and normals for a sphere
void Sky::GenerateIcosahedron(float **ppPoints, eastl::vector<float> &vertices, eastl::vector<uint32_t> &indices, int numberOfDivisions, float radius)
{
	sphere.CreateIcosphere(numberOfDivisions, vertices, indices);

	int numVertex = (int)vertices.size() / 3;

	float3* pPoints = (float3*)conf_malloc(numVertex * (sizeof(float3) * 3));

	uint32_t vertexCounter = 0;

	const float* position = vertices.data();

	float minVal = 10.0f;
	float maxVal = -10.0f;

	for (int i = 0; i < numVertex; ++i)
	{
		vec3 tempPosition = vec3(position[i * 3], position[i * 3 + 1], position[i * 3 + 2]);
		pPoints[vertexCounter++] = v3ToF3(tempPosition) * SpaceScale;

		vec3 normalizedPosition = normalize(tempPosition);
		pPoints[vertexCounter++] = v3ToF3(normalizedPosition);

		vec3 normalizedPosition01 = normalizedPosition + vec3(1.0f, 1.0f, 1.0f);
		normalizedPosition01 *= 0.5f; // normalized to [0,1]
		normalizedPosition01 *= NebulaScale; // scale it

		float NoiseValue01 = noiseGenerator.perlinNoise3D(normalizedPosition01.getX(), normalizedPosition01.getY(), normalizedPosition01.getZ());

		maxVal = max(maxVal, NoiseValue01);
		minVal = min(minVal, NoiseValue01);

		vec3 normalizedPosition02 = vec3(normalizedPosition.getX(), -normalizedPosition.getY(), normalizedPosition.getZ()) + vec3(1.0f, 1.0f, 1.0f);
		normalizedPosition02 *= 0.5f; // normalized to [0,1]
		normalizedPosition02 *= NebulaScale; // scale it

		float NoiseValue02 = noiseGenerator.perlinNoise3D(normalizedPosition02.getX(), normalizedPosition02.getY(), normalizedPosition02.getZ());

		maxVal = max(maxVal, NoiseValue02);
		minVal = min(minVal, NoiseValue02);

		vec3 normalizedPosition03 = vec3(-normalizedPosition.getX(), normalizedPosition.getY(), -normalizedPosition.getZ()) + vec3(1.0f, 1.0f, 1.0f);
		normalizedPosition03 *= 0.5f; // normalized to [0,1]
		normalizedPosition03 *= NebulaScale; // scale it

		float NoiseValue03 = noiseGenerator.perlinNoise3D(normalizedPosition03.getX(), normalizedPosition03.getY(), normalizedPosition03.getZ());

		maxVal = max(maxVal, NoiseValue03);
		minVal = min(minVal, NoiseValue03);

		pPoints[vertexCounter++] = float3(NoiseValue01, NoiseValue02, NoiseValue03); //   float3((float)rand()/RAND_MAX, 0.0f, 0.0f);
	}

	float len = maxVal - minVal;

	for (int i = 0; i < numVertex; ++i)
	{
		// Nebula Density
		int index = i * 3 + 2;
		pPoints[index] = (pPoints[index] - float3(minVal, minVal, minVal)) / len;

		float Density = (pPoints[index].getX() + pPoints[index].getY() + pPoints[index].getZ()) / 3.0f;
		Density = pow(Density, 3.0f);
		int maxStar = (int)(StarDensity * Density);
		for (int j = 0; j < maxStar; j++)
		{
			vec3 Positions = f3Tov3(pPoints[index - 2]);
			Positions += (vec3((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX) * 2.0f - vec3(1.0f, 1.0f, 1.0f)) * StarDistribution;

			float temperature = ((float)rand() / RAND_MAX) * 30000.0f + 3700.0f;
			vec3 StarColor = f3Tov3(ColorTemperatureToRGB(temperature));
			vec4 Colors = vec4(StarColor, (((float)rand() / RAND_MAX * 0.9f) + 0.1f) * StarIntensity);

			float starSize = (((float)rand() / RAND_MAX * 1.1f) + 0.5f);
			starSize *= starSize;

			vec4 Info = vec4(temperature, starSize * ParticleSize, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX);

			ParticleData tempParticleData;
			tempParticleData.ParticlePositions = vec4(Positions, 1.0f);
			tempParticleData.ParticleColors = Colors;
			tempParticleData.ParticleInfo = Info;

			gParticleSystem.particleDataSet.ParticleDataArray.push_back(tempParticleData);
		}
	}

	(*ppPoints) = (float*)pPoints;
}

bool Sky::Init(Renderer* const renderer)
{
	/*
	Timer t;

	B_Spline test;
	eastl::vector<float> times;


	uint numberOfPts = 64;

	for (uint i = 0; i < numberOfPts; i++)
	{
	  times.push_back((float)i / (float)numberOfPts);
	}

	times.push_back(0.0f);
	times.push_back(0.1f);
	times.push_back(0.2f);
	times.push_back(0.3f);
	times.push_back(0.4f);
	times.push_back(0.5f);
	times.push_back(0.6f);
	times.push_back(0.7f);
	times.push_back(0.8f);
	times.push_back(0.9f);
	times.push_back(1.0f);

	eastl::vector<vec3> points;
	points.push_back(vec3(-1.0f, 0.0, 0.0f));
	points.push_back(vec3(-0.5f, 0.0, 0.5f));
	points.push_back(vec3(0.5f, 0.0, -0.5f));
	points.push_back(vec3(1.0f, 0.0, 0.0f));

	eastl::vector<float> knots;
	knots.push_back(0.0f);
	knots.push_back(0.0f);
	knots.push_back(0.0f);
	knots.push_back(1.0f);
	knots.push_back(2.0f);
	knots.push_back(2.0f);
	knots.push_back(2.0f);
	eastl::vector<float> weights;
	eastl::vector<vec3> results;
	test.interpolate(times, 2, points, knots, weights, results);

	LOGF(LogLevel::eINFO, "B_Spline CPU: %f", t.GetMSec(false) * 0.001f);
	*/


	//gAurora.Init(100000.0f,4000.0f,64, 1);



	eastl::vector< AuroraParticle > initialAuroraData;

	for (uint32_t x = 0; x < AuroraParticleNum; x++)
	{
		AuroraParticle tempParticle;
		tempParticle.Position = float4(AuroraWidth * ((float)x / (float)AuroraParticleNum), 0.0f, 0.0f, 1.0f);
		tempParticle.PrevPosition = float4(tempParticle.Position.getX(), tempParticle.Position.getY(), tempParticle.Position.getZ(), 1.0);
		tempParticle.Acceleration = float4(0.0f, 0.0f, 0.0f, 0.0f);

		initialAuroraData.push_back(tempParticle);
	}

	eastl::vector< AuroraConstraint > initialAuroraConstraintData;

	for (uint32_t x = 0; x < AuroraParticleNum - 1; x++)
	{
		AuroraConstraint tempParticle;
		tempParticle.IndexP0 = x;
		tempParticle.IndexP1 = x + 1;

		float3 p0 = initialAuroraData[x].Position.getXYZ();
		float3 p1 = initialAuroraData[x + 1].Position.getXYZ();

		float x2 = p0.getX() - p1.getX();
		x2 *= x2;

		float y2 = p0.getY() - p1.getY();
		y2 *= y2;

		float z2 = p0.getZ() - p1.getZ();
		z2 *= z2;

		tempParticle.RestDistance = sqrt(x2 + y2 + z2);

		tempParticle.Padding00 = 0.0f;
		tempParticle.Padding01 = 0.0f;
		tempParticle.Padding02 = 0.0f;
		tempParticle.Padding03 = 0.0f;
		tempParticle.Padding04 = 0.0f;

		initialAuroraConstraintData.push_back(tempParticle);
	}

	pRenderer = renderer;

	RasterizerStateDesc rasterizerStateDesc = {};
	rasterizerStateDesc.mCullMode = CULL_MODE_NONE;
	addRasterizerState(pRenderer, &rasterizerStateDesc, &pRasterizerForSky);

	//////////////////////////////////// Samplers ///////////////////////////////////////////////////

	SamplerDesc samplerClampDesc = {
	FILTER_LINEAR, FILTER_LINEAR, MIPMAP_MODE_LINEAR,
	ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE
	};
	addSampler(pRenderer, &samplerClampDesc, &pLinearClampSampler);

	//////////////////////////////////// Samplers ///////////////////////////////////////////////////

	samplerClampDesc = {
	FILTER_LINEAR, FILTER_LINEAR, MIPMAP_MODE_LINEAR,
	ADDRESS_MODE_CLAMP_TO_BORDER, ADDRESS_MODE_CLAMP_TO_BORDER, ADDRESS_MODE_CLAMP_TO_BORDER
	};

	addSampler(pRenderer, &samplerClampDesc, &pLinearBorderSampler);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(_DURANGO)
	eastl::string shaderPath("");
#elif defined(DIRECT3D12)
	eastl::string shaderPath("../../../../../Ephemeris/Sky/resources/Shaders/D3D12/");
#elif defined(VULKAN)
	eastl::string shaderPath("../../../../../Ephemeris/Sky/resources/Shaders/Vulkan/");
#elif defined(METAL)
	eastl::string shaderPath("../../../../../Ephemeris/Sky/resources/Shaders/Metal/");
#endif
	eastl::string shaderFullPath;

	ShaderLoadDesc skyShader = {};
	ShaderPath(shaderPath, (char*)"RenderSky.vert", shaderFullPath);
	skyShader.mStages[0] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
	ShaderPath(shaderPath, (char*)"RenderSky.frag", shaderFullPath);
	skyShader.mStages[1] = { shaderFullPath, NULL, 0, FSR_SrcShaders };

	addShader(pRenderer, &skyShader, &pPAS_Shader);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc spaceShader = {};
	ShaderPath(shaderPath, (char*)"Space.vert", shaderFullPath);
	spaceShader.mStages[0] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
	ShaderPath(shaderPath, (char*)"Space.frag", shaderFullPath);
	spaceShader.mStages[1] = { shaderFullPath, NULL, 0, FSR_SrcShaders };

	addShader(pRenderer, &spaceShader, &pSpaceShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc sunShader = {};
#if !defined(METAL)
	ShaderPath(shaderPath, (char*)"Sun.vert", shaderFullPath);
	sunShader.mStages[0] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
	ShaderPath(shaderPath, (char*)"Sun.geom", shaderFullPath);
	sunShader.mStages[1] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
	ShaderPath(shaderPath, (char*)"Sun.frag", shaderFullPath);
	sunShader.mStages[2] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
#else
	ShaderPath(shaderPath, (char*)"Sun.vert", shaderFullPath);
	sunShader.mStages[0] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
	ShaderPath(shaderPath, (char*)"Sun.frag", shaderFullPath);
	sunShader.mStages[1] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
#endif
	addShader(pRenderer, &sunShader, &pSunShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc starShader = {};
#if !defined(METAL)
	ShaderPath(shaderPath, (char*)"Star.vert", shaderFullPath);
	starShader.mStages[0] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
	ShaderPath(shaderPath, (char*)"Star.geom", shaderFullPath);
	starShader.mStages[1] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
	ShaderPath(shaderPath, (char*)"Star.frag", shaderFullPath);
	starShader.mStages[2] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
#else
	ShaderPath(shaderPath, (char*)"Star.vert", shaderFullPath);
	starShader.mStages[0] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
	ShaderPath(shaderPath, (char*)"Star.frag", shaderFullPath);
	starShader.mStages[1] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
#endif
	addShader(pRenderer, &starShader, &pStarShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc MilkyWayShader = {};
	ShaderPath(shaderPath, (char*)"MilkyWay.vert", shaderFullPath);
	MilkyWayShader.mStages[0] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
	ShaderPath(shaderPath, (char*)"MilkyWay.frag", shaderFullPath);
	MilkyWayShader.mStages[1] = { shaderFullPath, NULL, 0, FSR_SrcShaders };

	addShader(pRenderer, &MilkyWayShader, &pMilkyWayShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc AuroraShader = {};
#if !defined(METAL)
	ShaderPath(shaderPath, (char*)"Aurora.vert", shaderFullPath);
	AuroraShader.mStages[0] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
	ShaderPath(shaderPath, (char*)"Aurora.geom", shaderFullPath);
	AuroraShader.mStages[1] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
	ShaderPath(shaderPath, (char*)"Aurora.frag", shaderFullPath);
	AuroraShader.mStages[2] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
#else
	ShaderPath(shaderPath, (char*)"Aurora.vert", shaderFullPath);
	AuroraShader.mStages[0] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
	ShaderPath(shaderPath, (char*)"Aurora.frag", shaderFullPath);
	AuroraShader.mStages[1] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
#endif
	addShader(pRenderer, &AuroraShader, &pAuroraShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc AuroraComputeShader = {};
	ShaderPath(shaderPath, (char*)"Aurora.comp", shaderFullPath);
	AuroraComputeShader.mStages[0] = { shaderFullPath, NULL, 0, FSR_SrcShaders };

	addShader(pRenderer, &AuroraComputeShader, &pAuroraComputeShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	const char* pSkySamplerNames[] = { "g_LinearClamp", "g_LinearBorder" };
	Sampler* pSkySamplers[] = { pLinearClampSampler, pLinearBorderSampler };
	Shader*           shaders[] = { pPAS_Shader, pSpaceShader, pSunShader, pStarShader, pMilkyWayShader, pAuroraShader };
	RootSignatureDesc rootDesc = {};
	rootDesc.mShaderCount = 6;
	rootDesc.ppShaders = shaders;
	rootDesc.mStaticSamplerCount = 2;
	rootDesc.ppStaticSamplerNames = pSkySamplerNames;
	rootDesc.ppStaticSamplers = pSkySamplers;

	addRootSignature(pRenderer, &rootDesc, &pSkyRootSignature);

	Shader*           shaderComputes[] = { pAuroraComputeShader };
	rootDesc = {};
	rootDesc.mShaderCount = 1;
	rootDesc.ppShaders = shaderComputes;

	addRootSignature(pRenderer, &rootDesc, &pSkyRootSignatureCompute);

	DescriptorSetDesc setDesc = { pSkyRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
	addDescriptorSet(pRenderer, &setDesc, &pSkyDescriptorSet[0]);
	setDesc = { pSkyRootSignature, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
	addDescriptorSet(pRenderer, &setDesc, &pSkyDescriptorSet[1]);

	BlendStateDesc blendStateSpaceDesc = {};
	blendStateSpaceDesc.mBlendModes[0] = BM_ADD;
	blendStateSpaceDesc.mBlendAlphaModes[0] = BM_ADD;

	blendStateSpaceDesc.mSrcFactors[0] = BC_ONE;
	blendStateSpaceDesc.mDstFactors[0] = BC_ONE_MINUS_SRC_ALPHA;

	blendStateSpaceDesc.mSrcAlphaFactors[0] = BC_ONE;
	blendStateSpaceDesc.mDstAlphaFactors[0] = BC_ZERO;

	blendStateSpaceDesc.mMasks[0] = ALL;
	blendStateSpaceDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;
	addBlendState(pRenderer, &blendStateSpaceDesc, &pBlendStateSpace);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	BlendStateDesc blendStateSunDesc = {};
	blendStateSunDesc.mBlendModes[0] = BM_ADD;
	blendStateSunDesc.mBlendAlphaModes[0] = BM_ADD;

	blendStateSunDesc.mSrcFactors[0] = BC_SRC_ALPHA;
	blendStateSunDesc.mDstFactors[0] = BC_ONE;

	blendStateSunDesc.mSrcAlphaFactors[0] = BC_ONE;
	blendStateSunDesc.mDstAlphaFactors[0] = BC_ZERO;

	blendStateSunDesc.mMasks[0] = ALL;
	blendStateSunDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;
	addBlendState(pRenderer, &blendStateSunDesc, &pBlendStateSun);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	BlendStateDesc blendStateAdditiveDesc = {};
	blendStateAdditiveDesc.mBlendModes[0] = BM_ADD;
	blendStateAdditiveDesc.mBlendAlphaModes[0] = BM_ADD;

	blendStateAdditiveDesc.mSrcFactors[0] = BC_SRC_ALPHA;
	blendStateAdditiveDesc.mDstFactors[0] = BC_ONE;

	blendStateAdditiveDesc.mSrcAlphaFactors[0] = BC_SRC_ALPHA;
	blendStateAdditiveDesc.mDstAlphaFactors[0] = BC_ONE;

	blendStateAdditiveDesc.mMasks[0] = ALL;
	blendStateAdditiveDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;
	addBlendState(pRenderer, &blendStateAdditiveDesc, &pBlendStateStar);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////


	float screenQuadPoints[] = {
		0.0f,  0.0f, 0.5f, 0.0f, 0.0f,
		0.0f,  0.0f, 0.5f, 0.0f, 0.0f,
		0.0f,  0.0f, 0.5f, 0.0f, 0.0f,
		0.0f,  0.0f, 0.5f, 0.0f, 0.0f
	};

	BufferLoadDesc TriangularVbDesc = {};
	TriangularVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	TriangularVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	TriangularVbDesc.mDesc.mVertexStride = sizeof(float) * 5;
	TriangularVbDesc.mDesc.mSize = (uint64_t)(TriangularVbDesc.mDesc.mVertexStride * 4);
	TriangularVbDesc.pData = screenQuadPoints;
	TriangularVbDesc.ppBuffer = &pGlobalTriangularVertexBuffer;
	addResource(&TriangularVbDesc);

	CalculateLookupData();

#if USING_MILKYWAY

	eastl::vector<float> MilkyWayVertices;
	eastl::vector<uint32_t> MilkyWayIndices;

	GenerateRing(MilkyWayVertices, MilkyWayIndices, 128, 10, 1000000.0f, 100000.0f);

	BufferLoadDesc MilkyWayVbDesc = {};
	MilkyWayVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	MilkyWayVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	MilkyWayVbDesc.mDesc.mVertexStride = sizeof(float3) * 3;
	MilkyWayVbDesc.mDesc.mSize = (uint64_t)MilkyWayVertices.size() / 3 * MilkyWayVbDesc.mDesc.mVertexStride;
	MilkyWayVbDesc.pData = MilkyWayVertices.data();
	MilkyWayVbDesc.ppBuffer = &pMilkyWayVertexBuffer;
	addResource(&MilkyWayVbDesc);

	BufferLoadDesc MilkyWayIbDesc = {};
	MilkyWayIbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
	MilkyWayIbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	MilkyWayIbDesc.mDesc.mSize = (uint64_t)MilkyWayIndices.size() * sizeof(uint32_t);
	MilkyWayIbDesc.mDesc.mIndexType = INDEX_TYPE_UINT32;
	MilkyWayIbDesc.pData = MilkyWayIndices.data();
	MilkyWayIbDesc.ppBuffer = &pMilkyWayIndexBuffer;
	addResource(&MilkyWayIbDesc);

	MilkyWayIndexCount = (uint32_t)MilkyWayIndices.size();

#endif

	// Generate sphere vertex buffer
	eastl::vector<float> IcosahedronVertices;
	eastl::vector<uint32_t> IcosahedronIndices;

	GenerateIcosahedron(&pSpherePoints, IcosahedronVertices, IcosahedronIndices, gSphereResolution, gSphereDiameter);

	BufferLoadDesc particleBufferDesc = {};
	particleBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	particleBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	particleBufferDesc.mDesc.mVertexStride = sizeof(float) * 6;
	particleBufferDesc.mDesc.mSize = particleBufferDesc.mDesc.mVertexStride * 4;
	particleBufferDesc.ppBuffer = &gParticleSystem.pParticleVertexBuffer;


	float screenQuadPointsForStar[] = {
		0.0f,  0.0f, 0.5f, 1.0f, 0.0f, 0.0f,
		0.0f,  0.0f, 0.5f, 1.0f, 0.0f, 0.0f,
		0.0f,  0.0f, 0.5f, 1.0f, 0.0f, 0.0f,
		0.0f,  0.0f, 0.5f, 1.0f, 0.0f, 0.0f
	};

	particleBufferDesc.pData = screenQuadPointsForStar;

	addResource(&particleBufferDesc);

	particleBufferDesc = {};
	particleBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	particleBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	particleBufferDesc.mDesc.mVertexStride = sizeof(ParticleData);
	particleBufferDesc.mDesc.mSize = (uint64_t)gParticleSystem.particleDataSet.ParticleDataArray.size() * particleBufferDesc.mDesc.mVertexStride;
	particleBufferDesc.pData = gParticleSystem.particleDataSet.ParticleDataArray.data();
	particleBufferDesc.ppBuffer = &gParticleSystem.pParticleInstanceBuffer;
	addResource(&particleBufferDesc);

	BufferLoadDesc sphereVbDesc = {};
	sphereVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	sphereVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	sphereVbDesc.mDesc.mVertexStride = sizeof(float3) * 3;
	sphereVbDesc.mDesc.mSize = (uint64_t)IcosahedronVertices.size() / 3 * sphereVbDesc.mDesc.mVertexStride;
	sphereVbDesc.pData = pSpherePoints;
	sphereVbDesc.ppBuffer = &pSphereVertexBuffer;
	addResource(&sphereVbDesc);

	BufferLoadDesc sphereIbDesc = {};
	sphereIbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
	sphereIbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	sphereIbDesc.mDesc.mSize = (uint64_t)IcosahedronIndices.size() * sizeof(uint32_t);
	sphereIbDesc.mDesc.mIndexType = INDEX_TYPE_UINT32;
	sphereIbDesc.pData = IcosahedronIndices.data();
	sphereIbDesc.ppBuffer = &pSphereIndexBuffer;
	addResource(&sphereIbDesc);

	BufferLoadDesc AuroraVbDesc = {};
	AuroraVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	AuroraVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	AuroraVbDesc.mDesc.mVertexStride = sizeof(float3) * 3;
	AuroraVbDesc.mDesc.mSize = (uint64_t)AuroraParticleNum * AuroraVbDesc.mDesc.mVertexStride;
	AuroraVbDesc.ppBuffer = &pAuroraVertexBuffer;
	addResource(&AuroraVbDesc);

	BufferLoadDesc AuroraParticleDesc = {};
	AuroraParticleDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_BUFFER | DESCRIPTOR_TYPE_RW_BUFFER;
	AuroraParticleDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	AuroraParticleDesc.mDesc.mElementCount = (uint64_t)AuroraParticleNum;
	AuroraParticleDesc.mDesc.mStructStride = sizeof(AuroraParticle);
	AuroraParticleDesc.mDesc.mSize = AuroraParticleDesc.mDesc.mElementCount * AuroraParticleDesc.mDesc.mStructStride;
	AuroraParticleDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT | BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
	AuroraParticleDesc.pData = initialAuroraData.data();

	// for(unsigned int i=0; i<gImageCount; i++)
	// {
	AuroraParticleDesc.ppBuffer = &pAuroraParticle;
	addResource(&AuroraParticleDesc);
	// }

	BufferLoadDesc AuroraConstraintDesc = {};
	AuroraConstraintDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_BUFFER | DESCRIPTOR_TYPE_RW_BUFFER;
	AuroraConstraintDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	AuroraConstraintDesc.mDesc.mElementCount = (uint64_t)AuroraParticleNum - 1;
	AuroraConstraintDesc.mDesc.mStructStride = sizeof(AuroraConstraint);
	AuroraConstraintDesc.mDesc.mSize = AuroraConstraintDesc.mDesc.mElementCount * AuroraConstraintDesc.mDesc.mStructStride;
	AuroraConstraintDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT | BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
	AuroraConstraintDesc.pData = initialAuroraConstraintData.data();

	// for (unsigned int i = 0; i < gImageCount; i++)
	// {
	AuroraConstraintDesc.ppBuffer = &pAuroraConstraint;
	addResource(&AuroraConstraintDesc);
	// }


	BufferLoadDesc AuroraUniformDesc = {};
	AuroraUniformDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	AuroraUniformDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	AuroraUniformDesc.mDesc.mElementCount = 1;
	AuroraUniformDesc.mDesc.mStructStride = sizeof(AuroraUniformStruct);
	AuroraUniformDesc.mDesc.mSize = AuroraUniformDesc.mDesc.mElementCount * AuroraUniformDesc.mDesc.mStructStride;
	AuroraUniformDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT | BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;

	// for (unsigned int i = 0; i < gImageCount; i++)
	// {
	AuroraUniformDesc.ppBuffer = &pAuroraUniformBuffer;
	addResource(&AuroraUniformDesc);
	// }


	BufferLoadDesc renderSkybDesc = {};
	renderSkybDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	renderSkybDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	renderSkybDesc.mDesc.mSize = sizeof(RenderSkyUniformBuffer);
	renderSkybDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT | BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
	renderSkybDesc.pData = NULL;

	for (uint i = 0; i < gImageCount; i++)
	{
		renderSkybDesc.ppBuffer = &pRenderSkyUniformBuffer[i];
		addResource(&renderSkybDesc);
	}

	BufferLoadDesc spaceUniformDesc = {};
	spaceUniformDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	spaceUniformDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	spaceUniformDesc.mDesc.mSize = sizeof(SpaceUniformBuffer);
	spaceUniformDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT | BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
	spaceUniformDesc.pData = NULL;

	for (uint i = 0; i < gImageCount; i++)
	{
		spaceUniformDesc.ppBuffer = &pSpaceUniformBuffer[i];
		addResource(&spaceUniformDesc);
	}

	BufferLoadDesc sunUniformDesc = {};
	sunUniformDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	sunUniformDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	sunUniformDesc.mDesc.mSize = sizeof(SunUniformBuffer);
	sunUniformDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT | BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
	sunUniformDesc.pData = NULL;

	for (uint i = 0; i < gImageCount; i++)
	{
		sunUniformDesc.ppBuffer = &pSunUniformBuffer[i];
		addResource(&sunUniformDesc);

		sunUniformDesc.ppBuffer = &pStarUniformBuffer[i];
		addResource(&sunUniformDesc);
	}

	// Need to free memory;
	conf_free(pSpherePoints);

	///////////////////////////////////////////////////////////////////
	// UI
	///////////////////////////////////////////////////////////////////

	GuiDesc guiDesc = {};
	guiDesc.mStartPosition = vec2(1280.0f / getDpiScale().getX(), 700.0f / getDpiScale().getY());
	guiDesc.mStartSize = vec2(300.0f / getDpiScale().getX(), 250.0f / getDpiScale().getY());
	pGuiWindow = pGAppUI->AddGuiComponent("Sky", &guiDesc);

	gSkySettings.SkyInfo.x = 0.12f;
	gSkySettings.SkyInfo.y = 3.0f;
	gSkySettings.SkyInfo.z = 1.0f;
	gSkySettings.SkyInfo.w = 0.322f;

	gSkySettings.OriginLocation.x = -167.0f;
	gSkySettings.OriginLocation.y = -532.0f;
	gSkySettings.OriginLocation.z = 788.0f;
	gSkySettings.OriginLocation.w = 1.0f;

	CollapsingHeaderWidget CollapsingPAS("Precomputed Atmosphere Scattering");

	CollapsingPAS.AddSubWidget(SliderFloatWidget("Exposure", &gSkySettings.SkyInfo.x, float(0.0f), float(1.0f), float(0.001f)));
	CollapsingPAS.AddSubWidget(SliderFloatWidget("InscatterIntensity", &gSkySettings.SkyInfo.y, float(0.0f), float(3.0f), float(0.001f)));
	CollapsingPAS.AddSubWidget(SliderFloatWidget("InscatterContrast", &gSkySettings.SkyInfo.z, float(0.0f), float(2.0f), float(0.001f)));
	CollapsingPAS.AddSubWidget(SliderFloatWidget("UnitsToM", &gSkySettings.SkyInfo.w, float(0.0f), float(2.0f), float(0.001f)));

	pGuiWindow->AddWidget(CollapsingPAS);

	CollapsingHeaderWidget CollapsingNebula("Nebula");

	CollapsingNebula.AddSubWidget(SliderFloatWidget("Nebula Scale", &NebulaScale, float(0.0f), float(20.0f), float(0.01f)));
	CollapsingNebula.AddSubWidget(ColorPickerWidget("Nebula High Color", &NebulaHighColor));
	CollapsingNebula.AddSubWidget(ColorPickerWidget("Nebula Mid Color", &NebulaMidColor));
	CollapsingNebula.AddSubWidget(ColorPickerWidget("Nebula Low Color", &NebulaLowColor));

	pGuiWindow->AddWidget(CollapsingNebula);

	return true;
}

void Sky::Exit()
{
	removeBlendState(pBlendStateSpace);
	removeBlendState(pBlendStateSun);
	removeBlendState(pBlendStateStar);

	removeShader(pRenderer, pPAS_Shader);
	removeShader(pRenderer, pSpaceShader);
	removeShader(pRenderer, pMilkyWayShader);
	removeShader(pRenderer, pAuroraShader);
	removeShader(pRenderer, pAuroraComputeShader);
	removeShader(pRenderer, pSunShader);
	removeShader(pRenderer, pStarShader);


	removeRootSignature(pRenderer, pSkyRootSignature);
	removeRootSignature(pRenderer, pSkyRootSignatureCompute);
	removeDescriptorSet(pRenderer, pSkyDescriptorSet[0]);
	removeDescriptorSet(pRenderer, pSkyDescriptorSet[1]);


	removeSampler(pRenderer, pLinearClampSampler);
	removeSampler(pRenderer, pLinearBorderSampler);

	removeResource(pSphereVertexBuffer);
	removeResource(pSphereIndexBuffer);

	removeResource(pAuroraVertexBuffer);

	/*
  for (uint i = 0; i < gImageCount; i++)
  {
	removeResource(pAuroraParticle[i]);
	removeResource(pAuroraConstraint[i]);
	removeResource(pAuroraUniformBuffer[i]);
  }
  */

	removeResource(pAuroraParticle);
	removeResource(pAuroraConstraint);
	removeResource(pAuroraUniformBuffer);

#if USING_MILKYWAY
	removeResource(pMilkyWayVertexBuffer);
	removeResource(pMilkyWayIndexBuffer);
#endif

	removeResource(gParticleSystem.pParticleVertexBuffer);
	removeResource(gParticleSystem.pParticleInstanceBuffer);

	for (uint i = 0; i < gImageCount; i++)
	{
		removeResource(pRenderSkyUniformBuffer[i]);
		removeResource(pSpaceUniformBuffer[i]);
		removeResource(pSunUniformBuffer[i]);
		removeResource(pStarUniformBuffer[i]);
	}

	removeResource(pTransmittanceTexture);
	removeResource(pIrradianceTexture);
	removeResource(pInscatterTexture);
	removeResource(pMoonTexture);

	removeResource(pGlobalTriangularVertexBuffer);
	removeRasterizerState(pRasterizerForSky);
}

bool Sky::Load(RenderTarget** rts, uint32_t count)
{
	pPreStageRenderTarget = rts[0];

	mWidth = pPreStageRenderTarget->mDesc.mWidth;
	mHeight = pPreStageRenderTarget->mDesc.mHeight;

	float aspect = (float)mWidth / (float)mHeight;
	float aspectInverse = 1.0f / aspect;
	float horizontal_fov = PI / 3.0f;
	//float vertical_fov = 2.0f * atan(tan(horizontal_fov*0.5f) * aspectInverse);

	SkyProjectionMatrix = mat4::perspective(horizontal_fov, aspectInverse, SKY_NEAR, SKY_FAR);

	//layout and pipeline for ScreenQuad
	VertexLayout vertexLayout = {};
	vertexLayout.mAttribCount = 2;
	vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
	vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
	vertexLayout.mAttribs[0].mBinding = 0;
	vertexLayout.mAttribs[0].mLocation = 0;
	vertexLayout.mAttribs[0].mOffset = 0;

	vertexLayout.mAttribs[1].mSemantic = SEMANTIC_TEXCOORD0;
	vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32_SFLOAT;
	vertexLayout.mAttribs[1].mBinding = 0;
	vertexLayout.mAttribs[1].mLocation = 1;
	vertexLayout.mAttribs[1].mOffset = 3 * sizeof(float);

	RenderTargetDesc SkyRenderTarget = {};
	SkyRenderTarget.mArraySize = 1;
	SkyRenderTarget.mDepth = 1;
	SkyRenderTarget.mFormat = TinyImageFormat_B10G11R11_UFLOAT;
	SkyRenderTarget.mSampleCount = SAMPLE_COUNT_1;
	SkyRenderTarget.mSampleQuality = 0;
	//SkyRenderTarget.mSrgb = false;
	SkyRenderTarget.mWidth = pPreStageRenderTarget->mDesc.mWidth;
	SkyRenderTarget.mHeight = pPreStageRenderTarget->mDesc.mHeight;
	SkyRenderTarget.pDebugName = L"Sky RenderTarget";
	SkyRenderTarget.mFlags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT;
	addRenderTarget(pRenderer, &SkyRenderTarget, &pSkyRenderTarget);

#if defined(VULKAN)
	TransitionRenderTargets(pSkyRenderTarget, ResourceState::RESOURCE_STATE_RENDER_TARGET, pRenderer, ppTransCmds[0], pGraphicsQueue, pTransitionCompleteFences);
#endif

	PipelineDesc pipelineDescPAS;
	{
		pipelineDescPAS.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescPAS.mGraphicsDesc;

		pipelineSettings = { 0 };

		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;

		pipelineSettings.pColorFormats = &pSkyRenderTarget->mDesc.mFormat;
		//pipelineSettings.pSrgbValues = &pSkyRenderTarget->mDesc.mSrgb;
		pipelineSettings.mSampleCount = pSkyRenderTarget->mDesc.mSampleCount;
		pipelineSettings.mSampleQuality = pSkyRenderTarget->mDesc.mSampleQuality;

		pipelineSettings.pRootSignature = pSkyRootSignature;
		pipelineSettings.pShaderProgram = pPAS_Shader;
		pipelineSettings.pVertexLayout = &vertexLayout;
		pipelineSettings.pRasterizerState = pRasterizerForSky;

		addPipeline(pRenderer, &pipelineDescPAS, &pPAS_Pipeline);
	}

	vertexLayout = {};
	vertexLayout.mAttribCount = 3;
	vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
	vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
	vertexLayout.mAttribs[0].mBinding = 0;
	vertexLayout.mAttribs[0].mLocation = 0;
	vertexLayout.mAttribs[0].mOffset = 0;
	vertexLayout.mAttribs[1].mSemantic = SEMANTIC_NORMAL;
	vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
	vertexLayout.mAttribs[1].mBinding = 0;
	vertexLayout.mAttribs[1].mLocation = 1;
	vertexLayout.mAttribs[1].mOffset = 3 * sizeof(float);
	vertexLayout.mAttribs[2].mSemantic = SEMANTIC_TEXCOORD0;
	vertexLayout.mAttribs[2].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
	vertexLayout.mAttribs[2].mBinding = 0;
	vertexLayout.mAttribs[2].mLocation = 2;
	vertexLayout.mAttribs[2].mOffset = 6 * sizeof(float);

	PipelineDesc pipelineDescSpace;
	{
		pipelineDescSpace.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescSpace.mGraphicsDesc;

		pipelineSettings = { 0 };

		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;

		pipelineSettings.pColorFormats = &pSkyRenderTarget->mDesc.mFormat;
		//pipelineSettings.pSrgbValues = &pSkyRenderTarget->mDesc.mSrgb;
		pipelineSettings.mSampleCount = pSkyRenderTarget->mDesc.mSampleCount;
		pipelineSettings.mSampleQuality = pSkyRenderTarget->mDesc.mSampleQuality;

		pipelineSettings.pRootSignature = pSkyRootSignature;
		pipelineSettings.pShaderProgram = pSpaceShader;
		pipelineSettings.pVertexLayout = &vertexLayout;
		pipelineSettings.pRasterizerState = pRasterizerForSky;
		pipelineSettings.pBlendState = pBlendStateSpace;

		addPipeline(pRenderer, &pipelineDescSpace, &pSpacePipeline);
	}


	PipelineDesc pipelineDescMilkyWay;
	{
		pipelineDescMilkyWay.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescMilkyWay.mGraphicsDesc;

		pipelineSettings = { 0 };

		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;

		pipelineSettings.pColorFormats = &pSkyRenderTarget->mDesc.mFormat;
		//pipelineSettings.pSrgbValues = &pSkyRenderTarget->mDesc.mSrgb;
		pipelineSettings.mSampleCount = pSkyRenderTarget->mDesc.mSampleCount;
		pipelineSettings.mSampleQuality = pSkyRenderTarget->mDesc.mSampleQuality;

		pipelineSettings.pRootSignature = pSkyRootSignature;
		pipelineSettings.pShaderProgram = pMilkyWayShader;
		pipelineSettings.pVertexLayout = &vertexLayout;
		pipelineSettings.pRasterizerState = pRasterizerForSky;
		pipelineSettings.pBlendState = pBlendStateSpace;

		addPipeline(pRenderer, &pipelineDescMilkyWay, &pMilkyWayPipeline);
	}

	PipelineDesc pipelineDescAurora;
	{
		pipelineDescAurora.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescAurora.mGraphicsDesc;

		pipelineSettings = { 0 };

		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_POINT_LIST;
		pipelineSettings.mRenderTargetCount = 1;

		pipelineSettings.pColorFormats = &pSkyRenderTarget->mDesc.mFormat;
		//pipelineSettings.pSrgbValues      = &pSkyRenderTarget->mDesc.mSrgb;
		pipelineSettings.mSampleCount = pSkyRenderTarget->mDesc.mSampleCount;
		pipelineSettings.mSampleQuality = pSkyRenderTarget->mDesc.mSampleQuality;

		pipelineSettings.pRootSignature = pSkyRootSignature;
		pipelineSettings.pShaderProgram = pAuroraShader;
		pipelineSettings.pVertexLayout = &vertexLayout;
		pipelineSettings.pRasterizerState = pRasterizerForSky;
		pipelineSettings.pBlendState = pBlendStateSun;

		addPipeline(pRenderer, &pipelineDescAurora, &pAuroraPipeline);
	}

	PipelineDesc pipelineDescAuroraCompute;
	{
		pipelineDescAuroraCompute.mType = PIPELINE_TYPE_COMPUTE;
		ComputePipelineDesc &pipelineSettings = pipelineDescAuroraCompute.mComputeDesc;

		pipelineSettings = { 0 };

		pipelineSettings.pRootSignature = pSkyRootSignatureCompute;
		pipelineSettings.pShaderProgram = pAuroraComputeShader;

		addPipeline(pRenderer, &pipelineDescAuroraCompute, &pAuroraComputePipeline);
	}


	PipelineDesc pipelineDesSun;
	{
		pipelineDesSun.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDesSun.mGraphicsDesc;

		pipelineSettings = { 0 };

#if !defined(METAL)
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_POINT_LIST;
#else
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_STRIP;
#endif
		pipelineSettings.mRenderTargetCount = 1;

		pipelineSettings.pColorFormats = &pSkyRenderTarget->mDesc.mFormat;
		//pipelineSettings.pSrgbValues = &pSkyRenderTarget->mDesc.mSrgb;
		pipelineSettings.mSampleCount = pSkyRenderTarget->mDesc.mSampleCount;
		pipelineSettings.mSampleQuality = pSkyRenderTarget->mDesc.mSampleQuality;

		pipelineSettings.pRootSignature = pSkyRootSignature;
		pipelineSettings.pShaderProgram = pSunShader;
		pipelineSettings.pVertexLayout = &vertexLayout;
		pipelineSettings.pRasterizerState = pRasterizerForSky;
		pipelineSettings.pBlendState = pBlendStateSun;

		addPipeline(pRenderer, &pipelineDesSun, &pSunPipeline);
	}

	vertexLayout = {};
	vertexLayout.mAttribCount = 5;
	vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
	vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
	vertexLayout.mAttribs[0].mBinding = 0;
	vertexLayout.mAttribs[0].mLocation = 0;
	vertexLayout.mAttribs[0].mOffset = 0;

	vertexLayout.mAttribs[1].mSemantic = SEMANTIC_TEXCOORD0;
	vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32_SFLOAT;
	vertexLayout.mAttribs[1].mBinding = 0;
	vertexLayout.mAttribs[1].mLocation = 1;
	vertexLayout.mAttribs[1].mOffset = 4 * sizeof(float);

	vertexLayout.mAttribs[2].mSemantic = SEMANTIC_TEXCOORD1;
	vertexLayout.mAttribs[2].mFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
	vertexLayout.mAttribs[2].mBinding = 1;
	vertexLayout.mAttribs[2].mLocation = 2;
	vertexLayout.mAttribs[2].mOffset = 0;
	vertexLayout.mAttribs[2].mRate = VERTEX_ATTRIB_RATE_INSTANCE;

	vertexLayout.mAttribs[3].mSemantic = SEMANTIC_TEXCOORD2;
	vertexLayout.mAttribs[3].mFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
	vertexLayout.mAttribs[3].mBinding = 1;
	vertexLayout.mAttribs[3].mLocation = 3;
	vertexLayout.mAttribs[3].mOffset = 4 * sizeof(float);
	vertexLayout.mAttribs[3].mRate = VERTEX_ATTRIB_RATE_INSTANCE;

	vertexLayout.mAttribs[4].mSemantic = SEMANTIC_TEXCOORD3;
	vertexLayout.mAttribs[4].mFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
	vertexLayout.mAttribs[4].mBinding = 1;
	vertexLayout.mAttribs[4].mLocation = 4;
	vertexLayout.mAttribs[4].mOffset = 8 * sizeof(float);
	vertexLayout.mAttribs[4].mRate = VERTEX_ATTRIB_RATE_INSTANCE;

	PipelineDesc pipelineDescStar;
	{
		pipelineDescStar.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescStar.mGraphicsDesc;

		pipelineSettings = { 0 };

#if !defined(METAL)
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_POINT_LIST;
#else
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_STRIP;
#endif
		pipelineSettings.mRenderTargetCount = 1;

		pipelineSettings.pColorFormats = &pSkyRenderTarget->mDesc.mFormat;
		//pipelineSettings.pSrgbValues = &pSkyRenderTarget->mDesc.mSrgb;
		pipelineSettings.mSampleCount = pSkyRenderTarget->mDesc.mSampleCount;
		pipelineSettings.mSampleQuality = pSkyRenderTarget->mDesc.mSampleQuality;

		pipelineSettings.pRootSignature = pSkyRootSignature;
		pipelineSettings.pShaderProgram = pStarShader;
		pipelineSettings.pVertexLayout = &vertexLayout;
		pipelineSettings.pRasterizerState = pRasterizerForSky;
		pipelineSettings.pBlendState = pBlendStateStar;

		addPipeline(pRenderer, &pipelineDescStar, &pStarPipeline);
	}

	// Prepare descriptor sets
	// Sky
	{
		DescriptorData ScParams[7] = {};
		ScParams[0].pName = "SceneColorTexture";
		ScParams[0].ppTextures = &pPreStageRenderTarget->pTexture;
		ScParams[1].pName = "Depth";
		ScParams[1].ppTextures = &pDepthBuffer->pTexture;
		ScParams[2].pName = "TransmittanceTexture";
		ScParams[2].ppTextures = &pTransmittanceTexture;
		ScParams[3].pName = "InscatterTexture";
		ScParams[3].ppTextures = &pInscatterTexture;
		ScParams[4].pName = "TransmittanceColor";
		ScParams[4].ppBuffers = &pTransmittanceBuffer;
		ScParams[5].pName = "depthTexture";
		ScParams[5].ppTextures = &pDepthBuffer->pTexture;
		ScParams[6].pName = "moonAtlas";
		ScParams[6].ppTextures = &pMoonTexture;
		updateDescriptorSet(pRenderer, 0, pSkyDescriptorSet[0], 7, ScParams);

		for (uint32_t i = 0; i < gImageCount; ++i)
		{
			ScParams[0].pName = "RenderSkyUniformBuffer";
			ScParams[0].ppBuffers = &pRenderSkyUniformBuffer[i];
			ScParams[1].pName = "SpaceUniform";
			ScParams[1].ppBuffers = &pSpaceUniformBuffer[i];
			ScParams[2].pName = "StarUniform";
			ScParams[2].ppBuffers = &pStarUniformBuffer[i];
			ScParams[3].pName = "SunUniform";
			ScParams[3].ppBuffers = &pSunUniformBuffer[i];
			updateDescriptorSet(pRenderer, i, pSkyDescriptorSet[1], 4, ScParams);
		}
	}


	return false;
}

void Sky::Unload()
{
	removeRenderTarget(pRenderer, pSkyRenderTarget);
	removePipeline(pRenderer, pPAS_Pipeline);
	removePipeline(pRenderer, pSpacePipeline);
	removePipeline(pRenderer, pMilkyWayPipeline);
	removePipeline(pRenderer, pAuroraPipeline);
	removePipeline(pRenderer, pAuroraComputePipeline);
	removePipeline(pRenderer, pSunPipeline);
	removePipeline(pRenderer, pStarPipeline);
}

void Sky::Update(float deltaTime)
{
	g_ElapsedTime += deltaTime;

	/*
  gAurora.addForce(normalize(vec3(1.0f, 0.0f, 1.0f)));
  gAurora.update(deltaTime);

  eastl::vector<float3> tempPos;

  for(uint i = 0; i < (uint)gAurora.particles.size(); i++)
  {
	tempPos.push_back( v3ToF3(gAurora.particles[i].position) );
  }

  BufferUpdateDesc BufferAuroraInfoSettingDesc = { pAuroraInfoBuffer[gFrameIndex], tempPos.data() };
  updateResource(&BufferAuroraInfoSettingDesc);
  */

	gAuroraUniformStruct.maxVertex = (uint32_t)AuroraParticleNum;
	gAuroraUniformStruct.heightOffset = 4000.0f;
	gAuroraUniformStruct.height = 4000.0f;
	gAuroraUniformStruct.deltaTime = deltaTime;
	gAuroraUniformStruct.ViewProjMat = SkyProjectionMatrix * pCameraController->getViewMatrix();

	//BufferUpdateDesc BufferAuroraUniformSettingDesc = { pAuroraUniformBuffer[gFrameIndex], &gAuroraUniformStruct };
	BufferUpdateDesc BufferAuroraUniformSettingDesc = { pAuroraUniformBuffer, &gAuroraUniformStruct };
	updateResource(&BufferAuroraUniformSettingDesc);


}


void Sky::Draw(Cmd* cmd)
{
	cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Sky", true);

	vec4 lightDir = vec4(f3Tov3(LightDirection));

	//if (lightDir.getY() >= 0.0f)
	{
		cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Atmospheric Scattering", true);

		TextureBarrier barriersSky[] = {
				{ pSkyRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pPreStageRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pTransmittanceTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pIrradianceTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pInscatterTexture, RESOURCE_STATE_SHADER_RESOURCE }
		};

		cmdResourceBarrier(cmd, 0, NULL, 5, barriersSky);

		LoadActionsDesc loadActions = {};
		loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
		loadActions.mClearColorValues[0].r = 0.0f;
		loadActions.mClearColorValues[0].g = 0.0f;
		loadActions.mClearColorValues[0].b = 0.0f;
		loadActions.mClearColorValues[0].a = 0.0f;

		cmdBindRenderTargets(cmd, 1, &pSkyRenderTarget, NULL, &loadActions, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pSkyRenderTarget->mDesc.mWidth, (float)pSkyRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pSkyRenderTarget->mDesc.mWidth, pSkyRenderTarget->mDesc.mHeight);

		cmdBindPipeline(cmd, pPAS_Pipeline);
		cmdBindDescriptorSet(cmd, 0, pSkyDescriptorSet[0]);
		cmdBindDescriptorSet(cmd, gFrameIndex, pSkyDescriptorSet[1]);

		RenderSkyUniformBuffer _cbRootConstantStruct;

		_cbRootConstantStruct.InvViewMat = inverse(pCameraController->getViewMatrix());
		_cbRootConstantStruct.InvProjMat = inverse(SkyProjectionMatrix);
		_cbRootConstantStruct.LightDirection = float4(lightDir.getX(), lightDir.getY(), lightDir.getZ(), gSkySettings.SkyInfo.x);

		///////////////////////////////////////////////////////////////////////////////////////////////

		//	Update params
		float fUnitsToKM = gSkySettings.SkyInfo.w * 0.001f;

		float4	offsetScaleToLocalKM = -gSkySettings.OriginLocation * fUnitsToKM;
		offsetScaleToLocalKM.setW(fUnitsToKM);

		offsetScaleToLocalKM.y += Rg + 0.001f;

		float2	inscatterParams = float2(gSkySettings.SkyInfo.y * (1.0f - gSkySettings.SkyInfo.z), gSkySettings.SkyInfo.y * gSkySettings.SkyInfo.z);
		float3	localCamPosKM = (v3ToF3(pCameraController->getViewPosition()) * offsetScaleToLocalKM.w) + offsetScaleToLocalKM.getXYZ();

		float	Q = (float)(SKY_FAR / (SKY_FAR - SKY_NEAR));
		float4	QNNear = float4(Q, SKY_NEAR * fUnitsToKM, SKY_NEAR, SKY_FAR);

		//////////////////////////////////////////////////////////////////////////////////////////////

		_cbRootConstantStruct.CameraPosition = float4(localCamPosKM.getX(), localCamPosKM.getY(), localCamPosKM.getZ(), 1.0f);
		_cbRootConstantStruct.QNNear = QNNear;
		_cbRootConstantStruct.InScatterParams = float4(inscatterParams.x, inscatterParams.y, 0.0f, 0.0f);
		_cbRootConstantStruct.LightIntensity = LightColorAndIntensity;

		BufferUpdateDesc BufferUniformSettingDesc = { pRenderSkyUniformBuffer[gFrameIndex], &_cbRootConstantStruct };
		updateResource(&BufferUniformSettingDesc);

		cmdBindVertexBuffer(cmd, 1, &pGlobalTriangularVertexBuffer, NULL);
		cmdDraw(cmd, 3, 0);

		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

		cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
	}

	/////////////////////////////////////////////////////////////////////////////////////


	mat4 rotMat = mat4::rotationY(-Azimuth) * mat4::rotationZ(Elevation);

	if (lightDir.getY() < 0.2f)
	{
		cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Draw Night Sky", true);

		cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Draw Space", true);

		TextureBarrier barriersSky[] =
		{
			{ pSkyRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET }
		};

		cmdResourceBarrier(cmd, 0, NULL, 1, barriersSky);

		cmdBindRenderTargets(cmd, 1, &pSkyRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pSkyRenderTarget->mDesc.mWidth, (float)pSkyRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pSkyRenderTarget->mDesc.mWidth, pSkyRenderTarget->mDesc.mHeight);

		struct Data
		{
			mat4    viewProjMat;
			float4  LightDirection;
			float4  ScreenSize;
			float4  NebulaHighColor;
			float4  NebulaMidColor;
			float4  NebulaLowColor;
		} data;

		vec4 customColor;

		uint32_t red = (NebulaHighColor & 0xFF000000) >> 24;
		uint32_t green = (NebulaHighColor & 0x00FF0000) >> 16;
		uint32_t blue = (NebulaHighColor & 0x0000FF00) >> 8;
		uint32_t alpha = (NebulaHighColor & 0x000000FF);

		data.NebulaHighColor = float4((float)red / 255.0f, (float)green / 255.0f, (float)blue / 255.0f, (float)alpha / 255.0f);

		red = (NebulaMidColor & 0xFF000000) >> 24;
		green = (NebulaMidColor & 0x00FF0000) >> 16;
		blue = (NebulaMidColor & 0x0000FF00) >> 8;
		alpha = (NebulaMidColor & 0x000000FF);

		data.NebulaMidColor = float4((float)red / 255.0f, (float)green / 255.0f, (float)blue / 255.0f, (float)alpha / 255.0f);

		red = (NebulaLowColor & 0xFF000000) >> 24;
		green = (NebulaLowColor & 0x00FF0000) >> 16;
		blue = (NebulaLowColor & 0x0000FF00) >> 8;
		alpha = (NebulaLowColor & 0x000000FF);

		data.NebulaLowColor = float4((float)red / 255.0f, (float)green / 255.0f, (float)blue / 255.0f, (float)alpha / 255.0f);


		data.viewProjMat = SkyProjectionMatrix * pCameraController->getViewMatrix() * rotMat;
		data.LightDirection = float4(LightDirection, 0.0f);
		data.ScreenSize = float4((float)pSkyRenderTarget->mDesc.mWidth, (float)pSkyRenderTarget->mDesc.mHeight, 0.0f, 0.0f);

		BufferUpdateDesc BufferUniformSettingDesc = { pSpaceUniformBuffer[gFrameIndex], &data };
		updateResource(&BufferUniformSettingDesc);

		DescriptorData SpaceParams[2] = {};

		cmdBindPipeline(cmd, pSpacePipeline);
		cmdBindDescriptorSet(cmd, 0, pSkyDescriptorSet[0]);
		cmdBindDescriptorSet(cmd, gFrameIndex, pSkyDescriptorSet[1]);

		cmdBindVertexBuffer(cmd, 1, &pSphereVertexBuffer, NULL);
		cmdBindIndexBuffer(cmd, pSphereIndexBuffer, 0);
		cmdDrawIndexed(cmd, (uint32_t)sphere.triangleIndices.size() * 3, 0, 0);

		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

		cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);

#if USING_MILKYWAY
		{
			cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Draw MilkyWay", true);

			TextureBarrier barriersSky[] =
			{
			  { pSkyRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET }
			};

			cmdResourceBarrier(cmd, 0, NULL, 1, barriersSky, false);

			cmdBindRenderTargets(cmd, 1, &pSkyRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
			cmdSetViewport(cmd, 0.0f, 0.0f, (float)pSkyRenderTarget->mDesc.mWidth, (float)pSkyRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pSkyRenderTarget->mDesc.mWidth, pSkyRenderTarget->mDesc.mHeight);

			DescriptorData MilkyWayParams[2] = {};

			cmdBindPipeline(cmd, pMilkyWayPipeline);

			MilkyWayParams[0].pName = "SpaceUniform";
			MilkyWayParams[0].ppBuffers = &pSpaceUniformBuffer[gFrameIndex];
			MilkyWayParams[1].pName = "depthTexture";
			MilkyWayParams[1].ppTextures = &pDepthBuffer->pTexture;

			cmdBindDescriptors(cmd, pSkyDescriptorBinder, pSkyRootSignature, 2, MilkyWayParams);

			cmdBindVertexBuffer(cmd, 1, &pMilkyWayVertexBuffer, NULL);
			cmdBindIndexBuffer(cmd, pMilkyWayIndexBuffer, 0);
			cmdDrawIndexed(cmd, (uint32_t)MilkyWayIndexCount, 0, 0);

			cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

			cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
		}
#endif


		cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Draw Starfield", true);

		TextureBarrier barriersSky2[] =
		{
		  { pSkyRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET }
		};

		cmdResourceBarrier(cmd, 0, NULL, 1, barriersSky2);

		cmdBindRenderTargets(cmd, 1, &pSkyRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pSkyRenderTarget->mDesc.mWidth, (float)pSkyRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pSkyRenderTarget->mDesc.mWidth, pSkyRenderTarget->mDesc.mHeight);

		struct StarData
		{
			mat4 RotMat;
			mat4 ViewProjMat;
			float4 LightDirection;
			float4 Dx;
			float4 Dy;
		} starData;

		starData.RotMat = rotMat;
		starData.ViewProjMat = SkyProjectionMatrix * pCameraController->getViewMatrix();
		starData.LightDirection = float4(LightDirection, g_ElapsedTime * 2.0f);

#if !defined(METAL)
		starData.Dx = v4ToF4(pCameraController->getViewMatrix().getRow(0));
		starData.Dy = v4ToF4(pCameraController->getViewMatrix().getRow(1));
#else
		starData.Dx = v4ToF4(pCameraController->getViewMatrix().getRow(0) * 100000.0f);
		starData.Dy = v4ToF4(pCameraController->getViewMatrix().getRow(1) * 100000.0f);
#endif



		BufferUpdateDesc BufferUniformSettingDesc2 = { pStarUniformBuffer[gFrameIndex], &starData };
		updateResource(&BufferUniformSettingDesc2);

		DescriptorData StarParams[3] = {};

		cmdBindPipeline(cmd, pStarPipeline);
		cmdBindDescriptorSet(cmd, 0, pSkyDescriptorSet[0]);
		cmdBindDescriptorSet(cmd, gFrameIndex, pSkyDescriptorSet[1]);

		Buffer* particleVertexBuffers[] = { gParticleSystem.pParticleVertexBuffer, gParticleSystem.pParticleInstanceBuffer };
		cmdBindVertexBuffer(cmd, 2, particleVertexBuffers, NULL);
#if !defined(METAL)
		cmdDrawInstanced(cmd, 1, 0, (uint32_t)gParticleSystem.particleDataSet.ParticleDataArray.size(), 0);
#else
		cmdDrawInstanced(cmd, 4, 0, (uint32_t)gParticleSystem.particleDataSet.ParticleDataArray.size(), 0);
#endif


		cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
		cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
	}



	////////////////////////////////////////////////////////////////////////
	{
		cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Draw Sun and Moon", true);

		TextureBarrier barriersSky[] =
		{
			{ pSkyRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET }
		};

		cmdResourceBarrier(cmd, 0, NULL, 1, barriersSky);

		cmdBindRenderTargets(cmd, 1, &pSkyRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pSkyRenderTarget->mDesc.mWidth, (float)pSkyRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pSkyRenderTarget->mDesc.mWidth, pSkyRenderTarget->mDesc.mHeight);

		struct Data
		{
			mat4 ViewMat;
			mat4 ViewProjMat;
			float4 LightDirection;
			float4 Dx;
			float4 Dy;
		} data;

		data.ViewMat = pCameraController->getViewMatrix();
		data.ViewProjMat = SkyProjectionMatrix * pCameraController->getViewMatrix();
		data.LightDirection = float4(LightDirection * 1000000.0f, 0.0f);
		data.Dx = v4ToF4(pCameraController->getViewMatrix().getRow(0) * SunSize);
		data.Dy = v4ToF4(pCameraController->getViewMatrix().getRow(1) * SunSize);

		BufferUpdateDesc BufferUniformSettingDesc = { pSunUniformBuffer[gFrameIndex], &data };
		updateResource(&BufferUniformSettingDesc);

		cmdBindPipeline(cmd, pSunPipeline);
		cmdBindDescriptorSet(cmd, 0, pSkyDescriptorSet[0]);
		cmdBindDescriptorSet(cmd, gFrameIndex, pSkyDescriptorSet[1]);

#if !defined(METAL)
		cmdDraw(cmd, 1, 0);
#else
		cmdBindVertexBuffer(cmd, 1, &pGlobalTriangularVertexBuffer, NULL);
		cmdDraw(cmd, 4, 0);
#endif
		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

		TextureBarrier barriersSkyEnd[] = {
					{ pSkyRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
		};

		cmdResourceBarrier(cmd, 0, NULL, 1, barriersSkyEnd);

		cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
	}




	////////////////////////////////////////////////////////////////////////
  /*
  #if !defined(METAL)
	{
	  TextureBarrier barriersSky[] =
	  {
		{ pSkyRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET }
	  };

	  BufferBarrier barrierAurora[] =
	  {
		{ pAuroraParticle, RESOURCE_STATE_UNORDERED_ACCESS },
		{ pAuroraConstraint, RESOURCE_STATE_UNORDERED_ACCESS },
		{ pAuroraUniformBuffer, RESOURCE_STATE_SHADER_RESOURCE }
	  };

	  cmdResourceBarrier(cmd, 2, barrierAurora, 1, barriersSky, false);

	  cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Update Aurora", true);

	  cmdBindPipeline(cmd, pAuroraComputePipeline);

	  DescriptorData AuroraComputeParams[3] = {};
	  AuroraComputeParams[0].pName = "AuroraParticleBuffer";
	  AuroraComputeParams[0].ppBuffers = &pAuroraParticle;

	  AuroraComputeParams[1].pName = "AuroraConstraintBuffer";
	  AuroraComputeParams[1].ppBuffers = &pAuroraConstraint;

	  AuroraComputeParams[2].pName = "AuroraUniformBuffer";
	  AuroraComputeParams[2].ppBuffers = &pAuroraUniformBuffer;

	  cmdBindDescriptors(cmd, pSkyDescriptorBinder, pSkyRootSignatureCompute, 3, AuroraComputeParams);
	  uint32_t* pThreadGroupSize = pAuroraComputeShader->mReflection.mStageReflections[0].mNumThreadsPerGroup;
	  cmdDispatch(cmd, (uint32_t)ceil((float)AuroraParticleNum / (float)pThreadGroupSize[0]), 1, 1);

	  cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);

	  cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Draw Aurora", true);

	  cmdBindRenderTargets(cmd, 1, &pSkyRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
	  cmdSetViewport(cmd, 0.0f, 0.0f, (float)pSkyRenderTarget->mDesc.mWidth, (float)pSkyRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
	  cmdSetScissor(cmd, 0, 0, pSkyRenderTarget->mDesc.mWidth, pSkyRenderTarget->mDesc.mHeight);

	  DescriptorData AuroraParams[2] = {};

	  cmdBindPipeline(cmd, pAuroraPipeline);

	  AuroraParams[0].pName = "AuroraParticleBuffer";
	  AuroraParams[0].ppBuffers = &pAuroraParticle;
	  AuroraParams[1].pName = "AuroraUniformBuffer";
	  AuroraParams[1].ppBuffers = &pAuroraUniformBuffer;



	  cmdBindDescriptors(cmd, pSkyDescriptorBinder, pSkyRootSignature, 2, AuroraParams);

	  cmdBindVertexBuffer(cmd, 1, &pAuroraVertexBuffer, NULL);
	  cmdDraw(cmd, (uint32_t)AuroraParticleNum, 0);
	  cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

	  cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
	}
  #else
	{
	  cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Draw Aurora", true);

	  cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
	}
  #endif
  */

	cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
}

void Sky::Initialize(uint InImageCount,
	ICameraController* InCameraController, Queue*	InGraphicsQueue,
	Cmd** InTransCmds, Fence* InTransitionCompleteFences, GpuProfiler*	InGraphicsGpuProfiler, UIApp* InGAppUI, Buffer*	InTransmittanceBuffer)
{
	gImageCount = InImageCount;

	pCameraController = InCameraController;
	pGraphicsQueue = InGraphicsQueue;
	ppTransCmds = InTransCmds;
	pTransitionCompleteFences = InTransitionCompleteFences;
	pGraphicsGpuProfiler = InGraphicsGpuProfiler;
	pGAppUI = InGAppUI;
	pTransmittanceBuffer = InTransmittanceBuffer;
}

void Sky::InitializeWithLoad(RenderTarget* InDepthRenderTarget, RenderTarget* InLinearDepthRenderTarget)
{
	pDepthBuffer = InDepthRenderTarget;
	pLinearDepthBuffer = InLinearDepthRenderTarget;
}

static const float AVERAGE_GROUND_REFLECTANCE = 0.1f;

// Rayleigh
static const float HR = 8.0f;
static const vec3 betaR = vec3(5.8e-3f, 1.35e-2f, 3.31e-2f);
/*
// Mie
// DEFAULT
static const float HM = 1.2;
static const float3 betaMSca = (4e-3).xxx;
static const float3 betaMEx = betaMSca / 0.9;
static const float mieG = 0.8;
*/

// CLEAR SKY
static const float HM = 1.2f;
static const vec3 betaMSca = vec3(20e-3f, 20e-3f, 20e-3f);
static const vec3 betaMEx = betaMSca / 0.9f;
static const float mieG = 0.76f;

// nearest intersection of ray r,mu with ground or top atmosphere boundary
// mu=cos(ray zenith angle at ray origin)
float limit(float r, float mu) {
	float dout = -r * mu + sqrt(r * r * (mu * mu - 1.0f) + RL * RL);
	// 	float delta2 = r * r * (mu * mu - 1.0) + Rg * Rg;
	// 	if (delta2 >= 0.0) {
	// 		float din = -r * mu - sqrt(delta2);
	// 		if (din >= 0.0) {
	// 			dout = min(dout, din);
	// 		}
	// 	}
	return dout;
}

// optical depth for ray (r,mu) of length d, using analytic formula
// (mu=cos(view zenith angle)), intersections with ground ignored
// H=height scale of exponential density function
float opticalDepth(float H, float r, float mu, float d) {

	float a = sqrt((0.5f / H)*r);
	vec2 a01 = a * vec2(mu, mu + d / r);
	vec2 a01s = vec2(sign(a01.getX()), sign(a01.getY()));
	vec2 a01sq = vec2(a01.getX() * a01.getX(), a01.getY() * a01.getY());
	float x = a01s.getY() > a01s.getX() ? exp(a01sq.getX()) : 0.0f;
	vec2 y = a01s;
	vec2 denom = (2.3193f*vec2(fabs(a01.getX()), fabs(a01.getY())) + vec2(sqrt(1.52f*a01sq.getX() + 4.0f), sqrt(1.52f*a01sq.getY() + 4.0f)));

	y = vec2(y.getX() / denom.getX(), y.getY() / denom.getY());

	vec2 denom2 = vec2(1.0f, exp(-d / H * (d / (2.0f*r) + mu)));

	y = vec2(y.getX() * denom2.getX(), y.getY() * denom2.getY());

	//denom = vec2(denom.getX() * denom2.getX(), denom.getY() * denom2.getY());



	return sqrt((6.2831f*H)*r) * exp((Rg - r) / H) * (x + dot(y, vec2(1.0f, -1.0f)));
}

// transmittance(=transparency) of atmosphere for ray (r,mu) of length d
// (mu=cos(view zenith angle)), intersections with ground ignored
// uses analytic formula instead of transmittance texture
vec3 analyticTransmittance(float r, float mu, float d) {
	vec3 arg = -betaR * opticalDepth(HR, r, mu, d) - betaMEx * opticalDepth(HM, r, mu, d);
	return vec3(exp(arg.getX()), exp(arg.getY()), exp(arg.getZ()));
}

vec3 analyticTransmittance(float r, float mu) {
	return analyticTransmittance(r, mu, limit(r, mu));
}


float SmoothStep(float edge0, float edge1, float x)
{
	// Scale, bias and saturate x to 0..1 range
	x = clamp((x - edge0) / (edge1 - edge0), 0, 1);
	// Evaluate polynomial
	return x * x*(3 - 2 * x);
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu)
// (mu=cos(view zenith angle)), or zero if ray intersects ground
vec3 transmittanceWithShadowSmooth(float r, float mu) {
	//	TODO: check if it is reasonably fast
	//	TODO: check if it is mathematically correct
	//    return mu < -sqrt(1.0 - (Rg / r) * (Rg / r)) ? (0.0).xxx : transmittance(r, mu);

	float eps = 0.5f*PI / 180.0f;
	float eps1 = 0.1f*PI / 180.0f;
	float horizMu = -sqrt(1.0f - (Rg / r) * (Rg / r));
	float horizAlpha = acos(horizMu);
	float horizMuMin = cos(horizAlpha + eps);
	//float horizMuMax = cos(horizAlpha-eps);
	float horizMuMax = cos(horizAlpha + eps1);

	float t = SmoothStep(horizMuMin, horizMuMax, mu);
	vec3 analy = analyticTransmittance(r, mu);
	return lerp(t, vec3(0.0), analy);
}

float3 Sky::GetSunColor()
{
	//float4		SkyInfo; // x: fExposure, y: fInscatterIntencity, z: fInscatterContrast, w: fUnitsToM
	//float4		OriginLocation;

	const float	fUnitsToKM = gSkySettings.SkyInfo.w*0.001f;
	float4	offsetScaleToLocalKM = float4(-gSkySettings.OriginLocation.getXYZ()*fUnitsToKM, fUnitsToKM);
	offsetScaleToLocalKM.y += Rg + 0.001f;

	//float2	inscatterParams = float2(gSkySettings.SkyInfo.y*(1 - gSkySettings.SkyInfo.z), gSkySettings.SkyInfo.y*gSkySettings.SkyInfo.z);
	float3 localCamPosKM = (float3(0.0f, 0.0f, 0.0f)*offsetScaleToLocalKM.w) + offsetScaleToLocalKM.getXYZ();


	//	TODO: Igor: Simplify

	vec3 ray = f3Tov3(LightDirection);
	vec3 x = f3Tov3(localCamPosKM);
	vec3 v = normalize(ray);

	float r = length(x);
	float mu = dot(x, v) / r;

	return v3ToF3(transmittanceWithShadowSmooth(r, mu));
}
