#import "engine_metal.h"
#import "engine_main.h"
#import "engine_world.h"
#import "engine_2d.h"
// engine_metal_shaders.h removed to avoid typedef conflicts
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

// UI Uniforms structure is already defined in engine_2d.h

// Define missing constants that were in engine_metal_shaders.h
#define VertexAttributePosition 0
#define VertexAttributeTexcoord 1
#define VertexAttributeNormal 2

#define BufferIndexVertices 0
#define BufferIndexIndices 1
#define BufferIndexUniforms 2

#define TextureIndexColorMap 0
#define SamplerIndexColorMap 0

#define VertexPositionOffset 0
#define VertexTexcoordOffset 16
#define VertexNormalOffset 24
#define VertexStride 36

// Performance and rendering constants
#define FALLBACK_TEXTURE_SIZE 512
#define CHECKERBOARD_TILE_SIZE 64
#define VERTEX_COMPONENT_COUNT 8
#define MATRIX_COMPONENT_COUNT 16
#define CUBE_VERTEX_COUNT 8
#define CUBE_INDEX_COUNT 36
#define CUBE_FACE_COUNT 6
#define CUBE_TRIANGLES_PER_FACE 2

// Error handling macros
#define METAL_ERROR(fmt, ...) fprintf(stderr, "Metal Error: " fmt "\n", ##__VA_ARGS__)
#define METAL_SUCCESS 1
#define METAL_FAILURE 0

// Debug macros
#define METAL_DEBUG(fmt, ...) fprintf(stderr, "Metal Debug: " fmt "\n", ##__VA_ARGS__)
#define METAL_INFO(fmt, ...) fprintf(stderr, "Metal Info: " fmt "\n", ##__VA_ARGS__)

// Define MetalUniforms struct for uniform buffer sizing
typedef struct {
    float projectionMatrix[16];    // 4x4 matrix
    float modelViewMatrix[16];     // 4x4 matrix
    float modelMatrix[16];         // 4x4 matrix
    float viewMatrix[16];          // 4x4 matrix
    float normalMatrix[16];        // 4x4 matrix
    float cameraPosition[3];       // 3D vector
    float time;                    // float
} MetalUniforms;

// MetalModel structure to hold uploaded model data
typedef struct MetalModel {
    __strong id<MTLBuffer>* vertexBuffers;      // Array of vertex buffers (one per mesh)
    __strong id<MTLBuffer>* indexBuffers;       // Array of index buffers (one per mesh)
    uint32_t* indexCounts;              // Array of index counts (one per mesh)
    uint32_t meshCount;                 // Number of meshes in the model
    char* name;                         // Model name
} MetalModel;

#import <MetalKit/MetalKit.h>
#import <ModelIO/ModelIO.h>
#import <simd/simd.h>

// ============================================================================
// METAL ENGINE IMPLEMENTATION
// ============================================================================

// Forward declarations
struct EngineStateStruct; // Forward declaration to avoid circular includes

// Helper function to convert mat4_t to float array (column-major order for Metal)
static void mat4_to_float_array(mat4_t* mat, float* arr) {
    // Convert from mat4_t structure to column-major float array
    // mat4_t stores: x, y, z, w where each is a vec4_t
    // We need column-major order for Metal: [col0, col1, col2, col3]
    
    // Use direct memory copy for better performance
    memcpy(arr, mat, sizeof(mat4_t));
}

// Metal device and pipeline state
typedef struct {
    id<MTLDevice> device;
    id<MTLCommandQueue> commandQueue;
    id<MTLRenderPipelineState> renderPipelineState;
    id<MTLDepthStencilState> depthState;
    MTLVertexDescriptor* mtlVertexDescriptor;
} MetalDeviceState;

// Metal buffer and resource management
typedef struct {
    id<MTLBuffer> dynamicUniformBuffer;
    id<MTLTexture> colorMap;
    MTKMesh* mesh;
    MetalModel* uploadedModel;
    
    // Buffer management
    uint32_t uniformBufferOffset;
    uint8_t uniformBufferIndex;
    void* uniformBufferAddress;
    
    // Manual mesh buffers (for our custom cube)
    id<MTLBuffer> vertexBuffer;
    id<MTLBuffer> indexBuffer;
    uint32_t indexCount;
} MetalResourceState;

// Metal rendering and game state
typedef struct {
    uint32_t frameCount;
    float rotationAngle;
    int viewportWidth;
    int viewportHeight;
    
    // Matrix state
    mat4_t projectionMatrix;
    mat4_t viewMatrix;
    mat4_t modelMatrix;
    
    // Engine state
    int isInitialized;
    void* engineState;
} MetalRenderState;

// Metal 3.0 feature support
typedef struct {
    int supportsMeshShading;
    int supportsObjectCapture;
    int supportsDynamicLibraries;
    int supportsRaytracing;
    int supportsBCTextureCompression;
    int supportsCounters;
} MetalFeatureState;

// Main Metal engine implementation structure
// UI 2D state
typedef struct {
    id<MTLRenderPipelineState> uiPipelineState;
    id<MTLDepthStencilState> uiDepthState;
    id<MTLSamplerState> uiSamplerState;
    id<MTLBuffer> uiVertexBuffer;
    id<MTLBuffer> uiIndexBuffer;
    id<MTLBuffer> uiUniformBuffer;
} MetalUIState;

typedef struct {
    MetalDeviceState device;
    MetalResourceState resources;
    MetalRenderState render;
    MetalFeatureState features;
    MetalUIState ui;
} MetalEngineImpl;

// Constants
static const NSUInteger kMaxBuffersInFlight = 3;
static const size_t kAlignedUniformsSize = (sizeof(MetalUniforms) & ~0xFF) + 0x100;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Internal helper function to render a single mesh
static void render_single_mesh(id<MTLRenderCommandEncoder> encoder, 
                              id<MTLBuffer> vertexBuffer, 
                              id<MTLBuffer> indexBuffer, 
                              uint32_t indexCount, 
                              uint32_t meshIndex,
                              int debugMode) {
    if (!vertexBuffer || !indexBuffer || indexCount == 0) {
        METAL_ERROR("Invalid mesh %u for rendering", meshIndex);
        return;
    }
    
    if (debugMode) {
        METAL_DEBUG("Rendering mesh %u: vertexBuffer=%p, indexBuffer=%p, indexCount=%u", 
                   meshIndex, vertexBuffer, indexBuffer, indexCount);
        METAL_DEBUG("Drawing mesh %u: indexCount=%u, indexBuffer.length=%lu", 
                   meshIndex, indexCount, (unsigned long)indexBuffer.length);
        METAL_DEBUG("Buffer pointer during draw: %p", indexBuffer);
    }
    
    // Set vertex buffer
    [encoder setVertexBuffer:vertexBuffer offset:0 atIndex:BufferIndexVertices];
    
    // Draw indexed primitives
    [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                        indexCount:indexCount
                         indexType:MTLIndexTypeUInt32
                       indexBuffer:indexBuffer
                 indexBufferOffset:0];
}

