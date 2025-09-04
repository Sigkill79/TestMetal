# Font Manager Conversion Specification

## Overview

This document outlines the technical specification for converting the OpenGL-based font manager (`font_manager.cpp/.hpp`) to work with the Metal engine and SDF rendering system. The conversion involves replacing OpenGL rendering calls with Metal-based SDF rendering, adapting the C++ code to C naming conventions, and integrating with the existing engine architecture.

## Current Font Manager Analysis

### Existing Font Manager Structure

**Files**: `font_manager.hpp`, `font_manager.cpp`

**Key Components**:
1. **Font Structure**: Contains texture ID, glyph map, line spacing, and kerning data
2. **Glyph Structure**: Stores rendering position, bitmap location, and advance width
3. **Kerning Structure**: Character pair kerning information
4. **Core Functions**: `createFont()`, `renderText()`, `renderTextBox()`

**Dependencies**:
- OpenGL (`opengl/gl.h`)
- STB TrueType (`stb_truetype.h`)
- Custom math library (`base/math/Math.h`)
- C++ STL containers (`std::vector`, `std::map`)

**Current Rendering Pipeline**:
1. Load TTF font file using STB TrueType
2. Generate SDF bitmaps for each character (already using `stbtt_GetCodepointSDF`)
3. Pack glyphs into texture atlas
4. Upload atlas to OpenGL as RGBA texture
5. Render text using immediate mode OpenGL calls

## Conversion Requirements

### 1. Language and Naming Convention Conversion

#### C++ to C Conversion
- **Remove C++ features**: `std::vector`, `std::map`, `std::cerr`, `std::cout`
- **Replace with C equivalents**: Use stretchy buffers (`stretchy_buffer.h`) for dynamic arrays
- **Function naming**: Convert to `engine_font_*` prefix following engine conventions
- **Header guards**: Change from `font_manager_hpp` to `ENGINE_FONT_H`

#### Naming Convention Mapping
```
C++ Original          → C Engine Convention
createFont()          → engine_font_create()
renderText()          → engine_font_render_text()
renderTextBox()       → engine_font_render_text_box()
font                  → EngineFont
glyph                 → FontGlyph
kerning               → FontKerning
```

### 2. Data Structure Conversion

#### Font Structure Conversion
```c
// Original C++ structure
struct font {
    unsigned int            textureId;
    std::map< char, glyph>  glyphs;
    unsigned int            lineSpacing;
    std::vector<kerning>    kernings;
};

// Converted C structure
typedef struct {
    MetalTextureHandle      texture;           // Metal texture instead of OpenGL ID
    FontGlyph*              glyphs;            // Dynamic array instead of std::map
    uint32_t                glyphCount;        // Number of glyphs
    uint32_t                maxGlyphs;         // Maximum glyph capacity
    float                   lineSpacing;       // Changed to float for consistency
    FontKerning*            kernings;          // Dynamic array instead of std::vector
    uint32_t                kerningCount;      // Number of kerning pairs
    uint32_t                maxKernings;       // Maximum kerning capacity
    int                     isInitialized;     // Initialization state
} EngineFont;
```

#### Glyph Structure Conversion
```c
// Original C++ structure
struct glyph {
    int             startX, startY;
    unsigned int    width, height;
    int             bitmapTop, bitmapLeft;
    int             advance;
};

// Converted C structure
typedef struct {
    float           startX, startY;        // Changed to float for subpixel precision
    uint32_t        width, height;         // Keep as uint32_t for texture dimensions
    float           bitmapTop, bitmapLeft; // Changed to float for texture coordinates
    float           advance;               // Changed to float for subpixel precision
    char            character;             // Store character for lookup
} FontGlyph;
```

#### Kerning Structure Conversion
```c
// Original C++ structure
struct kerning {
    char    first;
    char    second;
    int     kerning;
};

// Converted C structure
typedef struct {
    char            first;                 // First character
    char            second;                // Second character
    float           kerning;               // Changed to float for subpixel precision
} FontKerning;
```

### 3. OpenGL to Metal Rendering Conversion

#### Texture Creation Conversion
```c
// Original OpenGL texture creation
glGenTextures(1, &fnt->textureId);
glBindTexture(GL_TEXTURE_RECTANGLE_ARB, fnt->textureId);
glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8, atlasWidth, atlasHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaAtlas);

// Converted Metal texture creation
MetalTextureHandle fontTexture = texture_loader_create_atlas_texture(
    textureLoader, 
    rgbaAtlas, 
    atlasWidth, 
    atlasHeight, 
    MTLPixelFormatRGBA8Unorm
);
```

