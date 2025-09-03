#include <metal_stdlib>
#include <metal_geometric>
#include <metal_math>
#include "ShaderTypes.h"

using namespace metal;

// ============================================================================
// UI 2D SHADER STRUCTURES
// ============================================================================
// Note: UIUniforms and SDFUniforms are now defined in ShaderTypes.h

// UI vertex input structure
struct UI2DVertexIn {
    float2 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
};

// UI vertex output structure
struct UI2DVertexOut {
    float4 position [[position]];
    float2 texcoord;
};

// ============================================================================
// UI VERTEX SHADER
// ============================================================================

vertex UI2DVertexOut ui_vertex_main(UI2DVertexIn in [[stage_in]],
                                   constant UIUniforms& uniforms [[buffer(1)]]) {
    UI2DVertexOut out;
    
    // Validate uniform values to prevent NaN
    if (uniforms.screenWidth <= 0.0 || uniforms.screenHeight <= 0.0) {
        out.position = float4(in.position,0,1.0);
        out.texcoord = in.texcoord;
        return out;
    }
    
    // Convert screen coordinates to clip space
    // Screen: (0,0) top-left, (width,height) bottom-right
    // Clip: (-1,-1) bottom-left, (1,1) top-right
    float2 screenPos = in.position;
    float2 clipPos = float2(
       /* (screenPos.x / (uniforms.screenWidth/2)) * 2.0 - 1.0,
        1.0 - (screenPos.y / (uniforms.screenHeight/2)) * 2.0*/
                            
                            2*(screenPos.x/uniforms.screenWidth) - 1,
                            1 - 2*(screenPos.y/uniforms.screenHeight)
    );
    
    out.position = float4(clipPos, 0.0, 1.0);
    out.texcoord = in.texcoord;
    
    return out;
}

// ============================================================================
// UI FRAGMENT SHADER
// ============================================================================

fragment float4 ui_fragment_main(UI2DVertexOut in [[stage_in]],
                                texture2d<float> uiTexture [[texture(0)]],
                                sampler uiSampler [[sampler(0)]]) {
    return uiTexture.sample(uiSampler, in.texcoord);
}

// ============================================================================
// SDF FRAGMENT SHADER
// ============================================================================

fragment float4 sdf_fragment_main(UI2DVertexOut in [[stage_in]],
                                 texture2d<float> sdfTexture [[texture(0)]],
                                 sampler sdfSampler [[sampler(0)]],
                                 constant SDFUniforms& sdfUniforms [[buffer(1)]]) {
    // Sample SDF texture (distance values in red channel)
    float distance = sdfTexture.sample(sdfSampler, in.texcoord).r;
    
    // Calculate fill alpha based on distance
    float fillAlpha = smoothstep(sdfUniforms.edgeDistance - sdfUniforms.smoothing,
                                sdfUniforms.edgeDistance + sdfUniforms.smoothing,
                                distance);
    
    float4 finalColor = sdfUniforms.fillColor * fillAlpha;
    
    // Add outline if enabled
    if (sdfUniforms.hasOutline) {
        float outlineAlpha = smoothstep(sdfUniforms.outlineDistance - sdfUniforms.smoothing,
                                      sdfUniforms.outlineDistance + sdfUniforms.smoothing,
                                      distance);
        float4 outlineColor = sdfUniforms.outlineColor * outlineAlpha;
        finalColor = mix(outlineColor, finalColor, fillAlpha);
    }
    
    return finalColor;
}
