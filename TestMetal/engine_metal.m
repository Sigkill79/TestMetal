#import "engine_metal.h"
#import "engine_main.h"
#import "engine_world.h"
// engine_metal_shaders.h removed to avoid typedef conflicts
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

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
    
    // Column 0: x components
    arr[0] = mat->x.x;  // x.x
    arr[1] = mat->x.y;  // x.y  
    arr[2] = mat->x.z;  // x.z
    arr[3] = mat->x.w;  // x.w
    
    // Column 1: y components
    arr[4] = mat->y.x;  // y.x
    arr[5] = mat->y.y;  // y.y
    arr[6] = mat->y.z;  // y.z
    arr[7] = mat->y.w;  // y.w
    
    // Column 2: z components
    arr[8] = mat->z.x;  // z.x
    arr[9] = mat->z.y;  // z.y
    arr[10] = mat->z.z; // z.z
    arr[11] = mat->z.w; // z.w
    
    // Column 3: w components
    arr[12] = mat->w.x; // w.x
    arr[13] = mat->w.y; // w.y
    arr[14] = mat->w.z; // w.z
    arr[15] = mat->w.w; // w.w
}

// Forward declarations for Metal objects
typedef struct {
    id<MTLDevice> device;
    id<MTLCommandQueue> commandQueue;
    id<MTLRenderPipelineState> renderPipelineState;
    id<MTLBuffer> dynamicUniformBuffer;
    id<MTLDepthStencilState> depthState;
    id<MTLTexture> colorMap;
    MTLVertexDescriptor* mtlVertexDescriptor;
    MTKMesh* mesh;
    MetalModel* uploadedModel;
    
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
    mat4_t viewMatrix;
    mat4_t modelMatrix;
    
    // Engine state
    int isInitialized;
    
    // Metal 3.0 features
    int supportsMeshShading;
    int supportsObjectCapture;
    int supportsDynamicLibraries;
    int supportsRaytracing;
    int supportsBCTextureCompression;
    int supportsCounters;
    
    // Engine integration
    void* engineState;
    
    
    
    // Manual mesh buffers (for our custom cube)
    id<MTLBuffer> vertexBuffer;
    id<MTLBuffer> indexBuffer;
    uint32_t indexCount;
    
} MetalEngineImpl;

// Constants
static const NSUInteger kMaxBuffersInFlight = 3;
static const size_t kAlignedUniformsSize = (sizeof(MetalUniforms) & ~0xFF) + 0x100;

// ============================================================================
// METAL ENGINE FUNCTIONS
// ============================================================================

MetalEngine* metal_engine_init(void) {
    MetalEngineImpl* engine = (MetalEngineImpl*)malloc(sizeof(MetalEngineImpl));
    if (!engine) {
        fprintf(stderr, "Failed to allocate Metal engine\n");
        return NULL;
    }
    
    // Initialize all pointers to NULL
    memset((void*)engine, 0, sizeof(MetalEngineImpl));
    
    // Set default values
    engine->viewportWidth = 800;
    engine->viewportHeight = 600;
    engine->rotationAngle = 0.0f;
    engine->frameCount = 0;
    engine->isInitialized = 0;
    
    fprintf(stderr, "Metal engine initialized\n");
    return (MetalEngine*)engine;
}

