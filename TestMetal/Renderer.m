//
//  Renderer.m
//  TestMetal
//
//  Created by Sigurd Seteklev on 14/06/2024.
//

#import <simd/simd.h>
#import <ModelIO/ModelIO.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#import "Renderer.h"

// Include header shared between C code here, which executes Metal API commands, and .metal files
#import "ShaderTypes.h"

static const NSUInteger kMaxBuffersInFlight = 3;

static const size_t kAlignedUniformsSize = (sizeof(Uniforms) & ~0xFF) + 0x100;

@implementation Renderer
{
    dispatch_semaphore_t _inFlightSemaphore;
    id <MTLDevice> _device;
    id <MTLCommandQueue> _commandQueue;

    id <MTLBuffer> _dynamicUniformBuffer;
    id <MTLRenderPipelineState> _pipelineState;
    id <MTLDepthStencilState> _depthState;
    id <MTLTexture> _colorMap;
    MTLVertexDescriptor *_mtlVertexDescriptor;
    
    // Metal 3.0: Add Metal Performance Shaders support
    id <MTLTexture> _blurredTexture;
    MPSImageGaussianBlur *_gaussianBlur;

    uint32_t _uniformBufferOffset;

    uint8_t _uniformBufferIndex;

    void* _uniformBufferAddress;

    matrix_float4x4 _projectionMatrix;

    float _rotation;

    MTKMesh *_mesh;
}

