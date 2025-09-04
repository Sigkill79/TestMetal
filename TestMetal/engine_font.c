#include "engine_font.h"
#include "engine_texture_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include STB TrueType
#define STB_TRUETYPE_IMPLEMENTATION
#include "libraries/stb/stb_truetype.h"

// Include stretchy buffer
#include "stretchy_buffer.h"

// Debug logging
#define FONT_DEBUG(fmt, ...) printf("[FONT] " fmt "\n", ##__VA_ARGS__)
#define FONT_ERROR(fmt, ...) printf("[FONT ERROR] " fmt "\n", ##__VA_ARGS__)
#define FONT_INFO(fmt, ...) printf("[FONT INFO] " fmt "\n", ##__VA_ARGS__)

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

static MetalTextureHandle create_metal_texture_from_data(unsigned char* data, int width, int height, int channels, MetalEngineHandle metalEngine);
static int engine_font_generate_glyphs(EngineFontHandle font, stbtt_fontinfo* fontInfo, uint32_t size);
static int engine_font_create_atlas(EngineFontHandle font, stbtt_fontinfo* fontInfo, uint32_t size);

// ============================================================================
// CONSTANTS
// ============================================================================

#define FONT_ATLAS_PADDING 5
#define FONT_MAX_GLYPHS 256
#define FONT_MAX_KERNINGS 1024
#define FONT_ATLAS_GRID_SIZE 16

// Font error codes
typedef enum {
    FONT_SUCCESS = 0,
    FONT_ERROR_INVALID_PARAMS,
    FONT_ERROR_FILE_NOT_FOUND,
    FONT_ERROR_MEMORY_ALLOCATION,
    FONT_ERROR_TEXTURE_CREATION,
    FONT_ERROR_GLYPH_GENERATION,
    FONT_ERROR_NOT_INITIALIZED
} FontResult;

// ============================================================================
// ERROR HANDLING
// ============================================================================

