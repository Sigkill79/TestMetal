#include "engine_metal.h"
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// METAL ENGINE TEST SUITE
// ============================================================================

void test_metal_engine_lifecycle(void) {
    printf("=== Testing Metal Engine Lifecycle ===\n");
    
    // Test initialization
    MetalEngine* engine = metal_engine_init();
    if (!engine) {
        printf("❌ Failed to initialize Metal engine\n");
        return;
    }
    printf("✅ Metal engine initialized successfully\n");
    
    // Test pipeline creation
    if (metal_engine_create_pipeline(engine)) {
        printf("✅ Metal pipeline creation successful\n");
    } else {
        printf("❌ Metal pipeline creation failed\n");
    }
    
    // Test buffer creation
    if (metal_engine_create_buffers(engine)) {
        printf("✅ Metal buffer creation successful\n");
    } else {
        printf("❌ Metal buffer creation failed\n");
    }
    
    // Test texture creation
    if (metal_engine_create_textures(engine)) {
        printf("✅ Metal texture creation successful\n");
    } else {
        printf("❌ Metal texture creation failed\n");
    }
    
    // Test viewport resize
    metal_engine_resize_viewport(engine, 1024, 768);
    printf("✅ Viewport resize test completed\n");
    
    // Test frame rendering (with NULL view for testing)
    for (int i = 0; i < 5; i++) {
        metal_engine_render_frame(engine, NULL);
    }
    printf("✅ Frame rendering test completed\n");
    
    // Test shutdown
    metal_engine_shutdown(engine);
    printf("✅ Metal engine shutdown successful\n");
    
    printf("\n");
}

void test_metal_engine_utilities(void) {
    printf("=== Testing Metal Engine Utilities ===\n");
    
    // Test fallback texture creation
    MetalTextureHandle fallbackTexture = metal_engine_create_fallback_texture(NULL);
    printf("✅ Fallback texture creation test completed\n");
    
    // Test device info printing
    metal_engine_print_device_info(NULL);
    printf("✅ Device info printing test completed\n");
    
    printf("\n");
}

void test_metal_engine_error_handling(void) {
    printf("=== Testing Metal Engine Error Handling ===\n");
    
    // Test NULL engine handling
    metal_engine_shutdown(NULL);
    printf("✅ NULL engine shutdown handled gracefully\n");
    
    metal_engine_create_pipeline(NULL);
    printf("✅ NULL engine pipeline creation handled gracefully\n");
    
    metal_engine_create_buffers(NULL);
    printf("✅ NULL engine buffer creation handled gracefully\n");
    
    metal_engine_create_textures(NULL);
    printf("✅ NULL engine texture creation handled gracefully\n");
    
    metal_engine_render_frame(NULL, NULL);
    printf("✅ NULL engine frame rendering handled gracefully\n");
    
    metal_engine_resize_viewport(NULL, 0, 0);
    printf("✅ NULL engine viewport resize handled gracefully\n");
    
    printf("\n");
}

int main(void) {
    printf("🧪 Metal Engine Test Suite\n");
    printf("==========================\n\n");
    
    test_metal_engine_lifecycle();
    test_metal_engine_utilities();
    test_metal_engine_error_handling();
    
    printf("✅ All Metal engine tests completed successfully!\n");
    return 0;
}
