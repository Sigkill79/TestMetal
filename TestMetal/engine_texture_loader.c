#include "engine_texture_loader.h"
#include "engine_metal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

// Include Objective-C headers for Metal
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

// Include stb_image
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Metal pixel format constants for C compatibility (correct values from Metal headers)
#define MTLPixelFormatR8Unorm 10
#define MTLPixelFormatRG8Unorm 20
#define MTLPixelFormatRGBA8Unorm 70
#define MTLPixelFormatRGBA8Unorm_sRGB 71

// Debug logging
#define TEXTURE_DEBUG(fmt, ...) printf("[TEXTURE_LOADER] " fmt "\n", ##__VA_ARGS__)
#define TEXTURE_ERROR(fmt, ...) printf("[TEXTURE_LOADER ERROR] " fmt "\n", ##__VA_ARGS__)
#define TEXTURE_INFO(fmt, ...) printf("[TEXTURE_LOADER INFO] " fmt "\n", ##__VA_ARGS__)

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

typedef struct TextureLoader {
    MetalDeviceHandle device;                    // Metal device for texture creation
    TextureCacheEntry* cache;                    // Hash table of cached textures
    uint32_t cacheSize;                          // Current number of cached textures
    uint32_t maxCacheSize;                       // Maximum cache size
    char* resourcePath;                          // Base path for texture files
    MetalTextureHandle fallbackTexture;          // Fallback texture for failed loads
    int isInitialized;                           // Initialization state
    TextureCacheStats stats;                     // Cache statistics
} TextureLoader;

// ============================================================================
// ERROR HANDLING
// ============================================================================

