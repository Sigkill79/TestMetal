#ifndef ENGINE_METAL_H
#define ENGINE_METAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine_model.h"
#include "engine_math.h"
#include <stdint.h>

// ============================================================================
// METAL ENGINE INTERFACE
// ============================================================================

// Metal device and command queue handles (opaque pointers)
typedef struct MetalDevice* MetalDeviceHandle;
typedef struct MetalCommandQueue* MetalCommandQueueHandle;
typedef struct MetalRenderPipelineState* MetalRenderPipelineStateHandle;
typedef struct MetalBuffer* MetalBufferHandle;
typedef struct MetalTexture* MetalTextureHandle;
typedef struct MetalSamplerState* MetalSamplerStateHandle;
typedef struct MetalDepthStencilState* MetalDepthStencilStateHandle;
typedef struct MetalMesh* MetalMeshHandle;
typedef struct MetalModel* MetalModelHandle;
typedef struct MetalVertexDescriptor* MetalVertexDescriptorHandle;
typedef struct MetalLibrary* MetalLibraryHandle;
typedef struct MetalFunction* MetalFunctionHandle;
typedef struct MetalTextureLoader* MetalTextureLoaderHandle;
typedef struct MetalCommandBuffer* MetalCommandBufferHandle;
typedef struct MetalRenderCommandEncoder* MetalRenderCommandEncoderHandle;
typedef struct MetalRenderPassDescriptor* MetalRenderPassDescriptorHandle;
typedef struct MetalDrawable* MetalDrawableHandle;
typedef void* MetalViewHandle; // Use void* for C compatibility

// Metal engine state
typedef struct {
    MetalDeviceHandle device;
    MetalCommandQueueHandle commandQueue;
    MetalRenderPipelineStateHandle renderPipelineState;
    MetalBufferHandle dynamicUniformBuffer;
    MetalDepthStencilStateHandle depthState;
    MetalTextureHandle colorMap;
    MetalVertexDescriptorHandle mtlVertexDescriptor;
    MetalMeshHandle mesh;
    MetalModelHandle uploadedModel;
    
    // Buffer management
    uint32_t uniformBufferOffset;
    uint8_t uniformBufferIndex;
    void* uniformBufferAddress;
    
    // Rendering state
    uint32_t frameCount;
    float rotationAngle;
    int viewportWidth;
    int viewportHeight;
    
    // Matrix state
    mat4_t projectionMatrix;
    
    // Engine state
    int isInitialized;
    
    // Metal 3.0 features
    int supportsMeshShading;
    int supportsObjectCapture;
    int supportsDynamicLibraries;
    int supportsRaytracing;
    int supportsBCTextureCompression;
    int supportsCounters;
} MetalEngine;

// ============================================================================
// METAL ENGINE FUNCTIONS
// ============================================================================

// Initialize Metal engine
MetalEngine* metal_engine_init(void);

// Shutdown Metal engine
void metal_engine_shutdown(MetalEngine* engine);

// Initialize Metal engine with view
MetalEngine* metal_engine_init_with_view(MetalViewHandle view);

// Load Metal state and initialize renderer
int metal_engine_load_metal_with_view(MetalEngine* engine, MetalViewHandle view);

// Load assets (mesh, textures)
int metal_engine_load_assets(MetalEngine* engine);

// Create render pipeline state
int metal_engine_create_pipeline(MetalEngine* engine);

// Create buffers
int metal_engine_create_buffers(MetalEngine* engine);

// Create texture and sampler
int metal_engine_create_textures(MetalEngine* engine);

// Create mesh
int metal_engine_create_mesh(MetalEngine* engine);

// Upload Model3D to Metal buffers and return handle
MetalModelHandle metal_engine_upload_model(MetalEngine* engine, Model3D* model);

// Set the uploaded model for rendering
void metal_engine_set_uploaded_model(MetalEngine* engine, MetalModelHandle model);

// Render a specific model (direct Metal encoder version)
void metal_engine_render_model_direct(MetalEngine* engine, MetalModelHandle model, void* renderEncoder);

// Render a specific model with custom model matrix (for per-entity rendering)
void metal_engine_render_model_with_matrix(MetalEngine* engine, MetalModelHandle model, void* renderEncoder, mat4_t modelMatrix);

// Free uploaded model resources
void metal_engine_free_model(MetalModelHandle model);

// Update dynamic buffer state
void metal_engine_update_dynamic_buffer_state(MetalEngine* engine);

// Update game state and uniforms
void metal_engine_update_game_state(MetalEngine* engine);
void metal_engine_update_game_state_from_engine_state(MetalEngine* engine, void* engineState);

// Render frame
void metal_engine_render_frame(MetalEngine* engine, MetalViewHandle view, void* engineState);

// Handle viewport resize
void metal_engine_resize_viewport(MetalEngine* engine, int width, int height);



// Metal 3.0 feature enablement
void metal_engine_enable_object_capture(MetalEngine* engine);
void metal_engine_enable_mesh_shading(MetalEngine* engine);
void metal_engine_enable_dynamic_libraries(MetalEngine* engine);
void metal_engine_report_metal_features(MetalEngine* engine);

// ============================================================================
// METAL UTILITY FUNCTIONS
// ============================================================================

// Create fallback texture (checkerboard pattern)
MetalTextureHandle metal_engine_create_fallback_texture(MetalDeviceHandle device);

// Get Metal device capabilities
void metal_engine_print_device_info(MetalDeviceHandle device);

// Matrix utility functions
mat4_t metal_engine_matrix_translation(float tx, float ty, float tz);
mat4_t metal_engine_matrix_rotation(float radians, vec3_t axis);
mat4_t metal_engine_matrix_perspective_right_hand(float fovyRadians, float aspect, float nearZ, float farZ);

// Set engine state for integration
void metal_engine_set_engine_state(MetalEngine* engine, void* engineState);

#ifdef __cplusplus
}
#endif

#endif // ENGINE_METAL_H