// Internal helper function to render all meshes in a model
static void render_model_meshes(MetalModel* metalModel, 
                               id<MTLRenderCommandEncoder> encoder, 
                               int debugMode) {
    if (debugMode) {
        METAL_DEBUG("Rendering model: %s with %u meshes", metalModel->name, metalModel->meshCount);
    }
    
    // Render each mesh
    for (uint32_t i = 0; i < metalModel->meshCount; i++) {
        render_single_mesh(encoder, 
                          metalModel->vertexBuffers[i], 
                          metalModel->indexBuffers[i], 
                          metalModel->indexCounts[i], 
                          i, 
                          debugMode);
    }
}

// Internal helper function to convert vertex data to Metal format
static float* convert_vertex_data_to_metal(Mesh* mesh) {
    if (!mesh->vertices || mesh->vertex_count == 0) {
        return NULL;
    }
    
    // Convert vertices to Metal format (position + texcoord + normal)
    size_t vertexDataSize = mesh->vertex_count * sizeof(float) * VERTEX_COMPONENT_COUNT;
    float* vertexData = (float*)malloc(vertexDataSize);
    if (!vertexData) {
        METAL_ERROR("Failed to allocate vertex data for mesh");
        return NULL;
    }
    
    // Pack vertex data: position(3) + texcoord(2) + normal(3) = VERTEX_COMPONENT_COUNT floats
    for (uint32_t j = 0; j < mesh->vertex_count; j++) {
        Vertex* vertex = &mesh->vertices[j];
        float* dst = vertexData + j * VERTEX_COMPONENT_COUNT;
        
        // Position
        dst[0] = vertex->position.x;
        dst[1] = vertex->position.y;
        dst[2] = vertex->position.z;
        
        // TexCoord
        dst[3] = vertex->texcoord.x;
        dst[4] = vertex->texcoord.y;
        
        // Normal
        dst[5] = vertex->normal.x;
        dst[6] = vertex->normal.y;
        dst[7] = vertex->normal.z;
    }
    
    return vertexData;
}

// Internal helper function to create Metal buffers for a mesh
static int create_mesh_buffers(id<MTLDevice> device, 
                              Mesh* mesh, 
                              float* vertexData, 
                              uint32_t meshIndex,
                              const char* modelName,
                              id<MTLBuffer>* vertexBuffer,
                              id<MTLBuffer>* indexBuffer) {
    // Create vertex buffer
    size_t vertexDataSize = mesh->vertex_count * sizeof(float) * VERTEX_COMPONENT_COUNT;
    *vertexBuffer = [device newBufferWithBytes:vertexData
                                       length:vertexDataSize
                                      options:MTLResourceStorageModeShared];
    (*vertexBuffer).label = [NSString stringWithFormat:@"%@_Mesh%u_Vertices", 
                             [NSString stringWithUTF8String:modelName], meshIndex];
    
    // Create index buffer
    size_t indexBufferSize = mesh->index_count * sizeof(uint32_t);
    METAL_DEBUG("Creating index buffer: mesh->index_count=%u, sizeof(uint32_t)=%zu, total size=%zu", 
                mesh->index_count, sizeof(uint32_t), indexBufferSize);
    METAL_DEBUG("mesh->indices pointer: %p", mesh->indices);
    
    if (mesh->indices && mesh->index_count > 0) {
        METAL_DEBUG("First few indices: %u, %u, %u, %u, %u", 
                    mesh->indices[0], mesh->indices[1], mesh->indices[2], mesh->indices[3], mesh->indices[4]);
    }
    
    *indexBuffer = [device newBufferWithBytes:mesh->indices
                                      length:indexBufferSize
                                     options:MTLResourceStorageModeShared];
    (*indexBuffer).label = [NSString stringWithFormat:@"%@_Mesh%u_Indices", 
                           [NSString stringWithUTF8String:modelName], meshIndex];
    METAL_DEBUG("Created index buffer with length: %lu", (unsigned long)(*indexBuffer).length);
    
    return METAL_SUCCESS;
}

// ============================================================================
// METAL ENGINE FUNCTIONS
// ============================================================================

MetalEngine* metal_engine_init(void) {
    MetalEngineImpl* engine = (MetalEngineImpl*)malloc(sizeof(MetalEngineImpl));
    if (!engine) {
        METAL_ERROR("Failed to allocate Metal engine");
        return NULL;
    }
    
    // Initialize all pointers to NULL
    memset((void*)engine, 0, sizeof(MetalEngineImpl));
    
    // Set default values
    engine->render.viewportWidth = 800;
    engine->render.viewportHeight = 600;
    
    NSLog(@"Metal Engine: Initial viewport set to %dx%d", engine->render.viewportWidth, engine->render.viewportHeight);
    engine->render.rotationAngle = 0.0f;
    engine->render.frameCount = 0;
    engine->render.isInitialized = 0;
    
    METAL_INFO("Metal engine initialized");
    return (MetalEngine*)engine;
}

MetalEngine* metal_engine_init_with_view(MetalViewHandle view) {
    MetalEngine* engine = metal_engine_init();
    if (!engine) return NULL;
    
    if (metal_engine_load_metal_with_view(engine, view)) {
        if (metal_engine_load_assets(engine)) {
            METAL_INFO("Metal engine initialized with view successfully");
            return engine;
        }
    }
    
    // Cleanup on failure
    metal_engine_shutdown(engine);
    return NULL;
}

void metal_engine_shutdown(MetalEngine* engine) {
    if (!engine) return;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    // Release Metal objects
    if (impl->device.device) {
        // Release our custom buffers
        if (impl->resources.vertexBuffer) {
            // Note: In a real implementation, we would release Metal objects here
            impl->resources.vertexBuffer = nil;
        }
        if (impl->resources.indexBuffer) {
            // Note: In a real implementation, we would release Metal objects here
            impl->resources.indexBuffer = nil;
        }
        
        // Release uploaded model
        if (impl->resources.uploadedModel) {
            metal_engine_free_model((MetalModelHandle)impl->resources.uploadedModel);
            impl->resources.uploadedModel = NULL;
        }
        
        // Note: In a real implementation, we would release other Metal objects here
    }
    
    METAL_INFO("Metal engine shutdown");
    free(impl);
}

