/* Write your header comments here */
#include <metal_stdlib>
using namespace metal;

struct Vertex_Shader
{
    struct AuroraParticle
    {
        float4 PrevPosition;
        float4 Position;
        float4 Acceleration;
    };
	device AuroraParticle* AuroraParticleBuffer;
    struct Uniforms_AuroraUniformBuffer
    {
        uint maxVertex;
        float heightOffset;
        float height;
        float deltaTime;
        float4x4 ViewProjMat;
    };
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
	device Vertex_Shader::AuroraParticle* AuroraParticleBuffer [[buffer(6)]],
    constant Vertex_Shader::Uniforms_AuroraUniformBuffer & AuroraUniformBuffer [[buffer(8)]])
{
    uint VertexID0;
    VertexID0 = VertexID;
    Vertex_Shader main(AuroraParticleBuffer, AuroraUniformBuffer);
    return main.main(VertexID0);
}