const char* engine_font_get_error_string(FontResult result) {
    switch (result) {
        case FONT_SUCCESS: return "Success";
        case FONT_ERROR_INVALID_PARAMS: return "Invalid parameters";
        case FONT_ERROR_FILE_NOT_FOUND: return "Font file not found";
        case FONT_ERROR_MEMORY_ALLOCATION: return "Memory allocation failed";
        case FONT_ERROR_TEXTURE_CREATION: return "Texture creation failed";
        case FONT_ERROR_GLYPH_GENERATION: return "Glyph generation failed";
        case FONT_ERROR_NOT_INITIALIZED: return "Font not initialized";
        default: return "Unknown error";
    }
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static int file_exists(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

static char* build_font_path(const char* resourcePath, const char* fontname) {
    if (!resourcePath || !fontname) return NULL;
    
    size_t pathLen = strlen(resourcePath);
    size_t filenameLen = strlen(fontname);
    size_t totalLen = pathLen + filenameLen + 2; // +2 for '/' and '\0'
    
    char* fullPath = (char*)malloc(totalLen);
    if (!fullPath) return NULL;
    
    strcpy(fullPath, resourcePath);
    if (resourcePath[pathLen - 1] != '/') {
        strcat(fullPath, "/");
    }
    strcat(fullPath, fontname);
    
    return fullPath;
}

// ============================================================================
// FONT CREATION AND DESTRUCTION
// ============================================================================

EngineFontHandle engine_font_create(const char* fontname, uint32_t size, float spacing, MetalEngineHandle metalEngine) {
    if (!fontname || size == 0 || !metalEngine) {
        FONT_ERROR("Invalid parameters for font creation");
        return NULL;
    }
    
    const int padding = 5;
    
    FONT_INFO("Creating font: %s, size: %u, spacing: %.2f", fontname, size, spacing);
    
    // Allocate font structure
    EngineFontHandle font = (EngineFontHandle)malloc(sizeof(EngineFont));
    if (!font) {
        FONT_ERROR("Failed to allocate font structure");
        return NULL;
    }
    
    // Initialize font structure
    memset(font, 0, sizeof(EngineFont));
    font->fontSize = size;
    font->spacing = spacing;
    font->metalEngine = metalEngine;
    font->isInitialized = 0;
    
    // Load TTF font file
    char ttfBuffer[1<<20];
    stbtt_fontinfo fontInfo;
    
    FILE* file = fopen(fontname, "rb");
    if (!file) {
        FONT_ERROR("Failed to load font %s", fontname);
        free(font);
        return NULL;
    }
    
    if (!fread(ttfBuffer, 1, 1<<20, file)) {
        FONT_ERROR("Failed to load font file %s", fontname);
        fclose(file);
        free(font);
        return NULL;
    }
    fclose(file);
    
    stbtt_InitFont(&fontInfo, (const unsigned char*)ttfBuffer, 
                   stbtt_GetFontOffsetForIndex((const unsigned char*)ttfBuffer, 0));
    
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, size);
    
    // Get font metrics
    int ascent, descent, linegap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &linegap);
    font->lineSpacing = (uint32_t)(scale * (ascent - descent + linegap));
    
    // Create atlas (16x16 grid)
    const int atlasWidth = 16 * (size + 2 * padding);
    const int atlasHeight = 16 * (size + 2 * padding);
    unsigned char* fontAtlas = (unsigned char*)malloc(atlasWidth * atlasHeight);
    memset(fontAtlas, 0, atlasWidth * atlasHeight);
    
    // Generate all 256 glyphs (ASCII 0-255)
    for (unsigned char ascii = 0; ascii < 255; ++ascii) {
        EngineFontGlyph* newGlyph = &font->glyphs[ascii];
        
        // Get codepoint metrics
        int advanceWidth, leftSideBearing;
        stbtt_GetCodepointHMetrics(&fontInfo, ascii, &advanceWidth, &leftSideBearing);
        newGlyph->advance = (int)(scale * advanceWidth);
        
        // Generate SDF bitmap
        int width, height, xoff, yoff;
        unsigned char* sdf = stbtt_GetCodepointSDF(&fontInfo, scale, ascii, padding, 128, 128/5.0,
                                                   &width, &height, &xoff, &yoff);
        
        if (width > size + 2 * padding || height > size + 2 * padding) {
            FONT_ERROR("Glyph %d oversized, expected %d got %d, %d", ascii, size + 2 * padding, width, height);
            free(fontAtlas);
            free(font);
            return NULL;
        }
        
        // Set glyph properties
        newGlyph->width = width;
        newGlyph->height = height;
        newGlyph->startX = xoff;  // Rendering offset
        newGlyph->startY = yoff;  // Rendering offset
        
        // Calculate atlas position (16x16 grid)
        newGlyph->bitmapTop = (size + 2 * padding) * (ascii / 16);
        newGlyph->bitmapLeft = (size + 2 * padding) * (ascii % 16);
        
        // Copy SDF data to atlas
        if (sdf) {
            for (int y = 0; y < height; ++y) {
                memcpy(&fontAtlas[(newGlyph->bitmapTop + y) * atlasWidth + newGlyph->bitmapLeft], 
                       &sdf[y * width], width);
            }
            stbtt_FreeSDF(sdf, NULL);
        }
    }
    
    // Generate kerning pairs
    for (unsigned char asc1 = 0; asc1 < 255; ++asc1) {
        for (unsigned char asc2 = 0; asc2 < 255; ++asc2) {
            int kern = stbtt_GetCodepointKernAdvance(&fontInfo, asc1, asc2);
            if (kern) {
                if (font->kerningCount >= font->maxKernings) {
                    font->maxKernings = font->maxKernings ? font->maxKernings * 2 : 256;
                    font->kernings = (EngineFontKerning*)realloc(font->kernings, 
                                                                 font->maxKernings * sizeof(EngineFontKerning));
                }
                
                EngineFontKerning* newKern = &font->kernings[font->kerningCount++];
                newKern->first = asc1;
                newKern->second = asc2;
                newKern->kerning = (int)(kern * scale);
            }
        }
    }
    
    // Convert grayscale atlas to RGBA format
    unsigned char* rgbaAtlas = (unsigned char*)malloc(atlasWidth * atlasHeight * 4);
    for (int y = 0; y < atlasHeight; ++y) {
        for (int x = 0; x < atlasWidth; ++x) {
            int index = (x + y * atlasWidth) * 4;
            rgbaAtlas[index + 0] = fontAtlas[x + y * atlasWidth];  // R
            rgbaAtlas[index + 1] = 255;  // G
            rgbaAtlas[index + 2] = 255;  // B
            rgbaAtlas[index + 3] = 255;  // A (from SDF)
        }
    }
    
    // Create Metal texture from atlas data
    font->texture = metal_engine_create_texture_from_data(font->metalEngine, fontAtlas, atlasWidth, atlasHeight, 1);

    // Clean up
    free(fontAtlas);
    free(rgbaAtlas);
    
    if (!font->texture) {
        FONT_ERROR("Failed to create Metal texture for font atlas");
        free(font);
        return NULL;
    }

    font->atlasWidth = atlasWidth;
    font->atlasHeight = atlasHeight;
    
    font->isInitialized = 1;
    FONT_INFO("Successfully created font with 256 glyphs and %u kerning pairs", font->kerningCount);
    
    return font;
}

