#include "engine_world.h"
#include "engine_main.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// WORLD MANAGEMENT FUNCTIONS
// ============================================================================

World* world_create(uint32_t max_entities) {
    if (max_entities == 0) {
        fprintf(stderr, "Error: Cannot create world with 0 max entities\n");
        return NULL;
    }
    
    World* world = (World*)malloc(sizeof(World));
    if (!world) {
        fprintf(stderr, "Error: Failed to allocate memory for world\n");
        return NULL;
    }
    
    world->entities = (WorldEntity*)calloc(max_entities, sizeof(WorldEntity));
    if (!world->entities) {
        fprintf(stderr, "Error: Failed to allocate memory for entity array\n");
        free(world);
        return NULL;
    }
    
    world->entity_count = 0;
    world->max_entities = max_entities;
    world->next_id = 1; // Start IDs at 1, 0 is reserved for invalid
    
    fprintf(stderr, "Created world with capacity for %u entities\n", max_entities);
    return world;
}

void world_destroy(World* world) {
    if (!world) {
        return;
    }
    
    // Free all entity names and models
    for (uint32_t i = 0; i < world->max_entities; i++) {
        WorldEntity* entity = &world->entities[i];
        if (entity->name) {
            free(entity->name);
            entity->name = NULL;
        }
        // Note: We don't free MetalModelHandle here as it's managed by the Metal engine
        entity->metal_model = NULL;
    }
    
    free(world->entities);
    free(world);
    
    fprintf(stderr, "Destroyed world\n");
}

uint32_t world_get_entity_count(const World* world) {
    return world ? world->entity_count : 0;
}

uint32_t world_get_max_entities(const World* world) {
    return world ? world->max_entities : 0;
}

// ============================================================================
// ENTITY MANAGEMENT FUNCTIONS
// ============================================================================

WorldEntity* world_create_entity(World* world, const char* name) {
    if (!world) {
        fprintf(stderr, "Error: Cannot create entity in NULL world\n");
        return NULL;
    }
    
    if (world->entity_count >= world->max_entities) {
        fprintf(stderr, "Error: World is at maximum capacity (%u entities)\n", world->max_entities);
        return NULL;
    }
    
    // Find the first available slot
    WorldEntity* entity = NULL;
    for (uint32_t i = 0; i < world->max_entities; i++) {
        if (world->entities[i].id == 0) { // 0 means unused slot
            entity = &world->entities[i];
            break;
        }
    }
    
    if (!entity) {
        fprintf(stderr, "Error: No available slots in world\n");
        return NULL;
    }
    
    // Initialize the entity
    entity->id = world->next_id++;
    entity->position = vec3_zero();
    entity->orientation = quat_identity();
    entity->metal_model = NULL;
    entity->is_active = 1;
    
    // Set name
    if (name) {
        entity->name = (char*)malloc(strlen(name) + 1);
        if (entity->name) {
            strcpy(entity->name, name);
        } else {
            fprintf(stderr, "Warning: Failed to allocate memory for entity name\n");
        }
    } else {
        entity->name = NULL;
    }
    
    world->entity_count++;
    
    fprintf(stderr, "Created entity ID %u with name '%s'\n", entity->id, name ? name : "unnamed");
    return entity;
}

int world_destroy_entity(World* world, uint32_t entity_id) {
    if (!world) {
        fprintf(stderr, "Error: Cannot destroy entity in NULL world\n");
        return 0;
    }
    
    if (entity_id == 0) {
        fprintf(stderr, "Error: Invalid entity ID (0)\n");
        return 0;
    }
    
    // Find the entity
    WorldEntity* entity = NULL;
    for (uint32_t i = 0; i < world->max_entities; i++) {
        if (world->entities[i].id == entity_id) {
            entity = &world->entities[i];
            break;
        }
    }
    
    if (!entity) {
        fprintf(stderr, "Error: Entity with ID %u not found\n", entity_id);
        return 0;
    }
    
    // Free entity resources
    if (entity->name) {
        free(entity->name);
        entity->name = NULL;
    }
    
    // Reset entity to unused state
    entity->id = 0;
    entity->position = vec3_zero();
    entity->orientation = quat_identity();
    entity->metal_model = NULL;
    entity->is_active = 0;
    
    world->entity_count--;
    
    fprintf(stderr, "Destroyed entity ID %u\n", entity_id);
    return 1;
}

WorldEntity* world_get_entity(World* world, uint32_t entity_id) {
    if (!world || entity_id == 0) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < world->max_entities; i++) {
        if (world->entities[i].id == entity_id) {
            return &world->entities[i];
        }
    }
    
    return NULL;
}

WorldEntity* world_get_entity_by_name(World* world, const char* name) {
    if (!world || !name) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < world->max_entities; i++) {
        WorldEntity* entity = &world->entities[i];
        if (entity->id != 0 && entity->name && strcmp(entity->name, name) == 0) {
            return entity;
        }
    }
    
    return NULL;
}

WorldEntity* world_get_all_entities(World* world) {
    return world ? world->entities : NULL;
}

// ============================================================================
// ENTITY OPERATIONS
// ============================================================================

void entity_set_position(WorldEntity* entity, vec3_t position) {
    if (entity) {
        entity->position = position;
    }
}

