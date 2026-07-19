struct VSInput {
    float3 pos;
    float3 normal;
    float2 tex_coord;
}

struct VSOutput {
    float4 pos: SV_Position;
    float3 normal;
    float2 tex_coord;
};

struct PushConstantBlock {
    float4x4 mvp;    
};

// NOTE: Descriptor sets are ranged from the least to the most specific;
// because updates on one would affect sets going after it.
[[vk::binding(0, 0)]]
Sampler2D diffuse;

[[vk::push_constant]] // mvp mat
ConstantBuffer<PushConstantBlock> push_constant;

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput output;
    output.pos = mul(push_constant.mvp, float4(input.pos, 1.0));
    output.normal = input.normal;
    output.tex_coord = input.tex_coord;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
    float4 diffuse_sample = diffuse.Sample(input.tex_coord);
    float3 ambient = float3(0.1, 0.1, 0.1); // or from environment
    float3 diffuse_light = diffuse_sample.rgb * ambient;
    return float4(diffuse_light, diffuse_sample.a);
}

// TODO:
// [shader("fragment")]
// float4 fragMain(VSOutput input) : SV_Target
// {
//     // Sample all textures
//     float4 diffuse_sample = diffuse.Sample(input.tex_coord);
//     float4 emissive_sample = emissive.Sample(input.tex_coord);
//     float4 occlusion_sample = occlussion.Sample(input.tex_coord);
    
//     // For PBR, you'd also sample normal and metallic/roughness maps
//     // float3 normal_sample = normal.Sample(input.tex_coord).rgb;
//     // float4 metallic_roughness = metallic_roughness_tex.Sample(input.tex_coord);
    
//     // Calculate lighting (simplified - you need full lighting model)
//     float3 ambient_light = float3(0.1, 0.1, 0.1); // or from environment
    
//     // Apply ambient occlusion to ambient light
//     float3 ambient = ambient_light * occlusion_sample.r;
    
//     // Diffuse contribution (modulated by AO)
//     float3 diffuse_light = diffuse_sample.rgb * ambient;
    
//     // Emissive is ADDITIVE (not lerp'd)
//     float3 final_color = diffuse_light + emissive_sample.rgb;
    
//     return float4(final_color, diffuse_sample.a);
// }
