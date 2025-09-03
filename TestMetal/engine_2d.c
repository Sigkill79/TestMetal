#include "engine_2d.h"
#include "engine_metal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// UI 2D IMPLEMENTATION
// ============================================================================

// Debug macros
#define UI_DEBUG(fmt, ...) fprintf(stderr, "UI Debug: " fmt "\n", ##__VA_ARGS__)
#define UI_ERROR(fmt, ...) fprintf(stderr, "UI Error: " fmt "\n", ##__VA_ARGS__)
#define UI_INFO(fmt, ...) fprintf(stderr, "UI Info: " fmt "\n", ##__VA_ARGS__)

// Internal helper function to create UI pipeline state
static int create_ui_pipeline_state(Engine2D* ui2d) {
    if (!ui2d || !ui2d->metalEngine) {
        UI_ERROR("Invalid parameters for pipeline creation");
        return 0;
    }
    
    UI_INFO("UI pipeline state will be created by Metal engine");
    
    // Mark as initialized - the actual pipeline creation is handled in the Metal engine
    ui2d->isInitialized = 1;
    
    return 1;
}

// Internal helper function to create UI buffers
static int create_ui_buffers(Engine2D* ui2d) {
    if (!ui2d) {
        UI_ERROR("Invalid UI2D parameter");
        return 0;
    }
    
    UI_INFO("Creating UI buffers");
    
    // Allocate element array
    ui2d->elements = (UIElement*)malloc(UI_MAX_ELEMENTS * sizeof(UIElement));
    if (!ui2d->elements) {
        UI_ERROR("Failed to allocate UI elements array");
        return 0;
    }
    
    // Initialize element array
    memset(ui2d->elements, 0, UI_MAX_ELEMENTS * sizeof(UIElement));
    
    // Set buffer limits
    ui2d->maxElements = UI_MAX_ELEMENTS;
    ui2d->maxVertices = UI_MAX_VERTICES;
    ui2d->maxIndices = UI_MAX_INDICES;
    
    UI_INFO("UI buffers created successfully");
    return 1;
}

// Internal helper function to generate quad vertices
static void generate_quad_vertices(UI2DVertex* vertices, float x, float y, float width, float height) {
    // Top-left
    vertices[0].position[0] = x;
    vertices[0].position[1] = y;
    vertices[0].texcoord[0] = 0.0f;
    vertices[0].texcoord[1] = 0.0f;
    
    // Top-right
    vertices[1].position[0] = x + width;
    vertices[1].position[1] = y;
    vertices[1].texcoord[0] = 1.0f;
    vertices[1].texcoord[1] = 0.0f;
    
    // Bottom-right
    vertices[2].position[0] = x + width;
    vertices[2].position[1] = y + height;
    vertices[2].texcoord[0] = 1.0f;
    vertices[2].texcoord[1] = 1.0f;
    
    // Bottom-left
    vertices[3].position[0] = x;
    vertices[3].position[1] = y + height;
    vertices[3].texcoord[0] = 0.0f;
    vertices[3].texcoord[1] = 1.0f;
}

// Internal helper function to generate quad indices
static void generate_quad_indices(uint32_t* indices, uint32_t startVertex, uint32_t startIndex) {
    // First triangle
    indices[startIndex + 0] = startVertex + 0;
    indices[startIndex + 1] = startVertex + 1;
    indices[startIndex + 2] = startVertex + 2;
    
    // Second triangle
    indices[startIndex + 3] = startVertex + 0;
    indices[startIndex + 4] = startVertex + 2;
    indices[startIndex + 5] = startVertex + 3;
}

// ============================================================================
// PUBLIC FUNCTIONS
// ============================================================================