const char* texture_loader_get_error_string(TextureLoaderResult result) {
    switch (result) {
        case TEXTURE_LOADER_SUCCESS: return "Success";
        case TEXTURE_LOADER_ERROR_INVALID_PARAMS: return "Invalid parameters";
        case TEXTURE_LOADER_ERROR_FILE_NOT_FOUND: return "File not found";
        case TEXTURE_LOADER_ERROR_INVALID_FORMAT: return "Invalid texture format";
        case TEXTURE_LOADER_ERROR_MEMORY_ALLOCATION: return "Memory allocation failed";
        case TEXTURE_LOADER_ERROR_METAL_CREATION: return "Metal texture creation failed";
        case TEXTURE_LOADER_ERROR_CACHE_FULL: return "Cache is full";
        case TEXTURE_LOADER_ERROR_NOT_INITIALIZED: return "Texture loader not initialized";
        default: return "Unknown error";
    }
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static uint64_t get_current_timestamp(void) {
    return (uint64_t)time(NULL);
}

static int file_exists(const char* filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

static char* build_full_path(const char* resourcePath, const char* filename) {
    if (!resourcePath || !filename) return NULL;
    
    size_t pathLen = strlen(resourcePath);
    size_t filenameLen = strlen(filename);
    size_t totalLen = pathLen + filenameLen + 2; // +2 for '/' and '\0'
    
    char* fullPath = (char*)malloc(totalLen);
    if (!fullPath) return NULL;
    
    strcpy(fullPath, resourcePath);
    if (resourcePath[pathLen - 1] != '/') {
        strcat(fullPath, "/");
    }
    strcat(fullPath, filename);
    
    return fullPath;
}

// ============================================================================
// HASH FUNCTIONS
// ============================================================================

uint32_t texture_loader_hash_filename(const char* filename) {
    if (!filename) return 0;
    
    uint32_t hash = 5381;
    int c;
    while ((c = *filename++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

// ============================================================================
// CACHE MANAGEMENT
// ============================================================================



TextureCacheEntry* texture_loader_find_entry(TextureLoaderHandle loader, const char* filename) {
    if (!loader || !filename) return NULL;
    
    TextureLoader* tl = (TextureLoader*)loader;
    uint32_t hash = texture_loader_hash_filename(filename) % tl->maxCacheSize;
    
    // Linear probing for collision resolution
    for (uint32_t i = 0; i < tl->maxCacheSize; i++) {
        uint32_t index = (hash + i) % tl->maxCacheSize;
        TextureCacheEntry* entry = &tl->cache[index];
        
        if (entry->isValid && strcmp(entry->filename, filename) == 0) {
            // Update access time and reference count
            entry->lastAccessed = get_current_timestamp();
            entry->refCount++;
            tl->stats.hitCount++;
            return entry;
        }
    }
    
    tl->stats.missCount++;
    return NULL;
}

int texture_loader_add_to_cache(TextureLoaderHandle loader, const char* filename, 
                               MetalTextureHandle texture, uint32_t width, uint32_t height, uint32_t channels) {
    if (!loader || !filename || !texture) return 0;
    
    TextureLoader* tl = (TextureLoader*)loader;
    
    // Check if cache is full
    if (tl->cacheSize >= tl->maxCacheSize) {
        if (!texture_loader_evict_lru(loader)) {
            return 0; // Failed to evict
        }
    }
    
    uint32_t hash = texture_loader_hash_filename(filename) % tl->maxCacheSize;
    
    // Find empty slot or replace existing
    for (uint32_t i = 0; i < tl->maxCacheSize; i++) {
        uint32_t index = (hash + i) % tl->maxCacheSize;
        TextureCacheEntry* entry = &tl->cache[index];
        
        if (!entry->isValid) {
            // Found empty slot
            strncpy(entry->filename, filename, TEXTURE_FILENAME_MAX_LENGTH - 1);
            entry->filename[TEXTURE_FILENAME_MAX_LENGTH - 1] = '\0';
            entry->texture = texture;
            entry->width = width;
            entry->height = height;
            entry->channels = channels;
            entry->lastAccessed = get_current_timestamp();
            entry->refCount = 1;
            entry->isValid = 1;
            
            tl->cacheSize++;
            tl->stats.memoryUsage += width * height * channels;
            return 1;
        }
    }
    
    return 0; // Cache is full and couldn't evict
}

int texture_loader_evict_lru(TextureLoaderHandle loader) {
    if (!loader) return 0;
    
    TextureLoader* tl = (TextureLoader*)loader;
    if (tl->cacheSize == 0) return 1; // Nothing to evict
    
    uint64_t oldestTime = UINT64_MAX;
    uint32_t oldestIndex = 0;
    
    // Find least recently used entry
    for (uint32_t i = 0; i < tl->maxCacheSize; i++) {
        TextureCacheEntry* entry = &tl->cache[i];
        if (entry->isValid && entry->lastAccessed < oldestTime) {
            oldestTime = entry->lastAccessed;
            oldestIndex = i;
        }
    }
    
    // Remove the oldest entry
    TextureCacheEntry* entry = &tl->cache[oldestIndex];
    if (entry->isValid) {
        // Note: In a real implementation, we would release the Metal texture here
        entry->isValid = 0;
        tl->cacheSize--;
        tl->stats.memoryUsage -= entry->width * entry->height * entry->channels;
        return 1;
    }
    
    return 0;
}

// ============================================================================
// TEXTURE LOADING
// ============================================================================

MetalTextureHandle texture_loader_load_from_file(TextureLoaderHandle loader, const char* filename, 
                                                const TextureLoadOptions* options) {
    TEXTURE_DEBUG("Loading texture from file: %s", filename);
    
    if (!loader || !filename) {
        TEXTURE_ERROR("Invalid parameters for load_from_file: loader=%p, filename=%s", loader, filename);
        return NULL;
    }
    
    TextureLoader* tl = (TextureLoader*)loader;
    if (!tl->isInitialized) {
        TEXTURE_ERROR("Texture loader not initialized in load_from_file");
        return NULL;
    }
    
    // Build full path
    char* fullPath = build_full_path(tl->resourcePath, filename);
    if (!fullPath) {
        TEXTURE_ERROR("Failed to build full path for: %s", filename);
        return NULL;
    }
    TEXTURE_DEBUG("Full path: %s", fullPath);
    
    // Check if file exists
    if (!file_exists(fullPath)) {
        TEXTURE_ERROR("File does not exist: %s", fullPath);
        free(fullPath);
        return NULL;
    }
    TEXTURE_DEBUG("File exists: %s", fullPath);
    
    // Load image data using stb_image
    int width, height, channels;
    TEXTURE_DEBUG("Loading image with stb_image...");
    unsigned char* data = stbi_load(fullPath, &width, &height, &channels, 0);
    free(fullPath);
    
    if (!data) {
        TEXTURE_ERROR("stb_image failed to load: %s", filename);
        return NULL; // stb_image failed to load
    }
    
    TEXTURE_DEBUG("Image loaded successfully: %dx%d, %d channels", width, height, channels);
    
    // Debug: Print first few pixels
    if (width > 0 && height > 0) {
        TEXTURE_DEBUG("First pixel data (channels=%d):", channels);
        for (int c = 0; c < channels && c < 4; c++) {
            TEXTURE_DEBUG("  Channel %d: %d", c, data[c]);
        }
        if (width > 1) {
            TEXTURE_DEBUG("Second pixel data:");
            for (int c = 0; c < channels && c < 4; c++) {
                TEXTURE_DEBUG("  Channel %d: %d", c, data[channels + c]);
            }
        }
    }
    
    // Validate dimensions
    if (width <= 0 || height <= 0 || width > TEXTURE_MAX_DIMENSION || height > TEXTURE_MAX_DIMENSION) {
        TEXTURE_ERROR("Invalid texture dimensions: %dx%d (max: %d)", width, height, TEXTURE_MAX_DIMENSION);
        stbi_image_free(data);
        return NULL;
    }
    
    // Create Metal texture
    id<MTLDevice> device = (__bridge id<MTLDevice>)tl->device;
    TEXTURE_DEBUG("Creating Metal texture with device: %p", device);
    
    // Determine pixel format based on channels and options
    TexturePixelFormat pixelFormat;
    int targetChannels = channels;
    unsigned char* uploadData = data;
    
    if (options && options->pixelFormat != 0) {
        pixelFormat = options->pixelFormat;
        TEXTURE_DEBUG("Using custom pixel format: %u", pixelFormat);
    } else {
        switch (channels) {
            case 1: 
                pixelFormat = MTLPixelFormatR8Unorm; 
                targetChannels = 1;
                break;
            case 2: 
                pixelFormat = MTLPixelFormatRG8Unorm; 
                targetChannels = 2;
                break;
            case 3: 
                // RGB not supported in Metal, need to convert to RGBA
                pixelFormat = MTLPixelFormatRGBA8Unorm; 
                targetChannels = 4;
                TEXTURE_DEBUG("Converting RGB to RGBA format");
                break;
            case 4: 
                pixelFormat = MTLPixelFormatRGBA8Unorm; 
                targetChannels = 4;
                break;
            default: 
                TEXTURE_ERROR("Unsupported channel count: %d", channels);
                stbi_image_free(data);
                return NULL;
        }
        TEXTURE_DEBUG("Selected pixel format: %u for %d channels -> %d target channels", pixelFormat, channels, targetChannels);
    }
    
    
    // Convert RGB to RGBA if needed
    if (channels == 3 && targetChannels == 4) {
        TEXTURE_DEBUG("Converting RGB data to RGBA format");
        size_t rgbSize = width * height * 3;
        size_t rgbaSize = width * height * 4;
        uploadData = (unsigned char*)malloc(rgbaSize);
        
        if (!uploadData) {
            TEXTURE_ERROR("Failed to allocate memory for RGBA conversion");
            stbi_image_free(data);
            return NULL;
        }
        
        // Convert RGB to RGBA by adding alpha channel
        for (size_t i = 0; i < width * height; i++) {
            uploadData[i * 4 + 0] = data[i * 3 + 0]; // R
            uploadData[i * 4 + 1] = data[i * 3 + 1]; // G
            uploadData[i * 4 + 2] = data[i * 3 + 2]; // B
            uploadData[i * 4 + 3] = 255;             // A (fully opaque)
        }
        
        TEXTURE_DEBUG("RGB to RGBA conversion completed: %zu bytes -> %zu bytes", rgbSize, rgbaSize);
        
        // Debug: Print first few converted pixels
        TEXTURE_DEBUG("First converted pixel data (RGBA):");
        for (int c = 0; c < 4; c++) {
            TEXTURE_DEBUG("  Channel %d: %d", c, uploadData[c]);
        }
        if (width > 1) {
            TEXTURE_DEBUG("Second converted pixel data:");
            for (int c = 0; c < 4; c++) {
                TEXTURE_DEBUG("  Channel %d: %d", c, uploadData[4 + c]);
            }
        }
    }
    
    // Create texture descriptor
    MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat
                                                                                            width:width
                                                                                           height:height
                                                                                        mipmapped:/*(options && options->generateMipmaps) ? YES :*/ NO];
    textureDesc.usage = MTLTextureUsageShaderRead;
    TEXTURE_DEBUG("Created texture descriptor: %dx%d, format=%u, targetChannels=%d", width, height, pixelFormat, targetChannels);
    
    id<MTLTexture> texture = [device newTextureWithDescriptor:textureDesc];
    if (!texture) {
        TEXTURE_ERROR("Failed to create Metal texture");
        if (uploadData != data) free(uploadData);
        stbi_image_free(data);
        return NULL;
    }
    TEXTURE_DEBUG("Metal texture created successfully: %p", texture);
    
    // Upload texture data
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    
    NSUInteger bytesPerRow = width * targetChannels;
    TEXTURE_DEBUG("Uploading texture data: %dx%d, %lu bytes per row (channels: %d->%d)",
                  width, height, (unsigned long)bytesPerRow, channels, targetChannels);
    
    [texture replaceRegion:region
                mipmapLevel:0
                withBytes:uploadData
                bytesPerRow:bytesPerRow
               ];
    
    // Generate mipmaps if requested
    if (options && options->generateMipmaps) {
        TEXTURE_DEBUG("Mipmap generation requested but not implemented yet");
        // Note: In a real implementation, we would generate mipmaps here
        // For now, we just create the texture without mipmaps
    }
    
    // Free image data
    if (uploadData != data) {
        free(uploadData);
        TEXTURE_DEBUG("Freed converted RGBA data");
    }
    stbi_image_free(data);
    
    TEXTURE_INFO("Successfully loaded and created texture: %s -> %p (format: %u, channels: %d->%d)", 
                 filename, texture, pixelFormat, channels, targetChannels);
    return (__bridge_retained MetalTextureHandle)texture;
}

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

TextureLoaderHandle texture_loader_init(MetalDeviceHandle device, const char* resourcePath) {
    TEXTURE_DEBUG("Initializing texture loader with device: %p, resourcePath: %s", device, resourcePath);
    
    if (!device) {
        TEXTURE_ERROR("Invalid device parameter");
        return NULL;
    }
    
    TextureLoader* loader = (TextureLoader*)malloc(sizeof(TextureLoader));
    if (!loader) {
        TEXTURE_ERROR("Failed to allocate memory for texture loader");
        return NULL;
    }
    
    memset(loader, 0, sizeof(TextureLoader));
    
    loader->device = device;
    loader->maxCacheSize = TEXTURE_CACHE_DEFAULT_SIZE;
    loader->isInitialized = 0;
    
    // Copy resource path
    if (resourcePath) {
        loader->resourcePath = strdup(resourcePath);
        if (!loader->resourcePath) {
            free(loader);
            return NULL;
        }
    } else {
        loader->resourcePath = strdup(".");
        if (!loader->resourcePath) {
            free(loader);
            return NULL;
        }
    }
    
    // Allocate cache
    loader->cache = (TextureCacheEntry*)calloc(loader->maxCacheSize, sizeof(TextureCacheEntry));
    if (!loader->cache) {
        free(loader->resourcePath);
        free(loader);
        return NULL;
    }
    
    // Create fallback texture
    loader->fallbackTexture = texture_loader_create_fallback(device);
    if (!loader->fallbackTexture) {
        free(loader->cache);
        free(loader->resourcePath);
        free(loader);
        return NULL;
    }
    
    loader->isInitialized = 1;
    
    printf("Texture loader initialized with cache size: %u, resource path: %s\n", 
           loader->maxCacheSize, loader->resourcePath);
    
    return (TextureLoaderHandle)loader;
}

void texture_loader_shutdown(TextureLoaderHandle loader) {
    if (!loader) return;
    
    TextureLoader* tl = (TextureLoader*)loader;
    
    // Clean cache first
    texture_loader_clean_cache(loader);
    
    // Free cache array
    if (tl->cache) {
        free(tl->cache);
    }
    
    // Free resource path
    if (tl->resourcePath) {
        free(tl->resourcePath);
    }
    
    // Note: In a real implementation, we would release the fallback texture here
    
    free(tl);
    
    printf("Texture loader shutdown\n");
}

void texture_loader_clean_cache(TextureLoaderHandle loader) {
    if (!loader) return;
    
    TextureLoader* tl = (TextureLoader*)loader;
    TEXTURE_DEBUG("Clearing texture cache...");
    
    // Properly release all cached textures
    for (uint32_t i = 0; i < tl->maxCacheSize; i++) {
        TextureCacheEntry* entry = &tl->cache[i];
        if (entry->isValid) {
            if (entry->texture) {
                CFRelease(entry->texture);
                TEXTURE_DEBUG("Released cached texture: %s", entry->filename);
            }
            entry->isValid = 0;
            entry->texture = NULL;
            entry->filename[0] = '\0';
            entry->width = 0;
            entry->height = 0;
            entry->channels = 0;
            entry->refCount = 0;
            entry->lastAccessed = 0;
        }
    }
    
    tl->cacheSize = 0;
    tl->stats.memoryUsage = 0;
    tl->stats.hitCount = 0;
    tl->stats.missCount = 0;
    TEXTURE_INFO("Texture cache cleared successfully");
    
    printf("Texture cache cleaned\n");
}

MetalTextureHandle texture_loader_load(TextureLoaderHandle loader, const char* filename) {
    TEXTURE_DEBUG("Loading texture: %s", filename);
    
    if (!loader || !filename) {
        TEXTURE_ERROR("Invalid parameters: loader=%p, filename=%s", loader, filename);
        return NULL;
    }
    
    TextureLoader* tl = (TextureLoader*)loader;
    if (!tl->isInitialized) {
        TEXTURE_ERROR("Texture loader not initialized");
        return NULL;
    }
    
    // Check cache first
    TEXTURE_DEBUG("Checking cache for: %s", filename);
    TextureCacheEntry* entry = texture_loader_find_entry(loader, filename);
    if (entry) {
        TEXTURE_DEBUG("Cache hit for: %s", filename);
        return entry->texture;
    }
    
    TEXTURE_DEBUG("Cache miss for: %s, loading from file", filename);
    
    // Load from file
    MetalTextureHandle texture = texture_loader_load_from_file(loader, filename, NULL);
    if (!texture) {
        TEXTURE_ERROR("Failed to load texture from file: %s, returning fallback", filename);
        // Return fallback texture on failure
        return tl->fallbackTexture;
    }
    
    TEXTURE_DEBUG("Successfully loaded texture: %s", filename);
    
    // Add to cache
    // Note: We need to get texture dimensions for cache entry
    // For now, we'll use placeholder values
    texture_loader_add_to_cache(loader, filename, texture, 256, 256, 4);
    
    return texture;
}

MetalTextureHandle texture_loader_load_with_options(TextureLoaderHandle loader, 
                                                   const char* filename,
                                                   const TextureLoadOptions* options) {
    if (!loader || !filename) return NULL;
    
    TextureLoader* tl = (TextureLoader*)loader;
    if (!tl->isInitialized) return NULL;
    
    // For now, we don't cache textures loaded with options
    // In a future implementation, we could include options in the cache key
    MetalTextureHandle texture = texture_loader_load_from_file(loader, filename, options);
    if (!texture) {
        return tl->fallbackTexture;
    }
    
    return texture;
}

MetalTextureHandle texture_loader_load_sdf(TextureLoaderHandle loader, const char* filename) {
    TEXTURE_DEBUG("Loading SDF texture: %s", filename);
    
    if (!loader || !filename) {
        TEXTURE_ERROR("Invalid parameters for SDF texture loading: loader=%p, filename=%s", loader, filename);
        return NULL;
    }
    
    TextureLoader* tl = (TextureLoader*)loader;
    if (!tl->isInitialized) {
        TEXTURE_ERROR("Texture loader not initialized for SDF loading");
        return NULL;
    }
    
    // Check cache first (SDF textures can be cached like regular textures)
    TEXTURE_DEBUG("Checking cache for SDF texture: %s", filename);
    TextureCacheEntry* entry = texture_loader_find_entry(loader, filename);
    if (entry) {
        TEXTURE_DEBUG("Cache hit for SDF texture: %s", filename);
        return entry->texture;
    }
    
    TEXTURE_DEBUG("Cache miss for SDF texture: %s, loading from file", filename);
    
    // Set up SDF-specific loading options
    TextureLoadOptions sdfOptions = {
        .pixelFormat = MTLPixelFormatR8Unorm,  // Single channel for SDF
        .generateMipmaps = 0,                   // SDFs typically don't use mipmaps
        .flipVertically = 0,
        .srgb = 0                               // SDFs are not color data
    };
    
    // Load from file with SDF options
    MetalTextureHandle texture = texture_loader_load_from_file(loader, filename, &sdfOptions);
    if (!texture) {
        TEXTURE_ERROR("Failed to load SDF texture from file: %s, returning fallback", filename);
        return tl->fallbackTexture;
    }
    
    TEXTURE_DEBUG("Successfully loaded SDF texture: %s", filename);
    
    // Add to cache
    // Note: We need to get texture dimensions for cache entry
    // For now, we'll use placeholder values
    texture_loader_add_to_cache(loader, filename, texture, 256, 256, 1);
    
    return texture;
}

int texture_loader_preload(TextureLoaderHandle loader, const char** filenames, uint32_t count) {
    if (!loader || !filenames || count == 0) return 0;
    
    int successCount = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (texture_loader_load(loader, filenames[i])) {
            successCount++;
        }
    }
    
    printf("Preloaded %d/%u textures\n", successCount, count);
    return successCount;
}

int texture_loader_get_info(TextureLoaderHandle loader, const char* filename, 
                           uint32_t* width, uint32_t* height, uint32_t* channels) {
    if (!loader || !filename) return 0;
    
    TextureCacheEntry* entry = texture_loader_find_entry(loader, filename);
    if (!entry) return 0;
    
    if (width) *width = entry->width;
    if (height) *height = entry->height;
    if (channels) *channels = entry->channels;
    
    return 1;
}

int texture_loader_remove(TextureLoaderHandle loader, const char* filename) {
    if (!loader || !filename) return 0;
    
    TextureLoader* tl = (TextureLoader*)loader;
    uint32_t hash = texture_loader_hash_filename(filename) % tl->maxCacheSize;
    
    // Find and remove entry
    for (uint32_t i = 0; i < tl->maxCacheSize; i++) {
        uint32_t index = (hash + i) % tl->maxCacheSize;
        TextureCacheEntry* entry = &tl->cache[index];
        
        if (entry->isValid && strcmp(entry->filename, filename) == 0) {
            // Note: In a real implementation, we would release the Metal texture here
            entry->isValid = 0;
            tl->cacheSize--;
            tl->stats.memoryUsage -= entry->width * entry->height * entry->channels;
            return 1;
        }
    }
    
    return 0;
}

void texture_loader_get_stats(TextureLoaderHandle loader, TextureCacheStats* stats) {
    if (!loader || !stats) return;
    
    TextureLoader* tl = (TextureLoader*)loader;
    *stats = tl->stats;
    stats->size = tl->cacheSize;
    stats->maxSize = tl->maxCacheSize;
}

void texture_loader_set_fallback(TextureLoaderHandle loader, MetalTextureHandle fallback) {
    if (!loader) return;
    
    TextureLoader* tl = (TextureLoader*)loader;
    // Note: In a real implementation, we would release the old fallback texture here
    tl->fallbackTexture = fallback;
}

MetalTextureHandle texture_loader_create_fallback(MetalDeviceHandle device) {
    TEXTURE_DEBUG("Creating fallback texture with device: %p", device);
    
    if (!device) {
        TEXTURE_ERROR("Invalid device for fallback texture creation");
        return NULL;
    }
    
    id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
    
    // Create texture descriptor
    MTLTextureDescriptor* fallbackDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm_sRGB
                                                                                            width:TEXTURE_FALLBACK_SIZE
                                                                                           height:TEXTURE_FALLBACK_SIZE
                                                                                        mipmapped:NO];
    fallbackDesc.usage = MTLTextureUsageShaderRead;
    TEXTURE_DEBUG("Created fallback texture descriptor: %dx%d", TEXTURE_FALLBACK_SIZE, TEXTURE_FALLBACK_SIZE);
    
    id<MTLTexture> fallbackTexture = [mtlDevice newTextureWithDescriptor:fallbackDesc];
    if (!fallbackTexture) {
        TEXTURE_ERROR("Failed to create fallback texture");
        return NULL;
    }
    TEXTURE_DEBUG("Fallback texture created successfully: %p", fallbackTexture);
    
    fallbackTexture.label = @"FallbackTexture";
    
    // Fill with checkerboard pattern
    uint8_t* textureData = malloc(TEXTURE_FALLBACK_SIZE * TEXTURE_FALLBACK_SIZE * 4);
    if (!textureData) return NULL;
    
    const int tileSize = 64;
    for (int y = 0; y < TEXTURE_FALLBACK_SIZE; y++) {
        for (int x = 0; x < TEXTURE_FALLBACK_SIZE; x++) {
            int index = (y * TEXTURE_FALLBACK_SIZE + x) * 4;
            int checker = ((x / tileSize) + (y / tileSize)) % 2;
            
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
    
    TEXTURE_DEBUG("Uploading checkerboard pattern to fallback texture");
    [fallbackTexture replaceRegion:MTLRegionMake2D(0, 0, TEXTURE_FALLBACK_SIZE, TEXTURE_FALLBACK_SIZE)
                       mipmapLevel:0
                         withBytes:textureData
                       bytesPerRow:TEXTURE_FALLBACK_SIZE * 4];
    
    free(textureData);
    
    TEXTURE_INFO("Fallback texture created successfully: %p", fallbackTexture);
    
    return (__bridge_retained MetalTextureHandle)fallbackTexture;
}

int texture_loader_is_cached(TextureLoaderHandle loader, const char* filename) {
    if (!loader || !filename) return 0;
    
    TextureCacheEntry* entry = texture_loader_find_entry(loader, filename);
    return (entry != NULL);
}
