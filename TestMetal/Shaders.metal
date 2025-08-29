//
//  Shaders.metal
//  TestMetal
//
//  Created by Sigurd Seteklev on 14/06/2024.
//

// File for Metal 3.0 kernel and shader functions

#include <metal_stdlib>
#include <simd/simd.h>

// Including header shared between this Metal shader code and Swift/C code executing Metal API commands
#import "ShaderTypes.h"

// Metal 3.0: Enable enhanced shader features
#define METAL_3_0 1

using namespace metal;

typedef struct
{
    float3 position [[attribute(VertexAttributePosition)]];
    float2 texCoord [[attribute(VertexAttributeTexcoord)]];
    // Metal 3.0: Add normal support for better lighting
    float3 normal [[attribute(VertexAttributeNormal)]];
} Vertex;

typedef struct
{
    float4 position [[position]];
    float2 texCoord;
    // Metal 3.0: Add normal and world position for lighting
    float3 worldNormal;
    float3 worldPosition;
} ColorInOut;

vertex ColorInOut vertexShader(Vertex in [[stage_in]],
                               constant Uniforms & uniforms [[ buffer(BufferIndexUniforms) ]])
{
    ColorInOut out;

    float4 position = float4(in.position, 1.0);
    out.position = uniforms.projectionMatrix * uniforms.modelViewMatrix * position;
    out.texCoord = in.texCoord;
    
    // Metal 3.0: Calculate world space normal and position for lighting
    out.worldNormal = (uniforms.normalMatrix * float4(in.normal, 0.0)).xyz;
    out.worldPosition = (uniforms.modelViewMatrix * position).xyz;

    return out;
}

fragment float4 fragmentShader(ColorInOut in [[stage_in]],
                               constant Uniforms & uniforms [[ buffer(BufferIndexUniforms) ]],
                               texture2d<half> colorMap     [[ texture(TextureIndexColor) ]])
{
    constexpr sampler colorSampler(mip_filter::linear,
                                   mag_filter::linear,
                                   min_filter::linear);

    half4 colorSample = colorMap.sample(colorSampler, in.texCoord.xy);
    
    // Metal 3.0: Add basic lighting calculation
    float3 lightDir = normalize(float3(1.0, 1.0, 1.0));
    float3 normal = normalize(in.worldNormal);
    float diffuse = max(dot(normal, lightDir), 0.0);
    
    // Add ambient lighting
    float3 ambient = float3(0.2, 0.2, 0.2);
    float3 diffuseColor = float3(diffuse, diffuse, diffuse);
    
    float3 finalColor = (ambient + diffuseColor) * float3(colorSample.rgb);
    
    return float4(finalColor, colorSample.a);
}
