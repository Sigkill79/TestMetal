//
//  Shaders.metal
//  TestMetal
//
//  Created by Sigurd Seteklev on 14/06/2024.
//

// File for Metal 3.0 kernel and shader functions

#include <metal_stdlib>
#include <metal_geometric>
#include <metal_math>

// Metal 3.0: Enable enhanced shader features
#define METAL_3_0 1

using namespace metal;

// Define vertex structure for Metal shaders
struct Vertex {
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
    float3 normal   [[attribute(2)]];
};

// Define uniform structure for Metal shaders
struct Uniforms {
    float4x4 projectionMatrix;
    float4x4 modelViewMatrix;
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 normalMatrix;
    float3 cameraPosition;
    float time;
};

// Vertex output structure
struct ColorInOut {
    float4 position [[position]];
    float2 texCoord;
    // Metal 3.0: Add normal and world position for lighting
    float3 worldNormal;
    float3 worldPosition;
};

vertex ColorInOut vertexShader(Vertex in [[stage_in]],
                               constant Uniforms & uniforms [[ buffer(2) ]])
{
    ColorInOut out;

    float4 position = float4(in.position, 1.0);
    // Apply transformations in the correct order: view * model * position
    // This ensures the object rotates around its own center first, then gets moved to camera view
    out.position = uniforms.projectionMatrix * uniforms.viewMatrix * uniforms.modelMatrix * position;
    out.texCoord = in.texcoord;
    
    // Metal 3.0: Calculate world space normal and position for lighting
    out.worldNormal = (uniforms.modelMatrix * float4(in.normal, 0.0)).xyz;
    out.worldPosition = (uniforms.modelMatrix * position).xyz;

    return out;
}

fragment float4 fragmentShader(ColorInOut in [[stage_in]],
                               constant Uniforms & uniforms [[ buffer(2) ]],
                               texture2d<half> colorMap     [[ texture(0) ]])
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