vec3_t entity_get_position(const WorldEntity* entity) {
    return entity ? entity->position : vec3_zero();
}

void entity_set_orientation(WorldEntity* entity, quat_t orientation) {
    if (entity) {
        entity->orientation = quat_normalize(orientation);
    }
}

quat_t entity_get_orientation(const WorldEntity* entity) {
    return entity ? entity->orientation : quat_identity();
}

void entity_set_orientation_euler(WorldEntity* entity, float x, float y, float z) {
    if (entity) {
        entity->orientation = quat_normalize(quat_from_euler(x, y, z));
    }
}

void entity_set_orientation_axis_angle(WorldEntity* entity, vec3_t axis, float angle) {
    if (entity) {
        entity->orientation = quat_normalize(quat_from_axis_angle(axis, angle));
    }
}

void entity_set_model(WorldEntity* entity, MetalModelHandle model) {
    if (entity) {
        entity->metal_model = model;
    }
}

MetalModelHandle entity_get_model(const WorldEntity* entity) {
    return entity ? entity->metal_model : NULL;
}

void entity_set_name(WorldEntity* entity, const char* name) {
    if (!entity) {
        return;
    }
    
    // Free existing name
    if (entity->name) {
        free(entity->name);
        entity->name = NULL;
    }
    
    // Set new name
    if (name) {
        entity->name = (char*)malloc(strlen(name) + 1);
        if (entity->name) {
            strcpy(entity->name, name);
        } else {
            fprintf(stderr, "Warning: Failed to allocate memory for entity name\n");
        }
    }
}

const char* entity_get_name(const WorldEntity* entity) {
    return entity ? entity->name : NULL;
}

void entity_set_active(WorldEntity* entity, int is_active) {
    if (entity) {
        entity->is_active = is_active ? 1 : 0;
    }
}

int entity_is_active(const WorldEntity* entity) {
    return entity ? entity->is_active : 0;
}

uint32_t entity_get_id(const WorldEntity* entity) {
    return entity ? entity->id : 0;
}

// ============================================================================
// WORLD RENDERING
// ============================================================================

void world_render(World* world, MetalEngine* metal_engine, void* engine_state) {
    if (!world || !metal_engine || !engine_state) {
        return;
    }
    
    // Render all active entities
    for (uint32_t i = 0; i < world->max_entities; i++) {
        WorldEntity* entity = &world->entities[i];
        if (entity->id != 0 && entity->is_active && entity->metal_model) {
            entity_render(entity, metal_engine, engine_state);
        }
    }
}

void entity_render(WorldEntity* entity, MetalEngine* metal_engine, void* engine_state) {
    if (!entity || !metal_engine || !engine_state || !entity->metal_model) {
        return;
    }
    
    // Get the entity's transformation matrix
    mat4_t entityTransform = entity_get_transform_matrix(entity);
    
    // Update the engine state's model matrix with the entity's transform
    EngineStateStruct* engineState = (EngineStateStruct*)engine_state;
    engineState->model_matrix = entityTransform;
    
    // The actual rendering will be handled by the Metal engine's render_model function
    // which is called from the Metal render frame after we update the model matrix
    // Entity rendered silently
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

mat4_t entity_get_transform_matrix(const WorldEntity* entity) {
    if (!entity) {
        return mat4_identity();
    }
    
    // Create transformation matrix: T * R (translate then rotate)
    mat4_t translation = mat4_translation(entity->position);
    mat4_t rotation = quat_to_mat4(entity->orientation);
    
    return mat4_mul_mat4(translation, rotation);
}

int entity_is_valid(const WorldEntity* entity) {
    return entity && entity->id != 0;
}

// ============================================================================
// DEBUG/PRINTING FUNCTIONS
// ============================================================================

void entity_print(const char* name, const WorldEntity* entity) {
    if (!entity) {
        printf("%s: NULL\n", name);
        return;
    }
    
    printf("%s:\n", name);
    printf("  ID: %u\n", entity->id);
    printf("  Name: %s\n", entity->name ? entity->name : "unnamed");
    printf("  Active: %s\n", entity->is_active ? "yes" : "no");
    printf("  Position: (%.3f, %.3f, %.3f)\n", 
           entity->position.x, entity->position.y, entity->position.z);
    printf("  Orientation: (%.3f, %.3f, %.3f, %.3f)\n", 
           entity->orientation.x, entity->orientation.y, 
           entity->orientation.z, entity->orientation.w);
    printf("  Metal Model: %p\n", entity->metal_model);
}

void world_print(const char* name, const World* world) {
    if (!world) {
        printf("%s: NULL\n", name);
        return;
    }
    
    printf("%s:\n", name);
    printf("  Entity Count: %u / %u\n", world->entity_count, world->max_entities);
    printf("  Next ID: %u\n", world->next_id);
    
    printf("  Entities:\n");
    for (uint32_t i = 0; i < world->max_entities; i++) {
        WorldEntity* entity = &world->entities[i];
        if (entity->id != 0) {
            printf("    [%u] %s (ID: %u, Active: %s)\n", 
                   i, entity->name ? entity->name : "unnamed", 
                   entity->id, entity->is_active ? "yes" : "no");
        }
    }
}
