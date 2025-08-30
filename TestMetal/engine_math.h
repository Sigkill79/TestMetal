#ifndef ENGINE_MATH_H
#define ENGINE_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <string.h>

// ============================================================================
// COMPILER-SPECIFIC VECTOR EXTENSIONS
// ============================================================================

// GCC/Clang vector extensions for SIMD operations
#if defined(__GNUC__) || defined(__clang__)
    #define VECTOR_ALIGN __attribute__((aligned(16)))
    #define FORCE_INLINE __attribute__((always_inline)) inline
    #define RESTRICT __restrict__
#else
    #define VECTOR_ALIGN
    #define FORCE_INLINE inline
    #define RESTRICT
#endif

// ============================================================================
// VECTOR TYPES
// ============================================================================

// 2D Vector using GCC vector extensions
typedef float vec2_t __attribute__((vector_size(8)));

// 3D Vector using GCC vector extensions
typedef float vec3_t __attribute__((vector_size(16)));

// 4D Vector using GCC vector extensions  
typedef float vec4_t __attribute__((vector_size(16)));

// 3x3 Matrix (stored as 3 vec3_t for optimal alignment)
typedef struct {
    vec3_t x, y, z;
} mat3_t VECTOR_ALIGN;

// 4x4 Matrix (stored as 4 vec4_t for optimal alignment)
typedef struct {
    vec4_t x, y, z, w;
} mat4_t VECTOR_ALIGN;

// ============================================================================
// VECTOR OPERATIONS
// ============================================================================

// Vector creation
FORCE_INLINE vec2_t vec2(float x, float y) {
    vec2_t v = {x, y};
    return v;
}

FORCE_INLINE vec3_t vec3(float x, float y, float z) {
    vec3_t v = {x, y, z, 0.0f};
    return v;
}

FORCE_INLINE vec4_t vec4(float x, float y, float z, float w) {
    vec4_t v = {x, y, z, w};
    return v;
}

// Vector constants
FORCE_INLINE vec2_t vec2_zero(void) { return (vec2_t){0.0f, 0.0f}; }
FORCE_INLINE vec2_t vec2_one(void) { return (vec2_t){1.0f, 1.0f}; }
FORCE_INLINE vec2_t vec2_unit_x(void) { return (vec2_t){1.0f, 0.0f}; }
FORCE_INLINE vec2_t vec2_unit_y(void) { return (vec2_t){0.0f, 1.0f}; }

FORCE_INLINE vec3_t vec3_zero(void) { return (vec3_t){0.0f, 0.0f, 0.0f, 0.0f}; }
FORCE_INLINE vec3_t vec3_one(void) { return (vec3_t){1.0f, 1.0f, 1.0f, 0.0f}; }
FORCE_INLINE vec3_t vec3_unit_x(void) { return (vec3_t){1.0f, 0.0f, 0.0f, 0.0f}; }
FORCE_INLINE vec3_t vec3_unit_y(void) { return (vec3_t){0.0f, 1.0f, 0.0f, 0.0f}; }
FORCE_INLINE vec3_t vec3_unit_z(void) { return (vec3_t){0.0f, 0.0f, 1.0f, 0.0f}; }

FORCE_INLINE vec4_t vec4_zero(void) { return (vec4_t){0.0f, 0.0f, 0.0f, 0.0f}; }
FORCE_INLINE vec4_t vec4_one(void) { return (vec4_t){1.0f, 1.0f, 1.0f, 1.0f}; }
FORCE_INLINE vec4_t vec4_unit_x(void) { return (vec4_t){1.0f, 0.0f, 0.0f, 0.0f}; }
FORCE_INLINE vec4_t vec4_unit_y(void) { return (vec4_t){0.0f, 1.0f, 0.0f, 0.0f}; }
FORCE_INLINE vec4_t vec4_unit_z(void) { return (vec4_t){0.0f, 0.0f, 1.0f, 0.0f}; }
FORCE_INLINE vec4_t vec4_unit_w(void) { return (vec4_t){0.0f, 0.0f, 0.0f, 1.0f}; }

