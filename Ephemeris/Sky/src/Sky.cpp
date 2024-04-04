/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This is a part of Ephemeris.
 * This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License
 * (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge. You can not use
 * this code for commercial purposes.
 *
 */

#include "Sky.h"

#include "../../../../The-Forge/Common_3/Application/Interfaces/IUI.h"
#include "../../../../The-Forge/Common_3/Game/Interfaces/IScripting.h"
#include "../../../../The-Forge/Common_3/Resources/ResourceLoader/Interfaces/IResourceLoader.h"
#include "../../../../The-Forge/Common_3/Utilities/Interfaces/IFileSystem.h"
#include "../../../../The-Forge/Common_3/Utilities/Interfaces/ILog.h"

#include "../../src/AppSettings.h"

#include "../../../../The-Forge/Common_3/Utilities/Interfaces/IMemory.h"

extern AppSettings  gAppSettings;
extern UIComponent* pGuiSkyWindow;

int gNumberOfSpherePoints;
int gNumberOfSphereIndices;

#ifdef _DEBUG
const int gSphereResolution = 3; // Increase for higher resolution spheres
#else
const int gSphereResolution = 6; // Increase for higher resolution spheres
#endif

const float gSphereDiameter = 1.0f;

// Precomputed Atmospheric Sacttering
Shader*   pPAS_Shader = NULL;
Pipeline* pPAS_Pipeline = NULL;

Shader*   pSpaceShader = NULL;
Pipeline* pSpacePipeline = NULL;

DescriptorSet* pSkyDescriptorSet[2] = { NULL };
RootSignature* pSkyRootSignature = NULL;

Buffer* pSphereVertexBuffer = NULL;
Buffer* pSphereIndexBuffer = NULL;

Buffer* pRenderSkyUniformBuffer[Sky::gDataBufferCount] = { NULL };
Buffer* pSpaceUniformBuffer[Sky::gDataBufferCount] = { NULL };

static float g_ElapsedTime = 0.0f;

uint32_t sphereIndexCount;
float*   pSpherePoints;
// Aurora                gAurora;

static float SpaceScale = PLANET_RADIUS * 10.0f;
static float NebulaScale = 1.453f;
static float StarIntensity = 1.5f;
static float StarDensity = 10.0f;
static float StarDistribution = 20000000.0f;
static float ParticleSize = 100000.0f;

float4 NebulaHighColor = unpackR8G8B8A8_SRGB(0x412C1D78);
float4 NebulaMidColor = unpackR8G8B8A8_SRGB(0x041D22FF);
float4 NebulaLowColor = unpackR8G8B8A8_SRGB(0x040315FF);

static mat4 rotMat = mat4::identity();
static mat4 rotMatStarField = mat4::identity();

struct RenderSkyUniformBuffer
{
    mat4 InvViewMat;
    mat4 InvProjMat;

    float4 LightDirection; // w:exposure
    float4 CameraPosition;
    float4 QNNear;
    float4 InScatterParams;

    float4 LightIntensity;
};

struct SpaceUniformBuffer
{
    mat4   ViewProjMat;
    float4 LightDirection;
    float4 ScreenSize;
    float4 NebulaHighColor;
    float4 NebulaMidColor;
    float4 NebulaLowColor;
};

mat4 MakeRotationMatrix(float angle, const vec3& axis)
{
    float s, c;
    // sincos(-angle, s, c);
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

bool Sky::Load(int32_t width, int32_t height)
{
    mWidth = width;
    mHeight = height;

    float aspect = (float)mWidth / (float)mHeight;
    float aspectInverse = 1.0f / aspect;
    float horizontal_fov = PI / 3.0f;
    // float vertical_fov = 2.0f * atan(tan(horizontal_fov*0.5f) * aspectInverse);

    SkyProjectionMatrix = mat4::perspectiveLH(horizontal_fov, aspectInverse, CAMERA_NEAR, CAMERA_FAR);
    SpaceProjectionMatrix = mat4::perspectiveLH(horizontal_fov, aspectInverse, SPACE_NEAR, SPACE_FAR);

    return false;
}

void Sky::CalculateLookupData()
{
    SyncToken token = {};

    TextureLoadDesc SkyTransmittanceTextureDesc = {};
    SkyTransmittanceTextureDesc.pFileName = "Sky/Transmittance.tex";
    SkyTransmittanceTextureDesc.ppTexture = &pTransmittanceTexture;
    addResource(&SkyTransmittanceTextureDesc, &token);

    TextureLoadDesc SkyIrradianceTextureDesc = {};
    SkyIrradianceTextureDesc.pFileName = "Sky/Irradiance.tex";
    SkyIrradianceTextureDesc.ppTexture = &pIrradianceTexture;
    addResource(&SkyIrradianceTextureDesc, &token);

    TextureLoadDesc SkyInscatterTextureDesc = {};
    SkyInscatterTextureDesc.pFileName = "Sky/Inscatter.tex";
    SkyInscatterTextureDesc.ppTexture = &pInscatterTexture;
    addResource(&SkyInscatterTextureDesc, &token);

    waitForToken(&token);
}

//    <https://www.shadertoy.com/view/4dS3Wd>
//    By Morgan McGuire @morgan3d, http://graphicscodex.com
//

float hash(float n) { return fmod(sin(n) * 10000.0f, 1.0f); }

float hash(vec2 p)
{
    return fmod(10000.0f * sin(17.0f * p.getX() + p.getY() * 0.1f) * (0.1f + abs(sin(p.getY() * 13.0f + p.getX()))), 1.0f);
}
float noise(float x)
{
    float i = floor(x);
    float f = fmod(x, 1.0f);
    float u = f * f * (3.0f - 2.0f * f);
    return lerp(hash(i), hash(i + 1.0f), u);
}
float noise(vec2 x)
{
    vec2  i = vec2(floor(x.getX()), floor(x.getY()));
    vec2  f = vec2(fmod(x.getX(), 1.0f), fmod(x.getY(), 1.0f));
    float a = hash(i);
    float b = hash(i + vec2(1.0f, 0.0f));
    float c = hash(i + vec2(0.0f, 1.0f));
    float d = hash(i + vec2(1.0f, 1.0f));
    vec2  u = vec2(f.getX() * f.getX() * (3.0f - 2.0f * f.getX()), f.getY() * f.getY() * (3.0f - 2.0f * f.getY()));
    return lerp(a, b, u.getX()) + (c - a) * u.getY() * (1.0f - u.getX()) + (d - b) * u.getX() * u.getY();
}

float noise(const vec3& x)
{
    // The noise function returns a value in the range -1.0f -> 1.0f

    vec3 p = vec3(floor(x.getX()), floor(x.getY()), floor(x.getZ()));
    vec3 f = vec3(fmod(x.getX(), 1.0f), fmod(x.getY(), 1.0f), fmod(x.getZ(), 1.0f));

    // f = f * f*(3.0 - 2.0*f);
    f = vec3(f.getX() * f.getX() * (3.0f - 2.0f * f.getX()), f.getY() * f.getY() * (3.0f - 2.0f * f.getY()),
             f.getZ() * f.getZ() * (3.0f - 2.0f * f.getZ()));

    float n = p.getX() + p.getY() * 57.0f + 113.0f * p.getZ();

    return lerp(lerp(lerp(hash(n + 0.0f), hash(n + 1.0f), f.getX()), lerp(hash(n + 57.0f), hash(n + 58.0f), f.getX()), f.getY()),
                lerp(lerp(hash(n + 113.0f), hash(n + 114.0f), f.getX()), lerp(hash(n + 170.0f), hash(n + 171.0f), f.getX()), f.getY()),
                f.getZ());
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

// Generates an array of vertices and normals for a sphere
void Sky::GenerateIcosahedron(float** ppPoints, VertexStbDsArray& vertices, IndexStbDsArray& indices, int numberOfDivisions, float radius)
{
    CreateIcosphere(numberOfDivisions, &vertices, &indices);

    int numVertex = (int)arrlen(vertices);

    float3* pPoints = (float3*)tf_malloc(numVertex * (sizeof(float3) * 3));

    uint32_t vertexCounter = 0;

    const float* position = vertices->pos;

    float minVal = 10.0f;
    float maxVal = -10.0f;

    for (int i = 0; i < numVertex; ++i)
    {
        vec3 tempPosition = vec3(position[i * 3], position[i * 3 + 1], position[i * 3 + 2]);
        pPoints[vertexCounter] = v3ToF3(tempPosition) * SpaceScale;
        pPoints[vertexCounter].setY(pPoints[vertexCounter].getY() - PLANET_RADIUS);
        vertexCounter++;
        vec3 normalizedPosition = normalize(tempPosition);
        pPoints[vertexCounter++] = v3ToF3(normalizedPosition);

        vec3 normalizedPosition01 = normalizedPosition + vec3(1.0f, 1.0f, 1.0f);
        normalizedPosition01 *= 0.5f;        // normalized to [0,1]
        normalizedPosition01 *= NebulaScale; // scale it

        float NoiseValue01 =
            noiseGenerator.perlinNoise3D(normalizedPosition01.getX(), normalizedPosition01.getY(), normalizedPosition01.getZ());

        maxVal = max(maxVal, NoiseValue01);
        minVal = min(minVal, NoiseValue01);

        vec3 normalizedPosition02 =
            vec3(normalizedPosition.getX(), -normalizedPosition.getY(), normalizedPosition.getZ()) + vec3(1.0f, 1.0f, 1.0f);
        normalizedPosition02 *= 0.5f;        // normalized to [0,1]
        normalizedPosition02 *= NebulaScale; // scale it

        float NoiseValue02 =
            noiseGenerator.perlinNoise3D(normalizedPosition02.getX(), normalizedPosition02.getY(), normalizedPosition02.getZ());

        maxVal = max(maxVal, NoiseValue02);
        minVal = min(minVal, NoiseValue02);

        vec3 normalizedPosition03 =
            vec3(-normalizedPosition.getX(), normalizedPosition.getY(), -normalizedPosition.getZ()) + vec3(1.0f, 1.0f, 1.0f);
        normalizedPosition03 *= 0.5f;        // normalized to [0,1]
        normalizedPosition03 *= NebulaScale; // scale it

        float NoiseValue03 =
            noiseGenerator.perlinNoise3D(normalizedPosition03.getX(), normalizedPosition03.getY(), normalizedPosition03.getZ());

        maxVal = max(maxVal, NoiseValue03);
        minVal = min(minVal, NoiseValue03);

        pPoints[vertexCounter++] = float3(NoiseValue01, NoiseValue02, NoiseValue03);
    }

    float len = maxVal - minVal;

    for (int i = 0; i < numVertex; ++i)
    {
        // Nebula Density
        int index = i * 3 + 2;
        pPoints[index] = (pPoints[index] - float3(minVal, minVal, minVal)) / len;

        float Density = (pPoints[index].getX() + pPoints[index].getY() + pPoints[index].getZ()) / 3.0f;
        Density = pow(Density, 1.5f);
        int maxStar = (int)(StarDensity * Density);
        for (int j = 0; j < maxStar; j++)
        {
            vec3 Positions = f3Tov3(pPoints[index - 1]) * SpaceScale;
            Positions += (vec3((float)rand() / (float)RAND_MAX, (float)rand() / (float)RAND_MAX, (float)rand() / (float)RAND_MAX) * 2.0f -
                          vec3(1.0f, 1.0f, 1.0f)) *
                         StarDistribution;
            Positions = normalize(Positions) * SpaceScale;
            Positions.setY(Positions.getY() - PLANET_RADIUS);

            float temperature = ((float)rand() / (float)RAND_MAX) * 30000.0f + 3700.0f;
            vec3  StarColor = f3Tov3(ColorTemperatureToRGB(temperature));
            vec4  Colors = vec4(StarColor, (((float)rand() / (float)RAND_MAX * 0.9f) + 0.1f) * StarIntensity);

            float starSize = (((float)rand() / (float)RAND_MAX * 1.1f) + 0.5f);
            starSize *= starSize;

            vec4 Info = vec4(temperature, starSize * ParticleSize, (float)rand() / (float)RAND_MAX, (float)rand() / (float)RAND_MAX);

            ParticleData tempParticleData;
            tempParticleData.ParticlePositions = vec4(Positions, 1.0f);
            tempParticleData.ParticleColors = Colors;
            tempParticleData.ParticleInfo = Info;

            arrpush(gParticleSystem.particleDataSet, tempParticleData);
        }
    }

    (*ppPoints) = (float*)pPoints;
}

bool Sky::Init(Renderer* const renderer, PipelineCache* pCache)
{
    pRenderer = renderer;
    pPipelineCache = pCache;
    //////////////////////////////////// Samplers ///////////////////////////////////////////////////

    SamplerDesc samplerClampDesc = {
        FILTER_LINEAR, FILTER_LINEAR, MIPMAP_MODE_LINEAR, ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE
    };
    addSampler(pRenderer, &samplerClampDesc, &pLinearClampSampler);

    //////////////////////////////////// Samplers ///////////////////////////////////////////////////

    samplerClampDesc = { FILTER_LINEAR,
                         FILTER_LINEAR,
                         MIPMAP_MODE_LINEAR,
                         ADDRESS_MODE_CLAMP_TO_BORDER,
                         ADDRESS_MODE_CLAMP_TO_BORDER,
                         ADDRESS_MODE_CLAMP_TO_BORDER };

    addSampler(pRenderer, &samplerClampDesc, &pLinearBorderSampler);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    CalculateLookupData();

    SyncToken token = {};

    // Generate sphere vertex buffer
    VertexStbDsArray IcosahedronVertices;
    IndexStbDsArray  IcosahedronIndices;
    {
        GenerateIcosahedron(&pSpherePoints, IcosahedronVertices, IcosahedronIndices, gSphereResolution, gSphereDiameter);
        sphereIndexCount = (uint32_t)arrlen(IcosahedronIndices);

        BufferLoadDesc sphereVbDesc = {};
        sphereVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
        sphereVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        sphereVbDesc.mDesc.mSize = (uint64_t)arrlen(IcosahedronVertices) * sizeof(float3) * 3;
        sphereVbDesc.pData = pSpherePoints;
        sphereVbDesc.ppBuffer = &pSphereVertexBuffer;
        addResource(&sphereVbDesc, &token);

        BufferLoadDesc sphereIbDesc = {};
        sphereIbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
        sphereIbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        sphereIbDesc.mDesc.mSize = sphereIndexCount * 4;
        sphereIbDesc.pData = IcosahedronIndices;
        sphereIbDesc.ppBuffer = &pSphereIndexBuffer;
        addResource(&sphereIbDesc, &token);
    }

    BufferLoadDesc particleBufferDesc = {};
    particleBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
    particleBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    particleBufferDesc.mDesc.mSize = sizeof(float) * 6 * 4;
    particleBufferDesc.ppBuffer = &gParticleSystem.pParticleVertexBuffer;

    float screenQuadPointsForStar[] = { 0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f,
                                        0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f };

    particleBufferDesc.pData = screenQuadPointsForStar;
    addResource(&particleBufferDesc, &token);

    particleBufferDesc = {};
    particleBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER | DESCRIPTOR_TYPE_BUFFER;
    particleBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    particleBufferDesc.mDesc.mStructStride = sizeof(ParticleData);
    particleBufferDesc.mDesc.mSize = arrlenu(gParticleSystem.particleDataSet) * sizeof(ParticleData);
    particleBufferDesc.mDesc.mElementCount = (uint32_t)(particleBufferDesc.mDesc.mSize / particleBufferDesc.mDesc.mStructStride);
    particleBufferDesc.pData = gParticleSystem.particleDataSet;
    particleBufferDesc.ppBuffer = &gParticleSystem.pParticleInstanceBuffer;
    addResource(&particleBufferDesc, &token);

    BufferLoadDesc renderSkybDesc = {};
    renderSkybDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    renderSkybDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
    renderSkybDesc.mDesc.mSize = sizeof(RenderSkyUniformBuffer);
    renderSkybDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
    renderSkybDesc.pData = NULL;

    for (uint i = 0; i < gDataBufferCount; i++)
    {
        renderSkybDesc.ppBuffer = &pRenderSkyUniformBuffer[i];
        addResource(&renderSkybDesc, &token);
    }

    BufferLoadDesc spaceUniformDesc = {};
    spaceUniformDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    spaceUniformDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
    spaceUniformDesc.mDesc.mSize = sizeof(SpaceUniformBuffer);
    spaceUniformDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
    spaceUniformDesc.pData = NULL;

    for (uint i = 0; i < gDataBufferCount; i++)
    {
        spaceUniformDesc.ppBuffer = &pSpaceUniformBuffer[i];
        addResource(&spaceUniformDesc, &token);
    }

    ///////////////////////////////////////////////////////////////////
    // UI
    ///////////////////////////////////////////////////////////////////

    UIComponentDesc UIComponentDesc = {};

    float          dpiScale[2];
    const uint32_t monitorIdx = getActiveMonitorIdx();
    getMonitorDpiScale(monitorIdx, dpiScale);
    UIComponentDesc.mStartPosition = vec2(1080.0f / dpiScale[0], 580.0f / dpiScale[1]);
    UIComponentDesc.mStartSize = vec2(300.0f / dpiScale[0], 250.0f / dpiScale[1]);
    uiCreateComponent("Sky", &UIComponentDesc, &pGuiSkyWindow);

    gAppSettings.SkyInfo.x = 0.15f;
    gAppSettings.SkyInfo.y = 0.57f;
    gAppSettings.SkyInfo.z = 0.225f;
    // lower value increase the size of the atmosphere region
    // we use a lower value to increase the sunset effect this leads to higher chances of sampling the "reddish" tone of the low altitude
    // sun
    gAppSettings.SkyInfo.w = 1.0f;

    gAppSettings.SunsetColorStrength = 0.92f;

    gAppSettings.OriginLocation.x = -16.70f;
    gAppSettings.OriginLocation.y = -53.20f;
    gAppSettings.OriginLocation.z = 78.80f;
    gAppSettings.OriginLocation.w = 1.0f;

    enum
    {
        PAS_WIDGET_EXPOSURE,
        PAS_WIDGET_INSCATTER_INTENSITY,
        PAS_WIDGET_INSCATTER_CONTRAST,
        PAS_WIDGET_UNITS_TO_M,
        PAS_WIDGET_SUNSET,

        PAS_WIDGET_COUNT
    };
    enum
    {
        NEBULA_WIDGET_SCALE,
        NEBULA_WIDGET_HIGH_COLOR,
        NEBULA_WIDGET_MID_COLOR,
        NEBULA_WIDGET_LOW_COLOR,

        NEBULA_WIDGET_COUNT
    };
    enum
    {
        SUN_WIDGET_AZIMUTH,
        SUN_WIDGET_ELEVATION,
        SUN_WIDGET_COLOR,
        SUN_WIDGET_MOVING_SPEED,
        SUN_WIDGET_IS_MOVING,

        SUN_WIDGET_COUNT
    };

    static const uint32_t maxWidgetCount = max(max((uint32_t)PAS_WIDGET_COUNT, (uint32_t)NEBULA_WIDGET_COUNT), (uint32_t)SUN_WIDGET_COUNT);

    UIWidget  widgetBases[maxWidgetCount] = {};
    UIWidget* widgets[maxWidgetCount];
    for (uint32_t i = 0; i < maxWidgetCount; ++i)
        widgets[i] = &widgetBases[i];

    CollapsingHeaderWidget collapsingPAS;
    collapsingPAS.pGroupedWidgets = widgets;
    collapsingPAS.mWidgetsCount = PAS_WIDGET_COUNT;

    SliderFloatWidget exposure;
    exposure.pData = &gAppSettings.SkyInfo.x;
    exposure.mMin = 0.0f;
    exposure.mMax = 1.0f;
    exposure.mStep = 0.001f;
    widgets[PAS_WIDGET_EXPOSURE]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[PAS_WIDGET_EXPOSURE]->mLabel, "Exposure");
    widgets[PAS_WIDGET_EXPOSURE]->pWidget = &exposure;

    SliderFloatWidget inscatterIntensity;
    inscatterIntensity.pData = &gAppSettings.SkyInfo.y;
    inscatterIntensity.mMin = 0.0f;
    inscatterIntensity.mMax = 3.0f;
    inscatterIntensity.mStep = 0.001f;
    widgets[PAS_WIDGET_INSCATTER_INTENSITY]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[PAS_WIDGET_INSCATTER_INTENSITY]->mLabel, "InscatterIntensity");
    widgets[PAS_WIDGET_INSCATTER_INTENSITY]->pWidget = &inscatterIntensity;

    SliderFloatWidget inscatterDepthFallOff;
    inscatterDepthFallOff.pData = &gAppSettings.SkyInfo.z;
    inscatterDepthFallOff.mMin = 0.005f;
    inscatterDepthFallOff.mMax = 1.0f;
    inscatterDepthFallOff.mStep = 0.001f;
    widgets[PAS_WIDGET_INSCATTER_CONTRAST]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[PAS_WIDGET_INSCATTER_CONTRAST]->mLabel, "InScatterDepthFallOff");
    widgets[PAS_WIDGET_INSCATTER_CONTRAST]->pWidget = &inscatterDepthFallOff;

    SliderFloatWidget unitsToM;
    unitsToM.pData = &gAppSettings.SkyInfo.w;
    unitsToM.mMin = 0.0f;
    unitsToM.mMax = 1.0f;
    unitsToM.mStep = 0.001f;
    widgets[PAS_WIDGET_UNITS_TO_M]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[PAS_WIDGET_UNITS_TO_M]->mLabel, "UnitsToM");
    widgets[PAS_WIDGET_UNITS_TO_M]->pWidget = &unitsToM;

    SliderFloatWidget sunsetEffect;
    sunsetEffect.pData = &gAppSettings.SunsetColorStrength;
    sunsetEffect.mMin = 0.0f;
    sunsetEffect.mMax = 1.0f;
    sunsetEffect.mStep = 0.001f;
    widgets[PAS_WIDGET_SUNSET]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[PAS_WIDGET_SUNSET]->mLabel, "SunsetColorStrength");
    widgets[PAS_WIDGET_SUNSET]->pWidget = &sunsetEffect;

    luaRegisterWidget(
        uiCreateComponentWidget(pGuiSkyWindow, "Precomputed Atmosphere Scattering", &collapsingPAS, WIDGET_TYPE_COLLAPSING_HEADER));

    CollapsingHeaderWidget collapsingNebula;
    collapsingNebula.pGroupedWidgets = widgets;
    collapsingNebula.mWidgetsCount = NEBULA_WIDGET_COUNT;

    SliderFloatWidget scale;
    scale.pData = &NebulaScale;
    scale.mMin = 0.0f;
    scale.mMax = 20.0f;
    scale.mStep = 0.01f;
    widgets[NEBULA_WIDGET_SCALE]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[NEBULA_WIDGET_SCALE]->mLabel, "Nebula Scale");
    widgets[NEBULA_WIDGET_SCALE]->pWidget = &scale;

    ColorPickerWidget highColor;
    highColor.pData = &NebulaHighColor;
    widgets[NEBULA_WIDGET_HIGH_COLOR]->mType = WIDGET_TYPE_COLOR_PICKER;
    strcpy(widgets[NEBULA_WIDGET_HIGH_COLOR]->mLabel, "Nebula High Color");
    widgets[NEBULA_WIDGET_HIGH_COLOR]->pWidget = &highColor;

    ColorPickerWidget midColor;
    midColor.pData = &NebulaMidColor;
    widgets[NEBULA_WIDGET_MID_COLOR]->mType = WIDGET_TYPE_COLOR_PICKER;
    strcpy(widgets[NEBULA_WIDGET_MID_COLOR]->mLabel, "Nebula Mid Color");
    widgets[NEBULA_WIDGET_MID_COLOR]->pWidget = &midColor;

    ColorPickerWidget lowColor;
    lowColor.pData = &NebulaLowColor;
    widgets[NEBULA_WIDGET_LOW_COLOR]->mType = WIDGET_TYPE_COLOR_PICKER;
    strcpy(widgets[NEBULA_WIDGET_LOW_COLOR]->mLabel, "Nebula Low Color");
    widgets[NEBULA_WIDGET_LOW_COLOR]->pWidget = &lowColor;

    luaRegisterWidget(uiCreateComponentWidget(pGuiSkyWindow, "Nebula", &collapsingNebula, WIDGET_TYPE_COLLAPSING_HEADER));

    CollapsingHeaderWidget collapsingSun;
    collapsingSun.pGroupedWidgets = widgets;
    collapsingSun.mWidgetsCount = SUN_WIDGET_COUNT;

    SliderFloatWidget AzimuthSliderFloat;
    AzimuthSliderFloat.pData = &gAppSettings.SunDirection.x;
    AzimuthSliderFloat.mMin = -180.0f;
    AzimuthSliderFloat.mMax = 180.0f;
    AzimuthSliderFloat.mStep = 0.001f;
    widgets[SUN_WIDGET_AZIMUTH]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[SUN_WIDGET_AZIMUTH]->mLabel, "Light Azimuth");
    widgets[SUN_WIDGET_AZIMUTH]->pWidget = &AzimuthSliderFloat;

    SliderFloatWidget ElevationSliderFloat;
    ElevationSliderFloat.pData = &gAppSettings.SunDirection.y;
    ElevationSliderFloat.mMin = 0.0f;
    ElevationSliderFloat.mMax = 360.0f;
    ElevationSliderFloat.mStep = 0.001f;
    widgets[SUN_WIDGET_ELEVATION]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[SUN_WIDGET_ELEVATION]->mLabel, "Light Elevation");
    widgets[SUN_WIDGET_ELEVATION]->pWidget = &ElevationSliderFloat;

    SliderFloat4Widget sliderFloat4;
    sliderFloat4.pData = &gAppSettings.SunColorAndIntensity;
    sliderFloat4.mMin = float4(0.0f);
    sliderFloat4.mMax = float4(10.0f);
    sliderFloat4.mStep = float4(0.01f);
    widgets[SUN_WIDGET_COLOR]->mType = WIDGET_TYPE_SLIDER_FLOAT4;
    strcpy(widgets[SUN_WIDGET_COLOR]->mLabel, "Light Color & Intensity");
    widgets[SUN_WIDGET_COLOR]->pWidget = &sliderFloat4;

    SliderFloatWidget sliderFloat;
    sliderFloat.pData = &gAppSettings.sunMovingSpeed;
    sliderFloat.mMin = -100.0f;
    sliderFloat.mMax = 100.0f;
    sliderFloat.mStep = 0.01f;
    widgets[SUN_WIDGET_MOVING_SPEED]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[SUN_WIDGET_MOVING_SPEED]->mLabel, "Sun Moving Speed");
    widgets[SUN_WIDGET_MOVING_SPEED]->pWidget = &sliderFloat;

    CheckboxWidget sunMoveCheckbox;
    sunMoveCheckbox.pData = &gAppSettings.bSunMove;
    widgets[SUN_WIDGET_IS_MOVING]->mType = WIDGET_TYPE_CHECKBOX;
    strcpy(widgets[SUN_WIDGET_IS_MOVING]->mLabel, "Automatic Sun Moving");
    widgets[SUN_WIDGET_IS_MOVING]->pWidget = &sunMoveCheckbox;

    luaRegisterWidget(uiCreateComponentWidget(pGuiSkyWindow, "Sun", &collapsingSun, WIDGET_TYPE_COLLAPSING_HEADER));

    waitForToken(&token);

    // Need to free memory;
    tf_free(pSpherePoints);
    arrfree(IcosahedronVertices);
    arrfree(IcosahedronIndices);

    return true;
}