int metal_engine_load_metal_with_view(MetalEngine* engine, MetalViewHandle view) {
    if (!engine || !view) return 0;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    MTKView* mtkView = (__bridge MTKView*)view;
    
    // Get device from view
    impl->device.device = mtkView.device;
    if (!impl->device.device) {
        METAL_ERROR("Failed to get Metal device from view");
        return 0;
    }
    
    // Configure view properties
    mtkView.depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    mtkView.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    mtkView.sampleCount = 1;
    
    // Create command queue
    impl->device.commandQueue = [impl->device.device newCommandQueue];
    if (!impl->device.commandQueue) {
        METAL_ERROR("Failed to create command queue");
        return 0;
    }
    
    // Create vertex descriptor
    impl->device.mtlVertexDescriptor = [[MTLVertexDescriptor alloc] init];
    
    impl->device.mtlVertexDescriptor.attributes[VertexAttributePosition].format = MTLVertexFormatFloat3;
    impl->device.mtlVertexDescriptor.attributes[VertexAttributePosition].offset = 0;
    impl->device.mtlVertexDescriptor.attributes[VertexAttributePosition].bufferIndex = 0; // Use buffer 0 for all vertex data
    
    impl->device.mtlVertexDescriptor.attributes[VertexAttributeTexcoord].format = MTLVertexFormatFloat2;
    impl->device.mtlVertexDescriptor.attributes[VertexAttributeTexcoord].offset = 12; // After position (3 floats * 4 bytes)
    impl->device.mtlVertexDescriptor.attributes[VertexAttributeTexcoord].bufferIndex = 0; // Use buffer 0 for all vertex data
    
    impl->device.mtlVertexDescriptor.attributes[VertexAttributeNormal].format = MTLVertexFormatFloat3;
    impl->device.mtlVertexDescriptor.attributes[VertexAttributeNormal].offset = 20; // After texcoord (3 + 2 floats * 4 bytes)
    impl->device.mtlVertexDescriptor.attributes[VertexAttributeNormal].bufferIndex = 0; // Use buffer 0 for all vertex data
    
    impl->device.mtlVertexDescriptor.layouts[0].stride = 32; // 8 floats * 4 bytes (3 pos + 2 tex + 3 normal)
    impl->device.mtlVertexDescriptor.layouts[0].stepRate = 1;
    impl->device.mtlVertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    
    // Create uniform buffer
    NSUInteger uniformBufferSize = kAlignedUniformsSize * kMaxBuffersInFlight;
    impl->resources.dynamicUniformBuffer = [impl->device.device newBufferWithLength:uniformBufferSize
                                                           options:MTLResourceStorageModeShared];
    impl->resources.dynamicUniformBuffer.label = @"UniformBuffer";
    
    // Create depth stencil state
    MTLDepthStencilDescriptor* depthStateDesc = [[MTLDepthStencilDescriptor alloc] init];
    depthStateDesc.depthCompareFunction = MTLCompareFunctionLess;
    depthStateDesc.depthWriteEnabled = YES;
    impl->device.depthState = [impl->device.device newDepthStencilStateWithDescriptor:depthStateDesc];
    
    // Create render pipeline state (required for rendering)
    if (!metal_engine_create_pipeline(engine)) {
        METAL_ERROR("Failed to create render pipeline state");
        return 0;
    }
    
    // Create UI pipeline state
    if (!metal_engine_create_ui_pipeline(engine)) {
         METAL_ERROR("Failed to create UI pipeline state");
         return 0;
    }
    
    // Initialize Metal 3.0 features
    metal_engine_enable_object_capture(engine);
    metal_engine_enable_mesh_shading(engine);
    metal_engine_enable_dynamic_libraries(engine);
    metal_engine_report_metal_features(engine);
    
    METAL_INFO("Metal state loaded successfully");
    return 1;
}

int metal_engine_create_buffers(MetalEngine* engine) {
    if (!engine) return 0;
    
    // Note: engine parameter not used in this function
    
    // Note: In a real implementation, this would create Metal buffers
    // For now, we just mark it as successful
    
    METAL_INFO("Metal buffer creation requested");
    return 1;
}

int metal_engine_create_pipeline(MetalEngine* engine) {
    if (!engine) return 0;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    // Get default library
    id<MTLLibrary> defaultLibrary = [impl->device.device newDefaultLibrary];
    if (!defaultLibrary) {
        METAL_ERROR("Failed to create default library");
        return 0;
    }
    
    // Get vertex and fragment functions
    id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragmentShader"];
    
    if (!vertexFunction || !fragmentFunction) {
        METAL_ERROR("Failed to get shader functions");
        return 0;
    }
    
    // Create pipeline state descriptor
    MTLRenderPipelineDescriptor* pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineStateDescriptor.label = @"MyPipeline";
    pipelineStateDescriptor.rasterSampleCount = 1;
    pipelineStateDescriptor.vertexFunction = vertexFunction;
    pipelineStateDescriptor.fragmentFunction = fragmentFunction;
    pipelineStateDescriptor.vertexDescriptor = impl->device.mtlVertexDescriptor;
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    pipelineStateDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    pipelineStateDescriptor.stencilAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    
    // Create pipeline state
    NSError* error = NULL;
    impl->device.renderPipelineState = [impl->device.device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    if (!impl->device.renderPipelineState) {
        METAL_ERROR("Failed to create pipeline state: %s", error.localizedDescription.UTF8String);
        return 0;
    }
    
    METAL_INFO("Metal pipeline created successfully");
    return 1;
}

int metal_engine_create_mesh(MetalEngine* engine) {
    if (!engine) return 0;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    NSError* error = nil;
    
    // Create cube vertices manually with exact format we need
    // Format: position (3 floats) + texcoord (2 floats) + normal (3 floats) = VERTEX_COMPONENT_COUNT floats = 32 bytes
    
    // Cube vertices: CUBE_VERTEX_COUNT vertices for a cube
    float vertices[] = {
        // Front face
        -1.0f, -1.0f,  1.0f,  0.0f, 0.0f,  0.0f,  0.0f,  1.0f,  // 0: bottom-left-front
         1.0f, -1.0f,  1.0f,  1.0f, 0.0f,  0.0f,  0.0f,  1.0f,  // 1: bottom-right-front
         1.0f,  1.0f,  1.0f,  1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  // 2: top-right-front
        -1.0f,  1.0f,  1.0f,  0.0f, 1.0f,  0.0f,  0.0f,  1.0f,  // 3: top-left-front
        
        // Back face
        -1.0f, -1.0f, -1.0f,  0.0f, 0.0f,  0.0f,  0.0f, -1.0f,  // 4: bottom-left-back
         1.0f, -1.0f, -1.0f,  1.0f, 0.0f,  0.0f,  0.0f, -1.0f,  // 5: bottom-right-back
         1.0f,  1.0f, -1.0f,  1.0f, 1.0f,  0.0f,  0.0f, -1.0f,  // 6: top-right-back
        -1.0f,  1.0f, -1.0f,  0.0f, 1.0f,  0.0f,  0.0f, -1.0f   // 7: top-left-back
    };
    
    // Cube indices: CUBE_INDEX_COUNT indices (CUBE_FACE_COUNT faces * CUBE_TRIANGLES_PER_FACE triangles each)
    uint16_t indices[] = {
        // Front face
        0, 1, 2,  0, 2, 3,
        // Back face
        5, 4, 7,  5, 7, 6,
        // Left face
        4, 0, 3,  4, 3, 7,
        // Right face
        1, 5, 6,  1, 6, 2,
        // Top face
        3, 2, 6,  3, 6, 7,
        // Bottom face
        4, 5, 1,  4, 1, 0
    };
    
    // Create vertex buffer
    id<MTLBuffer> vertexBuffer = [impl->device.device newBufferWithBytes:vertices
                                                           length:sizeof(vertices)
                                                          options:MTLResourceStorageModeShared];
    vertexBuffer.label = @"CubeVertices";
    
    // Create index buffer
    id<MTLBuffer> indexBuffer = [impl->device.device newBufferWithBytes:indices
                                                          length:sizeof(indices)
                                                         options:MTLResourceStorageModeShared];
    indexBuffer.label = @"CubeIndices";
    
    // Store the buffers for rendering
    impl->resources.vertexBuffer = vertexBuffer;
    impl->resources.indexBuffer = indexBuffer;
    impl->resources.indexCount = sizeof(indices) / sizeof(uint16_t);
    
    // Set mesh to NULL since we're not using MTKMesh anymore
    impl->resources.mesh = nil;
    
    METAL_INFO("Cube mesh created successfully");
    METAL_INFO("Vertex count: %d, Index count: %d", CUBE_VERTEX_COUNT, impl->resources.indexCount);
    
    return 1;
}

// Upload Model3D to Metal buffers and return handle
MetalModelHandle metal_engine_upload_model(MetalEngine* engine, Model3D* model) {
    if (!engine || !model || !model->meshes || model->mesh_count == 0) {
        fprintf(stderr, "Invalid model for upload\n");
        return NULL;
    }
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    // Allocate MetalModel structure
    MetalModel* metalModel = (MetalModel*)malloc(sizeof(MetalModel));
    if (!metalModel) {
        fprintf(stderr, "Failed to allocate MetalModel\n");
        return NULL;
    }
    
    metalModel->meshCount = model->mesh_count;
    metalModel->name = model->name ? strdup(model->name) : strdup("UnnamedModel");
    
    // Allocate arrays for buffers and counts
    metalModel->vertexBuffers = (__strong id<MTLBuffer>*)malloc(model->mesh_count * sizeof(id<MTLBuffer>));
    metalModel->indexBuffers = (__strong id<MTLBuffer>*)malloc(model->mesh_count * sizeof(id<MTLBuffer>));
    metalModel->indexCounts = (uint32_t*)malloc(model->mesh_count * sizeof(uint32_t));
    
    if (!metalModel->vertexBuffers || !metalModel->indexBuffers || !metalModel->indexCounts) {
        fprintf(stderr, "Failed to allocate MetalModel arrays\n");
        metal_engine_free_model((MetalModelHandle)metalModel);
        return NULL;
    }
    
    // Upload each mesh
    for (uint32_t i = 0; i < model->mesh_count; i++) {
        Mesh* mesh = &model->meshes[i];
        
        if (!mesh->vertices || !mesh->indices || mesh->vertex_count == 0 || mesh->index_count == 0) {
            METAL_ERROR("Invalid mesh %u for upload", i);
            continue;
        }
        
        // Convert vertex data to Metal format
        float* vertexData = convert_vertex_data_to_metal(mesh);
        if (!vertexData) {
            METAL_ERROR("Failed to convert vertex data for mesh %u", i);
            continue;
        }
        
        // Create Metal buffers
        id<MTLBuffer> vertexBuffer, indexBuffer;
        if (create_mesh_buffers(impl->device.device, mesh, vertexData, i, metalModel->name, &vertexBuffer, &indexBuffer) != METAL_SUCCESS) {
            free(vertexData);
            continue;
        }
        
        // Store buffers
        metalModel->vertexBuffers[i] = vertexBuffer;
        metalModel->indexBuffers[i] = indexBuffer;
        metalModel->indexCounts[i] = mesh->index_count;
        
        METAL_DEBUG("Stored buffers for mesh %u: vertexBuffer=%p, indexBuffer=%p, length=%lu", 
                i, vertexBuffer, indexBuffer, (unsigned long)indexBuffer.length);
        
        // Free temporary vertex data
        free(vertexData);
        
        METAL_DEBUG("Uploaded mesh %u: %u vertices, %u indices", 
                i, mesh->vertex_count, mesh->index_count);
    }
    
    METAL_INFO("Successfully uploaded model '%s' with %u meshes", 
            metalModel->name, metalModel->meshCount);
    
    return (MetalModelHandle)metalModel;
}

// Set the uploaded model for rendering
void metal_engine_set_uploaded_model(MetalEngine* engine, MetalModelHandle model) {
    if (!engine) return;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    // Release previous model if any
    if (impl->resources.uploadedModel) {
        metal_engine_free_model((MetalModelHandle)impl->resources.uploadedModel);
    }
    
    impl->resources.uploadedModel = (MetalModel*)model;
    METAL_INFO("Set uploaded model for rendering: %s", 
            impl->resources.uploadedModel ? impl->resources.uploadedModel->name : "NULL");
}

// Render a specific model (direct Metal encoder version)
void metal_engine_render_model_direct(MetalEngine* engine, MetalModelHandle model, void* renderEncoder) {
    if (!engine || !model || !renderEncoder) {
        METAL_ERROR("Invalid parameters for model rendering");
        return;
    }
    
    MetalModel* metalModel = (MetalModel*)model;
    id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)renderEncoder;
    
    render_model_meshes(metalModel, encoder, 1); // Enable debug mode
}

