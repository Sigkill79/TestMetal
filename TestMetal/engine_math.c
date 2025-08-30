#include "engine_math.h"
#include <stdio.h>
#include <math.h>

// ============================================================================
// DEBUG/PRINTING FUNCTIONS IMPLEMENTATION
// ============================================================================

void vec2_print(const char* name, vec2_t v) {
    printf("%s: [%.6f, %.6f]\n", name, v[0], v[1]);
}

void vec3_print(const char* name, vec3_t v) {
    printf("%s: [%.6f, %.6f, %.6f]\n", name, v[0], v[1], v[2]);
}

void vec4_print(const char* name, vec4_t v) {
    printf("%s: [%.6f, %.6f, %.6f, %.6f]\n", name, v[0], v[1], v[2], v[3]);
}

void mat3_print(const char* name, mat3_t m) {
    printf("%s:\n", name);
    printf("  [%.6f, %.6f, %.6f]\n", m.x[0], m.y[0], m.z[0]);
    printf("  [%.6f, %.6f, %.6f]\n", m.x[1], m.y[1], m.z[1]);
    printf("  [%.6f, %.6f, %.6f]\n", m.x[2], m.y[2], m.z[2]);
}

void mat4_print(const char* name, mat4_t m) {
    printf("%s:\n", name);
    printf("  [%.6f, %.6f, %.6f, %.6f]\n", m.x[0], m.y[0], m.z[0], m.w[0]);
    printf("  [%.6f, %.6f, %.6f, %.6f]\n", m.x[1], m.y[1], m.z[1], m.w[1]);
    printf("  [%.6f, %.6f, %.6f, %.6f]\n", m.x[2], m.y[2], m.z[2], m.w[2]);
    printf("  [%.6f, %.6f, %.6f, %.6f]\n", m.x[3], m.y[3], m.z[3], m.w[3]);
}