void Sky::Exit()
{
    removeSampler(pRenderer, pLinearClampSampler);
    removeSampler(pRenderer, pLinearBorderSampler);

    removeResource(pSphereVertexBuffer);
    removeResource(pSphereIndexBuffer);

    for (uint i = 0; i < gDataBufferCount; i++)
    {
        removeResource(pRenderSkyUniformBuffer[i]);
        removeResource(pSpaceUniformBuffer[i]);
    }

    removeResource(gParticleSystem.pParticleVertexBuffer);
    removeResource(gParticleSystem.pParticleInstanceBuffer);

    arrfree(gParticleSystem.particleDataSet);
    gParticleSystem.particleDataSet = NULL;

    removeResource(pTransmittanceTexture);
    removeResource(pIrradianceTexture);
    removeResource(pInscatterTexture);
}

bool Sky::Load(RenderTarget** rts, uint32_t count) { return false; }

void Sky::Unload() {}

void Sky::Update(float deltaTime)
{
    g_ElapsedTime += deltaTime;

    rotMat = mat4::translation(vec3(0.0f, -PLANET_RADIUS, 0.0f)) * (mat4::rotationY(-Azimuth) * mat4::rotationZ(Elevation)) *
             mat4::translation(vec3(0.0f, PLANET_RADIUS, 0.0f));
    rotMatStarField = (mat4::rotationY(-Azimuth) * mat4::rotationZ(Elevation));
}