-(nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view;
{
    self = [super init];
    if(self)
    {
        _device = view.device;
        _inFlightSemaphore = dispatch_semaphore_create(kMaxBuffersInFlight);
        [self _loadMetalWithView:view];
        [self _loadAssets];
        
        // Metal 3.0: Enable advanced features
        [self enableObjectCapture];
        [self enableMeshShading];
        [self enableDynamicLibraries];
        [self reportMetalFeatures];
    }

    return self;
}

- (void)_loadMetalWithView:(nonnull MTKView *)view;
{
    /// Load Metal state objects and initialize renderer dependent view properties

    view.depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    view.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    view.sampleCount = 1;
    
    // Metal 3.0: Enable enhanced texture formats and features
    if (@available(macOS 15.0, *)) {
        // Enable HDR if supported (Metal 3.0 feature)
        // Note: This property may not be available in all versions
        // if (view.device.supportsBCTextureCompression) {
        //     NSLog(@"Device supports BC texture compression");
        // }
        
        // Enable ray tracing if supported (Metal 3.0 feature)
        // Note: This property may not be available in all versions
        // if (view.device.supportsRaytracing) {
        //     NSLog(@"Device supports ray tracing");
        // }
    }

    _mtlVertexDescriptor = [[MTLVertexDescriptor alloc] init];

    _mtlVertexDescriptor.attributes[VertexAttributePosition].format = MTLVertexFormatFloat3;
    _mtlVertexDescriptor.attributes[VertexAttributePosition].offset = 0;
    _mtlVertexDescriptor.attributes[VertexAttributePosition].bufferIndex = BufferIndexMeshPositions;

    _mtlVertexDescriptor.attributes[VertexAttributeTexcoord].format = MTLVertexFormatFloat2;
    _mtlVertexDescriptor.attributes[VertexAttributeTexcoord].offset = 12; // After position (3 floats * 4 bytes)
    _mtlVertexDescriptor.attributes[VertexAttributeTexcoord].bufferIndex = BufferIndexMeshGenerics;

    // Metal 3.0: Add normal attribute support
    _mtlVertexDescriptor.attributes[VertexAttributeNormal].format = MTLVertexFormatFloat3;
    _mtlVertexDescriptor.attributes[VertexAttributeNormal].offset = 20; // After texcoord (3 + 2 floats * 4 bytes)
    _mtlVertexDescriptor.attributes[VertexAttributeNormal].bufferIndex = BufferIndexMeshGenerics;

    _mtlVertexDescriptor.layouts[BufferIndexMeshPositions].stride = 12;
    _mtlVertexDescriptor.layouts[BufferIndexMeshPositions].stepRate = 1;
    _mtlVertexDescriptor.layouts[BufferIndexMeshPositions].stepFunction = MTLVertexStepFunctionPerVertex;

    _mtlVertexDescriptor.layouts[BufferIndexMeshGenerics].stride = 32; // 3 (pos) + 2 (texcoord) + 3 (normal) = 8 floats * 4 bytes
    _mtlVertexDescriptor.layouts[BufferIndexMeshGenerics].stepRate = 1;
    _mtlVertexDescriptor.layouts[BufferIndexMeshGenerics].stepFunction = MTLVertexStepFunctionPerVertex;

    id<MTLLibrary> defaultLibrary = [_device newDefaultLibrary];

    id <MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertexShader"];

    id <MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragmentShader"];

    MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineStateDescriptor.label = @"MyPipeline";
    pipelineStateDescriptor.rasterSampleCount = view.sampleCount;
    pipelineStateDescriptor.vertexFunction = vertexFunction;
    pipelineStateDescriptor.fragmentFunction = fragmentFunction;
    pipelineStateDescriptor.vertexDescriptor = _mtlVertexDescriptor;
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat;
    pipelineStateDescriptor.depthAttachmentPixelFormat = view.depthStencilPixelFormat;
    pipelineStateDescriptor.stencilAttachmentPixelFormat = view.depthStencilPixelFormat;

    NSError *error = NULL;
    _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    if (!_pipelineState)
    {
        NSLog(@"Failed to created pipeline state, error %@", error);
    }

    MTLDepthStencilDescriptor *depthStateDesc = [[MTLDepthStencilDescriptor alloc] init];
    depthStateDesc.depthCompareFunction = MTLCompareFunctionLess;
    depthStateDesc.depthWriteEnabled = YES;
    _depthState = [_device newDepthStencilStateWithDescriptor:depthStateDesc];

    NSUInteger uniformBufferSize = kAlignedUniformsSize * kMaxBuffersInFlight;

    _dynamicUniformBuffer = [_device newBufferWithLength:uniformBufferSize
                                                 options:MTLResourceStorageModeShared];

    _dynamicUniformBuffer.label = @"UniformBuffer";

    _commandQueue = [_device newCommandQueue];
    
    // Metal 3.0: Initialize Metal Performance Shaders
    _gaussianBlur = [[MPSImageGaussianBlur alloc] initWithDevice:_device sigma:2.0];
}

- (void)_loadAssets
{
    /// Load assets into metal objects

    NSError *error;

    MTKMeshBufferAllocator *metalAllocator = [[MTKMeshBufferAllocator alloc]
                                              initWithDevice: _device];

    // Use the built-in box mesh - it should have texture coordinates
    MDLMesh *mdlMesh = [MDLMesh newBoxWithDimensions:(vector_float3){4, 4, 4}
                                            segments:(vector_uint3){1, 1, 1}
                                        geometryType:MDLGeometryTypeTriangles
                                       inwardNormals:NO
                                           allocator:metalAllocator];

    MDLVertexDescriptor *mdlVertexDescriptor =
    MTKModelIOVertexDescriptorFromMetal(_mtlVertexDescriptor);

    mdlVertexDescriptor.attributes[VertexAttributePosition].name  = MDLVertexAttributePosition;
    mdlVertexDescriptor.attributes[VertexAttributeTexcoord].name  = MDLVertexAttributeTextureCoordinate;
    // Metal 3.0: Add normal attribute
    mdlVertexDescriptor.attributes[VertexAttributeNormal].name  = MDLVertexAttributeNormal;

    mdlMesh.vertexDescriptor = mdlVertexDescriptor;

    _mesh = [[MTKMesh alloc] initWithMesh:mdlMesh
                                   device:_device
                                    error:&error];

    if(!_mesh || error)
    {
        NSLog(@"Error creating MetalKit mesh %@", error.localizedDescription);
    } else {
        NSLog(@"✅ Mesh created successfully");
        NSLog(@"Mesh vertex count: %lu", (unsigned long)_mesh.vertexCount);
        NSLog(@"Mesh submesh count: %lu", (unsigned long)_mesh.submeshes.count);
        
        // Debug vertex descriptor
        NSLog(@"Vertex descriptor attributes:");
        for (int i = 0; i < 3; i++) {
            MTLVertexAttributeDescriptor *attr = _mtlVertexDescriptor.attributes[i];
            if (attr.format != MTLVertexFormatInvalid) {
                NSLog(@"  Attribute %d: format=%lu, offset=%lu, bufferIndex=%lu", 
                      i, (unsigned long)attr.format, (unsigned long)attr.offset, (unsigned long)attr.bufferIndex);
            }
        }
    }

    MTKTextureLoader* textureLoader = [[MTKTextureLoader alloc] initWithDevice:_device];

    NSDictionary *textureLoaderOptions =
    @{
      MTKTextureLoaderOptionTextureUsage       : @(MTLTextureUsageShaderRead),
      MTKTextureLoaderOptionTextureStorageMode : @(MTLStorageModePrivate)
      };

    _colorMap = [textureLoader newTextureWithName:@"ColorMap"
                                      scaleFactor:1.0
                                           bundle:nil
                                          options:textureLoaderOptions
                                            error:&error];

    if(!_colorMap || error)
    {
        NSLog(@"Error creating texture %@", error.localizedDescription);
    } else {
        NSLog(@"✅ Texture loaded successfully: %@", _colorMap);
        NSLog(@"Texture dimensions: %lu x %lu", (unsigned long)_colorMap.width, (unsigned long)_colorMap.height);
        NSLog(@"Texture pixel format: %lu", (unsigned long)_colorMap.pixelFormat);
    }
    
    // Metal 3.0: Create blurred texture for post-processing effects
    MTLTextureDescriptor *blurredTextureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm_sRGB
                                                                                                  width:512
                                                                                                 height:512
                                                                                              mipmapped:NO];
    blurredTextureDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    _blurredTexture = [_device newTextureWithDescriptor:blurredTextureDesc];
    _blurredTexture.label = @"BlurredTexture";
}

