/* Write your header comments here */
#include <metal_stdlib>
using namespace metal;

#include "space_argument_buffers.h"

struct Vertex_Shader
{
	device AuroraParticle* AuroraParticleBuffer;
    constant Uniforms_AuroraUniformBuffer & AuroraUniformBuffer;
    struct VSOutput
    {
        float4 position [[position]];
        float2 texCoord;
        float3 color;
    };
    VSOutput main(uint VertexID)
    {
        VSOutput result;
        ((result).color = float3(0.0, 0.0, 0.0));
        ((result).position = ((AuroraParticleBuffer[VertexID]).Position));
        ((result).texCoord = (result).position.xy);

        return result;
    };

    Vertex_Shader(
device AuroraParticle* AuroraParticleBuffer,
constant Uniforms_AuroraUniformBuffer & AuroraUniformBuffer) :
AuroraParticleBuffer(AuroraParticleBuffer),
AuroraUniformBuffer(AuroraUniformBuffer) {}
};

vertex Vertex_Shader::VSOutput stageMain(
uint VertexID [[vertex_id]],
	 constant ArgData& argBufferStatic [[buffer(UPDATE_FREQ_NONE)]],
	 constant ArgDataPerFrame& argBufferPerFrame [[buffer(UPDATE_FREQ_PER_FRAME)]]
)
{
    uint VertexID0;
    VertexID0 = VertexID;
    Vertex_Shader main(argBufferStatic.AuroraParticleBuffer, argBufferPerFrame.AuroraUniformBuffer);
    return main.main(VertexID0);
}