void Sky::addDescriptorSets()
{
    DescriptorSetDesc setDesc = { pSkyRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
    addDescriptorSet(pRenderer, &setDesc, &pSkyDescriptorSet[0]);
    setDesc = { pSkyRootSignature, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gDataBufferCount };
    addDescriptorSet(pRenderer, &setDesc, &pSkyDescriptorSet[1]);
}

void Sky::removeDescriptorSets()
{
    removeDescriptorSet(pRenderer, pSkyDescriptorSet[0]);
    removeDescriptorSet(pRenderer, pSkyDescriptorSet[1]);
}

void Sky::addRootSignatures()
{
    const char*       pSkySamplerNames[] = { "g_LinearClamp", "g_LinearBorder" };
    Sampler*          pSkySamplers[] = { pLinearClampSampler, pLinearBorderSampler };
    Shader*           shaders[] = { pPAS_Shader, pSpaceShader };
    RootSignatureDesc rootDesc = {};
    rootDesc.mShaderCount = 2;
    rootDesc.ppShaders = shaders;
    rootDesc.mStaticSamplerCount = 2;
    rootDesc.ppStaticSamplerNames = pSkySamplerNames;
    rootDesc.ppStaticSamplers = pSkySamplers;

    addRootSignature(pRenderer, &rootDesc, &pSkyRootSignature);
}

void Sky::removeRootSignatures() { removeRootSignature(pRenderer, pSkyRootSignature); }

void Sky::addShaders()
{
    ShaderLoadDesc skyShader = {};
    skyShader.mStages[0].pFileName = "RenderSky.vert";
    skyShader.mStages[1].pFileName = "RenderSky.frag";
    addShader(pRenderer, &skyShader, &pPAS_Shader);

    ShaderLoadDesc spaceShader = {};
    spaceShader.mStages[0].pFileName = "Space.vert";
    spaceShader.mStages[1].pFileName = "Space.frag";
    addShader(pRenderer, &spaceShader, &pSpaceShader);
}

void Sky::removeShaders()
{
    removeShader(pRenderer, pPAS_Shader);
    removeShader(pRenderer, pSpaceShader);
}

void Sky::addPipelines()
{
    RasterizerStateDesc rasterizerStateDesc = {};
    rasterizerStateDesc.mCullMode = CULL_MODE_NONE;

    BlendStateDesc blendStateSpaceDesc = {};
    blendStateSpaceDesc.mBlendModes[0] = BM_ADD;
    blendStateSpaceDesc.mBlendAlphaModes[0] = BM_ADD;

    blendStateSpaceDesc.mSrcFactors[0] = BC_ONE;
    blendStateSpaceDesc.mDstFactors[0] = BC_ONE_MINUS_SRC_ALPHA;

    blendStateSpaceDesc.mSrcAlphaFactors[0] = BC_ONE;
    blendStateSpaceDesc.mDstAlphaFactors[0] = BC_ZERO;

    blendStateSpaceDesc.mColorWriteMasks[0] = COLOR_MASK_ALL;
    blendStateSpaceDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;

    PipelineDesc pipelineDescPAS = {};
    pipelineDescPAS.pCache = pPipelineCache;
    {
        pipelineDescPAS.mType = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipelineDesc& pipelineSettings = pipelineDescPAS.mGraphicsDesc;

        pipelineSettings = { 0 };

        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettings.mRenderTargetCount = 1;

        pipelineSettings.pColorFormats = &pSkyRenderTarget->mFormat;
        pipelineSettings.mSampleCount = pSkyRenderTarget->mSampleCount;
        pipelineSettings.mSampleQuality = pSkyRenderTarget->mSampleQuality;

        pipelineSettings.pRootSignature = pSkyRootSignature;
        pipelineSettings.pShaderProgram = pPAS_Shader;
        pipelineSettings.pVertexLayout = NULL;
        pipelineSettings.pRasterizerState = &rasterizerStateDesc;
        addPipeline(pRenderer, &pipelineDescPAS, &pPAS_Pipeline);
    }

    VertexLayout vertexLayout = {};
    vertexLayout.mBindingCount = 1;
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

    PipelineDesc pipelineDescSpace = {};
    pipelineDescSpace.pCache = pPipelineCache;
    {
        pipelineDescSpace.mType = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipelineDesc& pipelineSettings = pipelineDescSpace.mGraphicsDesc;

        pipelineSettings = { 0 };

        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettings.mRenderTargetCount = 1;

        pipelineSettings.pColorFormats = &pSkyRenderTarget->mFormat;
        pipelineSettings.mSampleCount = pSkyRenderTarget->mSampleCount;
        pipelineSettings.mSampleQuality = pSkyRenderTarget->mSampleQuality;

        pipelineSettings.pRootSignature = pSkyRootSignature;
        pipelineSettings.pShaderProgram = pSpaceShader;
        pipelineSettings.pVertexLayout = &vertexLayout;
        pipelineSettings.pRasterizerState = &rasterizerStateDesc;
        pipelineSettings.pBlendState = &blendStateSpaceDesc;

        addPipeline(pRenderer, &pipelineDescSpace, &pSpacePipeline);
    }
}

void Sky::removePipelines()
{
    removePipeline(pRenderer, pPAS_Pipeline);
    removePipeline(pRenderer, pSpacePipeline);
}

void Sky::addRenderTargets()
{
    RenderTargetDesc SkyRenderTarget = {};
    SkyRenderTarget.mArraySize = 1;
    SkyRenderTarget.mDepth = 1;
    SkyRenderTarget.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
    SkyRenderTarget.mFormat = TinyImageFormat_R10G10B10A2_UNORM;
    SkyRenderTarget.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
    SkyRenderTarget.mSampleCount = SAMPLE_COUNT_1;
    SkyRenderTarget.mSampleQuality = 0;
    // SkyRenderTarget.mSrgb = false;
    SkyRenderTarget.mWidth = mWidth;
    SkyRenderTarget.mHeight = mHeight;
    SkyRenderTarget.pName = "Sky RenderTarget";
    addRenderTarget(pRenderer, &SkyRenderTarget, &pSkyRenderTarget);
}

void Sky::removeRenderTargets() { removeRenderTarget(pRenderer, pSkyRenderTarget); }

void Sky::prepareDescriptorSets(RenderTarget** ppPreStageRenderTargets, uint32_t count)
{
    // Sky
    {
        DescriptorData ScParams[8] = {};
        ScParams[0].pName = "SceneColorTexture";
        ScParams[0].ppTextures = &ppPreStageRenderTargets[0]->pTexture;
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
        updateDescriptorSet(pRenderer, 0, pSkyDescriptorSet[0], 6, ScParams);

        for (uint32_t i = 0; i < gDataBufferCount; ++i)
        {
            ScParams[0].pName = "RenderSkyUniformBuffer";
            ScParams[0].ppBuffers = &pRenderSkyUniformBuffer[i];
            ScParams[1].pName = "SpaceUniform";
            ScParams[1].ppBuffers = &pSpaceUniformBuffer[i];
            updateDescriptorSet(pRenderer, i, pSkyDescriptorSet[1], 2, ScParams);
        }
    }
}

void Sky::Draw(Cmd* cmd)
{
    cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Sky");

    vec4 lightDir = vec4(f3Tov3(LightDirection));

    // if (lightDir.getY() >= 0.0f)
    {
        cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Atmospheric Scattering");

        RenderTargetBarrier rtBarriersSky[] = {
            { pSkyRenderTarget, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET },
        };
        BufferBarrier bufferBarriers[] = { { pTransmittanceBuffer, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_UNORDERED_ACCESS } };
        cmdResourceBarrier(cmd, TF_ARRAY_COUNT(bufferBarriers), bufferBarriers, 0, NULL, TF_ARRAY_COUNT(rtBarriersSky), rtBarriersSky);

        BindRenderTargetsDesc bindRenderTargets = {};
        bindRenderTargets.mRenderTargetCount = 1;
        bindRenderTargets.mRenderTargets[0] = { pSkyRenderTarget, LOAD_ACTION_CLEAR };
        cmdBindRenderTargets(cmd, &bindRenderTargets);
        cmdSetViewport(cmd, 0.0f, 0.0f, (float)pSkyRenderTarget->mWidth, (float)pSkyRenderTarget->mHeight, 0.0f, 1.0f);
        cmdSetScissor(cmd, 0, 0, pSkyRenderTarget->mWidth, pSkyRenderTarget->mHeight);

        cmdBindPipeline(cmd, pPAS_Pipeline);
        cmdBindDescriptorSet(cmd, 0, pSkyDescriptorSet[0]);
        cmdBindDescriptorSet(cmd, gFrameIndex, pSkyDescriptorSet[1]);

        RenderSkyUniformBuffer _cbRootConstantStruct;

        _cbRootConstantStruct.InvViewMat = inverse(pCameraController->getViewMatrix());
        _cbRootConstantStruct.InvProjMat = inverse(SkyProjectionMatrix);
        _cbRootConstantStruct.LightDirection = float4(lightDir.getX(), lightDir.getY(), lightDir.getZ(), gAppSettings.SkyInfo.x);

        ///////////////////////////////////////////////////////////////////////////////////////////////

        //    Update params
        float fUnitsToKM = gAppSettings.SkyInfo.w * 0.001f;

        float4 offsetScaleToLocalKM = -gAppSettings.OriginLocation * fUnitsToKM;
        offsetScaleToLocalKM.setW(fUnitsToKM);

        offsetScaleToLocalKM.y += Rg + 0.001f;

        float2 inscatterParams = float2(gAppSettings.SkyInfo.y, gAppSettings.SkyInfo.z);
        float3 localCamPosKM = (v3ToF3(pCameraController->getViewPosition()) * offsetScaleToLocalKM.w) + offsetScaleToLocalKM.getXYZ();

        float  Q = (float)(CAMERA_FAR / (CAMERA_FAR - CAMERA_NEAR));
        float4 QNNear = float4(Q, CAMERA_NEAR * fUnitsToKM, CAMERA_NEAR, CAMERA_FAR);

        //////////////////////////////////////////////////////////////////////////////////////////////

        _cbRootConstantStruct.CameraPosition = float4(localCamPosKM.getX(), localCamPosKM.getY(), localCamPosKM.getZ(), 1.0f);
        _cbRootConstantStruct.QNNear = QNNear;
        _cbRootConstantStruct.InScatterParams = float4(inscatterParams.x, inscatterParams.y, 1.0f - gAppSettings.SunsetColorStrength, 0.0f);
        _cbRootConstantStruct.LightIntensity = gAppSettings.SunColorAndIntensity;

        BufferUpdateDesc BufferUniformSettingDesc = { pRenderSkyUniformBuffer[gFrameIndex] };
        beginUpdateResource(&BufferUniformSettingDesc);
        memcpy(BufferUniformSettingDesc.pMappedData, &_cbRootConstantStruct, sizeof(_cbRootConstantStruct));
        endUpdateResource(&BufferUniformSettingDesc);

        cmdDraw(cmd, 3, 0);

        cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
    }

    /////////////////////////////////////////////////////////////////////////////////////

    if (lightDir.getY() < 0.2f)
    {
        cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Night Sky");

        cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Space");

        cmdSetViewport(cmd, 0.0f, 0.0f, (float)pSkyRenderTarget->mWidth, (float)pSkyRenderTarget->mHeight, 0.0f, 1.0f);
        cmdSetScissor(cmd, 0, 0, pSkyRenderTarget->mWidth, pSkyRenderTarget->mHeight);

        struct Data
        {
            mat4   viewProjMat;
            float4 LightDirection;
            float4 ScreenSize;
            float4 NebulaHighColor;
            float4 NebulaMidColor;
            float4 NebulaLowColor;
        } data;

        vec4 customColor;

        data.NebulaHighColor = NebulaHighColor;
        data.NebulaMidColor = NebulaMidColor;
        data.NebulaLowColor = NebulaLowColor;

        data.viewProjMat = SpaceProjectionMatrix * pCameraController->getViewMatrix() * rotMat;
        data.LightDirection = float4(LightDirection, 0.0f);
        data.ScreenSize = float4((float)pSkyRenderTarget->mWidth, (float)pSkyRenderTarget->mHeight, 0.0f, 0.0f);

        BufferUpdateDesc BufferUniformSettingDesc = { pSpaceUniformBuffer[gFrameIndex] };
        beginUpdateResource(&BufferUniformSettingDesc);
        memcpy(BufferUniformSettingDesc.pMappedData, &data, sizeof(data));
        endUpdateResource(&BufferUniformSettingDesc);

        cmdBindPipeline(cmd, pSpacePipeline);
        cmdBindDescriptorSet(cmd, 0, pSkyDescriptorSet[0]);
        cmdBindDescriptorSet(cmd, gFrameIndex, pSkyDescriptorSet[1]);

        const uint32_t sphereStride = sizeof(float3) * 3;
        cmdBindVertexBuffer(cmd, 1, &pSphereVertexBuffer, &sphereStride, NULL);
        cmdBindIndexBuffer(cmd, pSphereIndexBuffer, INDEX_TYPE_UINT32, 0);
        cmdDrawIndexed(cmd, sphereIndexCount, 0, 0);

        cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);

        cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
    }

    cmdBindRenderTargets(cmd, NULL);

    ////////////////////////////////////////////////////////////////////////

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

        BindRenderTargetsDesc bindRenderTargets = {};
        bindRenderTargets.mRenderTargetCount= 1;
        bindRenderTargets.ppRenderTargets= &pSkyRenderTarget;
        cmdBindRenderTargets(cmd, &bindRenderTargets);
        cmdSetViewport(cmd, 0.0f, 0.0f, (float)pSkyRenderTarget->mWidth, (float)pSkyRenderTarget->mHeight, 0.0f, 1.0f);
        cmdSetScissor(cmd, 0, 0, pSkyRenderTarget->mWidth, pSkyRenderTarget->mHeight);

        DescriptorData AuroraParams[2] = {};

        cmdBindPipeline(cmd, pAuroraPipeline);

        AuroraParams[0].pName = "AuroraParticleBuffer";
        AuroraParams[0].ppBuffers = &pAuroraParticle;
        AuroraParams[1].pName = "AuroraUniformBuffer";
        AuroraParams[1].ppBuffers = &pAuroraUniformBuffer;



        cmdBindDescriptors(cmd, pSkyDescriptorBinder, pSkyRootSignature, 2, AuroraParams);

        cmdBindVertexBuffer(cmd, 1, &pAuroraVertexBuffer, NULL);
        cmdDraw(cmd, (uint32_t)AuroraParticleNum, 0);
        cmdBindRenderTargets(cmd, NULL);

        cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
      }
    #else
      {
        cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Draw Aurora", true);

        cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
      }
    #endif
    */

    cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
}