// Render a specific model with custom model matrix (for per-entity rendering)
void metal_engine_render_model_with_matrix(MetalEngine* engine, MetalModelHandle model, void* renderEncoder, mat4_t modelMatrix) {
    if (!engine || !model || !renderEncoder) {
        METAL_ERROR("Invalid parameters for model rendering with matrix");
        return;
    }
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    MetalModel* metalModel = (MetalModel*)model;
    id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)renderEncoder;
    
    // Update the uniform buffer with the entity's model matrix
    MetalUniforms* uniforms = (MetalUniforms*)impl->resources.uniformBufferAddress;
    
    // Update model matrix for this entity
    mat4_to_float_array(&modelMatrix, uniforms->modelMatrix);
    
    // Recalculate modelViewMatrix with the new model matrix
    mat4_t viewMatrix = metal_engine_matrix_translation(0.0f, 0.0f, -8.0f);
    mat4_t modelViewMatrix = mat4_mul_mat4(modelMatrix, viewMatrix);
    mat4_to_float_array(&modelViewMatrix, uniforms->modelViewMatrix);
    
    // Recalculate normal matrix
    mat4_t normalMatrix = mat4_transpose(mat4_inverse(modelViewMatrix));
    mat4_to_float_array(&normalMatrix, uniforms->normalMatrix);
    
    // Bind uniform buffer to encoder for this entity
    [encoder setVertexBuffer:impl->resources.dynamicUniformBuffer
                      offset:impl->resources.uniformBufferOffset
                     atIndex:BufferIndexUniforms];
    
    [encoder setFragmentBuffer:impl->resources.dynamicUniformBuffer
                        offset:impl->resources.uniformBufferOffset
                       atIndex:BufferIndexUniforms];
    
    // Render all meshes in the model
    render_model_meshes(metalModel, encoder, 0);
}

// Render a specific model
void metal_engine_render_model(MetalEngine* engine, MetalModelHandle model, MetalRenderCommandEncoderHandle renderEncoder) {
    if (!engine || !model || !renderEncoder) {
        METAL_ERROR("Invalid parameters for model rendering");
        return;
    }
    
    MetalModel* metalModel = (MetalModel*)model;
    id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)renderEncoder;
    
    render_model_meshes(metalModel, encoder, 0); // Disable debug mode
}

// Free uploaded model resources
void metal_engine_free_model(MetalModelHandle model) {
    if (!model) return;
    
    MetalModel* metalModel = (MetalModel*)model;
    
    // Release Metal buffers
    if (metalModel->vertexBuffers) {
        for (uint32_t i = 0; i < metalModel->meshCount; i++) {
            if (metalModel->vertexBuffers[i]) {
                // Note: In a real implementation, we would release Metal objects here
                metalModel->vertexBuffers[i] = nil;
            }
        }
        free(metalModel->vertexBuffers);
    }
    
    if (metalModel->indexBuffers) {
        for (uint32_t i = 0; i < metalModel->meshCount; i++) {
            if (metalModel->indexBuffers[i]) {
                // Note: In a real implementation, we would release Metal objects here
                metalModel->indexBuffers[i] = nil;
            }
        }
        free(metalModel->indexBuffers);
    }
    
    if (metalModel->indexCounts) {
        free(metalModel->indexCounts);
    }
    
    if (metalModel->name) {
        free(metalModel->name);
    }
    
    free(metalModel);
    
    fprintf(stderr, "Freed MetalModel resources\n");
}

