#include "engine_texture_loader.h"
#include "engine_metal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// ============================================================================
// TEST UTILITIES
// ============================================================================

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            tests_passed++; \
            printf("✓ %s\n", message); \
        } else { \
            tests_failed++; \
            printf("✗ %s\n", message); \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL(expected, actual, message) \
    TEST_ASSERT((expected) == (actual), message)

#define TEST_ASSERT_NOT_NULL(ptr, message) \
    TEST_ASSERT((ptr) != NULL, message)

#define TEST_ASSERT_NULL(ptr, message) \
    TEST_ASSERT((ptr) == NULL, message)

// Mock Metal device for testing
static MetalDeviceHandle mock_device = (MetalDeviceHandle)0x12345678;

// ============================================================================
// TEST HELPER FUNCTIONS
// ============================================================================

static void create_test_texture_file(const char* filename, int width, int height, int channels) {
    FILE* file = fopen(filename, "wb");
    if (!file) return;
    
    // Create a simple test image (just write some dummy data)
    // In a real test, we'd create a proper image file
    unsigned char* data = malloc(width * height * channels);
    if (data) {
        for (int i = 0; i < width * height * channels; i++) {
            data[i] = (unsigned char)(i % 256);
        }
        fwrite(data, 1, width * height * channels, file);
        free(data);
    }
    
    fclose(file);
}

static void cleanup_test_files(void) {
    // Remove any test files created during testing
    remove("test_texture.png");
    remove("test_texture.jpg");
    remove("test_texture.bmp");
}

// ============================================================================
// INITIALIZATION TESTS
// ============================================================================

static void test_texture_loader_init(void) {
    printf("\n=== Testing Texture Loader Initialization ===\n");
    
    // Test valid initialization
    TextureLoaderHandle loader = texture_loader_init(mock_device, "/test/path");
    TEST_ASSERT_NOT_NULL(loader, "texture_loader_init should return valid handle");
    
    if (loader) {
        texture_loader_shutdown(loader);
    }
    
    // Test initialization with NULL device
    loader = texture_loader_init(NULL, "/test/path");
    TEST_ASSERT_NULL(loader, "texture_loader_init should return NULL for NULL device");
    
    // Test initialization with NULL resource path
    loader = texture_loader_init(mock_device, NULL);
    TEST_ASSERT_NOT_NULL(loader, "texture_loader_init should handle NULL resource path");
    
    if (loader) {
        texture_loader_shutdown(loader);
    }
}

static void test_texture_loader_shutdown(void) {
    printf("\n=== Testing Texture Loader Shutdown ===\n");
    
    // Test shutdown of valid loader
    TextureLoaderHandle loader = texture_loader_init(mock_device, "/test/path");
    TEST_ASSERT_NOT_NULL(loader, "Loader should be created for shutdown test");
    
    texture_loader_shutdown(loader);
    // Note: We can't easily test if shutdown worked without accessing internal state
    
    // Test shutdown of NULL loader (should not crash)
    texture_loader_shutdown(NULL);
    
    printf("✓ Shutdown tests completed (no crash)\n");
    tests_run++;
    tests_passed++;
}

// ============================================================================
// CACHE MANAGEMENT TESTS
// ============================================================================

static void test_cache_operations(void) {
    printf("\n=== Testing Cache Operations ===\n");
    
    TextureLoaderHandle loader = texture_loader_init(mock_device, "/test/path");
    TEST_ASSERT_NOT_NULL(loader, "Loader should be created for cache tests");
    
    if (!loader) return;
    
    // Test cache stats
    TextureCacheStats stats;
    texture_loader_get_stats(loader, &stats);
    TEST_ASSERT_EQUAL(0, stats.size, "Initial cache size should be 0");
    TEST_ASSERT_EQUAL(TEXTURE_CACHE_DEFAULT_SIZE, stats.maxSize, "Default cache max size should be correct");
    
    // Test is_cached for non-existent texture
    int isCached = texture_loader_is_cached(loader, "nonexistent.png");
    TEST_ASSERT_EQUAL(0, isCached, "Non-existent texture should not be cached");
    
    // Test get_info for non-existent texture
    uint32_t width, height, channels;
    int infoResult = texture_loader_get_info(loader, "nonexistent.png", &width, &height, &channels);
    TEST_ASSERT_EQUAL(0, infoResult, "get_info should fail for non-existent texture");
    
    // Test remove for non-existent texture
    int removeResult = texture_loader_remove(loader, "nonexistent.png");
    TEST_ASSERT_EQUAL(0, removeResult, "remove should fail for non-existent texture");
    
    texture_loader_shutdown(loader);
}

static void test_hash_function(void) {
    printf("\n=== Testing Hash Function ===\n");
    
    // Test hash function with various inputs
    uint32_t hash1 = texture_loader_hash_filename("test.png");
    uint32_t hash2 = texture_loader_hash_filename("test.png");
    uint32_t hash3 = texture_loader_hash_filename("different.png");
    
    TEST_ASSERT_EQUAL(hash1, hash2, "Same filename should produce same hash");
    TEST_ASSERT(hash1 != hash3, "Different filenames should produce different hashes");
    
    // Test with NULL input
    uint32_t hashNull = texture_loader_hash_filename(NULL);
    TEST_ASSERT_EQUAL(0, hashNull, "NULL filename should produce hash 0");
    
    // Test with empty string
    uint32_t hashEmpty = texture_loader_hash_filename("");
    TEST_ASSERT(hashEmpty != 0, "Empty string should produce non-zero hash");
}

// ============================================================================
// LOADING TESTS
// ============================================================================

static void test_texture_loading(void) {
    printf("\n=== Testing Texture Loading ===\n");
    
    TextureLoaderHandle loader = texture_loader_init(mock_device, ".");
    TEST_ASSERT_NOT_NULL(loader, "Loader should be created for loading tests");
    
    if (!loader) return;
    
    // Test loading non-existent file
    MetalTextureHandle texture = texture_loader_load(loader, "nonexistent.png");
    TEST_ASSERT_NOT_NULL(texture, "Loading non-existent file should return fallback texture");
    
    // Test loading with NULL parameters
    texture = texture_loader_load(NULL, "test.png");
    TEST_ASSERT_NULL(texture, "Loading with NULL loader should return NULL");
    
    texture = texture_loader_load(loader, NULL);
    TEST_ASSERT_NULL(texture, "Loading with NULL filename should return NULL");
    
    // Test loading with options
    TextureLoadOptions options = {
        .pixelFormat = MTLPixelFormatRGBA8Unorm,
        .generateMipmaps = 0,
        .flipVertically = 0,
        .srgb = 0
    };
    
    texture = texture_loader_load_with_options(loader, "nonexistent.png", &options);
    TEST_ASSERT_NOT_NULL(texture, "Loading with options should return fallback texture");
    
    texture_loader_shutdown(loader);
}

static void test_preload_operation(void) {
    printf("\n=== Testing Preload Operation ===\n");
    
    TextureLoaderHandle loader = texture_loader_init(mock_device, ".");
    TEST_ASSERT_NOT_NULL(loader, "Loader should be created for preload tests");
    
    if (!loader) return;
    
    // Test preload with NULL parameters
    int result = texture_loader_preload(NULL, NULL, 0);
    TEST_ASSERT_EQUAL(0, result, "Preload with NULL loader should return 0");
    
    result = texture_loader_preload(loader, NULL, 5);
    TEST_ASSERT_EQUAL(0, result, "Preload with NULL filenames should return 0");
    
    result = texture_loader_preload(loader, NULL, 0);
    TEST_ASSERT_EQUAL(0, result, "Preload with count 0 should return 0");
    
    // Test preload with valid parameters (but non-existent files)
    const char* filenames[] = {"test1.png", "test2.jpg", "test3.bmp"};
    result = texture_loader_preload(loader, filenames, 3);
    TEST_ASSERT_EQUAL(0, result, "Preload with non-existent files should return 0");
    
    texture_loader_shutdown(loader);
}

// ============================================================================
// FALLBACK TEXTURE TESTS
// ============================================================================

static void test_fallback_texture(void) {
    printf("\n=== Testing Fallback Texture ===\n");
    
    // Test fallback texture creation
    MetalTextureHandle fallback = texture_loader_create_fallback(mock_device);
    TEST_ASSERT_NOT_NULL(fallback, "Fallback texture creation should succeed");
    
    // Test fallback texture creation with NULL device
    fallback = texture_loader_create_fallback(NULL);
    TEST_ASSERT_NULL(fallback, "Fallback texture creation with NULL device should fail");
    
    // Test setting fallback texture
    TextureLoaderHandle loader = texture_loader_init(mock_device, "/test/path");
    TEST_ASSERT_NOT_NULL(loader, "Loader should be created for fallback tests");
    
    if (loader) {
        MetalTextureHandle newFallback = texture_loader_create_fallback(mock_device);
        if (newFallback) {
            texture_loader_set_fallback(loader, newFallback);
            printf("✓ Fallback texture set successfully\n");
            tests_run++;
            tests_passed++;
        }
        
        texture_loader_shutdown(loader);
    }
}

// ============================================================================
// ERROR HANDLING TESTS
// ============================================================================

static void test_error_handling(void) {
    printf("\n=== Testing Error Handling ===\n");
    
    // Test error string function
    const char* errorStr = texture_loader_get_error_string(TEXTURE_LOADER_SUCCESS);
    TEST_ASSERT_NOT_NULL(errorStr, "Error string should not be NULL");
    TEST_ASSERT(strlen(errorStr) > 0, "Error string should not be empty");
    
    // Test all error codes
    for (int i = 0; i <= TEXTURE_LOADER_ERROR_NOT_INITIALIZED; i++) {
        const char* str = texture_loader_get_error_string((TextureLoaderResult)i);
        TEST_ASSERT_NOT_NULL(str, "All error codes should have valid strings");
    }
    
    // Test invalid error code
    const char* invalidStr = texture_loader_get_error_string((TextureLoaderResult)999);
    TEST_ASSERT_NOT_NULL(invalidStr, "Invalid error code should return valid string");
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

static void test_integration_scenario(void) {
    printf("\n=== Testing Integration Scenario ===\n");
    
    // Create loader
    TextureLoaderHandle loader = texture_loader_init(mock_device, "/test/path");
    TEST_ASSERT_NOT_NULL(loader, "Integration test loader should be created");
    
    if (!loader) return;
    
    // Test cache operations
    TextureCacheStats stats;
    texture_loader_get_stats(loader, &stats);
    TEST_ASSERT_EQUAL(0, stats.hitCount, "Initial hit count should be 0");
    TEST_ASSERT_EQUAL(0, stats.missCount, "Initial miss count should be 0");
    
    // Test loading same texture multiple times (should hit cache)
    MetalTextureHandle texture1 = texture_loader_load(loader, "test.png");
    MetalTextureHandle texture2 = texture_loader_load(loader, "test.png");
    
    TEST_ASSERT_NOT_NULL(texture1, "First load should return fallback texture");
    TEST_ASSERT_NOT_NULL(texture2, "Second load should return fallback texture");
    
    // Test cache stats after operations
    texture_loader_get_stats(loader, &stats);
    TEST_ASSERT(stats.missCount > 0, "Should have cache misses");
    
    // Test cache cleanup
    texture_loader_clean_cache(loader);
    texture_loader_get_stats(loader, &stats);
    TEST_ASSERT_EQUAL(0, stats.size, "Cache size should be 0 after cleanup");
    
    texture_loader_shutdown(loader);
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

static void run_all_tests(void) {
    printf("Starting Texture Loader Unit Tests\n");
    printf("===================================\n");
    
    // Run all test suites
    test_texture_loader_init();
    test_texture_loader_shutdown();
    test_cache_operations();
    test_hash_function();
    test_texture_loading();
    test_preload_operation();
    test_fallback_texture();
    test_error_handling();
    test_integration_scenario();
    
    // Cleanup
    cleanup_test_files();
    
    // Print results
    printf("\n===================================\n");
    printf("Test Results:\n");
    printf("Total tests: %d\n", tests_run);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Success rate: %.1f%%\n", tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    
    if (tests_failed == 0) {
        printf("\n🎉 All tests passed!\n");
    } else {
        printf("\n❌ Some tests failed!\n");
    }
}

int main(void) {
    run_all_tests();
    return tests_failed > 0 ? 1 : 0;
}