void Sky::Initialize(ICameraController* InCameraController, ProfileToken InGraphicsGpuProfiler, Buffer* InTransmittanceBuffer)
{
    pCameraController = InCameraController;
    gGpuProfileToken = InGraphicsGpuProfiler;
    pTransmittanceBuffer = InTransmittanceBuffer;
}

void Sky::InitializeWithLoad(RenderTarget* InDepthRenderTarget, RenderTarget* InLinearDepthRenderTarget)
{
    pDepthBuffer = InDepthRenderTarget;
    pLinearDepthBuffer = InLinearDepthRenderTarget;
}

// static const float AVERAGE_GROUND_REFLECTANCE = 0.1f;

// Rayleigh
static const float HR = 8.0f; //-V707
static const vec3  betaR = vec3(5.8e-3f, 1.35e-2f, 3.31e-2f);
/*
// Mie
// DEFAULT
static const float HM = 1.2;
static const float3 betaMSca = (4e-3).xxx;
static const float3 betaMEx = betaMSca / 0.9;
static const float mieG = 0.8;
*/

// CLEAR SKY
static const float HM = 1.2f; //-V707
static const vec3  betaMSca = vec3(20e-3f, 20e-3f, 20e-3f);
static const vec3  betaMEx = betaMSca / 0.9f;
// static const float mieG = 0.76f;