int metal_engine_create_textures(MetalEngine* engine) {
    if (!engine) return 0;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    NSError* error = nil;
    
    // Create texture loader
    MTKTextureLoader* textureLoader = [[MTKTextureLoader alloc] initWithDevice:impl->device.device];
    
    NSDictionary* textureLoaderOptions = @{
        MTKTextureLoaderOptionTextureUsage: @(MTLTextureUsageShaderRead),
        MTKTextureLoaderOptionTextureStorageMode: @(MTLStorageModePrivate)
    };
    
    // Try loading from asset catalog first
    impl->resources.colorMap = [textureLoader newTextureWithName:@"ColorMap"
                                          scaleFactor:1.0
                                               bundle:[NSBundle mainBundle]
                                              options:textureLoaderOptions
                                                error:&error];
    
    // If that fails, try loading from the main bundle
    if (!impl->resources.colorMap) {
        fprintf(stderr, "Trying to load texture from main bundle...\n");
        impl->resources.colorMap = [textureLoader newTextureWithName:@"ColorMap"
                                              scaleFactor:1.0
                                                   bundle:[NSBundle mainBundle]
                                                  options:textureLoaderOptions
                                                    error:&error];
    }
    
    // If still no success, try loading the PNG file directly
    if (!impl->resources.colorMap) {
        fprintf(stderr, "Trying to load PNG file directly...\n");
        NSString* texturePath = [[NSBundle mainBundle] pathForResource:@"ColorMap" ofType:@"png"];
        if (texturePath) {
            fprintf(stderr, "Found texture at path: %s\n", texturePath.UTF8String);
            impl->resources.colorMap = [textureLoader newTextureWithContentsOfURL:[NSURL fileURLWithPath:texturePath]
                                                              options:textureLoaderOptions
                                                                error:&error];
            if (impl->resources.colorMap) {
                fprintf(stderr, "Texture loaded successfully from PNG file\n");
            } else {
                fprintf(stderr, "Failed to load texture from PNG file: %s\n", error.localizedDescription.UTF8String);
            }
        } else {
            fprintf(stderr, "Could not find ColorMap.png in bundle - will create programmatic texture\n");
        }
    }
    
    // If still no success, create fallback texture
    if (!impl->resources.colorMap) {
        fprintf(stderr, "Creating fallback colored texture...\n");
        impl->resources.colorMap = (__bridge id<MTLTexture>)metal_engine_create_fallback_texture((__bridge MetalDeviceHandle)impl->device.device);
    }
    
    if (!impl->resources.colorMap) {
        fprintf(stderr, "Error creating texture\n");
        return 0;
    }
    
    METAL_INFO("Texture loaded successfully");
    METAL_INFO("Texture dimensions: %lu x %lu", (unsigned long)impl->resources.colorMap.width, (unsigned long)impl->resources.colorMap.height);
    METAL_INFO("Texture pixel format: %lu", (unsigned long)impl->resources.colorMap.pixelFormat);
    
    
    
    return 1;
}

int metal_engine_load_assets(MetalEngine* engine) {
    if (!engine) return 0;
    
    // Create mesh
    if (!metal_engine_create_mesh(engine)) {
        return 0;
    }
    
    // Create textures
    if (!metal_engine_create_textures(engine)) {
        return 0;
    }
    
    fprintf(stderr, "All assets loaded successfully\n");
    return 1;
}

void metal_engine_update_dynamic_buffer_state(MetalEngine* engine) {
    if (!engine) return;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    impl->resources.uniformBufferIndex = (impl->resources.uniformBufferIndex + 1) % kMaxBuffersInFlight;
    impl->resources.uniformBufferOffset = kAlignedUniformsSize * impl->resources.uniformBufferIndex;
    impl->resources.uniformBufferAddress = ((uint8_t*)impl->resources.dynamicUniformBuffer.contents) + impl->resources.uniformBufferOffset;
}

void metal_engine_update_game_state(MetalEngine* engine) {
    if (!engine) return;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    MetalUniforms* uniforms = (MetalUniforms*)impl->resources.uniformBufferAddress;
    
    // Set projection matrix
    mat4_to_float_array(&impl->render.projectionMatrix, uniforms->projectionMatrix);
    
    // Create model matrix - rotate around the object's center
    vec3_t rotationAxis = vec3(1.0f, 1.0f, 0.0f);
    mat4_t modelMatrix = metal_engine_matrix_rotation(impl->render.rotationAngle, rotationAxis);
    
    // Create view matrix - move camera back from origin
    mat4_t viewMatrix = metal_engine_matrix_translation(0.0f, 0.0f, -8.0f);
    
    // For modelViewMatrix, we want to apply view transformation first, then model transformation
    // This ensures the object rotates around its own center, not around the camera
    mat4_t modelViewMatrix = mat4_mul_mat4(viewMatrix, modelMatrix);
    
    mat4_to_float_array(&modelViewMatrix, uniforms->modelViewMatrix);
    mat4_to_float_array(&modelMatrix, uniforms->modelMatrix);
    mat4_to_float_array(&viewMatrix, uniforms->viewMatrix);
    
    // Calculate normal matrix
    mat4_t normalMatrix = mat4_inverse(mat4_transpose(modelMatrix));
    mat4_to_float_array(&normalMatrix, uniforms->normalMatrix);
    
    // Calculate camera position
    mat4_t invViewMatrix = mat4_inverse(viewMatrix);
    vec4_t cameraPos = invViewMatrix.w;
    uniforms->cameraPosition[0] = cameraPos.x;
    uniforms->cameraPosition[1] = cameraPos.y;
    uniforms->cameraPosition[2] = cameraPos.z;
    
    uniforms->time = impl->render.rotationAngle;
    
    impl->render.rotationAngle += 0.01f;
}

