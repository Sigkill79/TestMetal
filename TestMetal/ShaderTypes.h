//
//  ShaderTypes.h
//  TestMetal
//
//  Created by Sigurd Seteklev on 14/06/2024.
//

//
//  Header containing types and enum constants shared between Metal shaders and Swift/ObjC source
//
#ifndef ShaderTypes_h
#define ShaderTypes_h

#ifdef __METAL_VERSION__
#define NS_ENUM(_type, _name) enum _name : _type _name; enum _name : _type
typedef metal::int32_t EnumBackingType;
#else
#import <Foundation/Foundation.h>
typedef NSInteger EnumBackingType;
#endif

#include <simd/simd.h>

// This file now includes the engine shader types for compatibility
#include "engine_metal_shaders.h"

// Legacy compatibility - map engine types to Metal types
typedef struct
{
    matrix_float4x4 projectionMatrix;
    matrix_float4x4 modelViewMatrix;
    // Metal 3.0: Add support for object capture and enhanced uniforms
    matrix_float4x4 normalMatrix;
    vector_float3 cameraPosition;
    float time;
} Uniforms;

// Legacy compatibility - map engine enums to Metal enums
#define BufferIndexVertices     BufferIndexVertices
#define BufferIndexIndices      BufferIndexIndices
#define BufferIndexUniforms     BufferIndexUniforms

#define VertexAttributePosition VertexAttributePosition
#define VertexAttributeTexcoord VertexAttributeTexcoord
#define VertexAttributeNormal   VertexAttributeNormal

#define TextureIndexColorMap    TextureIndexColorMap

#endif /* ShaderTypes_h */