// nearest intersection of ray r,mu with ground or top atmosphere boundary
// mu=cos(ray zenith angle at ray origin)
float limit(float r, float mu)
{
    float dout = -r * mu + sqrt(r * r * (mu * mu - 1.0f) + RL * RL);
    //     float delta2 = r * r * (mu * mu - 1.0) + Rg * Rg;
    //     if (delta2 >= 0.0) {
    //         float din = -r * mu - sqrt(delta2);
    //         if (din >= 0.0) {
    //             dout = min(dout, din);
    //         }
    //     }
    return dout;
}

// optical depth for ray (r,mu) of length d, using analytic formula
// (mu=cos(view zenith angle)), intersections with ground ignored
// H=height scale of exponential density function
float opticalDepth(float H, float r, float mu, float d)
{
    float a = sqrt((0.5f / H) * r);
    vec2  a01 = a * vec2(mu, mu + d / r);
    vec2  a01s = vec2(sign(a01.getX()), sign(a01.getY()));
    vec2  a01sq = vec2(a01.getX() * a01.getX(), a01.getY() * a01.getY());
    float x = a01s.getY() > a01s.getX() ? exp(a01sq.getX()) : 0.0f;
    vec2  y = a01s;
    vec2  denom =
        (2.3193f * vec2(fabs(a01.getX()), fabs(a01.getY())) + vec2(sqrt(1.52f * a01sq.getX() + 4.0f), sqrt(1.52f * a01sq.getY() + 4.0f)));

    y = vec2(y.getX() / denom.getX(), y.getY() / denom.getY());

    vec2 denom2 = vec2(1.0f, exp(-d / H * (d / (2.0f * r) + mu)));

    y = vec2(y.getX() * denom2.getX(), y.getY() * denom2.getY());

    // denom = vec2(denom.getX() * denom2.getX(), denom.getY() * denom2.getY());

    return sqrt((6.2831f * H) * r) * exp((Rg - r) / H) * (x + dot(y, vec2(1.0f, -1.0f)));
}