void metal_engine_update_game_state_from_engine_state(MetalEngine* engine, void* engineState) {
    // Function called silently
    
    if (!engine || !engineState) {
        NSLog(@"ERROR: engine=%p, engineState=%p", engine, engineState);
        return;
    }
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    MetalUniforms* uniforms = (MetalUniforms*)impl->resources.uniformBufferAddress;
    
    // Impl and uniforms accessed silently
    
    // Cast to access engine state fields directly
    // We know the structure layout from engine_main.h
    char* base = (char*)engineState;
    
    // Access fields using correct offsets based on EngineStateStruct layout
    // state (4) + camera_position (12) + camera_target (12) + camera_up (12) + padding (8) + view_matrix (64) + projection_matrix (64) + model_matrix (64) = 240
    float* projectionMatrix = (float*)(base + 112);  // projection_matrix starts at offset 112
    float* viewMatrix = (float*)(base + 48);         // view_matrix starts at offset 48
    float* modelMatrix = (float*)(base + 176);       // model_matrix starts at offset 176
    float* cameraPosition = (float*)(base + 4);      // camera_position starts at offset 4
    
    // Matrix offsets calculated silently
    
    // Read matrices directly from engine state and convert to proper format
    mat4_to_float_array((mat4_t*)projectionMatrix, uniforms->projectionMatrix);
    mat4_to_float_array((mat4_t*)viewMatrix, uniforms->viewMatrix);
    mat4_to_float_array((mat4_t*)modelMatrix, uniforms->modelMatrix);
    
    // Using matrices from engine state
    
    // Calculate modelViewMatrix from engine state
    mat4_t modelViewMatrix = mat4_mul_mat4(*(mat4_t*)viewMatrix, *(mat4_t*)modelMatrix);
    mat4_to_float_array(&modelViewMatrix, uniforms->modelViewMatrix);
    
    // Calculate normal matrix
    mat4_t normalMatrix = mat4_inverse(mat4_transpose(*(mat4_t*)modelMatrix));
    mat4_to_float_array(&normalMatrix, uniforms->normalMatrix);
    
    // Use camera position from engine state
    memcpy(uniforms->cameraPosition, cameraPosition, sizeof(float) * 3);
    
    // Use a simple time value for animation (can be replaced with actual time later)
    uniforms->time = impl->render.rotationAngle;
    
    // Matrix values updated silently
}

void metal_engine_render_frame(MetalEngine* engine, MetalViewHandle view, void* engineState) {
    if (!engine || !view || !engineState) {
        NSLog(@"ERROR: Render frame: engine=%p, view=%p, or engineState=%p is NULL", engine, view, engineState);
        return;
    }
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    MTKView* mtkView = (__bridge MTKView*)view;
    
    // Wait for available buffer
    // Note: In a real implementation, we would use a semaphore here
    
    // Create command buffer
    id<MTLCommandBuffer> commandBuffer = [impl->device.commandQueue commandBuffer];
    commandBuffer.label = @"MyCommand";
    
    // Update buffer state
    metal_engine_update_dynamic_buffer_state(engine);
    
    // Update game state using engine state directly
    metal_engine_update_game_state_from_engine_state(engine, engineState);
    
    // Get render pass descriptor
    MTLRenderPassDescriptor* renderPassDescriptor = mtkView.currentRenderPassDescriptor;
    
    if (renderPassDescriptor != nil) {
        // Create render command encoder
        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        renderEncoder.label = @"MyRenderEncoder";
        
        [renderEncoder pushDebugGroup:@"DrawBox"];
        
        [renderEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
        [renderEncoder setCullMode:MTLCullModeBack];
        [renderEncoder setRenderPipelineState:impl->device.renderPipelineState];
        [renderEncoder setDepthStencilState:impl->device.depthState];
        
        // Set texture (uniforms will be bound per entity)
        if (impl->resources.colorMap) {
            METAL_DEBUG("Main 3D: Binding ColorMap texture %p, format=%u, width=%lu, height=%lu", 
                       impl->resources.colorMap, (unsigned int)impl->resources.colorMap.pixelFormat, 
                       (unsigned long)impl->resources.colorMap.width, (unsigned long)impl->resources.colorMap.height);
        }
        [renderEncoder setFragmentTexture:impl->resources.colorMap atIndex:TextureIndexColorMap];
        
        // Check if we have any entities to render
        EngineStateStruct* engineStateStruct = (EngineStateStruct*)engineState;
        uint32_t entityCount = engineStateStruct && engineStateStruct->world ? world_get_entity_count(engineStateStruct->world) : 0;
        
        // Metal render frame called silently
        
        if (entityCount > 0) {
            // Render entities using the world system with per-entity matrices
            if (engineStateStruct->world) {
                for (uint32_t i = 0; i < engineStateStruct->world->max_entities; i++) {
                    WorldEntity* entity = &engineStateStruct->world->entities[i];
                    if (entity->id != 0 && entity->is_active && entity->metal_model) {
                        // Get the entity's transformation matrix
                        mat4_t entityTransform = entity_get_transform_matrix(entity);
                        
                        // Render the entity's model with its specific transformation matrix
                        metal_engine_render_model_with_matrix(engine, entity->metal_model, (__bridge void*)renderEncoder, entityTransform);
                    }
                }
            }
        } else if (impl->resources.uploadedModel) {
            // Fallback: render uploaded model if no entities but model exists
            mat4_t identityMatrix = mat4_identity();
            metal_engine_render_model_with_matrix(engine, (MetalModelHandle)impl->resources.uploadedModel, (__bridge void*)renderEncoder, identityMatrix);
        } else {
            // No entities and no uploaded model - just clear the screen
            METAL_INFO("No entities to render, clearing screen");
            // Don't draw anything, just let the clear color show through
        }
        
        // Render UI pass if UI elements exist
        if (engineStateStruct->ui_2d && engineStateStruct->ui_2d->elementCount > 0) {
             metal_engine_render_ui_pass(engine, (__bridge void*)renderEncoder, engineStateStruct->ui_2d);
        }
        
        [renderEncoder popDebugGroup];
        [renderEncoder endEncoding];
        
        // Present drawable
        [commandBuffer presentDrawable:mtkView.currentDrawable];
    }
    
    [commandBuffer commit];
    
    impl->render.frameCount++;
}

void metal_engine_resize_viewport(MetalEngine* engine, int width, int height) {
    if (!engine) return;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    impl->render.viewportWidth = width;
    impl->render.viewportHeight = height;
    
    NSLog(@"Metal Engine: Viewport resized to %dx%d", width, height);
    
    float aspect = (float)width / (float)height;
    impl->render.projectionMatrix = metal_engine_matrix_perspective_right_hand(65.0f * (M_PI / 180.0f), aspect, 0.1f, 100.0f);
    
    METAL_INFO("Viewport resized: %dx%d", width, height);
}



// Metal 3.0 feature enablement
void metal_engine_enable_object_capture(MetalEngine* engine) {
    if (!engine) return;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    if (@available(macOS 15.0, *)) {
        impl->features.supportsObjectCapture = 1;
        METAL_INFO("Object capture enabled for Metal 3.0");
    }
}

void metal_engine_enable_mesh_shading(MetalEngine* engine) {
    if (!engine) return;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    if (@available(macOS 15.0, *)) {
        impl->features.supportsMeshShading = 1;
        METAL_INFO("Mesh shading enabled for Metal 3.0");
    }
}

void metal_engine_enable_dynamic_libraries(MetalEngine* engine) {
    if (!engine) return;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    if (@available(macOS 15.0, *)) {
        impl->features.supportsDynamicLibraries = 1;
        METAL_INFO("Dynamic libraries enabled for Metal 3.0");
    }
}

void metal_engine_report_metal_features(MetalEngine* engine) {
    if (!engine) return;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    if (@available(macOS 15.0, *)) {
        METAL_INFO("=== Metal 3.0 Feature Report ===");
        METAL_INFO("Device: %s", impl->device.device.name.UTF8String);
        METAL_INFO("Max Threads Per Threadgroup: %lu", (unsigned long)impl->device.device.maxThreadsPerThreadgroup.width);
        METAL_INFO("Max Threads Per Threadgroup (3D): %lu", (unsigned long)impl->device.device.maxThreadsPerThreadgroup.depth);
        METAL_INFO("=================================");
    }
}

// ============================================================================
// METAL UTILITY FUNCTIONS
// ============================================================================

MetalTextureHandle metal_engine_create_fallback_texture(MetalDeviceHandle device) {
    if (!device) return NULL;
    
    id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
    
    // Create texture descriptor
    MTLTextureDescriptor* fallbackDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm_sRGB
                                                                                            width:FALLBACK_TEXTURE_SIZE
                                                                                           height:FALLBACK_TEXTURE_SIZE
                                                                                        mipmapped:NO];
    fallbackDesc.usage = MTLTextureUsageShaderRead;
    
    id<MTLTexture> fallbackTexture = [mtlDevice newTextureWithDescriptor:fallbackDesc];
    fallbackTexture.label = @"FallbackColorMap";
    
    // Fill with checkerboard pattern
    uint8_t* textureData = malloc(FALLBACK_TEXTURE_SIZE * FALLBACK_TEXTURE_SIZE * 4);
    for (int y = 0; y < FALLBACK_TEXTURE_SIZE; y++) {
        for (int x = 0; x < FALLBACK_TEXTURE_SIZE; x++) {
            int index = (y * FALLBACK_TEXTURE_SIZE + x) * 4;
            int checker = ((x / CHECKERBOARD_TILE_SIZE) + (y / CHECKERBOARD_TILE_SIZE)) % 2;
            
            if (checker == 0) {
                textureData[index + 0] = 255; // Red
                textureData[index + 1] = 64;  // Green
                textureData[index + 2] = 128; // Blue
                textureData[index + 3] = 255; // Alpha
            } else {
                textureData[index + 0] = 128; // Red
                textureData[index + 1] = 255; // Green
                textureData[index + 2] = 64;  // Blue
                textureData[index + 3] = 255; // Alpha
            }
        }
    }
    
    [fallbackTexture replaceRegion:MTLRegionMake2D(0, 0, FALLBACK_TEXTURE_SIZE, FALLBACK_TEXTURE_SIZE)
                       mipmapLevel:0
                         withBytes:textureData
                       bytesPerRow:FALLBACK_TEXTURE_SIZE * 4];
    
    free(textureData);
    METAL_INFO("Fallback texture created successfully with checkerboard pattern");
    
    return (__bridge_retained MetalTextureHandle)fallbackTexture;
}

