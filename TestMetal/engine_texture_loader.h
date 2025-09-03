#ifndef ENGINE_TEXTURE_LOADER_H
#define ENGINE_TEXTURE_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine_metal.h"
#include "engine_math.h"
#include <stdint.h>
#include <stddef.h>

// Forward declarations for Metal types
// MTLPixelFormat is defined in Metal framework
// We'll use uint32_t for C compatibility
typedef uint32_t TexturePixelFormat;

// ============================================================================
// TEXTURE LOADER CONFIGURATION
// ============================================================================

#define TEXTURE_CACHE_DEFAULT_SIZE 64        // Default cache size
#define TEXTURE_CACHE_MAX_SIZE 256           // Maximum cache size
#define TEXTURE_FALLBACK_SIZE 512            // Fallback texture size
#define TEXTURE_MAX_DIMENSION 4096           // Maximum texture dimension
#define TEXTURE_FILENAME_MAX_LENGTH 256      // Maximum filename length

// ============================================================================
// TEXTURE LOADER TYPES
// ============================================================================

// Forward declaration
typedef struct TextureLoader* TextureLoaderHandle;

// Texture cache entry
typedef struct {
    char filename[TEXTURE_FILENAME_MAX_LENGTH];  // Original filename (key)
    MetalTextureHandle texture;                  // Metal texture handle
    uint32_t width;                              // Texture width
    uint32_t height;                             // Texture height
    uint32_t channels;                           // Number of channels (3=RGB, 4=RGBA)
    uint64_t lastAccessed;                       // Timestamp for LRU cache management
    uint32_t refCount;                           // Reference counting
    int isValid;                                 // Whether this entry is valid
} TextureCacheEntry;

// Texture loading options
typedef struct {
    TexturePixelFormat pixelFormat;              // Target pixel format
    int generateMipmaps;                         // Generate mipmaps
    int flipVertically;                          // Flip texture vertically
    int srgb;                                    // Use sRGB color space
} TextureLoadOptions;

// Error codes
typedef enum {
    TEXTURE_LOADER_SUCCESS = 0,
    TEXTURE_LOADER_ERROR_INVALID_PARAMS,
    TEXTURE_LOADER_ERROR_FILE_NOT_FOUND,
    TEXTURE_LOADER_ERROR_INVALID_FORMAT,
    TEXTURE_LOADER_ERROR_MEMORY_ALLOCATION,
    TEXTURE_LOADER_ERROR_METAL_CREATION,
    TEXTURE_LOADER_ERROR_CACHE_FULL,
    TEXTURE_LOADER_ERROR_NOT_INITIALIZED
} TextureLoaderResult;

// Cache statistics
typedef struct {
    uint32_t size;                               // Current cache size
    uint32_t maxSize;                            // Maximum cache size
    size_t memoryUsage;                          // Estimated memory usage in bytes
    uint32_t hitCount;                           // Cache hits
    uint32_t missCount;                          // Cache misses
} TextureCacheStats;

// ============================================================================
// TEXTURE LOADER API
// ============================================================================

// Initialization & Lifecycle
TextureLoaderHandle texture_loader_init(MetalDeviceHandle device, const char* resourcePath);
void texture_loader_shutdown(TextureLoaderHandle loader);
void texture_loader_clean_cache(TextureLoaderHandle loader);

// Core Loading Functions
MetalTextureHandle texture_loader_load(TextureLoaderHandle loader, const char* filename);
MetalTextureHandle texture_loader_load_with_options(TextureLoaderHandle loader, 
                                                   const char* filename,
                                                   const TextureLoadOptions* options);

// Batch Operations
int texture_loader_preload(TextureLoaderHandle loader, const char** filenames, uint32_t count);

// Cache Management
int texture_loader_get_info(TextureLoaderHandle loader, const char* filename, 
                           uint32_t* width, uint32_t* height, uint32_t* channels);
int texture_loader_remove(TextureLoaderHandle loader, const char* filename);
void texture_loader_get_stats(TextureLoaderHandle loader, TextureCacheStats* stats);

// Utility Functions
void texture_loader_set_fallback(TextureLoaderHandle loader, MetalTextureHandle fallback);
MetalTextureHandle texture_loader_create_fallback(MetalDeviceHandle device);
int texture_loader_is_cached(TextureLoaderHandle loader, const char* filename);

// Error Handling
const char* texture_loader_get_error_string(TextureLoaderResult result);

// ============================================================================
// INTERNAL FUNCTIONS (for testing)
// ============================================================================

// Hash function for filename
uint32_t texture_loader_hash_filename(const char* filename);

// Find cache entry by filename
TextureCacheEntry* texture_loader_find_entry(TextureLoaderHandle loader, const char* filename);

// Add entry to cache
int texture_loader_add_to_cache(TextureLoaderHandle loader, const char* filename, 
                               MetalTextureHandle texture, uint32_t width, uint32_t height, uint32_t channels);

// Remove least recently used entry
int texture_loader_evict_lru(TextureLoaderHandle loader);

// Load texture from file using stb_image
MetalTextureHandle texture_loader_load_from_file(TextureLoaderHandle loader, const char* filename, 
                                                const TextureLoadOptions* options);

#ifdef __cplusplus
}
#endif

#endif // ENGINE_TEXTURE_LOADER_H