// Vector arithmetic operations
FORCE_INLINE vec2_t vec2_add(vec2_t a, vec2_t b) { return a + b; }
FORCE_INLINE vec2_t vec2_sub(vec2_t a, vec2_t b) { return a - b; }
FORCE_INLINE vec2_t vec2_mul(vec2_t a, vec2_t b) { return a * b; }
FORCE_INLINE vec2_t vec2_div(vec2_t a, vec2_t b) { return a / b; }
FORCE_INLINE vec2_t vec2_scale(vec2_t a, float s) { return a * (vec2_t){s, s}; }
FORCE_INLINE vec2_t vec2_neg(vec2_t a) { return -a; }

FORCE_INLINE vec3_t vec3_add(vec3_t a, vec3_t b) { return a + b; }
FORCE_INLINE vec3_t vec3_sub(vec3_t a, vec3_t b) { return a - b; }
FORCE_INLINE vec3_t vec3_mul(vec3_t a, vec3_t b) { return a * b; }
FORCE_INLINE vec3_t vec3_div(vec3_t a, vec3_t b) { return a / b; }
FORCE_INLINE vec3_t vec3_scale(vec3_t a, float s) { return a * (vec3_t){s, s, s, 0.0f}; }
FORCE_INLINE vec3_t vec3_neg(vec3_t a) { return -a; }

FORCE_INLINE vec4_t vec4_add(vec4_t a, vec4_t b) { return a + b; }
FORCE_INLINE vec4_t vec4_sub(vec4_t a, vec4_t b) { return a - b; }
FORCE_INLINE vec4_t vec4_mul(vec4_t a, vec4_t b) { return a * b; }
FORCE_INLINE vec4_t vec4_div(vec4_t a, vec4_t b) { return a / b; }
FORCE_INLINE vec4_t vec4_scale(vec4_t a, float s) { return a * (vec4_t){s, s, s, s}; }
FORCE_INLINE vec4_t vec4_neg(vec4_t a) { return -a; }

// Vector dot product (horizontal reduction)
FORCE_INLINE float vec2_dot(vec2_t a, vec2_t b) {
    vec2_t prod = a * b;
    return prod[0] + prod[1];
}

FORCE_INLINE float vec3_dot(vec3_t a, vec3_t b) {
    vec3_t prod = a * b;
    return prod[0] + prod[1] + prod[2];
}

FORCE_INLINE float vec4_dot(vec4_t a, vec4_t b) {
    vec4_t prod = a * b;
    return prod[0] + prod[1] + prod[2] + prod[3];
}

// Vector cross product (3D only)
FORCE_INLINE vec3_t vec3_cross(vec3_t a, vec3_t b) {
    return (vec3_t){
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
        0.0f
    };
}

// Vector length and normalization
FORCE_INLINE float vec2_length(vec2_t a) {
    return sqrtf(vec2_dot(a, a));
}

FORCE_INLINE float vec3_length(vec3_t a) {
    return sqrtf(vec3_dot(a, a));
}

FORCE_INLINE float vec4_length(vec4_t a) {
    return sqrtf(vec4_dot(a, a));
}

FORCE_INLINE vec2_t vec2_normalize(vec2_t a) {
    float len = vec2_length(a);
    return len > 0.0f ? a / (vec2_t){len, len} : vec2_zero();
}

FORCE_INLINE vec3_t vec3_normalize(vec3_t a) {
    float len = vec3_length(a);
    return len > 0.0f ? a / (vec3_t){len, len, len, 0.0f} : vec3_zero();
}

FORCE_INLINE vec4_t vec4_normalize(vec4_t a) {
    float len = vec4_length(a);
    return len > 0.0f ? a / (vec4_t){len, len, len, len} : vec4_zero();
}

// Vector distance
FORCE_INLINE float vec2_distance(vec2_t a, vec2_t b) {
    return vec2_length(vec2_sub(a, b));
}

FORCE_INLINE float vec3_distance(vec3_t a, vec3_t b) {
    return vec3_length(vec3_sub(a, b));
}

