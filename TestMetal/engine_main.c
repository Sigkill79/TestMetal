#include "engine_main.h"
#include "engine_metal.h"
#include "engine_asset_fbx.h"
#include "engine_world.h"
#include "engine_2d.h"
#include "engine_texture_loader.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


// Initialize engine with Metal setup
EngineStateStruct* engine_initialize(MetalViewHandle view, float viewport_width, float viewport_height, const char* resource_path) {
    fprintf(stderr, "=== ENGINE_INITIALIZE START ===\n");
    fflush(stderr);
    
    EngineStateStruct* engineState = (EngineStateStruct*)malloc(sizeof(EngineStateStruct));
    if (engineState) {
        engineState->state = ENGINE_STATE_INITIALIZING;
        
        // Initialize math components
        engineState->camera_position = vec3(0.0f, 0.0f, -5.0f);
        engineState->camera_target = vec3(0.0f, 0.0f, 0.0f);
        engineState->camera_up = vec3_unit_y();
        
        // Debug: Print camera position BEFORE look_at
        fprintf(stderr, "=== INITIALIZATION DEBUG ===\n");
        fprintf(stderr, "Camera position BEFORE look_at: (%.3f, %.3f, %.3f)\n", 
                engineState->camera_position.x, 
                engineState->camera_position.y, 
                engineState->camera_position.z);
        fprintf(stderr, "Camera target: (%.3f, %.3f, %.3f)\n", 
                engineState->camera_target.x, 
                engineState->camera_target.y, 
                engineState->camera_target.z);
        
        engineState->view_matrix = mat4_look_at(engineState->camera_position, engineState->camera_target, engineState->camera_up);
    
        // Debug: Print camera position AFTER look_at
        fprintf(stderr, "Camera position AFTER look_at: (%.3f, %.3f, %.3f)\n", 
                engineState->camera_position.x, 
                engineState->camera_position.y, 
                engineState->camera_position.z);
        
        // Debug: Print view matrix after calculation
        fprintf(stderr, "View matrix after look_at:\n");
        fprintf(stderr, "  [0]: %.3f, %.3f, %.3f, %.3f\n", 
                engineState->view_matrix.x.x, engineState->view_matrix.x.y, engineState->view_matrix.x.z, engineState->view_matrix.x.w);
        fprintf(stderr, "  [1]: %.3f, %.3f, %.3f, %.3f\n", 
                engineState->view_matrix.y.x, engineState->view_matrix.y.y, engineState->view_matrix.y.z, engineState->view_matrix.y.w);
        fprintf(stderr, "  [2]: %.3f, %.3f, %.3f, %.3f\n", 
                engineState->view_matrix.z.x, engineState->view_matrix.z.y, engineState->view_matrix.z.z, engineState->view_matrix.z.w);
        fprintf(stderr, "  [3]: %.3f, %.3f, %.3f, %.3f\n", 
                engineState->view_matrix.w.x, engineState->view_matrix.w.y, engineState->view_matrix.w.z, engineState->view_matrix.w.w);
        
        engineState->projection_matrix = mat4_perspective(M_PI / 3.0f, 16.0f / 9.0f, 0.1f, 100.0f);
        
        fprintf(stderr, "===========================\n");
        
        // Initialize world system
        engineState->world = world_create(100); // Support up to 100 entities
        if (!engineState->world) {
            fprintf(stderr, "Failed to create world\n");
            engine_shutdown(engineState);
            return NULL;
        }
        
        
    
        engineState->viewport_width = viewport_width;
        engineState->viewport_height = viewport_height;
        engineState->view_handle = view;
        
        // Store resource path
        engineState->resource_path = resource_path ? strdup(resource_path) : NULL;
    
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
        
        // Load assets (non-critical - engine can run without assets)
        fprintf(stderr, "About to call engine_load_assets...\n");
        fflush(stderr);
        if (!engine_load_assets(engineState)) {
            fprintf(stderr, "Warning: Failed to load assets, continuing without them\n");
            // Don't shutdown engine - continue without assets
        }
        
        // Set initial viewport size
        engine_resize_viewport(engineState, viewport_width, viewport_height);
        
        
                // Initialize UI 2D system
        engineState->ui_2d = engine_2d_init(engineState->metal_engine);
        if (!engineState->ui_2d) {
             fprintf(stderr, "Failed to initialize UI 2D system\n");
             engine_shutdown(engineState);
             return NULL;
         }
        
        // Initialize texture loader system
        MetalDeviceHandle device = metal_engine_get_device(engineState->metal_engine);
        // Append /assets/ to the resource path for texture loading
        char texture_path[1024];
        snprintf(texture_path, sizeof(texture_path), "%s/assets", resource_path);
        engineState->texture_loader = texture_loader_init(device, texture_path);
        if (!engineState->texture_loader) {
             fprintf(stderr, "Failed to initialize texture loader system\n");
             engine_shutdown(engineState);
             return NULL;
         }
        
        // Set engine state to running
        engineState->state = ENGINE_STATE_RUNNING;
        
        fprintf(stderr, "Engine initialized successfully with Metal\n");
    }

    return engineState;
}