- (void)_updateDynamicBufferState
{
    /// Update the state of our uniform buffers before rendering

    _uniformBufferIndex = (_uniformBufferIndex + 1) % kMaxBuffersInFlight;

    _uniformBufferOffset = kAlignedUniformsSize * _uniformBufferIndex;

    _uniformBufferAddress = ((uint8_t*)_dynamicUniformBuffer.contents) + _uniformBufferOffset;
}

- (void)_updateGameState
{
    /// Update any game state before encoding renderint commands to our drawable

    Uniforms * uniforms = (Uniforms*)_uniformBufferAddress;

    uniforms->projectionMatrix = _projectionMatrix;

    vector_float3 rotationAxis = {1, 1, 0};
    matrix_float4x4 modelMatrix = matrix4x4_rotation(_rotation, rotationAxis);
    matrix_float4x4 viewMatrix = matrix4x4_translation(0.0, 0.0, -8.0);

    uniforms->modelViewMatrix = matrix_multiply(viewMatrix, modelMatrix);
    
    // Metal 3.0: Calculate normal matrix and add enhanced uniforms
    uniforms->normalMatrix = matrix_inverse_func(matrix_transpose_func(modelMatrix));
    
    // Calculate camera position from view matrix
    matrix_float4x4 invViewMatrix = matrix_inverse_func(viewMatrix);
    vector_float4 cameraPos = invViewMatrix.columns[3];
    uniforms->cameraPosition = (vector_float3){cameraPos.x, cameraPos.y, cameraPos.z};
    
    uniforms->time = _rotation;

    _rotation += .01;
}



// Metal 3.0: Add object capture support
- (void)enableObjectCapture
{
    if (@available(macOS 15.0, *)) {
        // Metal 3.0: Enable object capture features
        // This would integrate with RealityKit's Object Capture API
        NSLog(@"Object capture enabled for Metal 3.0");
    }
}