// transmittance(=transparency) of atmosphere for ray (r,mu) of length d
// (mu=cos(view zenith angle)), intersections with ground ignored
// uses analytic formula instead of transmittance texture
vec3 analyticTransmittance(float r, float mu, float d)
{
    vec3 arg = -betaR * opticalDepth(HR, r, mu, d) - betaMEx * opticalDepth(HM, r, mu, d);
    return vec3(exp(arg.getX()), exp(arg.getY()), exp(arg.getZ()));
}

vec3 analyticTransmittance(float r, float mu) { return analyticTransmittance(r, mu, limit(r, mu)); }

float SmoothStep(float edge0, float edge1, float x)
{
    // Scale, bias and saturate x to 0..1 range
    x = clamp((x - edge0) / (edge1 - edge0), 0, 1);
    // Evaluate polynomial
    return x * x * (3 - 2 * x);
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu)
// (mu=cos(view zenith angle)), or zero if ray intersects ground
vec3 transmittanceWithShadowSmooth(float r, float mu)
{
    //    TODO: check if it is reasonably fast
    //    TODO: check if it is mathematically correct
    //    return mu < -sqrt(1.0 - (Rg / r) * (Rg / r)) ? (0.0).xxx : transmittance(r, mu);

    float eps = 0.5f * PI / 180.0f;
    float eps1 = 0.1f * PI / 180.0f;
    float horizMu = -sqrt(1.0f - (Rg / r) * (Rg / r));
    float horizAlpha = acos(horizMu);
    float horizMuMin = cos(horizAlpha + eps);
    // float horizMuMax = cos(horizAlpha-eps);
    float horizMuMax = cos(horizAlpha + eps1);

    float t = SmoothStep(horizMuMin, horizMuMax, mu);
    vec3  analy = analyticTransmittance(r, mu);
    return lerp(t, vec3(0.0), analy);
}

