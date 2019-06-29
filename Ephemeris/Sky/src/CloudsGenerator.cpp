
#include "CloudsGenerator.h"
//#include "CloudsManager.h"
//#include "../../../../../Middleware/Ephemeris_2/SkyDomeLib/Include/SkyDome.h"
//#include "../../../../../Middleware/Ephemeris_2/SkyDomeLib/Implementation/CloudsManager.h"

//#include <vector>
//#include <assert.h>

//#include "CloudsConfig_impl.h"


float RandomValue();
float RandomValueNormalized();



void loadDistantClouds(ICloudsManager &CloudsManager, const DistantCloudParams& params)
{
	//ICloudsManager &CloudsManager = pSkyDome->getCloudsManager();

	mat4	transform;

	// Cirrus clouds
	for (uint i = 0; i < params.cloudCount; i++)
	{
		float Orientation = RandomValue() * PI * 2.0f;
		float CloudSize = RandomValue() * 2000.0f + 2000.0f;
		CloudSize *= params.scaleMultiplier;

		float direction = RandomValue() * PI * 2.0f;

		vec3 CloudPosition = vec3(0.0f, params.baseHeight, 0.0f);
		CloudPosition[0] += cosf(direction) * RandomValue() * 200000.0f;
		CloudPosition[1] += RandomValue() * 5000.0f;
		CloudPosition[2] += sinf(direction) * RandomValue() * 200000.0f;

		transform = rotateZ(-CloudPosition[0] / 2000000.f * PI * 0.5f) * rotateX(CloudPosition[3] / 2000000.f * PI * 0.5f) * rotateY(Orientation) * scale(CloudSize, CloudSize, CloudSize);
		//transform.rows[0].w += CloudPosition.x;
		//transform.rows[1].w += CloudPosition.y;
		//transform.rows[2].w += CloudPosition.z;
    transform[3][0] += CloudPosition[0];
    transform[3][1] += CloudPosition[1];
    transform[3][2] += CloudPosition[2];

		CloudsManager.createDistantCloud(transform, NULL);
	}
}

void generateCumulusCloudParticles( eastl::vector<vec4> & particlePosScale, eastl::vector<ParticleProps> & particleProps, int puffDimX = 10, int puffDimY = 10, float puffScaleMin = 1.f, float puffScaleMax = 1.f)
{
	particlePosScale.clear();
	particleProps.clear();

	int dimX = max(0, min(15, puffDimX));
	int dimY = max(0, min(15, puffDimY));
	float scaleMin = max(0.f, min(10.f, puffScaleMin));
	float scaleMax = max(0.f, min(10.f, puffScaleMax));
	if( scaleMin > scaleMax ) scaleMax = scaleMin;

	// Create the puffs that make up our cumulus cloud
	for (int i = 0; i < dimX; ++i)
	{
		for (int j = 0; j < dimY; ++j)
		{
			vec4 offset;
			offset[0] = -1.0f + i * 2.0f / (dimX - 1) + 64.0f * (0.5f - (((float)rand()) / RAND_MAX)) / (dimX - 1);
			offset[1] = 2.0f * (0.5f - (((float)rand()) / RAND_MAX)) / 4.5f;
			offset[2] = -1.0f + j * 2.0f / (dimY - 1) + 64.0f * (0.5f - (((float)rand()) / RAND_MAX)) / (dimY - 1);

			// use w component to hold fade factor for this particle
			offset[3] = (scaleMax - scaleMin) * RandomValue() + scaleMin;

			offset[0] *= 1.0f;
			offset[2] *= 1.0f;

			if (sqrt((i - 4.5f)*(i - 4.5) + (j - 4.5f)*(j - 4.5f)) < 4.5)
			{
				//if (rand() % 10 < 8)
				{
					particlePosScale.push_back(offset);
					particleProps.push_back(ParticleProps());

					particleProps.back().texID = float((i + j * dimY) % 16);
					particleProps.back().angle = float((i + j * dimY) % 16);
					//particleProps.back().transparency = 0.5*float(i % 16)/15.0;
					//cloud.pushParticle(offset, 1, (i + j * dimY) % 16);
					//cloud.pushParticle(offset, 1, rand() % 16);
				}
			}
		}
	}
}

void loadCumulusClouds(ICloudsManager &CloudsManager, const CumulusCloudParams& params)
{
	mat4	transform;
	eastl::vector<vec4> particlePosScale;
  eastl::vector<ParticleProps>	particleProps;

	int i=0;
	int j=0;

	//ICloudsManager &CloudsManager = pSkyDome->getCloudsManager();
		
	float CloudSize = 5000.0f;
	float ParticleScale = CloudSize * 0.9f;
	//vec3 CloudPosition = vec3(0.0f, 6000.0f, 0.0f);
	vec3 CloudPosition = vec3(0.0f, params.baseHeight, 0.0f);
	CloudPosition[0] += i * 9000.0f + RandomValue()*10000.0f;
	//CloudPosition.y += RandomValue() * 2500.0f;
	CloudPosition[1] += RandomValue() * 5000.0f;
	CloudPosition[2] += j * 9000.0f + RandomValue()*10000.0f;

	transform = scale(CloudSize, CloudSize, CloudSize);
	transform[3][0] += CloudPosition[0];
	transform[3][1] += CloudPosition[1];
	transform[3][2] += CloudPosition[2];

	generateCumulusCloudParticles( particlePosScale, particleProps, params.cloudCountX, params.cloudCountZ);
			
	//assert( particlePosScale.size() == particleProps.size() );

	if ( ! particlePosScale.empty() )
	{
		CloudsManager.createCumulusCloud(transform, NULL, ParticleScale, 
			&particlePosScale[ 0 ], &particleProps[ 0 ], (uint32)particlePosScale.size(), true );
	}
}

void generateClouds( ICloudsManager &CloudsManager, const CumulusCloudParams& cClParams, const DistantCloudParams& dClParams, uint32 seed)
{
	srand(seed);

	loadCumulusClouds(CloudsManager, cClParams);
	loadDistantClouds(CloudsManager, dClParams);
}