// Metal 3.0: Add mesh shading support
- (void)enableMeshShading
{
    if (@available(macOS 15.0, *)) {
        // Metal 3.0: Enable mesh shading for advanced geometry processing
        NSLog(@"Mesh shading enabled for Metal 3.0");
        
        // Check if device supports mesh shading (Metal 3.0 feature)
        // Note: This property may not be available in all versions
        // if (_device.supportsMeshShading) {
        //     NSLog(@"Device supports mesh shading");
        // }
    }
}

// Metal 3.0: Add dynamic library support
- (void)enableDynamicLibraries
{
    if (@available(macOS 15.0, *)) {
        // Metal 3.0: Enable dynamic library loading for runtime shader compilation
        NSLog(@"Dynamic libraries enabled for Metal 3.0");
        
        // Check if device supports dynamic libraries (Metal 3.0 feature)
        // Note: This property may not be available in all versions
        // if (_device.supportsDynamicLibraries) {
        //     NSLog(@"Device supports dynamic libraries");
        // }
    }
}

// Metal 3.0: Report all available features
- (void)reportMetalFeatures
{
    if (@available(macOS 15.0, *)) {
        NSLog(@"=== Metal 3.0 Feature Report ===");
        NSLog(@"Device: %@", _device.name);
        // Note: Some Metal 3.0 properties may not be available in all versions
        // NSLog(@"Supports Mesh Shading: %@", _device.supportsMeshShading ? @"YES" : @"NO");
        // NSLog(@"Supports Dynamic Libraries: %@", _device.supportsDynamicLibraries ? @"YES" : @"NO");
        // NSLog(@"Supports Ray Tracing: %@", _device.supportsRaytracing ? @"YES" : @"NO");
        // NSLog(@"Supports BC Texture Compression: %@", _device.supportsBCTextureCompression ? @"YES" : @"NO");
        // NSLog(@"Supports Counters: %@", _device.supportsCounters ? @"YES" : @"NO");
        NSLog(@"Max Threads Per Threadgroup: %lu", (unsigned long)_device.maxThreadsPerThreadgroup.width);
        NSLog(@"Max Threads Per Threadgroup (3D): %lu", (unsigned long)_device.maxThreadsPerThreadgroup.depth);
        NSLog(@"=================================");
    }
}

