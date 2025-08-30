#include "engine_main.h"
#include "engine_metal.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Initialize engine and return new instance
EngineStateStruct* initialize(void) {
    EngineStateStruct* engineState = (EngineStateStruct*)malloc(sizeof(EngineStateStruct));
    if (engineState) {
        engineState->state = ENGINE_STATE_INITIALIZING;
        
        // Initialize math components
        engineState->camera_position = vec3(0.0f, 0.0f, 5.0f);
        engineState->camera_target = vec3(0.0f, 0.0f, 0.0f);
        engineState->view_matrix = mat4_look_at(engineState->camera_position, engineState->camera_target, vec3_unit_y());
        engineState->projection_matrix = mat4_perspective(M_PI / 3.0f, 16.0f / 9.0f, 0.1f, 100.0f);
        
        // Initialize Metal engine to NULL
        engineState->metal_engine = NULL;
        engineState->view_handle = NULL;
        engineState->viewport_width = 800.0f;
        engineState->viewport_height = 600.0f;
    }
    return engineState;
}

// Initialize engine with Metal setup
EngineStateStruct* initialize_with_metal(MetalViewHandle view, float viewport_width, float viewport_height) {
    EngineStateStruct* engineState = initialize();
    if (!engineState) {
        fprintf(stderr, "Failed to initialize base engine\n");
        return NULL;
    }
    
    // Set viewport dimensions and store view handle
    engineState->viewport_width = viewport_width;
    engineState->viewport_height = viewport_height;
    engineState->view_handle = view;
    
    // Initialize Metal engine
    engineState->metal_engine = (MetalEngineHandle)metal_engine_init();
    if (!engineState->metal_engine) {
        fprintf(stderr, "Failed to initialize Metal engine\n");
        engine_shutdown(engineState);
        return NULL;
    }
    
    // Load Metal with view
    if (!metal_engine_load_metal_with_view((MetalEngine*)engineState->metal_engine, view)) {
        fprintf(stderr, "Failed to load Metal with view\n");
        engine_shutdown(engineState);
        return NULL;
    }
    
    // Load assets
    if (!engine_load_assets(engineState)) {
        fprintf(stderr, "Failed to load assets\n");
        engine_shutdown(engineState);
        return NULL;
    }
    
    // Set initial viewport size
    engine_resize_viewport(engineState, viewport_width, viewport_height);
    
    // Set engine state to running
    engineState->state = ENGINE_STATE_RUNNING;
    
    fprintf(stderr, "Engine initialized successfully with Metal\n");
    return engineState;
}

// Load engine assets
int engine_load_assets(EngineStateStruct* engine) {
    if (!engine || !engine->metal_engine) {
        fprintf(stderr, "Engine or Metal engine is NULL\n");
        return 0;
    }
    
    // Load Metal assets
    if (!metal_engine_load_assets((MetalEngine*)engine->metal_engine)) {
        fprintf(stderr, "Failed to load Metal assets\n");
        return 0;
    }
    
    fprintf(stderr, "Assets loaded successfully\n");
    return 1;
}

// Render a frame
int engine_render_frame(EngineStateStruct* engine) {
    if (!engine || !engine->metal_engine) {
        fprintf(stderr, "Engine or Metal engine is NULL\n");
        return 0;
    }
    
    // Render frame using Metal engine
    // Use the stored view handle for proper rendering
    metal_engine_render_frame((MetalEngine*)engine->metal_engine, engine->view_handle);
    return 1;
}

// Resize viewport
void engine_resize_viewport(EngineStateStruct* engine, float width, float height) {
    if (!engine || !engine->metal_engine) {
        fprintf(stderr, "Engine or Metal engine is NULL\n");
        return;
    }
    
    engine->viewport_width = width;
    engine->viewport_height = height;
    
    // Update projection matrix with new aspect ratio
    float aspect = width / height;
    engine->projection_matrix = mat4_perspective(M_PI / 3.0f, aspect, 0.1f, 100.0f);
    
    // Resize Metal viewport
    metal_engine_resize_viewport((MetalEngine*)engine->metal_engine, (int)width, (int)height);
}

// Update engine state and render frame
void update(EngineStateStruct* engineState) {
    if(!engineState) {
        fprintf(stderr, "Error: engineState is NULL in update()\n");
        return;
    }

    if (engineState->state == ENGINE_STATE_RUNNING) {
        // Execute one step in the game loop
        // For now, this includes rendering the frame
    }

    engine_render_frame(engineState);
}

// Shutdown engine and free memory
void engine_shutdown(EngineStateStruct* engineState) {
    if (engineState) {
        engineState->state = ENGINE_STATE_SHUTDOWN;
        
        // Shutdown Metal engine
        if (engineState->metal_engine) {
            metal_engine_shutdown((MetalEngine*)engineState->metal_engine);
            engineState->metal_engine = NULL;
        }
        
        free(engineState);
        fprintf(stderr, "Engine shutdown successfully\n");
    }
}