float3 Sky::GetSunColor()
{
    // float4        SkyInfo; // x: fExposure, y: fInscatterIntencity, z: fInscatterContrast, w: fUnitsToM
    // float4        OriginLocation;

    const float fUnitsToKM = gAppSettings.SkyInfo.w * 0.001f;
    float4      offsetScaleToLocalKM = float4(-gAppSettings.OriginLocation.getXYZ() * fUnitsToKM, fUnitsToKM);
    offsetScaleToLocalKM.y += Rg + 0.001f;

    // float2    inscatterParams = float2(gSkySettings.SkyInfo.y*(1 - gSkySettings.SkyInfo.z),
    // gSkySettings.SkyInfo.y*gSkySettings.SkyInfo.z);
    float3 localCamPosKM = (float3(0.0f, 0.0f, 0.0f) * offsetScaleToLocalKM.w) + offsetScaleToLocalKM.getXYZ();

    //    TODO: Igor: Simplify

    vec3 ray = f3Tov3(LightDirection);
    vec3 x = f3Tov3(localCamPosKM);
    vec3 v = normalize(ray);

    float r = length(x);
    float mu = dot(x, v) / r;

    return v3ToF3(transmittanceWithShadowSmooth(r, mu));
}

Buffer* Sky::GetParticleVertexBuffer() { return gParticleSystem.pParticleVertexBuffer; }

Buffer* Sky::GetParticleInstanceBuffer() { return gParticleSystem.pParticleInstanceBuffer; }

uint32_t Sky::GetParticleCount() { return (uint32_t)arrlen(gParticleSystem.particleDataSet); }