Engine2D *engine_2d_init(MetalEngineHandle metalEngine) {
    
    if (!metalEngine) {
        UI_ERROR("Invalid Metal engine parameter for UI2D initialization");
        return NULL;
    }
    
    Engine2D* ui2d = (Engine2D*)malloc(sizeof(Engine2D));
    if (!ui2d) {
        UI_ERROR("Failed to allocate memory for UI2D");
        return NULL;
    }
    
    UI_INFO("Initializing UI 2D system");
    
    // Initialize structure
    memset(ui2d, 0, sizeof(Engine2D));
    
    // Store Metal engine reference
    ui2d->metalEngine = metalEngine;
    
    // Create buffers
    if (!create_ui_buffers(ui2d)) {
        UI_ERROR("Failed to create UI buffers");
        free(ui2d);
        return NULL;
    }
    
    // Create pipeline state
    if (!create_ui_pipeline_state(ui2d)) {
        UI_ERROR("Failed to create UI pipeline state");
        free(ui2d);
        return NULL;
    }
    
    UI_INFO("UI 2D system initialized successfully");
    return ui2d;
}

void engine_2d_shutdown(Engine2D* ui2d) {
    if (!ui2d) return;
    
    UI_INFO("Shutting down UI 2D system");
    
    // Free element array
    if (ui2d->elements) {
        free(ui2d->elements);
        ui2d->elements = NULL;
    }
    
    // Note: Metal objects will be released by the Metal engine
    ui2d->vertexBuffer = NULL;
    ui2d->indexBuffer = NULL;
    ui2d->uniformBuffer = NULL;
    ui2d->uiPipelineState = NULL;
    ui2d->uiDepthState = NULL;
    
    ui2d->isInitialized = 0;
    
    free(ui2d);
    
    UI_INFO("UI 2D system shutdown complete");
}

void engine_2d_clear_elements(Engine2D* ui2d) {
    if (!ui2d || !ui2d->elements) return;
    
    // Reset element count and clear active flags
    ui2d->elementCount = 0;
    ui2d->vertexCount = 0;
    ui2d->indexCount = 0;
    
    // Clear all elements
    for (uint32_t i = 0; i < ui2d->maxElements; i++) {
        ui2d->elements[i].isActive = 0;
    }
}

int engine_2d_draw_image(Engine2D* ui2d, float x, float y, MetalTextureHandle texture) {
    if (!ui2d || !ui2d->elements || !texture) {
        UI_ERROR("Invalid parameters for draw_image");
        return 0;
    }
    
    if (ui2d->elementCount >= ui2d->maxElements) {
        UI_ERROR("Maximum UI elements reached (%u)", ui2d->maxElements);
        return 0;
    }
    
    // Get the element
    UIElement* element = &ui2d->elements[ui2d->elementCount];
    
    // Set element properties
    element->texture = texture;
    element->x = x;
    element->y = y;
    element->width = 256.0f;  // Default size for testing - will be updated with actual texture size
    element->height = 256.0f; // Default size for testing - will be updated with actual texture size
    element->startIndex = ui2d->indexCount;
    element->indexCount = UI_INDICES_PER_ELEMENT;
    element->isActive = 1;
    
    // Update counts
    ui2d->elementCount++;
    ui2d->vertexCount += UI_VERTICES_PER_ELEMENT;
    ui2d->indexCount += UI_INDICES_PER_ELEMENT;
    
    UI_DEBUG("Added UI element: x=%.1f, y=%.1f, size=%.0fx%.0f, elementCount=%u", 
             x, y, element->width, element->height, ui2d->elementCount);
    
    return 1;
}

void engine_2d_render_pass(Engine2D* ui2d, void* renderEncoder, float screenWidth, float screenHeight) {
    if (!ui2d || !ui2d->elements || !renderEncoder || ui2d->elementCount == 0) {
        return;
    }
    
    UI_DEBUG("Rendering UI pass: %u elements, screen=%.0fx%.0f", ui2d->elementCount, screenWidth, screenHeight);
    
    // This function will be implemented in the Metal engine
    // where we have access to the actual Metal objects
    // For now, we just log the call
    UI_INFO("UI render pass called - implementation in Metal engine");
}
