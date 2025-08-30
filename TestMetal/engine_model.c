#include "engine_model.h"
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <string.h>

// ============================================================================
// MEMORY MANAGEMENT IMPLEMENTATION
// ============================================================================

Mesh* mesh_allocate(uint32_t vertex_count, uint32_t index_count) {
    Mesh* mesh = (Mesh*)malloc(sizeof(Mesh));
    if (!mesh) {
        fprintf(stderr, "Error: Failed to allocate memory for mesh\n");
        return NULL;
    }
    
    // Initialize mesh structure
    *mesh = mesh_create();
    mesh->vertex_count = vertex_count;
    mesh->index_count = index_count;
    mesh->triangle_count = mesh_calculate_triangle_count(index_count);
    
    // Allocate vertex array
    if (vertex_count > 0) {
        mesh->vertices = (Vertex*)malloc(vertex_count * sizeof(Vertex));
        if (!mesh->vertices) {
            fprintf(stderr, "Error: Failed to allocate memory for mesh vertices\n");
            free(mesh);
            return NULL;
        }
    }
    
    // Allocate index array
    if (index_count > 0) {
        mesh->indices = (uint32_t*)malloc(index_count * sizeof(uint32_t));
        if (!mesh->indices) {
            fprintf(stderr, "Error: Failed to allocate memory for mesh indices\n");
            if (mesh->vertices) free(mesh->vertices);
            free(mesh);
            return NULL;
        }
    }
    
    return mesh;
}

void mesh_free(Mesh* mesh) {
    if (mesh) {
        if (mesh->vertices) {
            free(mesh->vertices);
            mesh->vertices = NULL;
        }
        if (mesh->indices) {
            free(mesh->indices);
            mesh->indices = NULL;
        }
        mesh->vertex_count = 0;
        mesh->index_count = 0;
        mesh->triangle_count = 0;
    }
}

Model3D* model3d_allocate(uint32_t mesh_count) {
    Model3D* model = (Model3D*)malloc(sizeof(Model3D));
    if (!model) {
        fprintf(stderr, "Error: Failed to allocate memory for 3D model\n");
        return NULL;
    }
    
    // Initialize model structure
    *model = model3d_create();
    model->mesh_count = mesh_count;
    
    // Allocate mesh array
    if (mesh_count > 0) {
        model->meshes = (Mesh*)malloc(mesh_count * sizeof(Mesh));
        if (!model->meshes) {
            fprintf(stderr, "Error: Failed to allocate memory for model meshes\n");
            free(model);
            return NULL;
        }
        
        // Initialize all meshes as empty
        for (uint32_t i = 0; i < mesh_count; i++) {
            model->meshes[i] = mesh_create();
        }
    }
    
    return model;
}

void model3d_free(Model3D* model) {
    if (model) {
        if (model->meshes) {
            // Free all meshes
            for (uint32_t i = 0; i < model->mesh_count; i++) {
                mesh_free(&model->meshes[i]);
            }
            free(model->meshes);
            model->meshes = NULL;
        }
        
        if (model->name) {
            free(model->name);
            model->name = NULL;
        }
        
        model->mesh_count = 0;
        model->bounding_min = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
        model->bounding_max = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        model->center = vec3_zero();
        model->radius = 0.0f;
    }
}

// ============================================================================
// BOUNDING BOX CALCULATIONS IMPLEMENTATION
// ============================================================================

void mesh_calculate_bounds(Mesh* mesh, vec3_t* min, vec3_t* max) {
    if (!mesh || !mesh->vertices || mesh->vertex_count == 0) {
        *min = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
        *max = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        return;
    }
    
    // Initialize bounds with first vertex
    *min = mesh->vertices[0].position;
    *max = mesh->vertices[0].position;
    
    // Find min/max across all vertices
    for (uint32_t i = 1; i < mesh->vertex_count; i++) {
        vec3_t pos = mesh->vertices[i].position;
        
        // Update minimum bounds component by component
        if (pos[0] < (*min)[0]) (*min)[0] = pos[0];
        if (pos[1] < (*min)[1]) (*min)[1] = pos[1];
        if (pos[2] < (*min)[2]) (*min)[2] = pos[2];
        
        // Update maximum bounds component by component
        if (pos[0] > (*max)[0]) (*max)[0] = pos[0];
        if (pos[1] > (*max)[1]) (*max)[1] = pos[1];
        if (pos[2] > (*max)[2]) (*max)[2] = pos[2];
    }
}

