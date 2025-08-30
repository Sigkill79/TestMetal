#ifndef ENGINE_MAIN_H
#define ENGINE_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine_math.h"

// Forward declarations for Metal types
struct MetalEngine; // Forward declaration
typedef struct MetalEngine* MetalEngineHandle;
typedef void* MetalViewHandle; // Use void* for C compatibility

// Engine states
typedef enum {
    ENGINE_STATE_INITIALIZING,
    ENGINE_STATE_RUNNING,
    ENGINE_STATE_PAUSED,
    ENGINE_STATE_SHUTDOWN
} EngineState;

// Main engine state structure
typedef struct {
    EngineState state;
    // Math library integration
    vec3_t camera_position;
    vec3_t camera_target;
    mat4_t view_matrix;
    mat4_t projection_matrix;
    
    // Metal engine handle
    MetalEngineHandle metal_engine;
    
    // View handle for rendering
    MetalViewHandle view_handle;
    
    // Viewport dimensions
    float viewport_width;
    float viewport_height;
} EngineStateStruct;

// Engine initialization with Metal setup
EngineStateStruct* initialize_with_metal(MetalViewHandle view, float viewport_width, float viewport_height);
EngineStateStruct* initialize(void);
void update(EngineStateStruct* engineState);
void engine_shutdown(EngineStateStruct* engineState);

// Metal-specific functions (now part of engine_main interface)
int engine_load_assets(EngineStateStruct* engine);
int engine_render_frame(EngineStateStruct* engine);
void engine_resize_viewport(EngineStateStruct* engine, float width, float height);

#ifdef __cplusplus
}
#endif

#endif // ENGINE_MAIN_H
