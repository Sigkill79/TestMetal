# SDF (Signed Distance Field) Textured Rendering Specification

## Overview

This document outlines the technical specification for adding SDF (Signed Distance Field) textured rendering support to the 2D engine. SDF rendering enables crisp, scalable text and vector graphics that remain sharp at any resolution and size.

## Current Architecture Analysis

### Existing 2D Engine Components

1. **Engine2D Structure** (`engine_2d.h/c`)
   - UI element management with `UIElement` struct
   - Vertex/index buffer management
   - Basic texture rendering pipeline

2. **Current Shader System** (`engine_metal_ui_shaders.metal`)
   - Simple vertex shader for screen-space positioning
   - Basic fragment shader for texture sampling
   - No SDF-specific processing

3. **Texture Loading System** (`engine_texture_loader.h/c`)
   - Comprehensive texture caching and loading
   - Support for multiple formats via stb_image
   - Metal texture creation and management

4. **Metal Integration** (`engine_metal.m`)
   - Pipeline state management
   - Render pass coordination
   - Resource binding

## SDF Rendering Requirements

### 1. SDF Texture Format

**Current State**: The engine supports standard RGBA textures via the texture loader.

**SDF Requirements**:
- SDF textures store distance values in the alpha channel (or red channel for single-channel SDFs)
- Distance values are normalized (0.0 = inside shape, 0.5 = edge, 1.0 = outside shape)
- The `sdf_texture.png` asset should be loaded as a single-channel texture (R8Unorm format)

### 2. SDF Rendering Parameters

**New Parameters Needed**:
- **Edge Distance**: Controls the sharpness of edges (typically 0.5 for standard SDFs)
- **Outline Distance**: Distance for outline rendering (optional)
- **Fill Color**: Color for the filled shape
- **Outline Color**: Color for the outline (optional)
- **Smoothing**: Anti-aliasing factor for edge smoothing

### 3. Shader Modifications

#### New SDF Fragment Shader

**File**: `engine_metal_ui_shaders.metal`

**New Structure**:
```metal
struct SDFUniforms {
    float4 fillColor;           // RGBA fill color
    float4 outlineColor;        // RGBA outline color
    float edgeDistance;         // Distance threshold for edge detection
    float outlineDistance;      // Distance threshold for outline
    float smoothing;            // Anti-aliasing factor
    int hasOutline;             // Whether to render outline
};
```

**SDF Fragment Shader Logic**:
```metal
fragment float4 sdf_fragment_main(UI2DVertexOut in [[stage_in]],
                                 texture2d<float> sdfTexture [[texture(0)]],
                                 sampler sdfSampler [[sampler(0)]],
                                 constant SDFUniforms& sdfUniforms [[buffer(1)]]) {
    // Sample SDF texture
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
```

### 4. Engine API Extensions

#### New UI Element Type

**File**: `engine_2d.h`

**Extended UIElement Structure**:
```c
typedef enum {
    UI_ELEMENT_TYPE_TEXTURE = 0,
    UI_ELEMENT_TYPE_SDF = 1
} UIElementType;

typedef struct {
    MetalTextureHandle texture;
    uint32_t startIndex;
    uint32_t indexCount;
    float x, y;
    float width, height;
    int isActive;
    
    // SDF-specific properties
    UIElementType type;
    float4 fillColor;           // RGBA fill color
    float4 outlineColor;        // RGBA outline color
    float edgeDistance;         // Edge threshold (default: 0.5)
    float outlineDistance;      // Outline threshold (default: 0.4)
    float smoothing;            // Anti-aliasing (default: 0.1)
    int hasOutline;             // Enable outline rendering
} UIElement;
```

#### New API Functions

**File**: `engine_2d.h`

```c
// SDF-specific rendering functions
int engine_2d_draw_sdf(Engine2D* ui2d, float x, float y, MetalTextureHandle sdfTexture,
                      float4 fillColor, float4 outlineColor, float edgeDistance, 
                      float outlineDistance, float smoothing, int hasOutline);

// Convenience functions with defaults
int engine_2d_draw_sdf_simple(Engine2D* ui2d, float x, float y, MetalTextureHandle sdfTexture,
                             float4 fillColor);

int engine_2d_draw_sdf_with_outline(Engine2D* ui2d, float x, float y, MetalTextureHandle sdfTexture,
                                   float4 fillColor, float4 outlineColor);
```

### 5. Pipeline State Management

#### New SDF Pipeline State

