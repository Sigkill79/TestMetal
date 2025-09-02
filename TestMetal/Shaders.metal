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

// Define uniform structure for Metal shaders - must match MetalUniforms in engine_metal.m exactly
struct Uniforms {
    float projectionMatrix[16];    // 4x4 matrix as float array
    float modelViewMatrix[16];     // 4x4 matrix as float array
    float modelMatrix[16];         // 4x4 matrix as float array
    float viewMatrix[16];          // 4x4 matrix as float array
    float normalMatrix[16];        // 4x4 matrix as float array
    float cameraPosition[3];       // 3D vector
    float time;                    // float
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
    
    // Convert float arrays to float4x4 matrices for Metal shader operations
    float4x4 projectionMatrix = float4x4(
        float4(uniforms.projectionMatrix[0], uniforms.projectionMatrix[1], uniforms.projectionMatrix[2], uniforms.projectionMatrix[3]),
        float4(uniforms.projectionMatrix[4], uniforms.projectionMatrix[5], uniforms.projectionMatrix[6], uniforms.projectionMatrix[7]),
        float4(uniforms.projectionMatrix[8], uniforms.projectionMatrix[9], uniforms.projectionMatrix[10], uniforms.projectionMatrix[11]),
        float4(uniforms.projectionMatrix[12], uniforms.projectionMatrix[13], uniforms.projectionMatrix[14], uniforms.projectionMatrix[15])
    );
    
    float4x4 modelViewMatrix = float4x4(
        float4(uniforms.modelViewMatrix[0], uniforms.modelViewMatrix[1], uniforms.modelViewMatrix[2], uniforms.modelViewMatrix[3]),
        float4(uniforms.modelViewMatrix[4], uniforms.modelViewMatrix[5], uniforms.modelViewMatrix[6], uniforms.modelViewMatrix[7]),
        float4(uniforms.modelViewMatrix[8], uniforms.modelViewMatrix[9], uniforms.modelViewMatrix[10], uniforms.modelViewMatrix[11]),
        float4(uniforms.modelViewMatrix[12], uniforms.modelViewMatrix[13], uniforms.modelViewMatrix[14], uniforms.modelViewMatrix[15])
    );
    
    float4x4 normalMatrix = float4x4(
        float4(uniforms.normalMatrix[0], uniforms.normalMatrix[1], uniforms.normalMatrix[2], uniforms.normalMatrix[3]),
        float4(uniforms.normalMatrix[4], uniforms.normalMatrix[5], uniforms.normalMatrix[6], uniforms.normalMatrix[7]),
        float4(uniforms.normalMatrix[8], uniforms.normalMatrix[9], uniforms.normalMatrix[10], uniforms.normalMatrix[11]),
        float4(uniforms.normalMatrix[12], uniforms.normalMatrix[13], uniforms.normalMatrix[14], uniforms.normalMatrix[15])
    );
    
    float4x4 modelMatrix = float4x4(
        float4(uniforms.modelMatrix[0], uniforms.modelMatrix[1], uniforms.modelMatrix[2], uniforms.modelMatrix[3]),
        float4(uniforms.modelMatrix[4], uniforms.modelMatrix[5], uniforms.modelMatrix[6], uniforms.modelMatrix[7]),
        float4(uniforms.modelMatrix[8], uniforms.modelMatrix[9], uniforms.modelMatrix[10], uniforms.modelMatrix[11]),
        float4(uniforms.modelMatrix[12], uniforms.modelMatrix[13], uniforms.modelMatrix[14], uniforms.modelMatrix[15])
    );
    
    // Use the pre-computed modelViewMatrix from the engine
    // This ensures proper transformation order: projection * (view * model) * position
    out.position = projectionMatrix * modelViewMatrix * position;
    out.texCoord = in.texcoord;
    
    // Metal 3.0: Calculate world space normal and position for lighting
    out.worldNormal = (normalMatrix * float4(in.normal, 0.0)).xyz;
    out.worldPosition = (modelMatrix * position).xyz;

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
    float3 diffuseColor = float3(0.8, 0.8, 0.8) * diffuse;
    
    float3 finalColor = (ambient + diffuseColor) * float3(colorSample.rgb);
    
    return float4(finalColor, colorSample.a);
}