FORCE_INLINE float vec4_distance(vec4_t a, vec4_t b) {
    return vec4_length(vec4_sub(a, b));
}

// Vector comparison
FORCE_INLINE int vec2_equal(vec2_t a, vec2_t b) {
    vec2_t diff = a - b;
    float epsilon = 1e-6f;
    return (fabsf(diff[0]) < epsilon) && 
           (fabsf(diff[1]) < epsilon);
}

FORCE_INLINE int vec3_equal(vec3_t a, vec3_t b) {
    vec3_t diff = a - b;
    float epsilon = 1e-6f;
    return (fabsf(diff[0]) < epsilon) && 
           (fabsf(diff[1]) < epsilon) && 
           (fabsf(diff[2]) < epsilon);
}

FORCE_INLINE int vec4_equal(vec4_t a, vec4_t b) {
    vec4_t diff = a - b;
    float epsilon = 1e-6f;
    return (fabsf(diff[0]) < epsilon) && 
           (fabsf(diff[1]) < epsilon) && 
           (fabsf(diff[2]) < epsilon) && 
           (fabsf(diff[3]) < epsilon);
}

// ============================================================================
// MATRIX OPERATIONS
// ============================================================================

// Matrix creation
FORCE_INLINE mat3_t mat3_identity(void) {
    mat3_t m;
    m.x = vec3_unit_x();
    m.y = vec3_unit_y();
    m.z = vec3_unit_z();
    return m;
}

FORCE_INLINE mat4_t mat4_identity(void) {
    mat4_t m;
    m.x = vec4_unit_x();
    m.y = vec4_unit_y();
    m.z = vec4_unit_z();
    m.w = vec4_unit_w();
    return m;
}

FORCE_INLINE mat3_t mat3_from_vec3(vec3_t x, vec3_t y, vec3_t z) {
    mat3_t m;
    m.x = x;
    m.y = y;
    m.z = z;
    return m;
}

FORCE_INLINE mat4_t mat4_from_vec4(vec4_t x, vec4_t y, vec4_t z, vec4_t w) {
    mat4_t m;
    m.x = x;
    m.y = y;
    m.z = z;
    m.w = w;
    return m;
}

// Matrix-vector multiplication
FORCE_INLINE vec3_t mat3_mul_vec3(mat3_t m, vec3_t v) {
    return vec3(
        vec3_dot(m.x, v),
        vec3_dot(m.y, v),
        vec3_dot(m.z, v)
    );
}

FORCE_INLINE vec4_t mat4_mul_vec4(mat4_t m, vec4_t v) {
    return vec4(
        vec4_dot(m.x, v),
        vec4_dot(m.y, v),
        vec4_dot(m.z, v),
        vec4_dot(m.w, v)
    );
}

// Matrix-matrix multiplication
FORCE_INLINE mat3_t mat3_mul_mat3(mat3_t a, mat3_t b) {
    mat3_t result;
    result.x = vec3(
        vec3_dot(a.x, (vec3_t){b.x[0], b.y[0], b.z[0], 0.0f}),
        vec3_dot(a.x, (vec3_t){b.x[1], b.y[1], b.z[1], 0.0f}),
        vec3_dot(a.x, (vec3_t){b.x[2], b.y[2], b.z[2], 0.0f})
    );
    result.y = vec3(
        vec3_dot(a.y, (vec3_t){b.x[0], b.y[0], b.z[0], 0.0f}),
        vec3_dot(a.y, (vec3_t){b.x[1], b.y[1], b.z[1], 0.0f}),
        vec3_dot(a.y, (vec3_t){b.x[2], b.y[2], b.z[2], 0.0f})
    );
    result.z = vec3(
        vec3_dot(a.z, (vec3_t){b.x[0], b.y[0], b.z[0], 0.0f}),
        vec3_dot(a.z, (vec3_t){b.x[1], b.y[1], b.z[1], 0.0f}),
        vec3_dot(a.z, (vec3_t){b.x[2], b.y[2], b.z[2], 0.0f})
    );
    return result;
}

