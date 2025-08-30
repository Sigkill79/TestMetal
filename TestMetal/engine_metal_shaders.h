#ifndef ENGINE_METAL_SHADERS_H
#define ENGINE_METAL_SHADERS_H

#ifdef __cplusplus
extern "C" {
#endif

// Metal-compatible math types (no C standard library dependencies)
// For C/C++ code, we use our custom types
typedef float vec2_t __attribute__((vector_size(8)));
typedef float vec3_t __attribute__((vector_size(16)));
typedef float vec4_t __attribute__((vector_size(16)));

// Metal-compatible matrix types
typedef struct {
    vec4_t x, y, z, w;
} mat4_t;

// ============================================================================
// METAL SHADER TYPES AND STRUCTURES
// ============================================================================

// Vertex attributes
typedef enum {
    VertexAttributePosition = 0,
    VertexAttributeTexcoord = 1,
    VertexAttributeNormal = 2
} VertexAttribute;

// Vertex structure for Metal shaders
typedef struct {
    vec3_t position;      // 3D position (x, y, z)
    vec2_t texcoord;      // Texture coordinates (u, v)
    vec3_t normal;        // Surface normal (nx, ny, nz)
} MetalVertex;

// Uniform buffer structure for Metal shaders
typedef struct {
    mat4_t projectionMatrix;           // Projection matrix
    mat4_t modelViewMatrix;            // Combined model-view matrix
    mat4_t modelMatrix;                // Model matrix
    mat4_t viewMatrix;                 // View matrix
    mat4_t normalMatrix;               // Normal matrix for lighting
    vec3_t cameraPosition;             // Camera position in world space
    float time;                        // Time for animations
} MetalUniforms;

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

// Vertex descriptor offsets and strides
#define VertexPositionOffset   0
#define VertexTexcoordOffset   16
#define VertexNormalOffset     24
#define VertexStride           36

#ifdef __cplusplus
}
#endif

#endif // ENGINE_METAL_SHADERS_H