**File**: `engine_metal.m`

**Requirements**:
- Separate render pipeline state for SDF rendering
- Different fragment shader function (`sdf_fragment_main`)
- Same vertex shader as regular UI elements
- Additional uniform buffer for SDF parameters

**Implementation**:
```objc
// New SDF pipeline state creation
- (id<MTLRenderPipelineState>)createSDFPipelineState {
    MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
    
    // Vertex function (same as regular UI)
    pipelineDesc.vertexFunction = [self.library newFunctionWithName:@"ui_vertex_main"];
    
    // Fragment function (SDF-specific)
    pipelineDesc.fragmentFunction = [self.library newFunctionWithName:@"sdf_fragment_main"];
    
    // Color attachment
    pipelineDesc.colorAttachments[0].pixelFormat = self.colorPixelFormat;
    pipelineDesc.colorAttachments[0].blendingEnabled = YES;
    pipelineDesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    pipelineDesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    pipelineDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipelineDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    
    // Vertex descriptor
    MTLVertexDescriptor* vertexDesc = [[MTLVertexDescriptor alloc] init];
    vertexDesc.attributes[0].format = MTLVertexFormatFloat2;
    vertexDesc.attributes[0].offset = 0;
    vertexDesc.attributes[0].bufferIndex = 0;
    vertexDesc.attributes[1].format = MTLVertexFormatFloat2;
    vertexDesc.attributes[1].offset = 8;
    vertexDesc.attributes[1].bufferIndex = 0;
    vertexDesc.layouts[0].stride = 16;
    vertexDesc.layouts[0].stepRate = 1;
    vertexDesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    
    pipelineDesc.vertexDescriptor = vertexDesc;
    
    NSError* error = nil;
    id<MTLRenderPipelineState> pipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
    
    if (error) {
        NSLog(@"Failed to create SDF pipeline state: %@", error);
        return nil;
    }
    
    return pipelineState;
}
```

### 6. Render Pass Modifications

#### SDF Rendering Integration

**File**: `engine_metal.m`

**Render Pass Changes**:
1. Render all UI elements in the order they were added (maintaining draw order)
2. Switch pipeline states as needed for each element type
3. Bind appropriate uniform buffers for each element type
4. Handle blending correctly for both element types

**Implementation Strategy**:
```objc
- (void)renderUI2DPass:(id<MTLRenderCommandEncoder>)encoder {
    // Render all elements in order - no sorting by type
    [self renderUIElementsInOrder:encoder];
}

- (void)renderUIElementsInOrder:(id<MTLRenderCommandEncoder>)encoder {
    id<MTLRenderPipelineState> currentPipelineState = nil;
    
    for (uint32_t i = 0; i < ui->elementCount; i++) {
        UIElement* element = &ui->elements[i];
        if (!element->isActive) continue;
        
        // Determine required pipeline state
        id<MTLRenderPipelineState> requiredPipelineState;
        if (element->type == UI_ELEMENT_TYPE_SDF) {
            requiredPipelineState = self.sdfPipelineState;
        } else {
            requiredPipelineState = self.uiPipelineState;
        }
        
        // Switch pipeline state if needed (accept the cost for proper draw order)
        if (currentPipelineState != requiredPipelineState) {
            [encoder setRenderPipelineState:requiredPipelineState];
            currentPipelineState = requiredPipelineState;
        }
        
        // Set element-specific uniforms and resources
        if (element->type == UI_ELEMENT_TYPE_SDF) {
            // Set SDF uniforms
            SDFUniforms sdfUniforms = {
                .fillColor = element->fillColor,
                .outlineColor = element->outlineColor,
                .edgeDistance = element->edgeDistance,
                .outlineDistance = element->outlineDistance,
                .smoothing = element->smoothing,
                .hasOutline = element->hasOutline
            };
            [encoder setFragmentBytes:&sdfUniforms length:sizeof(SDFUniforms) atIndex:1];
        } else {
            // Set regular UI uniforms (existing implementation)
            UIUniforms uiUniforms = {
                .screenWidth = screenWidth,
                .screenHeight = screenHeight
            };
            [encoder setVertexBytes:&uiUniforms length:sizeof(UIUniforms) atIndex:1];
        }
        
        // Set texture
        id<MTLTexture> texture = (__bridge id<MTLTexture> _Nullable)(element->texture);
        [encoder setFragmentTexture:texture atIndex:0];
        
        // Draw element
        [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                            indexCount:element->indexCount
                             indexType:MTLIndexTypeUInt32
                           indexBuffer:impl->ui.uiIndexBuffer
                     indexBufferOffset:sizeof(int)*element->startIndex];
    }
}
```

