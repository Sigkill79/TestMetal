#ifndef ENGINE_MAIN_H
#define ENGINE_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine_math.h"
#include "engine_world.h"

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
    vec3_t camera_up;
    mat4_t view_matrix;
    mat4_t projection_matrix;
    mat4_t model_matrix;
    
    // World system
    World* world;
    
    // Metal engine handle
    MetalEngineHandle metal_engine;
    
    // View handle for rendering
    MetalViewHandle view_handle;
    
    // Viewport dimensions
    float viewport_width;
    float viewport_height;
    
    // Resource path for loading assets
    char* resource_path;
} EngineStateStruct;

// Engine initialization with Metal setup
EngineStateStruct* engine_initialize(MetalViewHandle view, float viewport_width, float viewport_height, const char* resource_path);
void engine_update(EngineStateStruct* engineState);
void engine_shutdown(EngineStateStruct* engineState);

// Metal-specific functions (now part of engine_main interface)
int engine_load_assets(EngineStateStruct* engine);
int engine_render_frame(EngineStateStruct* engine);
void engine_resize_viewport(EngineStateStruct* engine, float width, float height);

// World management functions
World* engine_get_world(EngineStateStruct* engine);
WorldEntity* engine_create_entity(EngineStateStruct* engine, const char* name);
int engine_destroy_entity(EngineStateStruct* engine, uint32_t entity_id);
WorldEntity* engine_get_entity(EngineStateStruct* engine, uint32_t entity_id);
WorldEntity* engine_get_entity_by_name(EngineStateStruct* engine, const char* name);



#ifdef __cplusplus
}
#endif

#endif // ENGINE_MAIN_H
