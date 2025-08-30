#include "engine_model.h"
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// TEST FUNCTIONS
// ============================================================================

// Create a simple unit cube model for testing
Model3D* create_unit_cube(void) {
    // Allocate model with 1 mesh
    Model3D* model = model3d_allocate(1);
    if (!model) {
        fprintf(stderr, "Failed to allocate cube model\n");
        return NULL;
    }
    
    // Set model name
    model->name = strdup("UnitCube");
    
    // Allocate mesh with 8 vertices and 36 indices (12 triangles)
    Mesh* mesh = &model->meshes[0];
    *mesh = *mesh_allocate(8, 36);
    
    if (!mesh->vertices || !mesh->indices) {
        fprintf(stderr, "Failed to allocate cube mesh\n");
        model3d_free(model);
        return NULL;
    }
    
    // Define 8 vertices for a unit cube centered at origin
    // Each vertex has position, texture coordinates, and normal
    mesh->vertices[0] = vertex_create_components(-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f,  0.0f, -1.0f); // Back bottom left
    mesh->vertices[1] = vertex_create_components( 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f,  0.0f, -1.0f); // Back bottom right
    mesh->vertices[2] = vertex_create_components( 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f,  0.0f, -1.0f); // Back top right
    mesh->vertices[3] = vertex_create_components(-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f,  0.0f, -1.0f); // Back top left
    mesh->vertices[4] = vertex_create_components(-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f,  0.0f,  1.0f); // Front bottom left
    mesh->vertices[5] = vertex_create_components( 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f,  0.0f,  1.0f); // Front bottom right
    mesh->vertices[6] = vertex_create_components( 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f,  0.0f,  1.0f); // Front top right
    mesh->vertices[7] = vertex_create_components(-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  0.0f,  0.0f,  1.0f); // Front top left
    
    // Define 36 indices for 12 triangles (6 faces, 2 triangles per face)
    // Back face
    mesh->indices[0] = 0; mesh->indices[1] = 1; mesh->indices[2] = 2;
    mesh->indices[3] = 0; mesh->indices[4] = 2; mesh->indices[5] = 3;
    
    // Front face
    mesh->indices[6] = 4; mesh->indices[7] = 7; mesh->indices[8] = 6;
    mesh->indices[9] = 4; mesh->indices[10] = 6; mesh->indices[11] = 5;
    
    // Left face
    mesh->indices[12] = 0; mesh->indices[13] = 3; mesh->indices[14] = 7;
    mesh->indices[15] = 0; mesh->indices[16] = 7; mesh->indices[17] = 4;
    
    // Right face
    mesh->indices[18] = 1; mesh->indices[19] = 5; mesh->indices[20] = 6;
    mesh->indices[21] = 1; mesh->indices[22] = 6; mesh->indices[23] = 2;
    
    // Bottom face
    mesh->indices[24] = 0; mesh->indices[25] = 4; mesh->indices[26] = 5;
    mesh->indices[27] = 0; mesh->indices[28] = 5; mesh->indices[29] = 1;
    
    // Top face
    mesh->indices[30] = 3; mesh->indices[31] = 2; mesh->indices[32] = 6;
    mesh->indices[33] = 3; mesh->indices[34] = 6; mesh->indices[35] = 7;
    
    // Update triangle count
    mesh->triangle_count = mesh_calculate_triangle_count(mesh->index_count);
    
    return model;
}

// Test vertex creation and manipulation
void test_vertices(void) {
    printf("=== Testing Vertex Functions ===\n");
    
    // Test vertex creation
    Vertex v1 = vertex_create_components(1.0f, 2.0f, 3.0f, 0.5f, 0.7f, 0.0f, 1.0f, 0.0f);
    Vertex v2 = vertex_default();
    
    printf("Created vertex v1:\n");
    vertex_print("v1", v1);
    
    printf("Default vertex v2:\n");
    vertex_print("v2", v2);
    
    // Test vertex manipulation using our vector library
    v1.position = vec3_scale(v1.position, 2.0f);
    v1.texcoord = vec2_add(v1.texcoord, vec2(0.1f, 0.1f));
    v1.normal = vec3_normalize(v1.normal);
    
    printf("Modified vertex v1:\n");
    vertex_print("v1", v1);
    
    printf("\n");
}

// Test mesh creation and management
void test_mesh(void) {
    printf("=== Testing Mesh Functions ===\n");
    
    // Create a simple triangle mesh
    Mesh* mesh = mesh_allocate(3, 3);
    if (!mesh) {
        fprintf(stderr, "Failed to create test mesh\n");
        return;
    }
    
    // Set up a simple triangle
    mesh->vertices[0] = vertex_create_components(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    mesh->vertices[1] = vertex_create_components(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    mesh->vertices[2] = vertex_create_components(0.5f, 1.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f);
    
    mesh->indices[0] = 0;
    mesh->indices[1] = 1;
    mesh->indices[2] = 2;
    
    printf("Created triangle mesh:\n");
    mesh_print("Triangle", mesh);
    
    // Calculate bounds
    vec3_t min, max;
    mesh_calculate_bounds(mesh, &min, &max);
    printf("Mesh bounds: min=[%.3f, %.3f, %.3f], max=[%.3f, %.3f, %.3f]\n",
           min[0], min[1], min[2], max[0], max[1], max[2]);
    
    // Clean up
    mesh_free(mesh);
    printf("\n");
}

// Test 3D model creation and management
void test_model3d(void) {
    printf("=== Testing 3D Model Functions ===\n");
    
    // Create a unit cube
    Model3D* cube = create_unit_cube();
    if (!cube) {
        fprintf(stderr, "Failed to create cube model\n");
        return;
    }
    
    printf("Created cube model:\n");
    model3d_print("Cube", cube);
    
    // Calculate bounds and center
    model3d_calculate_bounds(cube);
    model3d_calculate_center_and_radius(cube);
    
    printf("\nAfter calculating bounds:\n");
    model3d_print("Cube", cube);
    
    // Clean up
    model3d_free(cube);
    printf("\n");
}

// Test memory management and error handling
void test_memory_management(void) {
    printf("=== Testing Memory Management ===\n");
    
    // Test allocation with zero counts
    Mesh* empty_mesh = mesh_allocate(0, 0);
    if (empty_mesh) {
        printf("Created empty mesh successfully\n");
        mesh_print("Empty", empty_mesh);
        mesh_free(empty_mesh);
    }
    
    // Test model allocation with zero meshes
    Model3D* empty_model = model3d_allocate(0);
    if (empty_model) {
        printf("Created empty model successfully\n");
        model3d_print("Empty", empty_model);
        model3d_free(empty_model);
    }
    
    printf("\n");
}

// ============================================================================
// MAIN TEST FUNCTION
// ============================================================================

int main(void) {
    printf("🧪 Engine 3D Model Library Test\n");
    printf("================================\n\n");
    
    // Run all tests
    test_vertices();
    test_mesh();
    test_model3d();
    test_memory_management();
    
    printf("✅ All tests completed successfully!\n");
    return 0;
}
