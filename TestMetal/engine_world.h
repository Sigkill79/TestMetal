#ifndef ENGINE_WORLD_H
#define ENGINE_WORLD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine_math.h"
#include "engine_metal.h"
#include <stdint.h>
#include <stddef.h>

// ============================================================================
// WORLD ENTITY SYSTEM
// ============================================================================

// Forward declarations
struct MetalEngine;
typedef struct MetalEngine* MetalEngineHandle;

// World entity structure
typedef struct {
    uint32_t id;                    // Unique entity ID
    vec3_t position;                // World position
    quat_t orientation;             // Quaternion rotation
    MetalModelHandle metal_model;   // Metal model to render
    char* name;                     // Entity name/identifier
    int is_active;                  // Active/inactive flag
} WorldEntity;

// World container structure
typedef struct {
    WorldEntity* entities;          // Array of entities
    uint32_t entity_count;          // Number of active entities
    uint32_t max_entities;          // Maximum capacity
    uint32_t next_id;               // Next available ID
} World;

// ============================================================================
// WORLD MANAGEMENT FUNCTIONS
// ============================================================================

// Create a new world with specified maximum entity count
World* world_create(uint32_t max_entities);

// Destroy a world and free all associated memory
void world_destroy(World* world);

// Get the number of active entities in the world
uint32_t world_get_entity_count(const World* world);

// Get the maximum number of entities the world can hold
uint32_t world_get_max_entities(const World* world);

// ============================================================================
// ENTITY MANAGEMENT FUNCTIONS
// ============================================================================

// Create a new entity in the world
WorldEntity* world_create_entity(World* world, const char* name);

// Destroy an entity by ID
int world_destroy_entity(World* world, uint32_t entity_id);

// Get an entity by ID
WorldEntity* world_get_entity(World* world, uint32_t entity_id);

// Get an entity by name (returns first match)
WorldEntity* world_get_entity_by_name(World* world, const char* name);

// Get all entities in the world (for iteration)
WorldEntity* world_get_all_entities(World* world);

// ============================================================================
// ENTITY OPERATIONS
// ============================================================================

// Set entity position
void entity_set_position(WorldEntity* entity, vec3_t position);

// Get entity position
vec3_t entity_get_position(const WorldEntity* entity);

// Set entity orientation using quaternion
void entity_set_orientation(WorldEntity* entity, quat_t orientation);

// Get entity orientation
quat_t entity_get_orientation(const WorldEntity* entity);

// Set entity orientation using Euler angles (convenience function)
void entity_set_orientation_euler(WorldEntity* entity, float x, float y, float z);

// Set entity orientation using axis-angle (convenience function)
void entity_set_orientation_axis_angle(WorldEntity* entity, vec3_t axis, float angle);

// Set the Metal model for an entity
void entity_set_model(WorldEntity* entity, MetalModelHandle model);

// Get the Metal model for an entity
MetalModelHandle entity_get_model(const WorldEntity* entity);

// Set entity name
void entity_set_name(WorldEntity* entity, const char* name);

// Get entity name
const char* entity_get_name(const WorldEntity* entity);

// Set entity active state
void entity_set_active(WorldEntity* entity, int is_active);

// Get entity active state
int entity_is_active(const WorldEntity* entity);

// Get entity ID
uint32_t entity_get_id(const WorldEntity* entity);

// ============================================================================
// WORLD RENDERING
// ============================================================================

// Render all active entities in the world
void world_render(World* world, MetalEngine* metal_engine, void* engine_state);

// Render a specific entity
void entity_render(WorldEntity* entity, MetalEngine* metal_engine, void* engine_state);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Calculate the transformation matrix for an entity (position + orientation)
mat4_t entity_get_transform_matrix(const WorldEntity* entity);

// Check if an entity is valid (not NULL and has valid ID)
int entity_is_valid(const WorldEntity* entity);

// ============================================================================
// DEBUG/PRINTING FUNCTIONS
// ============================================================================

// Print entity information
void entity_print(const char* name, const WorldEntity* entity);

// Print world information
void world_print(const char* name, const World* world);

#ifdef __cplusplus
}
#endif

#endif // ENGINE_WORLD_H
