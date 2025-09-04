#ifndef ENGINE_FONT_H
#define ENGINE_FONT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine_metal.h"
#include "engine_math.h"
#include "engine_2d.h"
#include <stdint.h>

// ============================================================================
// FONT ENGINE INTERFACE
// ============================================================================

// Forward declarations
struct EngineFont;
typedef struct EngineFont* EngineFontHandle;

// Font glyph structure (matches original font_manager.hpp)
typedef struct {
    int             startX, startY;        // Rendering position (bitmap offsets)
    uint32_t        width, height;         // Glyph dimensions
    int             bitmapTop, bitmapLeft; // Atlas coordinates (where glyph is in atlas texture)
    int             advance;               // Horizontal advance
} EngineFontGlyph;

// Font kerning structure (matches original font_manager.hpp)
typedef struct {
    char            first;                 // First character
    char            second;                // Second character
    int             kerning;               // Kerning value
} EngineFontKerning;

// Font structure (simplified, matches original font_manager.hpp)
typedef struct EngineFont {
    MetalTextureHandle      texture;           // Atlas texture (replaces OpenGL textureId)
    EngineFontGlyph         glyphs[256];       // Fixed array of 256 glyphs (ASCII 0-255)
    uint32_t                lineSpacing;       // Line spacing
    EngineFontKerning*      kernings;          // Dynamic array of kernings
    uint32_t                kerningCount;      // Number of kerning pairs
    uint32_t                maxKernings;       // Maximum kerning capacity
    uint32_t                fontSize;          // Font size
    float                   spacing;           // Character spacing
    MetalEngineHandle       metalEngine;       // Metal engine for texture creation
    int                     isInitialized;     // Initialization state
    int                     atlasWidth;        // Atlas texture width
    int                     atlasHeight;       // Atlas texture height
} EngineFont;

// ============================================================================
// FONT API FUNCTIONS
// ============================================================================

// Core font management
EngineFontHandle engine_font_create(const char* fontname, uint32_t size, float spacing, MetalEngineHandle metalEngine);
void engine_font_destroy(EngineFontHandle font);

// Text rendering
int engine_font_render_text(EngineFontHandle font, Engine2D* ui2d, vec2_t pos, float scale, const char* text);

#ifdef __cplusplus
}
#endif

#endif // ENGINE_FONT_H
