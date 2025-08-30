#ifndef ENGINE_MODEL_H
#define ENGINE_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine_math.h"
#include <stdint.h>
#include <float.h>

// ============================================================================
// 3D MODEL DATA STRUCTURES
// ============================================================================

// Vertex structure using our vector library
typedef struct {
    vec3_t position;      // 3D position (x, y, z)
    vec2_t texcoord;      // Texture coordinates (u, v)
    vec3_t normal;        // Surface normal (nx, ny, nz)
} Vertex;

// Mesh structure containing vertices and indices
typedef struct {
    Vertex* vertices;     // Array of vertices
    uint32_t* indices;    // Array of triangle indices
    uint32_t vertex_count; // Number of vertices
    uint32_t index_count;  // Number of indices
    uint32_t triangle_count; // Number of triangles (index_count / 3)
} Mesh;

// 3D Model structure containing multiple meshes
typedef struct {
    Mesh* meshes;         // Array of meshes
    uint32_t mesh_count;  // Number of meshes
    char* name;           // Model name/identifier
    vec3_t bounding_min;  // Bounding box minimum
    vec3_t bounding_max;  // Bounding box maximum
    vec3_t center;        // Model center point
    float radius;         // Bounding sphere radius
} Model3D;

// ============================================================================
// VERTEX UTILITY FUNCTIONS
// ============================================================================

// Create a vertex with given components
FORCE_INLINE Vertex vertex_create(vec3_t position, vec2_t texcoord, vec3_t normal) {
    Vertex v;
    v.position = position;
    v.texcoord = texcoord;
    v.normal = normal;
    return v;
}

// Create a vertex with individual components
FORCE_INLINE Vertex vertex_create_components(float x, float y, float z, float u, float v, float nx, float ny, float nz) {
    return vertex_create(
        vec3(x, y, z),
        vec2(u, v),
        vec3(nx, ny, nz)
    );
}

// Create a default vertex at origin
FORCE_INLINE Vertex vertex_default(void) {
    return vertex_create(
        vec3_zero(),
        vec2_zero(),
        vec3_unit_z()
    );
}

// ============================================================================
// MESH UTILITY FUNCTIONS
// ============================================================================

// Create an empty mesh
FORCE_INLINE Mesh mesh_create(void) {
    Mesh mesh;
    mesh.vertices = NULL;
    mesh.indices = NULL;
    mesh.vertex_count = 0;
    mesh.index_count = 0;
    mesh.triangle_count = 0;
    return mesh;
}

// Calculate triangle count from index count
FORCE_INLINE uint32_t mesh_calculate_triangle_count(uint32_t index_count) {
    return index_count / 3;
}

// ============================================================================
// MODEL3D UTILITY FUNCTIONS
// ============================================================================

// Create an empty 3D model
FORCE_INLINE Model3D model3d_create(void) {
    Model3D model;
    model.meshes = NULL;
    model.mesh_count = 0;
    model.name = NULL;
    model.bounding_min = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    model.bounding_max = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    model.center = vec3_zero();
    model.radius = 0.0f;
    return model;
}

// ============================================================================
// MEMORY MANAGEMENT DECLARATIONS
// ============================================================================

// Allocate memory for a mesh with given vertex and index counts
Mesh* mesh_allocate(uint32_t vertex_count, uint32_t index_count);

// Free memory for a mesh
void mesh_free(Mesh* mesh);

// Allocate memory for a 3D model with given mesh count
Model3D* model3d_allocate(uint32_t mesh_count);

// Free memory for a 3D model
void model3d_free(Model3D* model);

// ============================================================================
// BOUNDING BOX CALCULATIONS
// ============================================================================

// Calculate bounding box for a mesh
void mesh_calculate_bounds(Mesh* mesh, vec3_t* min, vec3_t* max);

// Calculate bounding box for entire model
void model3d_calculate_bounds(Model3D* model);

// Calculate center and radius for a model
void model3d_calculate_center_and_radius(Model3D* model);

// ============================================================================
// DEBUG/PRINTING DECLARATIONS
// ============================================================================

// Print vertex information
void vertex_print(const char* name, Vertex v);

// Print mesh information
void mesh_print(const char* name, Mesh* mesh);

// Print 3D model information
void model3d_print(const char* name, Model3D* model);

#ifdef __cplusplus
}
#endif

#endif // ENGINE_MODEL_H