#### Rendering Pipeline Conversion
```c
// Original OpenGL immediate mode rendering
glBegin(GL_QUADS);
glColor4d(1,1,1,1.0);
glTexCoord2f(glyph.bitmapLeft, glyph.bitmapTop+glyph.height);
glVertex3f(penPos[0]+glyph.startX*scale, penPos[1]+(glyph.startY+glyph.height)*scale, 0);
// ... more vertices
glEnd();

// Converted Metal SDF rendering
engine_2d_draw_sdf_simple(
    ui2d,
    penPos[0] + glyph.startX * scale,
    penPos[1] + glyph.startY * scale,
    fontTexture,
    (vec4_t){1.0f, 1.0f, 1.0f, 1.0f}  // White fill color
);
```

### 4. Integration with Engine Architecture

#### New Engine Font Module Structure
```
engine_font.h          - Header file with public API
engine_font.c          - Implementation file
engine_font_internal.h - Internal structures and helper functions
```

#### Engine Font API Design
```c
// Core font management
EngineFont* engine_font_create(const char* fontname, uint32_t size, float spacing);
void engine_font_destroy(EngineFont* font);

// Text rendering
int engine_font_render_text(EngineFont* font, Engine2D* ui2d, vec2_t pos, float scale, const char* text);
int engine_font_render_text_box(EngineFont* font, Engine2D* ui2d, vec2_t pos, float scale, const char* text);

// Utility functions
float engine_font_get_text_width(EngineFont* font, const char* text, float scale);
float engine_font_get_text_height(EngineFont* font, const char* text, float scale);
vec2_t engine_font_get_text_bounds(EngineFont* font, const char* text, float scale);
```

### 5. SDF Rendering Integration

#### Font Atlas as SDF Texture
The existing font manager already generates SDF bitmaps using `stbtt_GetCodepointSDF()`, which is perfect for integration with the SDF rendering system. The conversion involves:

1. **Keep SDF Generation**: Continue using `stbtt_GetCodepointSDF()` for glyph generation
2. **Single-Channel Atlas**: Convert RGBA atlas to single-channel R8Unorm for SDF rendering
3. **Atlas Coordinates**: Each glyph occupies a specific region in the atlas texture, requiring texture coordinate specification
4. **SDF Rendering**: Use `engine_2d_draw_sdf_atlas()` for each glyph instead of OpenGL quads

**Critical Requirement**: Since the font atlas contains multiple glyphs in a single texture, we must specify which portion of the atlas corresponds to each glyph. The existing `engine_2d_draw_sdf_simple()` function assumes the entire texture is used, which won't work for font atlases where each glyph occupies only a small region of the texture.

#### SDF Atlas Creation
```c
// Create single-channel SDF atlas instead of RGBA
unsigned char* sdfAtlas = (unsigned char*)malloc(atlasWidth * atlasHeight);
memset(sdfAtlas, 0, atlasWidth * atlasHeight);

// Copy SDF data directly (no RGBA conversion needed)
for(int y = 0; y < newGlyph.height; ++y) {
    memcpy(&sdfAtlas[(newGlyph.bitmapTop+y)*atlasWidth+newGlyph.bitmapLeft], 
           &sdf[y*newGlyph.width], 
           newGlyph.width);
}

// Create Metal texture with R8Unorm format
MetalTextureHandle sdfTexture = texture_loader_create_atlas_texture(
    textureLoader,
    sdfAtlas,
    atlasWidth,
    atlasHeight,
    MTLPixelFormatR8Unorm
);
```