MetalEngine* metal_engine_init_with_view(MetalViewHandle view) {
    MetalEngine* engine = metal_engine_init();
    if (!engine) return NULL;
    
    if (metal_engine_load_metal_with_view(engine, view)) {
        if (metal_engine_load_assets(engine)) {
            fprintf(stderr, "Metal engine initialized with view successfully\n");
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
    if (impl->device) {
        // Release our custom buffers
        if (impl->vertexBuffer) {
            // Note: In a real implementation, we would release Metal objects here
            impl->vertexBuffer = nil;
        }
        if (impl->indexBuffer) {
            // Note: In a real implementation, we would release Metal objects here
            impl->indexBuffer = nil;
        }
        
        // Release uploaded model
        if (impl->uploadedModel) {
            metal_engine_free_model((MetalModelHandle)impl->uploadedModel);
            impl->uploadedModel = NULL;
        }
        
        // Note: In a real implementation, we would release other Metal objects here
    }
    
    fprintf(stderr, "Metal engine shutdown\n");
    free(impl);
}

int metal_engine_load_metal_with_view(MetalEngine* engine, MetalViewHandle view) {
    if (!engine || !view) return 0;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    MTKView* mtkView = (__bridge MTKView*)view;
    
    // Get device from view
    impl->device = mtkView.device;
    if (!impl->device) {
        fprintf(stderr, "Failed to get Metal device from view\n");
        return 0;
    }
    
    // Configure view properties
    mtkView.depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    mtkView.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    mtkView.sampleCount = 1;
    
    // Create command queue
    impl->commandQueue = [impl->device newCommandQueue];
    if (!impl->commandQueue) {
        fprintf(stderr, "Failed to create command queue\n");
        return 0;
    }
    
    // Create vertex descriptor
    impl->mtlVertexDescriptor = [[MTLVertexDescriptor alloc] init];
    
    impl->mtlVertexDescriptor.attributes[VertexAttributePosition].format = MTLVertexFormatFloat3;
    impl->mtlVertexDescriptor.attributes[VertexAttributePosition].offset = 0;
    impl->mtlVertexDescriptor.attributes[VertexAttributePosition].bufferIndex = 0; // Use buffer 0 for all vertex data
    
    impl->mtlVertexDescriptor.attributes[VertexAttributeTexcoord].format = MTLVertexFormatFloat2;
    impl->mtlVertexDescriptor.attributes[VertexAttributeTexcoord].offset = 12; // After position (3 floats * 4 bytes)
    impl->mtlVertexDescriptor.attributes[VertexAttributeTexcoord].bufferIndex = 0; // Use buffer 0 for all vertex data
    
    impl->mtlVertexDescriptor.attributes[VertexAttributeNormal].format = MTLVertexFormatFloat3;
    impl->mtlVertexDescriptor.attributes[VertexAttributeNormal].offset = 20; // After texcoord (3 + 2 floats * 4 bytes)
    impl->mtlVertexDescriptor.attributes[VertexAttributeNormal].bufferIndex = 0; // Use buffer 0 for all vertex data
    
    impl->mtlVertexDescriptor.layouts[0].stride = 32; // 8 floats * 4 bytes (3 pos + 2 tex + 3 normal)
    impl->mtlVertexDescriptor.layouts[0].stepRate = 1;
    impl->mtlVertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    
    // Create uniform buffer
    NSUInteger uniformBufferSize = kAlignedUniformsSize * kMaxBuffersInFlight;
    impl->dynamicUniformBuffer = [impl->device newBufferWithLength:uniformBufferSize
                                                           options:MTLResourceStorageModeShared];
    impl->dynamicUniformBuffer.label = @"UniformBuffer";
    
    // Create depth stencil state
    MTLDepthStencilDescriptor* depthStateDesc = [[MTLDepthStencilDescriptor alloc] init];
    depthStateDesc.depthCompareFunction = MTLCompareFunctionLess;
    depthStateDesc.depthWriteEnabled = YES;
    impl->depthState = [impl->device newDepthStencilStateWithDescriptor:depthStateDesc];
    
    // Create render pipeline state (required for rendering)
    if (!metal_engine_create_pipeline(engine)) {
        fprintf(stderr, "Failed to create render pipeline state\n");
        return 0;
    }
    
    // Initialize Metal 3.0 features
    metal_engine_enable_object_capture(engine);
    metal_engine_enable_mesh_shading(engine);
    metal_engine_enable_dynamic_libraries(engine);
    metal_engine_report_metal_features(engine);
    
    fprintf(stderr, "Metal state loaded successfully\n");
    return 1;
}

int metal_engine_create_buffers(MetalEngine* engine) {
    if (!engine) return 0;
    
    // MetalEngineImpl* impl = (MetalEngineImpl*)engine; // Unused variable
    
    // Note: In a real implementation, this would create Metal buffers
    // For now, we just mark it as successful
    
    fprintf(stderr, "Metal buffer creation requested\n");
    return 1;
}

int metal_engine_create_pipeline(MetalEngine* engine) {
    if (!engine) return 0;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    // Get default library
    id<MTLLibrary> defaultLibrary = [impl->device newDefaultLibrary];
    if (!defaultLibrary) {
        fprintf(stderr, "Failed to create default library\n");
        return 0;
    }
    
    // Get vertex and fragment functions
    id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragmentShader"];
    
    if (!vertexFunction || !fragmentFunction) {
        fprintf(stderr, "Failed to get shader functions\n");
        return 0;
    }
    
    // Create pipeline state descriptor
    MTLRenderPipelineDescriptor* pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineStateDescriptor.label = @"MyPipeline";
    pipelineStateDescriptor.rasterSampleCount = 1;
    pipelineStateDescriptor.vertexFunction = vertexFunction;
    pipelineStateDescriptor.fragmentFunction = fragmentFunction;
    pipelineStateDescriptor.vertexDescriptor = impl->mtlVertexDescriptor;
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    pipelineStateDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    pipelineStateDescriptor.stencilAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    
    // Create pipeline state
    NSError* error = NULL;
    impl->renderPipelineState = [impl->device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    if (!impl->renderPipelineState) {
        fprintf(stderr, "Failed to create pipeline state: %s\n", error.localizedDescription.UTF8String);
        return 0;
    }
    
    fprintf(stderr, "Metal pipeline created successfully\n");
    return 1;
}

int metal_engine_create_mesh(MetalEngine* engine) {
    if (!engine) return 0;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    NSError* error;
    
    // Create cube vertices manually with exact format we need
    // Format: position (3 floats) + texcoord (2 floats) + normal (3 floats) = 8 floats = 32 bytes
    
    // Cube vertices: 8 vertices for a cube
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
    
    // Cube indices: 12 triangles (6 faces * 2 triangles each)
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
    id<MTLBuffer> vertexBuffer = [impl->device newBufferWithBytes:vertices
                                                           length:sizeof(vertices)
                                                          options:MTLResourceStorageModeShared];
    vertexBuffer.label = @"CubeVertices";
    
    // Create index buffer
    id<MTLBuffer> indexBuffer = [impl->device newBufferWithBytes:indices
                                                          length:sizeof(indices)
                                                         options:MTLResourceStorageModeShared];
    indexBuffer.label = @"CubeIndices";
    
    // Store the buffers for rendering
    impl->vertexBuffer = vertexBuffer;
    impl->indexBuffer = indexBuffer;
    impl->indexCount = sizeof(indices) / sizeof(uint16_t);
    
    // Set mesh to NULL since we're not using MTKMesh anymore
    impl->mesh = nil;
    
    fprintf(stderr, "Cube mesh created successfully\n");
    fprintf(stderr, "Vertex count: 8, Index count: %d\n", impl->indexCount);
    
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
            fprintf(stderr, "Invalid mesh %u for upload\n", i);
            continue;
        }
        
        // Convert vertices to Metal format (position + texcoord + normal)
        size_t vertexDataSize = mesh->vertex_count * sizeof(float) * 8; // 8 floats per vertex
        float* vertexData = (float*)malloc(vertexDataSize);
        if (!vertexData) {
            fprintf(stderr, "Failed to allocate vertex data for mesh %u\n", i);
            continue;
        }
        
        // Pack vertex data: position(3) + texcoord(2) + normal(3) = 8 floats
        for (uint32_t j = 0; j < mesh->vertex_count; j++) {
            Vertex* vertex = &mesh->vertices[j];
            float* dst = vertexData + j * 8;
            
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
        
        // Create vertex buffer
        id<MTLBuffer> vertexBuffer = [impl->device newBufferWithBytes:vertexData
                                                               length:vertexDataSize
                                                              options:MTLResourceStorageModeShared];
        vertexBuffer.label = [NSString stringWithFormat:@"%@_Mesh%u_Vertices", 
                              [NSString stringWithUTF8String:metalModel->name], i];
        
        // Create index buffer
        size_t indexBufferSize = mesh->index_count * sizeof(uint32_t);
        fprintf(stderr, "Creating index buffer: mesh->index_count=%u, sizeof(uint32_t)=%zu, total size=%zu\n", 
                mesh->index_count, sizeof(uint32_t), indexBufferSize);
        fprintf(stderr, "mesh->indices pointer: %p\n", mesh->indices);
        if (mesh->indices && mesh->index_count > 0) {
            fprintf(stderr, "First few indices: %u, %u, %u, %u, %u\n", 
                    mesh->indices[0], mesh->indices[1], mesh->indices[2], mesh->indices[3], mesh->indices[4]);
        }
        id<MTLBuffer> indexBuffer = [impl->device newBufferWithBytes:mesh->indices
                                                             length:indexBufferSize
                                                            options:MTLResourceStorageModeShared];
        indexBuffer.label = [NSString stringWithFormat:@"%@_Mesh%u_Indices", 
                            [NSString stringWithUTF8String:metalModel->name], i];
        fprintf(stderr, "Created index buffer with length: %lu\n", (unsigned long)indexBuffer.length);
        
        // Store buffers
        metalModel->vertexBuffers[i] = vertexBuffer;
        metalModel->indexBuffers[i] = indexBuffer;
        metalModel->indexCounts[i] = mesh->index_count;
        
        fprintf(stderr, "Stored buffers for mesh %u: vertexBuffer=%p, indexBuffer=%p, length=%lu\n", 
                i, vertexBuffer, indexBuffer, (unsigned long)indexBuffer.length);
        
        // Free temporary vertex data
        free(vertexData);
        
        fprintf(stderr, "Uploaded mesh %u: %u vertices, %u indices\n", 
                i, mesh->vertex_count, mesh->index_count);
    }
    
    fprintf(stderr, "Successfully uploaded model '%s' with %u meshes\n", 
            metalModel->name, metalModel->meshCount);
    
    return (MetalModelHandle)metalModel;
}

// Set the uploaded model for rendering
void metal_engine_set_uploaded_model(MetalEngine* engine, MetalModelHandle model) {
    if (!engine) return;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    // Release previous model if any
    if (impl->uploadedModel) {
        metal_engine_free_model((MetalModelHandle)impl->uploadedModel);
    }
    
    impl->uploadedModel = (MetalModel*)model;
    fprintf(stderr, "Set uploaded model for rendering: %s\n", 
            impl->uploadedModel ? impl->uploadedModel->name : "NULL");
}

// Render a specific model (direct Metal encoder version)
void metal_engine_render_model_direct(MetalEngine* engine, MetalModelHandle model, void* renderEncoder) {
    if (!engine || !model || !renderEncoder) {
        fprintf(stderr, "Invalid parameters for model rendering\n");
        return;
    }
    
    // MetalEngineImpl* impl = (MetalEngineImpl*)engine; // Unused variable
    MetalModel* metalModel = (MetalModel*)model;
    id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)renderEncoder;
    
    fprintf(stderr, "Rendering model: %s with %u meshes\n", metalModel->name, metalModel->meshCount);
    
    // Render each mesh
    for (uint32_t i = 0; i < metalModel->meshCount; i++) {
        id<MTLBuffer> vertexBuffer = metalModel->vertexBuffers[i];
        id<MTLBuffer> indexBuffer = metalModel->indexBuffers[i];
        uint32_t indexCount = metalModel->indexCounts[i];
        
        if (!vertexBuffer || !indexBuffer || indexCount == 0) {
            fprintf(stderr, "Invalid mesh %u for rendering\n", i);
            continue;
        }
        
        fprintf(stderr, "Rendering mesh %u: vertexBuffer=%p, indexBuffer=%p, indexCount=%u\n", 
                i, vertexBuffer, indexBuffer, indexCount);
        
        // Set vertex buffer
        [encoder setVertexBuffer:vertexBuffer offset:0 atIndex:BufferIndexVertices];
        
        // Draw indexed primitives
        fprintf(stderr, "Drawing mesh %u: indexCount=%u, indexBuffer.length=%lu\n", 
                i, indexCount, (unsigned long)indexBuffer.length);
        fprintf(stderr, "Buffer pointer during draw: %p\n", indexBuffer);
        [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                indexCount:indexCount
                                 indexType:MTLIndexTypeUInt32
                               indexBuffer:indexBuffer
                         indexBufferOffset:0];
    }
}

// Render a specific model
void metal_engine_render_model(MetalEngine* engine, MetalModelHandle model, MetalRenderCommandEncoderHandle renderEncoder) {
    if (!engine || !model || !renderEncoder) {
        fprintf(stderr, "Invalid parameters for model rendering\n");
        return;
    }
    
    // MetalEngineImpl* impl = (MetalEngineImpl*)engine; // Unused variable
    MetalModel* metalModel = (MetalModel*)model;
    id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)renderEncoder;
    
    // Render each mesh
    for (uint32_t i = 0; i < metalModel->meshCount; i++) {
        id<MTLBuffer> vertexBuffer = metalModel->vertexBuffers[i];
        id<MTLBuffer> indexBuffer = metalModel->indexBuffers[i];
        uint32_t indexCount = metalModel->indexCounts[i];
        
        if (!vertexBuffer || !indexBuffer || indexCount == 0) {
            fprintf(stderr, "Invalid mesh %u for rendering\n", i);
            continue;
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
    
    NSError* error;
    
    // Create texture loader
    MTKTextureLoader* textureLoader = [[MTKTextureLoader alloc] initWithDevice:impl->device];
    
    NSDictionary* textureLoaderOptions = @{
        MTKTextureLoaderOptionTextureUsage: @(MTLTextureUsageShaderRead),
        MTKTextureLoaderOptionTextureStorageMode: @(MTLStorageModePrivate)
    };
    
    // Try loading from asset catalog first
    impl->colorMap = [textureLoader newTextureWithName:@"ColorMap"
                                          scaleFactor:1.0
                                               bundle:[NSBundle mainBundle]
                                              options:textureLoaderOptions
                                                error:&error];
    
    // If that fails, try loading from the main bundle
    if (!impl->colorMap) {
        fprintf(stderr, "Trying to load texture from main bundle...\n");
        impl->colorMap = [textureLoader newTextureWithName:@"ColorMap"
                                              scaleFactor:1.0
                                                   bundle:[NSBundle mainBundle]
                                                  options:textureLoaderOptions
                                                    error:&error];
    }
    
    // If still no success, try loading the PNG file directly
    if (!impl->colorMap) {
        fprintf(stderr, "Trying to load PNG file directly...\n");
        NSString* texturePath = [[NSBundle mainBundle] pathForResource:@"ColorMap" ofType:@"png"];
        if (texturePath) {
            fprintf(stderr, "Found texture at path: %s\n", texturePath.UTF8String);
            impl->colorMap = [textureLoader newTextureWithContentsOfURL:[NSURL fileURLWithPath:texturePath]
                                                              options:textureLoaderOptions
                                                                error:&error];
            if (impl->colorMap) {
                fprintf(stderr, "Texture loaded successfully from PNG file\n");
            } else {
                fprintf(stderr, "Failed to load texture from PNG file: %s\n", error.localizedDescription.UTF8String);
            }
        } else {
            fprintf(stderr, "Could not find ColorMap.png in bundle - will create programmatic texture\n");
        }
    }
    
    // If still no success, create fallback texture
    if (!impl->colorMap) {
        fprintf(stderr, "Creating fallback colored texture...\n");
        impl->colorMap = (__bridge id<MTLTexture>)metal_engine_create_fallback_texture((__bridge MetalDeviceHandle)impl->device);
    }
    
    if (!impl->colorMap) {
        fprintf(stderr, "Error creating texture\n");
        return 0;
    }
    
    fprintf(stderr, "Texture loaded successfully\n");
    fprintf(stderr, "Texture dimensions: %lu x %lu\n", (unsigned long)impl->colorMap.width, (unsigned long)impl->colorMap.height);
    fprintf(stderr, "Texture pixel format: %lu\n", (unsigned long)impl->colorMap.pixelFormat);
    
    
    
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
    
    impl->uniformBufferIndex = (impl->uniformBufferIndex + 1) % kMaxBuffersInFlight;
    impl->uniformBufferOffset = kAlignedUniformsSize * impl->uniformBufferIndex;
    impl->uniformBufferAddress = ((uint8_t*)impl->dynamicUniformBuffer.contents) + impl->uniformBufferOffset;
}

void metal_engine_update_game_state(MetalEngine* engine) {
    if (!engine) return;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    MetalUniforms* uniforms = (MetalUniforms*)impl->uniformBufferAddress;
    
    // Set projection matrix
    memcpy(uniforms->projectionMatrix, &impl->projectionMatrix, sizeof(float) * 16);
    
    // Create model matrix - rotate around the object's center
    vec3_t rotationAxis = vec3(1.0f, 1.0f, 0.0f);
    mat4_t modelMatrix = metal_engine_matrix_rotation(impl->rotationAngle, rotationAxis);
    
    // Create view matrix - move camera back from origin
    mat4_t viewMatrix = metal_engine_matrix_translation(0.0f, 0.0f, -8.0f);
    
    // For modelViewMatrix, we want to apply view transformation first, then model transformation
    // This ensures the object rotates around its own center, not around the camera
    mat4_t modelViewMatrix = mat4_mul_mat4(viewMatrix, modelMatrix);
    
    memcpy(uniforms->modelViewMatrix, &modelViewMatrix, sizeof(float) * 16);
    memcpy(uniforms->modelMatrix, &modelMatrix, sizeof(float) * 16);
    memcpy(uniforms->viewMatrix, &viewMatrix, sizeof(float) * 16);
    
    // Calculate normal matrix
    mat4_t normalMatrix = mat4_inverse(mat4_transpose(modelMatrix));
    memcpy(uniforms->normalMatrix, &normalMatrix, sizeof(float) * 16);
    
    // Calculate camera position
    mat4_t invViewMatrix = mat4_inverse(viewMatrix);
    vec4_t cameraPos = invViewMatrix.w;
    uniforms->cameraPosition[0] = cameraPos.x;
    uniforms->cameraPosition[1] = cameraPos.y;
    uniforms->cameraPosition[2] = cameraPos.z;
    
    uniforms->time = impl->rotationAngle;
    
    impl->rotationAngle += 0.01f;
}

void metal_engine_update_game_state_from_engine_state(MetalEngine* engine, void* engineState) {
    // Function called silently
    
    if (!engine || !engineState) {
        NSLog(@"ERROR: engine=%p, engineState=%p", engine, engineState);
        return;
    }
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    MetalUniforms* uniforms = (MetalUniforms*)impl->uniformBufferAddress;
    
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
    uniforms->time = impl->rotationAngle;
    
    // Matrix values updated silently
}

void metal_engine_render_frame(MetalEngine* engine, MetalViewHandle view, void* engineState) {
    /* NSLog(@"=== RENDER FRAME START ==="); */
    
    if (!engine || !view || !engineState) {
        NSLog(@"ERROR: Render frame: engine=%p, view=%p, or engineState=%p is NULL", engine, view, engineState);
        return;
    }
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    MTKView* mtkView = (__bridge MTKView*)view;
    
    /* NSLog(@"Render frame: engine=%p, view=%p, mtkView=%p, engineState=%p", engine, view, mtkView, engineState); */
    
    // Wait for available buffer
    // Note: In a real implementation, we would use a semaphore here
    
    // Create command buffer
    id<MTLCommandBuffer> commandBuffer = [impl->commandQueue commandBuffer];
    commandBuffer.label = @"MyCommand";
    
    /* NSLog(@"About to update buffer state..."); */
    
    // Update buffer state
    metal_engine_update_dynamic_buffer_state(engine);
    
    /* NSLog(@"About to update game state..."); */
    
    // Update game state using engine state directly
    metal_engine_update_game_state_from_engine_state(engine, engineState);
    
    /* NSLog(@"About to get render pass descriptor..."); */
    
    // Get render pass descriptor
    MTLRenderPassDescriptor* renderPassDescriptor = mtkView.currentRenderPassDescriptor;
    
    /* NSLog(@"Render pass descriptor: %p", renderPassDescriptor); */
    
    if (renderPassDescriptor != nil) {
        /* fprintf(stderr, "Creating render encoder...\n"); */
        
        // Create render command encoder
        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        renderEncoder.label = @"MyRenderEncoder";
        
        /* fprintf(stderr, "Render encoder: %p, pipeline state: %p, depth state: %p\n", 
                renderEncoder, impl->renderPipelineState, impl->depthState); */
        
        [renderEncoder pushDebugGroup:@"DrawBox"];
        
        [renderEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
        [renderEncoder setCullMode:MTLCullModeBack];
        [renderEncoder setRenderPipelineState:impl->renderPipelineState];
        [renderEncoder setDepthStencilState:impl->depthState];
        
        // Set uniform buffer
        [renderEncoder setVertexBuffer:impl->dynamicUniformBuffer
                                offset:impl->uniformBufferOffset
                               atIndex:BufferIndexUniforms];
        
        [renderEncoder setFragmentBuffer:impl->dynamicUniformBuffer
                                  offset:impl->uniformBufferOffset
                                 atIndex:BufferIndexUniforms];
        
        // Set texture
        [renderEncoder setFragmentTexture:impl->colorMap atIndex:TextureIndexColorMap];
        
        // Check if we have any entities to render
        EngineStateStruct* engineStateStruct = (EngineStateStruct*)engineState;
        uint32_t entityCount = engineStateStruct && engineStateStruct->world ? world_get_entity_count(engineStateStruct->world) : 0;
        
        // Metal render frame called silently
        
        if (entityCount > 0) {
            // Render entities using the world system
            // Rendering entities silently
            world_render(engineStateStruct->world, engine, engineState);
            
            // After world rendering, render the first entity's model
            if (engineStateStruct->world) {
                for (uint32_t i = 0; i < engineStateStruct->world->max_entities; i++) {
                    WorldEntity* entity = &engineStateStruct->world->entities[i];
                    if (entity->id != 0 && entity->is_active && entity->metal_model) {
                        // Render the entity's model
                        fprintf(stderr, "About to render entity %u model: %p\n", entity->id, entity->metal_model);
                        metal_engine_render_model_direct(engine, entity->metal_model, (__bridge void*)renderEncoder);
                        break; // Only render the first entity for now
                    }
                }
            }
        } else if (impl->uploadedModel) {
            // Fallback: render uploaded model if no entities but model exists
            /* fprintf(stderr, "Rendering uploaded model: %s\n", impl->uploadedModel->name); */
            metal_engine_render_model_direct(engine, (MetalModelHandle)impl->uploadedModel, (__bridge void*)renderEncoder);
        } else {
            // No entities and no uploaded model - just clear the screen
            fprintf(stderr, "\rNo entities to render, clearing screen");
            // Don't draw anything, just let the clear color show through
        }
        
        [renderEncoder popDebugGroup];
        [renderEncoder endEncoding];
        
        // Present drawable
        [commandBuffer presentDrawable:mtkView.currentDrawable];
    }
    
    [commandBuffer commit];
    
    impl->frameCount++;
}

void metal_engine_resize_viewport(MetalEngine* engine, int width, int height) {
    if (!engine) return;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    impl->viewportWidth = width;
    impl->viewportHeight = height;
    
    float aspect = (float)width / (float)height;
    impl->projectionMatrix = metal_engine_matrix_perspective_right_hand(65.0f * (M_PI / 180.0f), aspect, 0.1f, 100.0f);
    
    fprintf(stderr, "Viewport resized: %dx%d\n", width, height);
}



// Metal 3.0 feature enablement
void metal_engine_enable_object_capture(MetalEngine* engine) {
    if (!engine) return;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    if (@available(macOS 15.0, *)) {
        impl->supportsObjectCapture = 1;
        fprintf(stderr, "Object capture enabled for Metal 3.0\n");
    }
}

void metal_engine_enable_mesh_shading(MetalEngine* engine) {
    if (!engine) return;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    if (@available(macOS 15.0, *)) {
        impl->supportsMeshShading = 1;
        fprintf(stderr, "Mesh shading enabled for Metal 3.0\n");
    }
}

void metal_engine_enable_dynamic_libraries(MetalEngine* engine) {
    if (!engine) return;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    if (@available(macOS 15.0, *)) {
        impl->supportsDynamicLibraries = 1;
        fprintf(stderr, "Dynamic libraries enabled for Metal 3.0\n");
    }
}

void metal_engine_report_metal_features(MetalEngine* engine) {
    if (!engine) return;
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
    if (@available(macOS 15.0, *)) {
        fprintf(stderr, "=== Metal 3.0 Feature Report ===\n");
        fprintf(stderr, "Device: %s\n", impl->device.name.UTF8String);
        fprintf(stderr, "Max Threads Per Threadgroup: %lu\n", (unsigned long)impl->device.maxThreadsPerThreadgroup.width);
        fprintf(stderr, "Max Threads Per Threadgroup (3D): %lu\n", (unsigned long)impl->device.maxThreadsPerThreadgroup.depth);
        fprintf(stderr, "=================================\n");
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
                                                                                            width:512
                                                                                           height:512
                                                                                        mipmapped:NO];
    fallbackDesc.usage = MTLTextureUsageShaderRead;
    
    id<MTLTexture> fallbackTexture = [mtlDevice newTextureWithDescriptor:fallbackDesc];
    fallbackTexture.label = @"FallbackColorMap";
    
    // Fill with checkerboard pattern
    uint8_t* textureData = malloc(512 * 512 * 4);
    for (int y = 0; y < 512; y++) {
        for (int x = 0; x < 512; x++) {
            int index = (y * 512 + x) * 4;
            int checker = ((x / 64) + (y / 64)) % 2;
            
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
    
    [fallbackTexture replaceRegion:MTLRegionMake2D(0, 0, 512, 512)
                       mipmapLevel:0
                         withBytes:textureData
                       bytesPerRow:512 * 4];
    
    free(textureData);
    fprintf(stderr, "Fallback texture created successfully with checkerboard pattern\n");
    
    return (__bridge_retained MetalTextureHandle)fallbackTexture;
}

void metal_engine_print_device_info(MetalDeviceHandle device) {
    if (!device) return;
    
    id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
    fprintf(stderr, "Device info print requested for: %s\n", mtlDevice.name.UTF8String);
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
    impl->engineState = engineState;
}