void model3d_calculate_bounds(Model3D* model) {
    if (!model || !model->meshes || model->mesh_count == 0) {
        model->bounding_min = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
        model->bounding_max = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        return;
    }
    
    // Initialize bounds
    model->bounding_min = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    model->bounding_max = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    
    // Calculate bounds for each mesh and combine
    for (uint32_t i = 0; i < model->mesh_count; i++) {
        vec3_t mesh_min, mesh_max;
        mesh_calculate_bounds(&model->meshes[i], &mesh_min, &mesh_max);
        
        // Update model bounds component by component
        if (mesh_min[0] < model->bounding_min[0]) model->bounding_min[0] = mesh_min[0];
        if (mesh_min[1] < model->bounding_min[1]) model->bounding_min[1] = mesh_min[1];
        if (mesh_min[2] < model->bounding_min[2]) model->bounding_min[2] = mesh_min[2];
        
        if (mesh_max[0] > model->bounding_max[0]) model->bounding_max[0] = mesh_max[0];
        if (mesh_max[1] > model->bounding_max[1]) model->bounding_max[1] = mesh_max[1];
        if (mesh_max[2] > model->bounding_max[2]) model->bounding_max[2] = mesh_max[2];
    }
}

void model3d_calculate_center_and_radius(Model3D* model) {
    if (!model) return;
    
    // Calculate center as midpoint of bounding box
    model->center = vec3_scale(vec3_add(model->bounding_min, model->bounding_max), 0.5f);
    
    // Calculate radius as distance from center to furthest vertex
    model->radius = 0.0f;
    for (uint32_t i = 0; i < model->mesh_count; i++) {
        if (model->meshes[i].vertices) {
            for (uint32_t j = 0; j < model->meshes[i].vertex_count; j++) {
                vec3_t pos = model->meshes[i].vertices[j].position;
                float dist = vec3_distance(model->center, pos);
                if (dist > model->radius) {
                    model->radius = dist;
                }
            }
        }
    }
}

// ============================================================================
// DEBUG/PRINTING IMPLEMENTATION
// ============================================================================

void vertex_print(const char* name, Vertex v) {
    printf("%s:\n", name);
    printf("  Position: [%.6f, %.6f, %.6f]\n", v.position[0], v.position[1], v.position[2]);
    printf("  TexCoord: [%.6f, %.6f]\n", v.texcoord[0], v.texcoord[1]);
    printf("  Normal:   [%.6f, %.6f, %.6f]\n", v.normal[0], v.normal[1], v.normal[2]);
}

void mesh_print(const char* name, Mesh* mesh) {
    if (!mesh) {
        printf("%s: NULL\n", name);
        return;
    }
    
    printf("%s:\n", name);
    printf("  Vertices: %u\n", mesh->vertex_count);
    printf("  Indices:  %u\n", mesh->index_count);
    printf("  Triangles: %u\n", mesh->triangle_count);
    
    if (mesh->vertices && mesh->vertex_count > 0) {
        printf("  First vertex:\n");
        vertex_print("    ", mesh->vertices[0]);
        
        if (mesh->vertex_count > 1) {
            printf("  Last vertex:\n");
            vertex_print("    ", mesh->vertices[mesh->vertex_count - 1]);
        }
    }
    
    if (mesh->indices && mesh->index_count > 0) {
        printf("  First triangle indices: [%u, %u, %u]\n", 
               mesh->indices[0], mesh->indices[1], mesh->indices[2]);
        
        if (mesh->index_count >= 6) {
            printf("  Second triangle indices: [%u, %u, %u]\n", 
                   mesh->indices[3], mesh->indices[4], mesh->indices[5]);
        }
    }
}

void model3d_print(const char* name, Model3D* model) {
    if (!model) {
        printf("%s: NULL\n", name);
        return;
    }
    
    printf("%s:\n", name);
    printf("  Name: %s\n", model->name ? model->name : "(unnamed)");
    printf("  Meshes: %u\n", model->mesh_count);
    printf("  Bounding Box:\n");
    printf("    Min: [%.6f, %.6f, %.6f]\n", 
           model->bounding_min[0], model->bounding_min[1], model->bounding_min[2]);
    printf("    Max: [%.6f, %.6f, %.6f]\n", 
           model->bounding_max[0], model->bounding_max[1], model->bounding_max[2]);
    printf("  Center: [%.6f, %.6f, %.6f]\n", 
           model->center[0], model->center[1], model->center[2]);
    printf("  Radius: %.6f\n", model->radius);
    
    // Print each mesh
    for (uint32_t i = 0; i < model->mesh_count; i++) {
        char mesh_name[64];
        snprintf(mesh_name, sizeof(mesh_name), "  Mesh %u", i);
        mesh_print(mesh_name, &model->meshes[i]);
    }
}