#### Glyph Rendering with SDF
```c
// Render each glyph as SDF element
for(const char* chr = text; *chr; ++chr) {
    FontGlyph* glyph = engine_font_find_glyph(font, *chr);
    if(!glyph) continue;
    
    // Calculate glyph position
    vec2_t glyphPos = {
        penPos.x + glyph->startX * scale,
        penPos.y + glyph->startY * scale
    };
    
    // Calculate texture coordinates for this glyph in the atlas
    vec2_t texCoord = {glyph->bitmapLeft, glyph->bitmapTop};
    vec2_t texSize = {glyph->width, glyph->height};
    
    // Render glyph as SDF with atlas coordinates
    engine_2d_draw_sdf_atlas(
        ui2d,
        glyphPos.x,
        glyphPos.y,
        font->texture,  // SDF atlas texture
        texCoord,       // Texture coordinate offset in atlas
        texSize,        // Texture coordinate size in atlas
        (vec4_t){1.0f, 1.0f, 1.0f, 1.0f},  // White fill
        (vec4_t){0.0f, 0.0f, 0.0f, 0.0f},  // No outline
        0.5f,           // Edge distance
        0.4f,           // Outline distance
        0.1f,           // Smoothing
        0               // No outline
    );
    
    // Advance pen position
    penPos.x += glyph->advance * scale;
    
    // Apply kerning
    penPos.x += engine_font_get_kerning(font, *chr, *(chr+1)) * scale;
}
```

### 6. Required Engine Extensions

#### New Texture Loader Functions
```c
// Create texture from raw data (for font atlas)
MetalTextureHandle texture_loader_create_atlas_texture(
    TextureLoaderHandle loader,
    const unsigned char* data,
    uint32_t width,
    uint32_t height,
    TexturePixelFormat format
);

// Create single-channel SDF texture
MetalTextureHandle texture_loader_create_sdf_texture(
    TextureLoaderHandle loader,
    const unsigned char* sdfData,
    uint32_t width,
    uint32_t height
);
```

#### Enhanced 2D Engine Functions
```c
// Draw SDF with custom texture coordinates (for font atlas)
// This function is REQUIRED for font rendering since each glyph occupies
// a specific region within the atlas texture
int engine_2d_draw_sdf_atlas(Engine2D* ui2d, 
                            float x, float y, 
                            MetalTextureHandle sdfTexture,
                            vec2_t texCoord, vec2_t texSize,
                            vec4_t fillColor, vec4_t outlineColor,
                            float edgeDistance, float outlineDistance, 
                            float smoothing, int hasOutline);

// Convenience function for simple atlas rendering
int engine_2d_draw_sdf_atlas_simple(Engine2D* ui2d,
                                   float x, float y,
                                   MetalTextureHandle sdfTexture,
                                   vec2_t texCoord, vec2_t texSize,
                                   vec4_t fillColor);
```

#### New Shader Support
The existing SDF shader needs to support texture atlas coordinates:

```metal
// Enhanced SDF fragment shader for atlas support
struct SDFAtlasUniforms {
    float4 fillColor;
    float4 outlineColor;
    float edgeDistance;
    float outlineDistance;
    float smoothing;
    int hasOutline;
    float2 atlasOffset;      // Texture coordinate offset in atlas
    float2 atlasScale;       // Texture coordinate scale in atlas
};

fragment float4 sdf_atlas_fragment_main(UI2DVertexOut in [[stage_in]],
                                       texture2d<float> sdfTexture [[texture(0)]],
                                       sampler sdfSampler [[sampler(0)]],
                                       constant SDFAtlasUniforms& uniforms [[buffer(1)]]) {
    // Calculate atlas texture coordinates
    float2 atlasCoord = uniforms.atlasOffset + in.texcoord * uniforms.atlasScale;
    
    // Sample SDF texture from atlas
    float distance = sdfTexture.sample(sdfSampler, atlasCoord).r;
    
    // Apply SDF rendering logic (same as existing SDF shader)
    // ... rest of SDF rendering code
}
```

### 7. Memory Management

#### Dynamic Array Management
Replace C++ STL containers with stretchy buffers:

```c
// Glyph array management
#define FONT_MAX_GLYPHS 256
#define FONT_MAX_KERNINGS 1024

// Initialize font with stretchy buffers
int engine_font_init_buffers(EngineFont* font) {
    font->glyphs = NULL;  // Initialize as stretchy buffer
    font->kernings = NULL; // Initialize as stretchy buffer
    font->glyphCount = 0;
    font->kerningCount = 0;
    return 1;
}

// Add glyph to font
int engine_font_add_glyph(EngineFont* font, char character, FontGlyph glyph) {
    glyph.character = character;
    sb_push(font->glyphs, glyph);
    font->glyphCount++;
    return 1;
}

// Find glyph by character
FontGlyph* engine_font_find_glyph(EngineFont* font, char character) {
    for(uint32_t i = 0; i < font->glyphCount; i++) {
        if(font->glyphs[i].character == character) {
            return &font->glyphs[i];
        }
    }
    return NULL;
}
```

