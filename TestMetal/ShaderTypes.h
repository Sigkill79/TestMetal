//
//  ShaderTypes.h
//  TestMetal
//
//  Created by AI Assistant on 2024
//

#ifndef ShaderTypes_h
#define ShaderTypes_h

#include <simd/simd.h>

// ============================================================================
// SHARED SHADER TYPES
// ============================================================================

// UI uniforms structure
struct UIUniforms {
    float screenWidth;
    float screenHeight;
};

// SDF uniforms structure
struct SDFUniforms {
    simd_float4 fillColor;           // RGBA fill color
    simd_float4 outlineColor;        // RGBA outline color
    float edgeDistance;              // Distance threshold for edge detection
    float outlineDistance;           // Distance threshold for outline
    float smoothing;                 // Anti-aliasing factor
    int hasOutline;                  // Whether to render outline
};

#endif /* ShaderTypes_h */