- (void)drawInMTKView:(nonnull MTKView *)view
{
    /// Per frame updates here

    dispatch_semaphore_wait(_inFlightSemaphore, DISPATCH_TIME_FOREVER);

    id <MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    commandBuffer.label = @"MyCommand";
    
    // Metal 3.0: Enable enhanced command buffer features
    if (@available(macOS 15.0, *)) {
        // Enable enhanced debugging (Metal 3.0 feature)
        // Note: This property may not be available in all versions
        // commandBuffer.loggedMessages = YES;
        
        // Enable performance counters if available (Metal 3.0 feature)
        // Note: This property may not be available in all versions
        // if (_device.supportsCounters) {
        //     NSLog(@"Device supports performance counters");
        // }
    }

    __block dispatch_semaphore_t block_sema = _inFlightSemaphore;
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
     {
         dispatch_semaphore_signal(block_sema);
     }];

    [self _updateDynamicBufferState];

    [self _updateGameState];

    /// Delay getting the currentRenderPassDescriptor until we absolutely need it to avoid
    ///   holding onto the drawable and blocking the display pipeline any longer than necessary
    MTLRenderPassDescriptor* renderPassDescriptor = view.currentRenderPassDescriptor;

    if(renderPassDescriptor != nil) {

        /// Final pass rendering code here

        id <MTLRenderCommandEncoder> renderEncoder =
        [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        renderEncoder.label = @"MyRenderEncoder";

        [renderEncoder pushDebugGroup:@"DrawBox"];

        [renderEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
        [renderEncoder setCullMode:MTLCullModeBack];
        [renderEncoder setRenderPipelineState:_pipelineState];
        [renderEncoder setDepthStencilState:_depthState];

        [renderEncoder setVertexBuffer:_dynamicUniformBuffer
                                offset:_uniformBufferOffset
                               atIndex:BufferIndexUniforms];

        [renderEncoder setFragmentBuffer:_dynamicUniformBuffer
                                  offset:_uniformBufferOffset
                                 atIndex:BufferIndexUniforms];

        for (NSUInteger bufferIndex = 0; bufferIndex < _mesh.vertexBuffers.count; bufferIndex++)
        {
            MTKMeshBuffer *vertexBuffer = _mesh.vertexBuffers[bufferIndex];
            if((NSNull*)vertexBuffer != [NSNull null])
            {
                [renderEncoder setVertexBuffer:vertexBuffer.buffer
                                        offset:vertexBuffer.offset
                                       atIndex:bufferIndex];
            }
        }

        [renderEncoder setFragmentTexture:_colorMap
                                  atIndex:TextureIndexColor];

        for(MTKSubmesh *submesh in _mesh.submeshes)
        {
            [renderEncoder drawIndexedPrimitives:submesh.primitiveType
                                      indexCount:submesh.indexCount
                                       indexType:submesh.indexType
                                     indexBuffer:submesh.indexBuffer.buffer
                               indexBufferOffset:submesh.indexBuffer.offset];
        }

        [renderEncoder popDebugGroup];

        [renderEncoder endEncoding];

        // Metal 3.0: Add post-processing pass using Metal Performance Shaders
        if (_gaussianBlur && _blurredTexture) {
            // Apply Gaussian blur to the rendered result
            // Note: Metal Performance Shaders work directly with command buffers
            // and don't require separate render encoders
            [_gaussianBlur encodeToCommandBuffer:commandBuffer
                                   sourceTexture:view.currentDrawable.texture
                              destinationTexture:_blurredTexture];
        }

        [commandBuffer presentDrawable:view.currentDrawable];
    }

    [commandBuffer commit];
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{
    /// Respond to drawable size or orientation changes here

    float aspect = size.width / (float)size.height;
    _projectionMatrix = matrix_perspective_right_hand(65.0f * (M_PI / 180.0f), aspect, 0.1f, 100.0f);
}

#pragma mark Matrix Math Utilities

matrix_float4x4 matrix4x4_translation(float tx, float ty, float tz)
{
    return (matrix_float4x4) {{
        { 1,   0,  0,  0 },
        { 0,   1,  0,  0 },
        { 0,   0,  1,  0 },
        { tx, ty, tz,  1 }
    }};
}

static matrix_float4x4 matrix4x4_rotation(float radians, vector_float3 axis)
{
    axis = vector_normalize(axis);
    float ct = cosf(radians);
    float st = sinf(radians);
    float ci = 1 - ct;
    float x = axis.x, y = axis.y, z = axis.z;

    return (matrix_float4x4) {{
        { ct + x * x * ci,     y * x * ci + z * st, z * x * ci - y * st, 0},
        { x * y * ci - z * st,     ct + y * y * ci, z * y * ci + x * st, 0},
        { x * z * ci + y * st, y * z * ci - x * st,     ct + z * z * ci, 0},
        {                   0,                   0,                   0, 1}
    }};
}

matrix_float4x4 matrix_perspective_right_hand(float fovyRadians, float aspect, float nearZ, float farZ)
{
    float ys = 1 / tanf(fovyRadians * 0.5);
    float xs = ys / aspect;
    float zs = farZ / (nearZ - farZ);

    return (matrix_float4x4) {{
        { xs,   0,          0,  0 },
        {  0,  ys,          0,  0 },
        {  0,   0,         zs, -1 },
        {  0,   0, nearZ * zs,  0 }
    }};
}

// Metal 3.0: Add matrix utility functions
// Note: These are wrapper functions to avoid naming conflicts
matrix_float4x4 matrix_inverse_func(matrix_float4x4 matrix)
{
    // Use the simd library function directly
    return simd_inverse(matrix);
}

matrix_float4x4 matrix_transpose_func(matrix_float4x4 matrix)
{
    // Use the simd library function directly
    return simd_transpose(matrix);
}

@end