FORCE_INLINE mat4_t mat4_mul_mat4(mat4_t a, mat4_t b) {
    mat4_t result;
    result.x = vec4(
        vec4_dot(a.x, (vec4_t){b.x[0], b.y[0], b.z[0], b.w[0]}),
        vec4_dot(a.x, (vec4_t){b.x[1], b.y[1], b.z[1], b.w[1]}),
        vec4_dot(a.x, (vec4_t){b.x[2], b.y[2], b.z[2], b.w[2]}),
        vec4_dot(a.x, (vec4_t){b.x[3], b.y[3], b.z[3], b.w[3]})
    );
    result.y = vec4(
        vec4_dot(a.y, (vec4_t){b.x[0], b.y[0], b.z[0], b.w[0]}),
        vec4_dot(a.y, (vec4_t){b.x[1], b.y[1], b.z[1], b.w[1]}),
        vec4_dot(a.y, (vec4_t){b.x[2], b.y[2], b.z[2], b.w[2]}),
        vec4_dot(a.y, (vec4_t){b.x[3], b.y[3], b.z[3], b.w[3]})
    );
    result.z = vec4(
        vec4_dot(a.z, (vec4_t){b.x[0], b.y[0], b.z[0], b.w[0]}),
        vec4_dot(a.z, (vec4_t){b.x[1], b.y[1], b.z[1], b.w[1]}),
        vec4_dot(a.z, (vec4_t){b.x[2], b.y[2], b.z[2], b.w[2]}),
        vec4_dot(a.z, (vec4_t){b.x[3], b.y[3], b.z[3], b.w[3]})
    );
    result.w = vec4(
        vec4_dot(a.w, (vec4_t){b.x[0], b.y[0], b.z[0], b.w[0]}),
        vec4_dot(a.w, (vec4_t){b.x[1], b.y[1], b.z[1], b.w[1]}),
        vec4_dot(a.w, (vec4_t){b.x[2], b.y[2], b.z[2], b.w[2]}),
        vec4_dot(a.w, (vec4_t){b.x[3], b.y[3], b.z[3], b.w[3]})
    );
    return result;
}

// Matrix transpose
FORCE_INLINE mat3_t mat3_transpose(mat3_t m) {
    mat3_t result;
    result.x = (vec3_t){m.x[0], m.y[0], m.z[0], 0.0f};
    result.y = (vec3_t){m.x[1], m.y[1], m.z[1], 0.0f};
    result.z = (vec3_t){m.x[2], m.y[2], m.z[2], 0.0f};
    return result;
}

FORCE_INLINE mat4_t mat4_transpose(mat4_t m) {
    mat4_t result;
    result.x = (vec4_t){m.x[0], m.y[0], m.z[0], m.w[0]};
    result.y = (vec4_t){m.x[1], m.y[1], m.z[1], m.w[1]};
    result.z = (vec4_t){m.x[2], m.y[2], m.z[2], m.w[2]};
    result.w = (vec4_t){m.x[3], m.y[3], m.z[3], m.w[3]};
    return result;
}

// Matrix determinant (3x3)
FORCE_INLINE float mat3_determinant(mat3_t m) {
    return m.x[0] * (m.y[1] * m.z[2] - m.y[2] * m.z[1]) -
           m.x[1] * (m.y[0] * m.z[2] - m.y[2] * m.z[0]) +
           m.x[2] * (m.y[0] * m.z[1] - m.y[1] * m.z[0]);
}