void engine_font_destroy(EngineFontHandle font) {
    if (!font) return;
    
    FONT_INFO("Destroying font");
    
    // Free kerning array
    if (font->kernings) {
        free(font->kernings);
    }
    
    // Free font structure
    free(font);
}

// ============================================================================
// TEXT RENDERING
// ============================================================================

int engine_font_render_text(EngineFontHandle font, Engine2D* ui2d, vec2_t pos, float scale, const char* text) {
    if (!font || !ui2d || !text) {
        FONT_ERROR("Invalid parameters for text rendering");
        return 0;
    }
    
    if (!font->isInitialized) {
        FONT_ERROR("Font not initialized");
        return 0;
    }
    
    vec2_t penPos = pos;
    
    for (const char* chr = text; *chr; ++chr) {
        if (*chr == '\n') {
            penPos.x = pos.x;
            penPos.y += font->lineSpacing * scale;
        } else if (*chr == '\r') {
            penPos.x = pos.x;
        } else if (*chr == '\t') {
            // Tab = 4 spaces
            penPos.x += 4 * font->glyphs[' '].advance * scale;
        } else {
            // O(1) glyph lookup using ASCII index
            EngineFontGlyph* glyph = &font->glyphs[(unsigned char)*chr];
            
            // Calculate glyph position (using original coordinate system)
            vec2_t glyphPos = {
                penPos.x + glyph->startX * scale,  // startX is rendering offset
                penPos.y + glyph->startY * scale   // startY is rendering offset
            };
            
            // Calculate atlas texture coordinates (using original coordinate system)
            vec2_t texCoord = {
                glyph->bitmapLeft / (float)font->atlasWidth,                 // bitmapLeft is atlas X coordinate
                glyph->bitmapTop / (float)font->atlasHeight                  // bitmapTop is atlas Y coordinate
            };
            vec2_t texSize = {
                glyph->width / (float)font->atlasWidth,                      // Width in atlas
                glyph->height / (float)font->atlasHeight                     // Height in atlas
            };
            
            // Render glyph as SDF with atlas coordinates
            if (font->texture) {
                engine_2d_draw_sdf_atlas(
                    ui2d,
                    glyphPos.x,
                    glyphPos.y,
                    glyph->width * scale,           // Actual pixel width
                    glyph->height * scale,          // Actual pixel height
                    font->texture,
                    texCoord,
                    texSize,
                    vec4(1.0f, 1.0f, 1.0f, 1.0f),  // fillColor
                    vec4(0.0f, 0.0f, 0.0f, 1.0f),  // outlineColor
                    0.5f, 1.0f/255.0f, 0, 0             // edgeDistance, outlineDistance, smoothing, hasOutline
                );
            } else {
                FONT_ERROR("Font texture is NULL for glyph '%c'", *chr);
            }
            
            // Advance pen position
            penPos.x += glyph->advance * scale;
            
            // Apply kerning (matches original implementation)
            char next = *(chr + 1);
            if (next) {
                for (uint32_t i = 0; i < font->kerningCount; ++i) {
                    EngineFontKerning* kern = &font->kernings[i];
                    if (kern->first == *chr && kern->second == next) {
                        penPos.x += kern->kerning * scale;
                        break;
                    }
                }
            }
        }
    }
    
    return 1;
}



// ============================================================================
// TEXT UTILITIES
// ============================================================================

// Note: Utility functions removed - only essential functions remain

// ============================================================================
// METAL TEXTURE CREATION
// ============================================================================
/* 
static MetalTextureHandle create_metal_texture_from_data(unsigned char* data, int width, int height, int channels, MetalEngineHandle metalEngine) {
    if (!data || width <= 0 || height <= 0 || channels <= 0 || !metalEngine) {
        FONT_ERROR("Invalid parameters for texture creation");
        return NULL;
    }
    

    
    // Use the Metal engine helper function to create the texture
    MetalTextureHandle texture = metal_engine_create_texture_from_data(metalEngine, data, width, height, channels);
    if (!texture) {
        FONT_ERROR("Failed to create Metal texture via engine helper");
        return NULL;
    }
    

    return texture;
} */