### 7. Texture Loading Modifications

#### SDF Texture Loading

**File**: `engine_texture_loader.c`

**New Function**:
```c
MetalTextureHandle texture_loader_load_sdf(TextureLoaderHandle loader, const char* filename) {
    TextureLoadOptions sdfOptions = {
        .pixelFormat = MTLPixelFormatR8Unorm,  // Single channel for SDF
        .generateMipmaps = 0,                   // SDFs typically don't use mipmaps
        .flipVertically = 0,
        .srgb = 0                               // SDFs are not color data
    };
    
    return texture_loader_load_with_options(loader, filename, &sdfOptions);
}
```

### 8. Integration Points

#### Main Engine Integration

**File**: `engine_main.c`

**Testing Integration**:
For initial testing, render the SDF texture in the same location where regular images are currently rendered to enable side-by-side comparison.

**Usage Example**:
```c
// Load SDF texture
MetalTextureHandle sdfTexture = texture_loader_load_sdf(textureLoader, "sdf_texture.png");

// Test: Render original texture as regular image (for comparison)
engine_2d_draw_image(ui2d, 100, 100, sdfTexture);

// Test: Render same texture as SDF with simple fill
engine_2d_draw_sdf_simple(ui2d, 400, 100, sdfTexture, 
                         (float4){1.0f, 0.0f, 0.0f, 1.0f}); // Red fill

// Test: Render SDF with outline
engine_2d_draw_sdf_with_outline(ui2d, 700, 100, sdfTexture,
                               (float4){0.0f, 1.0f, 0.0f, 1.0f}, // Green fill
                               (float4){0.0f, 0.0f, 0.0f, 1.0f}); // Black outline
```

**Testing Layout**:
```
[Original Texture] [SDF Red Fill] [SDF Green Fill + Black Outline]
     (100,100)         (400,100)           (700,100)
```

This allows visual comparison of:
- How the original texture looks when rendered normally
- How the same texture data looks when processed as SDF
- Different SDF rendering parameters and effects

### 9. Performance Considerations

#### Design Trade-offs: Draw Order vs. Performance

**Decision**: Prioritize draw order over pipeline state optimization

**Rationale**:
- UI elements must render in the order they're specified for proper layering
- Mixing image and SDF elements is common in modern UIs (e.g., text over backgrounds)
- Pipeline state changes are relatively cheap in modern Metal implementations
- Correct visual output is more important than micro-optimizations

**Performance Impact**:
- Pipeline state changes occur only when element type changes
- Most UI scenes have relatively few elements, so state change overhead is minimal
- The cost of maintaining separate render passes would be higher than state changes

#### Optimization Strategies

1. **Draw Order Preservation**: Render elements in the order they were added to maintain proper layering
2. **Pipeline State Caching**: Track current pipeline state to minimize unnecessary state changes
3. **Uniform Buffer Management**: Use separate uniform buffers for SDF parameters
4. **Texture Format**: Use R8Unorm for single-channel SDFs to reduce memory usage
5. **Blending**: Optimize blending operations for SDF rendering
6. **State Change Minimization**: Only switch pipeline states when element type changes

#### Memory Management

1. **SDF Texture Caching**: Cache SDF textures separately from regular textures
2. **Uniform Buffer Pooling**: Reuse uniform buffers for SDF parameters
3. **Element Pooling**: Reuse UI element structures to reduce allocations

### 10. Testing and Validation

#### Initial Testing Strategy

**Side-by-Side Comparison Testing**:
Render the same `sdf_texture.png` file in multiple ways to validate SDF functionality:

```c
// In engine_main.c or test function
void test_sdf_rendering(Engine2D* ui2d, TextureLoaderHandle textureLoader) {
    // Load the SDF texture
    MetalTextureHandle sdfTexture = texture_loader_load_sdf(textureLoader, "sdf_texture.png");
    
    // Clear previous elements
    engine_2d_clear_elements(ui2d);
    
    // Test 1: Original texture as regular image
    engine_2d_draw_image(ui2d, 50, 50, sdfTexture);
    
    // Test 2: SDF with default parameters (red fill)
    engine_2d_draw_sdf_simple(ui2d, 350, 50, sdfTexture, 
                             (float4){1.0f, 0.0f, 0.0f, 1.0f});
    
    // Test 3: SDF with outline
    engine_2d_draw_sdf_with_outline(ui2d, 650, 50, sdfTexture,
                                   (float4){0.0f, 1.0f, 0.0f, 1.0f}, // Green fill
                                   (float4){0.0f, 0.0f, 0.0f, 1.0f}); // Black outline
    
    // Test 4: SDF with different edge distance
    engine_2d_draw_sdf(ui2d, 50, 350, sdfTexture,
                      (float4){0.0f, 0.0f, 1.0f, 1.0f}, // Blue fill
                      (float4){1.0f, 1.0f, 1.0f, 1.0f}, // White outline
                      0.3f, 0.2f, 0.05f, 1); // Different edge/outline distances
    
    // Test 5: SDF with custom parameters
    engine_2d_draw_sdf(ui2d, 350, 350, sdfTexture,
                      (float4){1.0f, 1.0f, 0.0f, 1.0f}, // Yellow fill
                      (float4){0.0f, 0.0f, 0.0f, 1.0f}, // Black outline
                      0.7f, 0.6f, 0.2f, 1); // Sharp edges, wide outline
}
```

**Expected Visual Results**:
- **Original texture**: Shows the raw SDF data (distance values as grayscale)
- **SDF red fill**: Should show crisp edges with red fill color
- **SDF with outline**: Should show green fill with black outline
- **Different parameters**: Should demonstrate edge sharpness and outline width variations

#### Test Cases

1. **Basic SDF Rendering**: Verify SDF texture loads and renders correctly
2. **Edge Sharpness**: Test different edge distance values
3. **Outline Rendering**: Verify outline functionality works
4. **Color Blending**: Test fill and outline color combinations
5. **Scaling**: Verify SDF remains crisp at different sizes
6. **Performance**: Benchmark SDF rendering vs regular texture rendering
7. **Draw Order**: Verify elements render in correct order (image, SDF, image, SDF)

#### Validation Tools

1. **SDF Inspector**: Tool to visualize SDF distance values
2. **Parameter Tweaker**: Real-time adjustment of SDF parameters
3. **Performance Profiler**: Monitor SDF rendering performance
4. **Side-by-Side Comparison**: Visual validation of SDF vs regular rendering

### 11. Implementation Phases

#### Phase 1: Core SDF Support
- [ ] Add SDF fragment shader
- [ ] Extend UIElement structure
- [ ] Add basic SDF rendering functions
- [ ] Create SDF pipeline state

#### Phase 2: Advanced Features
- [ ] Add outline support
- [ ] Implement parameter customization
- [ ] Add convenience functions
- [ ] Optimize rendering pipeline

#### Phase 3: Integration and Testing
- [ ] Integrate with main engine
- [ ] Add comprehensive testing
- [ ] Performance optimization
- [ ] Documentation and examples

### 12. File Modifications Summary

#### New Files
- None (extending existing architecture)

#### Modified Files
1. `engine_2d.h` - Extended UIElement structure and new API functions
2. `engine_2d.c` - Implementation of SDF rendering functions
3. `engine_metal_ui_shaders.metal` - New SDF fragment shader
4. `engine_metal.m` - SDF pipeline state and render pass modifications
5. `engine_texture_loader.h` - SDF texture loading function declaration
6. `engine_texture_loader.c` - SDF texture loading implementation
7. `engine_main.c` - Integration examples and testing

### 13. Dependencies

#### External Dependencies
- None (uses existing Metal and stb_image infrastructure)

#### Internal Dependencies
- Metal engine (`engine_metal.h/c`)
- Texture loader (`engine_texture_loader.h/c`)
- Math utilities (`engine_math.h/c`)

### 14. Backward Compatibility

#### Compatibility Guarantees
- All existing UI rendering functionality remains unchanged
- Regular texture rendering continues to work as before
- New SDF functionality is additive and optional

#### Migration Path
- No migration required for existing code
- SDF features can be adopted incrementally
- Existing UI elements continue to use regular texture rendering

## Conclusion

This specification provides a comprehensive roadmap for adding SDF textured rendering support to the 2D engine. The design maintains backward compatibility while adding powerful new capabilities for crisp, scalable graphics rendering. The implementation follows the existing architectural patterns and integrates seamlessly with the current Metal-based rendering pipeline.

The SDF support will enable the engine to render high-quality text, icons, and vector graphics that remain sharp at any resolution, making it suitable for modern UI applications that require crisp, professional-looking graphics.

