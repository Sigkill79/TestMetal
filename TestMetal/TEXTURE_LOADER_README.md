# Texture Loader Component

A comprehensive, cached texture management system for the Metal engine that provides efficient loading, caching, and management of textures using the stb_image library.

## 🎯 Features

- **Cached Loading**: Automatic caching of loaded textures to avoid redundant file I/O
- **Multiple Formats**: Support for PNG, JPEG, BMP, TGA via stb_image
- **Memory Management**: LRU cache eviction and reference counting
- **Error Handling**: Graceful fallback to default textures on load failures
- **Statistics**: Cache hit/miss tracking and memory usage monitoring
- **Thread-Safe Design**: Ready for future multi-threading support

## 📁 Files

- `engine_texture_loader.h` - Header file with API definitions
- `engine_texture_loader.c` - Implementation with stb_image integration
- `texture_loader_test.c` - Comprehensive unit tests
- `texture_loader_demo.c` - Working demo without Metal dependencies
- `Makefile.texture_loader` - Build system for the component
- `Makefile.demo` - Simple build for the demo

## 🚀 Quick Start

### Building the Demo

```bash
# Build and run the demo
make -f Makefile.demo clean
make -f Makefile.demo all
./texture_loader_demo
```

### Building the Full Component

```bash
# Build the texture loader library and tests
make -f Makefile.texture_loader clean
make -f Makefile.texture_loader all

# Run tests
make -f Makefile.texture_loader test
```

## 📖 API Reference

### Initialization

```c
// Initialize texture loader
TextureLoaderHandle loader = texture_loader_init(device, "/path/to/textures");

// Shutdown and cleanup
texture_loader_shutdown(loader);
```

### Loading Textures

```c
// Simple loading
MetalTextureHandle texture = texture_loader_load(loader, "wood.png");

// Loading with options
TextureLoadOptions options = {
    .pixelFormat = MTLPixelFormatRGBA8Unorm_sRGB,
    .generateMipmaps = 1,
    .flipVertically = 0,
    .srgb = 1
};
MetalTextureHandle texture = texture_loader_load_with_options(loader, "metal.jpg", &options);
```

### Cache Management

```c
// Check if texture is cached
int isCached = texture_loader_is_cached(loader, "fabric.tga");

// Get cache statistics
TextureCacheStats stats;
texture_loader_get_stats(loader, &stats);
printf("Cache hits: %u, misses: %u\n", stats.hitCount, stats.missCount);

// Clean cache
texture_loader_clean_cache(loader);
```

### Batch Operations

```c
// Preload multiple textures
const char* textures[] = {"wood.png", "metal.jpg", "fabric.tga"};
int loaded = texture_loader_preload(loader, textures, 3);
printf("Loaded %d/%d textures\n", loaded, 3);
```

## 🏗️ Architecture

### Core Components

1. **TextureCacheEntry**: Individual cache entry with metadata
2. **TextureLoader**: Main loader with cache and device management
3. **Hash Table**: Filename-based caching with collision resolution
4. **LRU Eviction**: Automatic cleanup of least recently used textures

### Data Flow

```
Filename → Hash → Cache Lookup → Load from File → Create Metal Texture → Cache Entry
```

### Memory Management

- **Reference Counting**: Track texture usage
- **LRU Eviction**: Remove unused textures when cache is full
- **Automatic Cleanup**: Proper resource deallocation on shutdown

## 🔧 Configuration

### Cache Settings

```c
#define TEXTURE_CACHE_DEFAULT_SIZE 64        // Default cache size
#define TEXTURE_CACHE_MAX_SIZE 256           // Maximum cache size
#define TEXTURE_FALLBACK_SIZE 512            // Fallback texture size
#define TEXTURE_MAX_DIMENSION 4096           // Maximum texture dimension
```

### Supported Formats

- **PNG**: Full support with alpha channels
- **JPEG**: RGB and grayscale support
- **BMP**: Standard Windows bitmap format
- **TGA**: Truevision Targa format
- **Fallback**: Programmatically generated checkerboard pattern

## 🧪 Testing

The component includes comprehensive unit tests covering:

- ✅ Initialization and shutdown
- ✅ Hash function consistency
- ✅ Cache operations
- ✅ Texture loading simulation
- ✅ Error handling
- ✅ Memory management
- ✅ Statistics tracking

### Running Tests

```bash
# Run all tests
make -f Makefile.texture_loader test

# Run with verbose output
make -f Makefile.texture_loader test-verbose
```

## 🔮 Future Enhancements

### Planned Features

- **Async Loading**: Background texture loading with callbacks
- **Texture Atlases**: Automatic atlas generation for small textures
- **Compression**: Automatic texture compression (BC1, BC3, etc.)
- **Streaming**: Large texture streaming for open worlds
- **Hot Reloading**: Runtime texture reloading for development

### API Extensions

- **Custom Loaders**: Plugin system for custom texture formats
- **Memory Pools**: Dedicated memory pools for texture data
- **GPU Upload**: Direct GPU upload without CPU staging
- **Texture Arrays**: Support for texture arrays and 3D textures

## 🐛 Error Handling

The component provides comprehensive error handling:

```c
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

// Get error description
const char* errorStr = texture_loader_get_error_string(result);
```

## 📊 Performance

### Cache Performance

- **Hash Table**: O(1) average lookup time
- **LRU Eviction**: O(n) eviction time (acceptable for small cache sizes)
- **Memory Overhead**: ~100 bytes per cached texture

### Loading Performance

- **File I/O**: Minimized through caching
- **Format Conversion**: Optimized stb_image integration
- **Metal Creation**: Direct GPU texture creation

## 🔗 Integration

### With Metal Engine

```c
// In your Metal engine initialization
TextureLoaderHandle textureLoader = texture_loader_init(metalDevice, resourcePath);

// In your rendering code
MetalTextureHandle texture = texture_loader_load(textureLoader, "model_diffuse.png");
[encoder setFragmentTexture:(__bridge id<MTLTexture>)texture atIndex:0];

// In your engine shutdown
texture_loader_shutdown(textureLoader);
```

### With Engine State

```c
// Add to EngineStateStruct
typedef struct {
    // ... existing fields ...
    TextureLoaderHandle texture_loader;
} EngineStateStruct;
```

## 📝 License

This component is part of the TestMetal engine project and follows the same licensing terms.

## 🤝 Contributing

1. Follow the existing code style and patterns
2. Add unit tests for new functionality
3. Update documentation for API changes
4. Ensure all tests pass before submitting

---

**Status**: ✅ Complete and tested  
**Version**: 1.0.0  
**Last Updated**: December 2024