void metal_engine_print_device_info(MetalDeviceHandle device) {
    if (!device) return;
    
    id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
    METAL_INFO("Device info print requested for: %s", mtlDevice.name.UTF8String);
}

MetalDeviceHandle metal_engine_get_device(MetalEngineHandle engine) {
    if (!engine) return NULL;
    
    // Cast to the actual implementation type (same as other functions in this file)
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    return (__bridge MetalDeviceHandle)impl->device.device;
}

// Matrix utility functions
mat4_t metal_engine_matrix_translation(float tx, float ty, float tz) {
    mat4_t m = mat4_identity();
    m.w = vec4(tx, ty, tz, 1.0f);
    return m;
}

mat4_t metal_engine_matrix_rotation(float radians, vec3_t axis) {
    // Normalize axis
    vec3_t normAxis = vec3_normalize(axis);
    float ct = cosf(radians);
    float st = sinf(radians);
    float ci = 1.0f - ct;
    float x = normAxis.x, y = normAxis.y, z = normAxis.z;
    
    mat4_t m;
    m.x = vec4(ct + x * x * ci, y * x * ci + z * st, z * x * ci - y * st, 0.0f);
    m.y = vec4(x * y * ci - z * st, ct + y * y * ci, z * y * ci + x * st, 0.0f);
    m.z = vec4(x * z * ci + y * st, y * z * ci - x * st, ct + z * z * ci, 0.0f);
    m.w = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    
    return m;
}

mat4_t metal_engine_matrix_perspective_right_hand(float fovyRadians, float aspect, float nearZ, float farZ) {
    float ys = 1.0f / tanf(fovyRadians * 0.5f);
    float xs = ys / aspect;
    float zs = farZ / (nearZ - farZ);
    
    mat4_t m;
    m.x = vec4(xs, 0.0f, 0.0f, 0.0f);
    m.y = vec4(0.0f, ys, 0.0f, 0.0f);
    m.z = vec4(0.0f, 0.0f, zs, -1.0f);
    m.w = vec4(0.0f, 0.0f, nearZ * zs, 0.0f);
    
    return m;
}

void metal_engine_set_engine_state(MetalEngine* engine, void* engineState) {
    if (!engine) return;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    impl->render.engineState = engineState;
}

// ============================================================================
// UI 2D RENDERING FUNCTIONS
// ============================================================================

int metal_engine_create_ui_pipeline(MetalEngine* engine) {
    if (!engine) return 0;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    // Get default library
    id<MTLLibrary> defaultLibrary = [impl->device.device newDefaultLibrary];
    if (!defaultLibrary) {
        METAL_ERROR("Failed to create default library for UI");
        return 0;
    }
    
    // Get UI vertex and fragment functions
    id<MTLFunction> uiVertexFunction = [defaultLibrary newFunctionWithName:@"ui_vertex_main"];
    id<MTLFunction> uiFragmentFunction = [defaultLibrary newFunctionWithName:@"ui_fragment_main"];
    
    if (!uiVertexFunction || !uiFragmentFunction) {
        METAL_ERROR("Failed to get UI shader functions");
        return 0;
    }
    
    // Create UI vertex descriptor
    MTLVertexDescriptor* uiVertexDescriptor = [[MTLVertexDescriptor alloc] init];
    uiVertexDescriptor.attributes[0].format = MTLVertexFormatFloat2;
    uiVertexDescriptor.attributes[0].offset = 0;
    uiVertexDescriptor.attributes[0].bufferIndex = 0;
    
    uiVertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
    uiVertexDescriptor.attributes[1].offset = 8;
    uiVertexDescriptor.attributes[1].bufferIndex = 0;
    
    uiVertexDescriptor.layouts[0].stride = 16; // 2 floats position + 2 floats texcoord
    uiVertexDescriptor.layouts[0].stepRate = 1;
    uiVertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    
    // Create UI pipeline state descriptor
    MTLRenderPipelineDescriptor* uiPipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    uiPipelineDescriptor.label = @"UIPipeline";
    uiPipelineDescriptor.rasterSampleCount = 1;
    uiPipelineDescriptor.vertexFunction = uiVertexFunction;
    uiPipelineDescriptor.fragmentFunction = uiFragmentFunction;
    uiPipelineDescriptor.vertexDescriptor = uiVertexDescriptor;
    uiPipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    uiPipelineDescriptor.colorAttachments[0].blendingEnabled = YES;
    uiPipelineDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    uiPipelineDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    uiPipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    uiPipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
    uiPipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    uiPipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    
    // Set depth and stencil pixel formats to match the framebuffer
    uiPipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    uiPipelineDescriptor.stencilAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    
    // Note: Triangle culling is controlled by the render command encoder, not the pipeline descriptor
    // We'll set cullMode to None in the UI rendering pass
    
    // Create UI pipeline state
    NSError* error = nil;
    impl->ui.uiPipelineState = [impl->device.device newRenderPipelineStateWithDescriptor:uiPipelineDescriptor error:&error];
    if (!impl->ui.uiPipelineState) {
        METAL_ERROR("Failed to create UI pipeline state: %s", error.localizedDescription.UTF8String);
        return 0;
    }
    
    // Create UI depth stencil state (disable depth testing for UI)
    MTLDepthStencilDescriptor* uiDepthDescriptor = [[MTLDepthStencilDescriptor alloc] init];
    uiDepthDescriptor.depthCompareFunction = MTLCompareFunctionAlways;
    uiDepthDescriptor.depthWriteEnabled = NO;
    impl->ui.uiDepthState = [impl->device.device newDepthStencilStateWithDescriptor:uiDepthDescriptor];
    
    // Create UI sampler state
    MTLSamplerDescriptor* uiSamplerDescriptor = [[MTLSamplerDescriptor alloc] init];
    uiSamplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
    uiSamplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
    uiSamplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
    uiSamplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
    impl->ui.uiSamplerState = [impl->device.device newSamplerStateWithDescriptor:uiSamplerDescriptor];
    
    // Create UI buffers
    size_t uiVertexBufferSize = UI_MAX_VERTICES * sizeof(float) * 4; // 4 floats per vertex
    impl->ui.uiVertexBuffer = [impl->device.device newBufferWithLength:uiVertexBufferSize options:MTLResourceStorageModeShared];
    impl->ui.uiVertexBuffer.label = @"UIVertexBuffer";
    
    size_t uiIndexBufferSize = UI_MAX_INDICES * sizeof(uint32_t);
    impl->ui.uiIndexBuffer = [impl->device.device newBufferWithLength:uiIndexBufferSize options:MTLResourceStorageModeShared];
    impl->ui.uiIndexBuffer.label = @"UIIndexBuffer";
    
    size_t uiUniformBufferSize = sizeof(UIUniforms);
    impl->ui.uiUniformBuffer = [impl->device.device newBufferWithLength:uiUniformBufferSize options:MTLResourceStorageModeShared];
    impl->ui.uiUniformBuffer.label = @"UIUniformBuffer";
    
    METAL_INFO("UI pipeline created successfully");
    return 1;
}

