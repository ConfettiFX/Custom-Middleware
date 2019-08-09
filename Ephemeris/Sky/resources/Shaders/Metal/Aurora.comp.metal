/* Write your header comments here */
#include <metal_stdlib>
#include <metal_compute>
using namespace metal;

struct Compute_Shader
{
    struct PsIn
    {
        float4 position;
        float2 texCoord;
        float3 color;
    };
    struct AuroraParticle
    {
        float4 PrevPosition;
        float4 Position;
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
    device AuroraParticle* AuroraParticleBuffer;
    device AuroraConstraint* AuroraConstraintBuffer;
    struct Uniforms_AuroraUniformBuffer
    {
        uint maxVertex;
        float heightOffset;
        float height;
        float deltaTime;
        float4x4 ViewProjMat;
    };
    constant Uniforms_AuroraUniformBuffer & AuroraUniformBuffer;
    void satisfyConstraint(uint threadID)
    {
        AuroraConstraint constraint = (AuroraConstraint)(AuroraConstraintBuffer[threadID]);
        AuroraParticle p0 = (AuroraParticle)(AuroraParticleBuffer[(constraint).IndexP0]);
        AuroraParticle p1 = (AuroraParticle)(AuroraParticleBuffer[(constraint).IndexP1]);
        float3 gap = normalize((((p0).Position).xyz - ((p1).Position).xyz));
        float current_distance = distance(((p0).Position).xyz, ((p1).Position).xyz);
        float3 correctionVector = ((gap * (float3)((abs((current_distance - (constraint).RestDistance)) / (constraint).RestDistance))) * (float3)(0.00010000000));
        (((p0).Position).xyz += correctionVector);
        (((p1).Position).xyz -= correctionVector);
        (AuroraParticleBuffer[(constraint).IndexP0] = (AuroraParticle)(p0));
        (AuroraParticleBuffer[(constraint).IndexP1] = (AuroraParticle)(p1));
    };
    void update(uint threadID)
    {
        const float DAMPING = 0.010000000;
        AuroraParticle p0 = (AuroraParticle)(AuroraParticleBuffer[threadID]);
        float3 temporaryPosition = ((p0).Position).xyz;
        (((p0).Position).xyz += (((((p0).Position).xyz - ((p0).PrevPosition).xyz) * (float3)((1.0 - DAMPING))) + (((p0).Acceleration).xyz * (float3)(AuroraUniformBuffer.deltaTime))));
        (((p0).PrevPosition).xyz = temporaryPosition);
        (((p0).Acceleration).xyz = float3(0.0, 0.0, 0.0));
        (AuroraParticleBuffer[threadID] = (AuroraParticle)(p0));
    };
    void addForce(uint threadID, float3 force)
    {
        AuroraParticle p0 = (AuroraParticle)(AuroraParticleBuffer[threadID]);
        (((p0).Acceleration).xyz += (force / (float3)(((p0).Position).a)));
        (AuroraParticleBuffer[threadID] = (AuroraParticle)(p0));
    };
    void main(uint3 GTid, uint3 Gid, uint3 DTid)
    {
        const uint CONSTRAINT_ITERATIONS = (const uint)(15);
        uint ThreadID = (DTid).x;
        uint even = (ThreadID % (uint)(2));
        if ((ThreadID < (AuroraUniformBuffer.maxVertex - (uint)(1))))
        {
            for (uint iteration = (uint)(0); (iteration < CONSTRAINT_ITERATIONS); (iteration++))
            {
                if ((even == (uint)(0)))
                {
                    satisfyConstraint(ThreadID);
                }
                simdgroup_barrier(mem_flags::mem_threadgroup);
                if ((even == (uint)(1)))
                {
                    satisfyConstraint(ThreadID);
                }
                simdgroup_barrier(mem_flags::mem_threadgroup);
            }
        }
        simdgroup_barrier(mem_flags::mem_threadgroup);
        update(ThreadID);
        simdgroup_barrier(mem_flags::mem_threadgroup);
        addForce(ThreadID, normalize(float3(1.0, 0.0, 0.0)));
    };

    Compute_Shader(
device AuroraParticle* AuroraParticleBuffer,
device AuroraConstraint* AuroraConstraintBuffer,
constant Uniforms_AuroraUniformBuffer & AuroraUniformBuffer) :
AuroraParticleBuffer(AuroraParticleBuffer),AuroraConstraintBuffer(AuroraConstraintBuffer),AuroraUniformBuffer(AuroraUniformBuffer) {}
};

//[numthreads(64, 1, 1)]
kernel void stageMain(
uint3 GTid [[thread_position_in_threadgroup]],
uint3 Gid [[threadgroup_position_in_grid]],
uint3 DTid [[thread_position_in_grid]],
	device Compute_Shader::AuroraParticle* AuroraParticleBuffer [[buffer(6)]],
    device Compute_Shader::AuroraConstraint* AuroraConstraintBuffer [[buffer(7)]],
    constant Compute_Shader::Uniforms_AuroraUniformBuffer & AuroraUniformBuffer [[buffer(8)]])
{
    uint3 GTid0;
    GTid0 = GTid;
    uint3 Gid0;
    Gid0 = Gid;
    uint3 DTid0;
    DTid0 = DTid;
    Compute_Shader main(
    AuroraParticleBuffer,
    AuroraConstraintBuffer,
    AuroraUniformBuffer);
    return main.main(GTid0, Gid0, DTid0);
}