// Load engine assets
int engine_load_assets(EngineStateStruct* engine) {
    fprintf(stderr, "=== ENGINE_LOAD_ASSETS START ===\n");
    fflush(stderr);
    if (!engine || !engine->metal_engine) {
        fprintf(stderr, "Engine or Metal engine is NULL\n");
        fflush(stderr);
        return 0;
    }
    
    // Construct full path to FBX model
    char full_path[1024];
    if (engine->resource_path) {
        snprintf(full_path, sizeof(full_path), "%s/assets/UnitSphere.fbx", engine->resource_path);
    } else {
        snprintf(full_path, sizeof(full_path), "assets/UnitSphere.fbx");
    }
    
    fprintf(stderr, "About to load FBX model from: %s\n", full_path);
    fflush(stderr);
    
    // Load FBX model
    char* error = NULL;
    Model3D* fbxModel = fbx_load_model(full_path, &error);
    if (!fbxModel) {
        fprintf(stderr, "Failed to load FBX model: %s\n", error ? error : "unknown error");
        fflush(stderr);
        fbx_free_error(error);
        return 0;
    }
    fbx_free_error(error);

    fprintf(stderr, "Loaded FBX model: %s with %u meshes\n", fbxModel->name, fbxModel->mesh_count);
    fflush(stderr);

    // Upload model to Metal
    fprintf(stderr, "About to upload model to Metal...\n");
    fflush(stderr);
    MetalModelHandle metalModel = metal_engine_upload_model((MetalEngine*)engine->metal_engine, fbxModel);
    if (!metalModel) {
        fprintf(stderr, "Failed to upload model to Metal\n");
        fflush(stderr);
        model3d_free(fbxModel);
        return 0;
    }

    // Create a world entity for the loaded model
    fprintf(stderr, "Creating world entity for loaded model...\n");
    fflush(stderr);
    WorldEntity* entity = world_create_entity(engine->world, fbxModel->name);
    if (!entity) {
        fprintf(stderr, "Failed to create world entity\n");
        metal_engine_free_model(metalModel);
        model3d_free(fbxModel);
        return 0;
    }
    
    // Set up the entity
    entity_set_model(entity, metalModel);
    entity_set_position(entity, vec3(0.0f, 0.0f, -2.0f)); // Position in front of camera (negative Z)
    entity_set_orientation(entity, quat_identity()); // No rotation initially
    
    fprintf(stderr, "Created entity '%s' with Metal model\n", entity_get_name(entity));
    fflush(stderr);

    // Free the original model data (Metal now has its own copy)
    model3d_free(fbxModel);
    
            // Load Metal assets
        if (!metal_engine_load_assets((MetalEngine*)engine->metal_engine)) {
            fprintf(stderr, "Failed to load Metal assets\n");
            fflush(stderr);
            return 0;
        }
        
        // Initialize UI 2D system after Metal engine is ready
        // if (!engine_2d_init(engine->ui_2d, engine->metal_engine)) {
        //     fprintf(stderr, "Failed to initialize UI 2D system\n");
        //     fflush(stderr);
        //     return 0;
        // }
        
        fprintf(stderr, "Assets loaded successfully with world entities and UI system\n");
        fprintf(stderr, "=== ENGINE_LOAD_ASSETS END ===\n");
        fflush(stderr);
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
void engine_update(EngineStateStruct* engineState) {
    
    if(!engineState) {
        fprintf(stderr, "Error: engineState is NULL in update()\n");
        return;
    }

    // Engine update called silently

    if (engineState->state == ENGINE_STATE_RUNNING) {
        
        // Clear UI elements from previous frame
        // if (engineState->ui_2d) {
        //     engine_2d_clear_elements(engineState->ui_2d);
        // }
        
        // Update world entities (for now, just animate the first entity if it exists)
        if (engineState->world && engineState->world->entity_count > 0) {
            // Find the first active entity
            WorldEntity* entity = NULL;
            for (uint32_t i = 0; i < engineState->world->max_entities; i++) {
                if (engineState->world->entities[i].id != 0 && engineState->world->entities[i].is_active) {
                    entity = &engineState->world->entities[i];
                    break;
                }
            }
            
            if (entity) {
                // Animate the entity with quaternion rotation
                static float rotation_time = 0.0f;
                rotation_time += 0.016f; // 60 FPS
                
                // Create a rotation around Y-axis using quaternions
                quat_t rotation = quat_from_axis_angle(vec3_unit_y(), rotation_time);
                entity_set_orientation(entity, rotation);
                
                // Entity rotation updated silently
            }
        }
        
        // Add UI elements for testing - render downloaded textures
        if (engineState->ui_2d && engineState->texture_loader) {
             // Load and display our downloaded textures in a row
             MetalTextureHandle woodTexture = texture_loader_load(engineState->texture_loader, "wood_texture.jpg");
             if (woodTexture) {
                 engine_2d_draw_image(engineState->ui_2d, 0.0f, 0.0f, woodTexture);
             }
             
             MetalTextureHandle metalTexture = texture_loader_load(engineState->texture_loader, "metal_texture.png");
             if (metalTexture) {
                 engine_2d_draw_image(engineState->ui_2d, 256.0f, 0.0f, metalTexture);
             }
             
             MetalTextureHandle fabricTexture = texture_loader_load(engineState->texture_loader, "fabric_texture.jpg");
             if (fabricTexture) {
                 engine_2d_draw_image(engineState->ui_2d, 512.0f, 0.0f, fabricTexture);
             }
        }

    } else {
        fprintf(stderr, "Engine is not running, state: %d\n", engineState->state);
    }

    if (engineState->metal_engine) {
        metal_engine_render_frame((MetalEngine*)engineState->metal_engine, engineState->view_handle, engineState);
    } else {
        fprintf(stderr, "ERROR: Engine or Metal engine is NULL - engine=%p, metal_engine=%p\n", engineState, engineState ? engineState->metal_engine : NULL);
    }

    engine_2d_clear_elements(engineState->ui_2d);
}

// Shutdown engine and free memory
void engine_shutdown(EngineStateStruct* engineState) {
    if (engineState) {
        engineState->state = ENGINE_STATE_SHUTDOWN;
        
        // Shutdown UI 2D system
        if (engineState->ui_2d) {
             engine_2d_shutdown(engineState->ui_2d);
             engineState->ui_2d = NULL;
        }
        
        // Shutdown texture loader system
        if (engineState->texture_loader) {
             texture_loader_shutdown(engineState->texture_loader);
             engineState->texture_loader = NULL;
        }
        
        // Shutdown world system
        if (engineState->world) {
            world_destroy(engineState->world);
            engineState->world = NULL;
        }
        
        // Shutdown Metal engine
        if (engineState->metal_engine) {
            metal_engine_shutdown((MetalEngine*)engineState->metal_engine);
            engineState->metal_engine = NULL;
        }
        
        // Free resource path
        if (engineState->resource_path) {
            free(engineState->resource_path);
            engineState->resource_path = NULL;
        }
        
        free(engineState);
        fprintf(stderr, "Engine shutdown successfully\n");
    }
}

// ============================================================================
// WORLD MANAGEMENT FUNCTIONS
// ============================================================================

World* engine_get_world(EngineStateStruct* engine) {
    return engine ? engine->world : NULL;
}

WorldEntity* engine_create_entity(EngineStateStruct* engine, const char* name) {
    if (!engine || !engine->world) {
        return NULL;
    }
    return world_create_entity(engine->world, name);
}

int engine_destroy_entity(EngineStateStruct* engine, uint32_t entity_id) {
    if (!engine || !engine->world) {
        return 0;
    }
    return world_destroy_entity(engine->world, entity_id);
}

WorldEntity* engine_get_entity(EngineStateStruct* engine, uint32_t entity_id) {
    if (!engine || !engine->world) {
        return NULL;
    }
    return world_get_entity(engine->world, entity_id);
}

WorldEntity* engine_get_entity_by_name(EngineStateStruct* engine, const char* name) {
    if (!engine || !engine->world) {
        return NULL;
    }
    return world_get_entity_by_name(engine->world, name);
}
