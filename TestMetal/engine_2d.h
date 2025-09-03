#ifndef ENGINE_2D_H
#define ENGINE_2D_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine_metal.h"
#include "engine_math.h"
#include <stdint.h>

// ============================================================================
// UI 2D RENDERING INTERFACE
// ============================================================================

// Forward declarations
struct MetalEngine;
typedef struct MetalEngine* MetalEngineHandle;

// UI 2D vertex structure
typedef struct {
    float position[2];    // Screen coordinates (x, y)
    float texcoord[2];    // Texture coordinates (u, v)
} UI2DVertex;

// UI element structure
typedef struct {
    MetalTextureHandle texture;
    uint32_t startIndex;
    uint32_t indexCount;
    float x, y;           // Top-left position in screen coordinates
    float width, height;  // Size in pixels (1:1 mapping)
    int isActive;
} UIElement;

// UI uniforms structure
typedef struct {
    float screenWidth;
    float screenHeight;
} UIUniforms;

// Main UI 2D engine structure
typedef struct {
    // Buffers for 2D rendering
    MetalBufferHandle vertexBuffer;
    MetalBufferHandle indexBuffer;
    MetalBufferHandle uniformBuffer;
    
    // Dynamic arrays for UI elements
    UIElement* elements;
    uint32_t elementCount;
    uint32_t maxElements;
    
    // Buffer management
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t maxVertices;
    uint32_t maxIndices;
    
    // Rendering state
    int isInitialized;
    MetalRenderPipelineStateHandle uiPipelineState;
    MetalDepthStencilStateHandle uiDepthState;
    
    // Metal engine reference
    MetalEngineHandle metalEngine;
} Engine2D;

// ============================================================================
// UI 2D FUNCTIONS
// ============================================================================

// Initialize UI 2D system
Engine2D *engine_2d_init(MetalEngineHandle metalEngine);

// Shutdown UI 2D system
void engine_2d_shutdown(Engine2D* ui2d);

// Frame management
void engine_2d_clear_elements(Engine2D* ui2d);

// UI element rendering
int engine_2d_draw_image(Engine2D* ui2d, float x, float y, MetalTextureHandle texture);

// Internal rendering (called from Metal engine)
void engine_2d_render_pass(Engine2D* ui2d, void* renderEncoder, float screenWidth, float screenHeight);

// ============================================================================
// CONSTANTS
// ============================================================================

#define UI_MAX_ELEMENTS 1000
#define UI_VERTICES_PER_ELEMENT 4
#define UI_INDICES_PER_ELEMENT 6
#define UI_MAX_VERTICES (UI_MAX_ELEMENTS * UI_VERTICES_PER_ELEMENT)
#define UI_MAX_INDICES (UI_MAX_ELEMENTS * UI_INDICES_PER_ELEMENT)

#ifdef __cplusplus
}
#endif

#endif // ENGINE_2D_H