MetalTextureHandle metal_engine_get_colormap_texture(MetalEngine* engine) {
    if (!engine) return NULL;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    return (__bridge MetalTextureHandle)impl->resources.colorMap;
}

void metal_engine_render_ui_pass(MetalEngine* engine, void* renderEncoder, void* ui2d) {
    if (!engine || !renderEncoder || !ui2d) {
        return;
    }
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    Engine2D* ui = (Engine2D*)ui2d;
    id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)renderEncoder;
    
    if (ui->elementCount == 0) {
        return;
    }
    
    METAL_DEBUG("Rendering UI pass: %u elements", ui->elementCount);
    
    // Set UI pipeline state
    [encoder setRenderPipelineState:impl->ui.uiPipelineState];
    [encoder setDepthStencilState:impl->ui.uiDepthState];
    
    // Debug: Log UI pipeline state information
    METAL_DEBUG("UI Pipeline: Using pipeline state %p", impl->ui.uiPipelineState);
    
    // Disable triangle culling for UI rendering (we want to see all faces)
    [encoder setCullMode:MTLCullModeNone];
    
    // Update uniform buffer with screen dimensions
    UIUniforms* uniforms = (UIUniforms*)impl->ui.uiUniformBuffer.contents;
    uniforms->screenWidth = (float)impl->render.viewportWidth;
    uniforms->screenHeight = (float)impl->render.viewportHeight;
    
    // Debug: Log uniform values
    NSLog(@"UI Uniforms: screenWidth=%.1f, screenHeight=%.1f", uniforms->screenWidth, uniforms->screenHeight);
    
    // Bind uniform buffer at index 1 (index 0 is used for vertex data)
    [encoder setVertexBuffer:impl->ui.uiUniformBuffer offset:0 atIndex:1];
    
    // Generate vertex and index data for all UI elements
    float* vertexData = (float*)impl->ui.uiVertexBuffer.contents;
    uint32_t* indexData = (uint32_t*)impl->ui.uiIndexBuffer.contents;
    
    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;
    
    for (uint32_t i = 0; i < ui->elementCount; i++) {
        UIElement* element = &ui->elements[i];
        if (!element->isActive) continue;
        
        // Generate quad vertices (4 vertices per element)
        float x = element->x;
        float y = element->y;
        float width = element->width;
        float height = element->height;
        
        // Top-left
        vertexData[vertexOffset + 0] = x;
        vertexData[vertexOffset + 1] = y;
        vertexData[vertexOffset + 2] = 0.0f; // u
        vertexData[vertexOffset + 3] = 0.0f; // v
        
        // Top-right
        vertexData[vertexOffset + 4] = x + width;
        vertexData[vertexOffset + 5] = y;
        vertexData[vertexOffset + 6] = 1.0f; // u
        vertexData[vertexOffset + 7] = 0.0f; // v
        
        // Bottom-right
        vertexData[vertexOffset + 8] = x + width;
        vertexData[vertexOffset + 9] = y + height;
        vertexData[vertexOffset + 10] = 1.0f; // u
        vertexData[vertexOffset + 11] = 1.0f; // v
        
        // Bottom-left
        vertexData[vertexOffset + 12] = x;
        vertexData[vertexOffset + 13] = y + height;
        vertexData[vertexOffset + 14] = 0.0f; // u
        vertexData[vertexOffset + 15] = 1.0f; // v
        
        // Generate quad indices (6 indices per element)
        uint32_t startVertex = vertexOffset / 4; // 4 floats per vertex
        indexData[indexOffset + 0] = startVertex + 0;
        indexData[indexOffset + 1] = startVertex + 1;
        indexData[indexOffset + 2] = startVertex + 2;
        indexData[indexOffset + 3] = startVertex + 0;
        indexData[indexOffset + 4] = startVertex + 2;
        indexData[indexOffset + 5] = startVertex + 3;
        
        vertexOffset += 16; // 4 vertices * 4 floats
        indexOffset += 6;   // 6 indices
    }
    
    // Bind vertex buffer
    [encoder setVertexBuffer:impl->ui.uiVertexBuffer offset:0 atIndex:0];
    [encoder setFragmentSamplerState:impl->ui.uiSamplerState atIndex:0];

    for (uint32_t i = 0; i < ui->elementCount; i++) {

        UIElement* element = &ui->elements[i];
        if (!element->isActive) continue;
        
        // Debug: Log texture information before binding
        id<MTLTexture> texture = (__bridge id<MTLTexture> _Nullable)(element->texture);
        if (texture) {
            METAL_DEBUG("UI Element %u: Binding texture %p, format=%u, width=%lu, height=%lu", 
                       i, texture, (unsigned int)texture.pixelFormat, (unsigned long)texture.width, (unsigned long)texture.height);
        } else {
            METAL_DEBUG("UI Element %u: NULL texture handle!", i);
        }
        
        // Set texture and sampler (use ColorMap for now)
        [encoder setFragmentTexture:texture atIndex:0];
        
        // Draw all UI elements
        [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                            indexCount:element->indexCount
                            indexType:MTLIndexTypeUInt32
                        indexBuffer:impl->ui.uiIndexBuffer
                    indexBufferOffset:sizeof(int)*element->startIndex];
    };
    
    METAL_DEBUG("UI pass rendered: %u vertices, %u indices", vertexOffset / 4, indexOffset);
}