// Matrix inverse (3x3)
FORCE_INLINE mat3_t mat3_inverse(mat3_t m) {
    float det = mat3_determinant(m);
    if (fabsf(det) < 1e-6f) {
        return mat3_identity(); // Return identity if not invertible
    }
    
    float inv_det = 1.0f / det;
    mat3_t result;
    
    result.x = vec3(
        (m.y[1] * m.z[2] - m.y[2] * m.z[1]) * inv_det,
        (m.x[2] * m.z[1] - m.x[1] * m.z[2]) * inv_det,
        (m.x[1] * m.y[2] - m.x[2] * m.y[1]) * inv_det
    );
    result.y = vec3(
        (m.y[2] * m.z[0] - m.y[0] * m.z[2]) * inv_det,
        (m.x[0] * m.z[2] - m.x[2] * m.z[0]) * inv_det,
        (m.x[2] * m.y[0] - m.x[0] * m.y[2]) * inv_det
    );
    result.z = vec3(
        (m.y[0] * m.z[1] - m.y[1] * m.z[0]) * inv_det,
        (m.x[1] * m.z[0] - m.x[0] * m.z[1]) * inv_det,
        (m.x[0] * m.y[1] - m.x[1] * m.y[0]) * inv_det
    );
    
    return result;
}

// Matrix inverse (4x4)
FORCE_INLINE mat4_t mat4_inverse(mat4_t m) {
    // For now, return identity - this is a complex operation
    // In a real implementation, you would implement proper 4x4 matrix inversion
    return mat4_identity();
}

// ============================================================================
// TRANSFORMATION MATRICES
// ============================================================================

// Translation matrix (4x4)
FORCE_INLINE mat4_t mat4_translation(vec3_t translation) {
    mat4_t m = mat4_identity();
    m.w = vec4(translation[0], translation[1], translation[2], 1.0f);
    return m;
}

// Rotation matrices (4x4)
FORCE_INLINE mat4_t mat4_rotation_x(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    mat4_t m = mat4_identity();
    m.y = vec4(0.0f, c, -s, 0.0f);
    m.z = vec4(0.0f, s, c, 0.0f);
    return m;
}

FORCE_INLINE mat4_t mat4_rotation_y(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    mat4_t m = mat4_identity();
    m.x = vec4(c, 0.0f, s, 0.0f);
    m.z = vec4(-s, 0.0f, c, 0.0f);
    return m;
}

FORCE_INLINE mat4_t mat4_rotation_z(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    mat4_t m = mat4_identity();
    m.x = vec4(c, -s, 0.0f, 0.0f);
    m.y = vec4(s, c, 0.0f, 0.0f);
    return m;
}

// Scale matrix (4x4)
FORCE_INLINE mat4_t mat4_scale(vec3_t scale) {
    mat4_t m = mat4_identity();
    m.x = vec4(scale[0], 0.0f, 0.0f, 0.0f);
    m.y = vec4(0.0f, scale[1], 0.0f, 0.0f);
    m.z = vec4(0.0f, 0.0f, scale[2], 0.0f);
    return m;
}

// Look-at matrix (4x4)
FORCE_INLINE mat4_t mat4_look_at(vec3_t eye, vec3_t target, vec3_t up) {
    vec3_t z = vec3_normalize(vec3_sub(eye, target));
    vec3_t x = vec3_normalize(vec3_cross(up, z));
    vec3_t y = vec3_cross(z, x);
    
    mat4_t m;
    m.x = vec4(x[0], y[0], z[0], 0.0f);
    m.y = vec4(x[1], y[1], z[1], 0.0f);
    m.z = vec4(x[2], y[2], z[2], 0.0f);
    m.w = vec4(-vec3_dot(x, eye), -vec3_dot(y, eye), -vec3_dot(z, eye), 1.0f);
    
    return m;
}

// Perspective projection matrix (4x4)
FORCE_INLINE mat4_t mat4_perspective(float fov_y, float aspect, float near, float far) {
    float f = 1.0f / tanf(fov_y * 0.5f);
    float range = near - far;
    
    mat4_t m;
    m.x = vec4(f / aspect, 0.0f, 0.0f, 0.0f);
    m.y = vec4(0.0f, f, 0.0f, 0.0f);
    m.z = vec4(0.0f, 0.0f, (near + far) / range, -1.0f);
    m.w = vec4(0.0f, 0.0f, (2.0f * near * far) / range, 0.0f);
    
    return m;
}

