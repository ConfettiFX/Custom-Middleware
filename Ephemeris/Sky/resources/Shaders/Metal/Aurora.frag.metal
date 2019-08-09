/* Write your header comments here */
#include <metal_stdlib>
using namespace metal;

struct Fragment_Shader
{
    struct PsIn
    {
        float4 position [[position]];
        float2 texCoord;
        float3 color;
    };
    float4 main(PsIn In)
    {
        return float4(((In).texCoord).x, ((In).texCoord).y, 0.0, 1.0);
    };

    Fragment_Shader(
) {}
};

struct main_output { float4 tmp [[color(0)]]; };


fragment main_output stageMain(
    Fragment_Shader::PsIn In [[stage_in]])
{
    Fragment_Shader::PsIn In0;
    In0.position = float4(In.position.xyz, 1.0 / In.position.w);
    In0.texCoord = In.texCoord;
    In0.color = In.color;
    Fragment_Shader main;
    main_output output; output.tmp = main.main(In0);
    return output;
}
