#include <metal_stdlib>
#include <metal_geometric>
#include <metal_math>

using namespace metal;

// ============================================================================
// METAL-COMPATIBLE STRUCTURES
// ============================================================================

// Metal-compatible uniform buffer structure
struct MetalUniforms {
    float4x4 projectionMatrix;         // Projection matrix
    float4x4 modelViewMatrix;          // Combined model-view matrix
    float4x4 modelMatrix;              // Model matrix
    float4x4 viewMatrix;               // View matrix
    float4x4 normalMatrix;             // Normal matrix for lighting
    float3 cameraPosition;             // Camera position in world space
    float time;                        // Time for animations
};

// ============================================================================
// SHADER CONSTANTS
// ============================================================================

// Buffer indices
#define BufferIndexVertices    0
#define BufferIndexIndices     1
#define BufferIndexUniforms    2

// Texture indices
#define TextureIndexColorMap   0

// Sampler indices
#define SamplerIndexColorMap   0

// Vertex attributes
#define VertexAttributePosition 0
#define VertexAttributeTexcoord 1
#define VertexAttributeNormal   2

// ============================================================================
// VERTEX SHADER
// ============================================================================

struct VertexIn {
    float3 position [[attribute(VertexAttributePosition)]];
    float2 texcoord [[attribute(VertexAttributeTexcoord)]];
    float3 normal   [[attribute(VertexAttributeNormal)]];
};

struct VertexOut {
    float4 position [[position]];
    float2 texcoord;
    float3 normal;
    float3 worldPosition;
};

vertex VertexOut vertex_main(VertexIn in [[stage_in]],
                           constant MetalUniforms& uniforms [[buffer(BufferIndexUniforms)]]) {
    VertexOut out;
    
    // Transform position to clip space
    out.position = uniforms.projectionMatrix * uniforms.modelViewMatrix * float4(in.position, 1.0);
    
    // Pass through texture coordinates
    out.texcoord = in.texcoord;
    
    // Transform normal to world space
    out.normal = (uniforms.modelMatrix * float4(in.normal, 0.0)).xyz;
    
    // Transform position to world space
    out.worldPosition = (uniforms.modelMatrix * float4(in.position, 1.0)).xyz;
    
    return out;
}

// ============================================================================
// FRAGMENT SHADER
// ============================================================================

fragment float4 fragment_main(VertexOut in [[stage_in]],
                             texture2d<float> colorMap [[texture(TextureIndexColorMap)]],
                             sampler colorSampler [[sampler(SamplerIndexColorMap)]]) {
    
    // Sample texture
    float4 colorSample = colorMap.sample(colorSampler, in.texcoord);
    
    // Simple lighting calculation
    float3 lightDirection = normalize(float3(1.0, 1.0, 1.0));
    float3 normal = normalize(in.normal);
    
    // Ambient lighting
    float3 ambient = float3(0.2, 0.2, 0.2);
    
    // Diffuse lighting
    float diffuseIntensity = max(dot(normal, lightDirection), 0.0);
    float3 diffuseColor = float3(0.8, 0.8, 0.8) * diffuseIntensity;
    
    // Combine lighting with texture
    float3 finalColor = (ambient + diffuseColor) * float3(colorSample.rgb);
    
    return float4(finalColor, colorSample.a);
}