// Orthographic projection matrix (4x4)
FORCE_INLINE mat4_t mat4_ortho(float left, float right, float bottom, float top, float near, float far) {
    float rl = right - left;
    float tb = top - bottom;
    float fn = far - near;
    
    mat4_t m;
    m.x = vec4(2.0f / rl, 0.0f, 0.0f, 0.0f);
    m.y = vec4(0.0f, 2.0f / tb, 0.0f, 0.0f);
    m.z = vec4(0.0f, 0.0f, -2.0f / fn, 0.0f);
    m.w = vec4(-(right + left) / rl, -(top + bottom) / tb, -(far + near) / fn, 1.0f);
    
    return m;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Vector to array conversion
FORCE_INLINE void vec2_to_array(vec2_t v, float* RESTRICT arr) {
    arr[0] = v[0];
    arr[1] = v[1];
}

FORCE_INLINE void vec3_to_array(vec3_t v, float* RESTRICT arr) {
    arr[0] = v[0];
    arr[1] = v[1];
    arr[2] = v[2];
}

FORCE_INLINE void vec4_to_array(vec4_t v, float* RESTRICT arr) {
    arr[0] = v[0];
    arr[1] = v[1];
    arr[2] = v[2];
    arr[3] = v[3];
}

// Array to vector conversion
FORCE_INLINE vec2_t array_to_vec2(const float* RESTRICT arr) {
    return vec2(arr[0], arr[1]);
}

FORCE_INLINE vec3_t array_to_vec3(const float* RESTRICT arr) {
    return vec3(arr[0], arr[1], arr[2]);
}

FORCE_INLINE vec4_t array_to_vec4(const float* RESTRICT arr) {
    return vec4(arr[0], arr[1], arr[2], arr[3]);
}

// Matrix to array conversion (column-major for OpenGL/Metal compatibility)
FORCE_INLINE void mat3_to_array(mat3_t m, float* RESTRICT arr) {
    arr[0] = m.x[0]; arr[3] = m.x[1]; arr[6] = m.x[2];
    arr[1] = m.y[0]; arr[4] = m.y[1]; arr[7] = m.y[2];
    arr[2] = m.z[0]; arr[5] = m.z[1]; arr[8] = m.z[2];
}

FORCE_INLINE void mat4_to_array(mat4_t m, float* RESTRICT arr) {
    arr[0] = m.x[0]; arr[4] = m.x[1]; arr[8] = m.x[2]; arr[12] = m.x[3];
    arr[1] = m.y[0]; arr[5] = m.y[1]; arr[9] = m.y[2]; arr[13] = m.y[3];
    arr[2] = m.z[0]; arr[6] = m.z[1]; arr[10] = m.z[2]; arr[14] = m.z[3];
    arr[3] = m.w[0]; arr[7] = m.w[1]; arr[11] = m.w[2]; arr[15] = m.w[3];
}

// Array to matrix conversion (column-major for OpenGL/Metal compatibility)
FORCE_INLINE mat3_t array_to_mat3(const float* RESTRICT arr) {
    mat3_t m;
    m.x = vec3(arr[0], arr[3], arr[6]);
    m.y = vec3(arr[1], arr[4], arr[7]);
    m.z = vec3(arr[2], arr[5], arr[8]);
    return m;
}

FORCE_INLINE mat4_t array_to_mat4(const float* RESTRICT arr) {
    mat4_t m;
    m.x = vec4(arr[0], arr[4], arr[8], arr[12]);
    m.y = vec4(arr[1], arr[5], arr[9], arr[13]);
    m.z = vec4(arr[2], arr[6], arr[10], arr[14]);
    m.w = vec4(arr[3], arr[7], arr[11], arr[15]);
    return m;
}

// ============================================================================
// DEBUG/PRINTING FUNCTIONS
// ============================================================================

// Print vector to stdout (for debugging)
void vec2_print(const char* name, vec2_t v);
void vec3_print(const char* name, vec3_t v);
void vec4_print(const char* name, vec4_t v);

// Print matrix to stdout (for debugging)
void mat3_print(const char* name, mat3_t m);
void mat4_print(const char* name, mat4_t m);

#ifdef __cplusplus
}
#endif

#endif // ENGINE_MATH_H
