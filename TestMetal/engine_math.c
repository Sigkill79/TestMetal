#include "engine_math.h"
#include <stdio.h>
#include <math.h>

// ============================================================================
// DEBUG/PRINTING FUNCTIONS IMPLEMENTATION
// ============================================================================

void vec2_print(const char* name, vec2_t v) {
    printf("%s: [%.6f, %.6f]\n", name, v.x, v.y);
}

void vec3_print(const char* name, vec3_t v) {
    printf("%s: [%.6f, %.6f, %.6f]\n", name, v.x, v.y, v.z);
}

void vec4_print(const char* name, vec4_t v) {
    printf("%s: [%.6f, %.6f, %.6f, %.6f]\n", name, v.x, v.y, v.z, v.w);
}

void mat3_print(const char* name, mat3_t m) {
    printf("%s:\n", name);
    printf("  [%.6f, %.6f, %.6f]\n", m.x.x, m.y.x, m.z.x);
    printf("  [%.6f, %.6f, %.6f]\n", m.x.y, m.y.y, m.z.y);
    printf("  [%.6f, %.6f, %.6f]\n", m.x.z, m.y.z, m.z.z);
}

void mat4_print(const char* name, mat4_t m) {
    printf("%s:\n", name);
    printf("  [%.6f, %.6f, %.6f, %.6f]\n", m.x.x, m.y.x, m.z.x, m.w.x);
    printf("  [%.6f, %.6f, %.6f, %.6f]\n", m.x.y, m.y.y, m.z.y, m.w.y);
    printf("  [%.6f, %.6f, %.6f, %.6f]\n", m.x.z, m.y.z, m.z.z, m.w.z);
    printf("  [%.6f, %.6f, %.6f, %.6f]\n", m.x.w, m.y.w, m.z.w, m.w.w);
}

void quat_print(const char* name, quat_t q) {
    printf("%s: [%.6f, %.6f, %.6f, %.6f] (w=%.6f)\n", name, q.x, q.y, q.z, q.w, q.w);
}