#### Resource Cleanup
```c
void engine_font_destroy(EngineFont* font) {
    if(!font) return;
    
    // Free stretchy buffers
    sb_free(font->glyphs);
    sb_free(font->kernings);
    
    // Texture cleanup is handled by texture loader
    // No need to manually free Metal texture
    
    // Free font structure
    free(font);
}
```

### 8. Error Handling and Logging

#### Replace C++ I/O with C Logging
```c
// Original C++ error handling
std::cerr << "Failed to load font " << fontname << std::endl;

// Converted C error handling
#define FONT_DEBUG(fmt, ...) printf("[FONT] " fmt "\n", ##__VA_ARGS__)
#define FONT_ERROR(fmt, ...) printf("[FONT ERROR] " fmt "\n", ##__VA_ARGS__)
#define FONT_INFO(fmt, ...) printf("[FONT INFO] " fmt "\n", ##__VA_ARGS__)

FONT_ERROR("Failed to load font %s", fontname);
```

#### Error Code System
```c
typedef enum {
    FONT_SUCCESS = 0,
    FONT_ERROR_INVALID_PARAMS,
    FONT_ERROR_FILE_NOT_FOUND,
    FONT_ERROR_MEMORY_ALLOCATION,
    FONT_ERROR_TEXTURE_CREATION,
    FONT_ERROR_GLYPH_GENERATION,
    FONT_ERROR_NOT_INITIALIZED
} FontResult;

const char* font_get_error_string(FontResult result);
```

### 9. Integration with Main Engine

#### Engine Initialization
```c
// In engine_main.c
EngineFont* defaultFont = NULL;

int engine_init_font_system(TextureLoaderHandle textureLoader) {
    // Load default font
    defaultFont = engine_font_create("assets/fonts/arial.ttf", 32, 1.0f);
    if(!defaultFont) {
        printf("Failed to load default font\n");
        return 0;
    }
    
    return 1;
}

void engine_shutdown_font_system(void) {
    if(defaultFont) {
        engine_font_destroy(defaultFont);
        defaultFont = NULL;
    }
}
```

#### Text Rendering Integration
```c
// In main render loop
void render_ui_text(Engine2D* ui2d, const char* text, float x, float y, float scale) {
    if(!defaultFont || !ui2d) return;
    
    vec2_t pos = {x, y};
    engine_font_render_text(defaultFont, ui2d, pos, scale, text);
}
```

### 10. Performance Optimizations

#### Glyph Lookup Optimization
```c
// Use hash table for O(1) glyph lookup instead of linear search
typedef struct {
    char character;
    FontGlyph* glyph;
} GlyphLookupEntry;

// Simple hash table for glyph lookup
#define GLYPH_HASH_SIZE 256
typedef struct {
    GlyphLookupEntry* entries[GLYPH_HASH_SIZE];
} GlyphLookupTable;

FontGlyph* engine_font_find_glyph_fast(EngineFont* font, char character) {
    uint32_t hash = (uint32_t)character % GLYPH_HASH_SIZE;
    GlyphLookupEntry* entry = font->lookupTable.entries[hash];
    
    while(entry) {
        if(entry->character == character) {
            return entry->glyph;
        }
        entry = entry->next;
    }
    return NULL;
}
```

#### Batch Rendering
```c
// Batch multiple text elements for efficient rendering
typedef struct {
    EngineFont* font;
    vec2_t position;
    float scale;
    const char* text;
    vec4_t color;
} TextRenderCommand;

// Collect text rendering commands
TextRenderCommand* textCommands = NULL;

void engine_font_queue_text(EngineFont* font, vec2_t pos, float scale, const char* text, vec4_t color) {
    TextRenderCommand cmd = {font, pos, scale, text, color};
    sb_push(textCommands, cmd);
}

// Render all queued text in batch
void engine_font_render_batch(Engine2D* ui2d) {
    for(uint32_t i = 0; i < sb_count(textCommands); i++) {
        TextRenderCommand* cmd = &textCommands[i];
        engine_font_render_text(cmd->font, ui2d, cmd->position, cmd->scale, cmd->text);
    }
    
    // Clear batch
    sb_free(textCommands);
    textCommands = NULL;
}
```

### 11. Testing and Validation

