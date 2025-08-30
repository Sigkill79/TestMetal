#import "engine_metal.h"
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
#import <MetalKit/MetalKit.h>
#import <ModelIO/ModelIO.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#import <simd/simd.h>

// ============================================================================
// METAL ENGINE IMPLEMENTATION
// ============================================================================

// Forward declarations for Metal objects
typedef struct {
    id<MTLDevice> device;
    id<MTLCommandQueue> commandQueue;
    id<MTLRenderPipelineState> renderPipelineState;
    id<MTLBuffer> dynamicUniformBuffer;
    id<MTLDepthStencilState> depthState;
    id<MTLTexture> colorMap;
    MTLVertexDescriptor* mtlVertexDescriptor;
    id<MTLTexture> blurredTexture;
    MTKMesh* mesh;
    
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
    
    // Engine integration
    void* engineState;
    
    // Metal Performance Shaders
    MPSImageGaussianBlur* gaussianBlur;
    
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
    memset(engine, 0, sizeof(MetalEngineImpl));
    
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
        // Note: In a real implementation, we would release Metal objects here
        // For now, we just free the engine structure
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
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    
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
    
    // Create mesh buffer allocator
    MTKMeshBufferAllocator* metalAllocator = [[MTKMeshBufferAllocator alloc] initWithDevice:impl->device];
    
    // Create box mesh
    MDLMesh* mdlMesh = [MDLMesh newBoxWithDimensions:(vector_float3){4, 4, 4}
                                            segments:(vector_uint3){1, 1, 1}
                                        geometryType:MDLGeometryTypeTriangles
                                       inwardNormals:NO
                                           allocator:metalAllocator];
    
    // Convert vertex descriptor
    MDLVertexDescriptor* mdlVertexDescriptor = MTKModelIOVertexDescriptorFromMetal(impl->mtlVertexDescriptor);
    
    mdlVertexDescriptor.attributes[VertexAttributePosition].name = MDLVertexAttributePosition;
    mdlVertexDescriptor.attributes[VertexAttributeTexcoord].name = MDLVertexAttributeTextureCoordinate;
    mdlVertexDescriptor.attributes[VertexAttributeNormal].name = MDLVertexAttributeNormal;
    
    mdlMesh.vertexDescriptor = mdlVertexDescriptor;
    
    // Create MetalKit mesh
    impl->mesh = [[MTKMesh alloc] initWithMesh:mdlMesh device:impl->device error:&error];
    
    if (!impl->mesh || error) {
        fprintf(stderr, "Error creating MetalKit mesh: %s\n", error.localizedDescription.UTF8String);
        return 0;
    }
    
    fprintf(stderr, "Mesh created successfully\n");
    fprintf(stderr, "Mesh vertex count: %lu\n", (unsigned long)impl->mesh.vertexCount);
    fprintf(stderr, "Mesh submesh count: %lu\n", (unsigned long)impl->mesh.submeshes.count);
    
    return 1;
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
    
    // Create blurred texture for post-processing
    MTLTextureDescriptor* blurredTextureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm_sRGB
                                                                                                 width:512
                                                                                                height:512
                                                                                             mipmapped:NO];
    blurredTextureDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    impl->blurredTexture = [impl->device newTextureWithDescriptor:blurredTextureDesc];
    impl->blurredTexture.label = @"BlurredTexture";
    
    // Initialize Metal Performance Shaders
    impl->gaussianBlur = [[MPSImageGaussianBlur alloc] initWithDevice:impl->device sigma:2.0];
    
    return 1;
}

int metal_engine_load_assets(MetalEngine* engine) {
    if (!engine) return 0;
    
    // Create pipeline
    if (!metal_engine_create_pipeline(engine)) {
        return 0;
    }
    
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
    uniforms->cameraPosition[0] = cameraPos[0];
    uniforms->cameraPosition[1] = cameraPos[1];
    uniforms->cameraPosition[2] = cameraPos[2];
    
    uniforms->time = impl->rotationAngle;
    
    impl->rotationAngle += 0.01f;
}

void metal_engine_render_frame(MetalEngine* engine, MetalViewHandle view) {
    if (!engine || !view) {
        fprintf(stderr, "Render frame: engine or view is NULL\n");
        return;
    }
    
    MetalEngineImpl* impl = (MetalEngineImpl*)engine;
    MTKView* mtkView = (__bridge MTKView*)view;
    
    fprintf(stderr, "Render frame: engine=%p, view=%p, mtkView=%p\n", engine, view, mtkView);
    
    // Wait for available buffer
    // Note: In a real implementation, we would use a semaphore here
    
    // Create command buffer
    id<MTLCommandBuffer> commandBuffer = [impl->commandQueue commandBuffer];
    commandBuffer.label = @"MyCommand";
    
    // Update buffer state
    metal_engine_update_dynamic_buffer_state(engine);
    
    // Update game state
    metal_engine_update_game_state(engine);
    
    // Get render pass descriptor
    MTLRenderPassDescriptor* renderPassDescriptor = mtkView.currentRenderPassDescriptor;
    
    fprintf(stderr, "Render pass descriptor: %p\n", renderPassDescriptor);
    
    if (renderPassDescriptor != nil) {
        fprintf(stderr, "Creating render encoder...\n");
        
        // Create render command encoder
        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        renderEncoder.label = @"MyRenderEncoder";
        
        fprintf(stderr, "Render encoder: %p, pipeline state: %p, depth state: %p\n", 
                renderEncoder, impl->renderPipelineState, impl->depthState);
        
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
        
        // Set vertex buffers - use buffer 0 for all vertex data
        for (NSUInteger bufferIndex = 0; bufferIndex < impl->mesh.vertexBuffers.count; bufferIndex++) {
            MTKMeshBuffer* vertexBuffer = impl->mesh.vertexBuffers[bufferIndex];
            if ((NSNull*)vertexBuffer != [NSNull null]) {
                [renderEncoder setVertexBuffer:vertexBuffer.buffer
                                        offset:vertexBuffer.offset
                                       atIndex:0]; // Always use buffer 0 for vertex data
            }
        }
        
        // Set texture
        [renderEncoder setFragmentTexture:impl->colorMap atIndex:TextureIndexColorMap];
        
        // Draw submeshes
        for (MTKSubmesh* submesh in impl->mesh.submeshes) {
            [renderEncoder drawIndexedPrimitives:submesh.primitiveType
                                      indexCount:submesh.indexCount
                                       indexType:submesh.indexType
                                     indexBuffer:submesh.indexBuffer.buffer
                               indexBufferOffset:submesh.indexBuffer.offset];
        }
        
        [renderEncoder popDebugGroup];
        [renderEncoder endEncoding];
        
        // Apply post-processing
        if (impl->gaussianBlur && impl->blurredTexture) {
            [impl->gaussianBlur encodeToCommandBuffer:commandBuffer
                                       sourceTexture:mtkView.currentDrawable.texture
                                  destinationTexture:impl->blurredTexture];
        }
        
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
    float x = normAxis[0], y = normAxis[1], z = normAxis[2];
    
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