#### Font Loading Tests
```c
void test_font_loading(void) {
    EngineFont* font = engine_font_create("assets/fonts/arial.ttf", 32, 1.0f);
    assert(font != NULL);
    assert(font->glyphCount > 0);
    assert(font->texture != NULL);
    
    // Test glyph lookup
    FontGlyph* glyphA = engine_font_find_glyph(font, 'A');
    assert(glyphA != NULL);
    assert(glyphA->character == 'A');
    
    engine_font_destroy(font);
}
```

#### Text Rendering Tests
```c
void test_text_rendering(Engine2D* ui2d) {
    EngineFont* font = engine_font_create("assets/fonts/arial.ttf", 32, 1.0f);
    
    // Test basic text rendering
    vec2_t pos = {100, 100};
    int result = engine_font_render_text(font, ui2d, pos, 1.0f, "Hello World");
    assert(result == 1);
    
    // Test text box rendering
    pos = (vec2_t){200, 200};
    result = engine_font_render_text_box(font, ui2d, pos, 1.0f, "Text Box Test");
    assert(result == 1);
    
    engine_font_destroy(font);
}
```

### 12. File Structure and Dependencies

#### New Files to Create
```
TestMetal/engine_font.h              - Public font API
TestMetal/engine_font.c              - Font implementation
TestMetal/engine_font_internal.h     - Internal structures and helpers
```

#### Modified Files
```
TestMetal/engine_2d.h                - Add SDF atlas rendering functions
TestMetal/engine_2d.c                - Implement SDF atlas rendering
TestMetal/engine_metal_ui_shaders.metal - Add SDF atlas shader
TestMetal/engine_texture_loader.h    - Add atlas texture creation functions
TestMetal/engine_texture_loader.c    - Implement atlas texture creation
TestMetal/engine_main.c              - Integrate font system
```

#### Dependencies
```
libraries/stb/stb_truetype.h         - TTF font loading (already included)
libraries/stb/stretchy_buffer.h      - Dynamic arrays (already included)
TestMetal/engine_2d.h                - 2D rendering system
TestMetal/engine_metal.h             - Metal engine interface
TestMetal/engine_math.h              - Math utilities
TestMetal/engine_texture_loader.h    - Texture loading system
```

### 13. Implementation Phases

#### Phase 1: Core Conversion
- [ ] Convert C++ structures to C equivalents
- [ ] Replace STL containers with stretchy buffers
- [ ] Convert OpenGL texture creation to Metal
- [ ] Implement basic font loading and glyph generation

#### Phase 2: SDF Integration
- [ ] Integrate with SDF rendering system
- [ ] Implement atlas-based SDF rendering
- [ ] Add SDF atlas shader support
- [ ] Test SDF text rendering

#### Phase 3: API Integration
- [ ] Integrate with 2D engine
- [ ] Add text rendering functions
- [ ] Implement text box rendering
- [ ] Add utility functions (text width, height, bounds)

#### Phase 4: Optimization and Testing
- [ ] Add glyph lookup optimization
- [ ] Implement batch rendering
- [ ] Add comprehensive testing
- [ ] Performance optimization

### 14. Backward Compatibility

#### Compatibility Considerations
- The converted font manager will be a new module, not a replacement
- Existing code using the old font manager will need to be updated
- The new API follows engine naming conventions for consistency
- SDF rendering provides better quality than the original OpenGL approach

#### Migration Path
1. **Parallel Implementation**: Implement new font system alongside existing code
2. **Gradual Migration**: Update code to use new font API incrementally
3. **Feature Parity**: Ensure all original functionality is preserved
4. **Enhanced Features**: Add new SDF-specific features (outlines, effects)

## Conclusion

This specification provides a comprehensive roadmap for converting the OpenGL-based font manager to work with the Metal engine and SDF rendering system. The conversion maintains all existing functionality while adding the benefits of SDF rendering (crisp text at any scale) and proper integration with the engine architecture.

The key advantages of this conversion include:
- **Better Text Quality**: SDF rendering provides crisp text at any resolution
- **Engine Integration**: Proper integration with the Metal-based rendering pipeline
- **Consistent API**: Follows engine naming conventions and architecture patterns
- **Performance**: Optimized for the Metal rendering system
- **Maintainability**: C-based implementation that fits with the rest of the engine

The implementation can be done incrementally, allowing for testing and validation at each phase while maintaining a working system throughout the conversion process.
