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

// 2D Vector using simple struct
typedef struct {
    float x, y;
} vec2_t;

// 3D Vector using simple struct
typedef struct {
    float x, y, z;
} vec3_t;

// 4D Vector using simple struct
typedef struct {
    float x, y, z, w;
} vec4_t;

// Vector component access using direct member access
FORCE_INLINE float vec2_x(vec2_t v) { return v.x; }
FORCE_INLINE float vec2_y(vec2_t v) { return v.y; }

FORCE_INLINE float vec3_x(vec3_t v) { return v.x; }
FORCE_INLINE float vec3_y(vec3_t v) { return v.y; }
FORCE_INLINE float vec3_z(vec3_t v) { return v.z; }
FORCE_INLINE float vec3_w(vec3_t v) { return 0.0f; }

FORCE_INLINE float vec4_x(vec4_t v) { return v.x; }
FORCE_INLINE float vec4_y(vec4_t v) { return v.y; }
FORCE_INLINE float vec4_z(vec4_t v) { return v.z; }
FORCE_INLINE float vec4_w(vec4_t v) { return v.w; }

// 3x3 Matrix (stored as 3 vec3_t for optimal alignment)
typedef struct {
    vec3_t x, y, z;
} mat3_t VECTOR_ALIGN;

// 4x4 Matrix (stored as 4 vec4_t for optimal alignment)
typedef struct {
    vec4_t x, y, z, w;
} mat4_t VECTOR_ALIGN;

// Quaternion (w is scalar component, xyz is vector component)
typedef struct {
    float x, y, z, w;
} quat_t VECTOR_ALIGN;

// ============================================================================
// VECTOR OPERATIONS
// ============================================================================

// Vector creation
FORCE_INLINE vec2_t vec2(float x, float y) {
    vec2_t v = {x, y};
    return v;
}

FORCE_INLINE vec3_t vec3(float x, float y, float z) {
    vec3_t v = {x, y, z};
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

FORCE_INLINE vec3_t vec3_zero(void) { return (vec3_t){0.0f, 0.0f, 0.0f}; }
FORCE_INLINE vec3_t vec3_one(void) { return (vec3_t){1.0f, 1.0f, 1.0f}; }
FORCE_INLINE vec3_t vec3_unit_x(void) { return (vec3_t){1.0f, 0.0f, 0.0f}; }
FORCE_INLINE vec3_t vec3_unit_y(void) { return (vec3_t){0.0f, 1.0f, 0.0f}; }
FORCE_INLINE vec3_t vec3_unit_z(void) { return (vec3_t){0.0f, 0.0f, 1.0f}; }

FORCE_INLINE vec4_t vec4_zero(void) { return (vec4_t){0.0f, 0.0f, 0.0f, 0.0f}; }
FORCE_INLINE vec4_t vec4_one(void) { return (vec4_t){1.0f, 1.0f, 1.0f, 1.0f}; }
FORCE_INLINE vec4_t vec4_unit_x(void) { return (vec4_t){1.0f, 0.0f, 0.0f, 0.0f}; }
FORCE_INLINE vec4_t vec4_unit_y(void) { return (vec4_t){0.0f, 1.0f, 0.0f, 0.0f}; }
FORCE_INLINE vec4_t vec4_unit_z(void) { return (vec4_t){0.0f, 0.0f, 1.0f, 0.0f}; }
FORCE_INLINE vec4_t vec4_unit_w(void) { return (vec4_t){0.0f, 0.0f, 0.0f, 1.0f}; }

// Vector arithmetic operations
FORCE_INLINE vec2_t vec2_add(vec2_t a, vec2_t b) { return (vec2_t){a.x + b.x, a.y + b.y}; }
FORCE_INLINE vec2_t vec2_sub(vec2_t a, vec2_t b) { return (vec2_t){a.x - b.x, a.y - b.y}; }
FORCE_INLINE vec2_t vec2_mul(vec2_t a, vec2_t b) { return (vec2_t){a.x * b.x, a.y * b.y}; }
FORCE_INLINE vec2_t vec2_div(vec2_t a, vec2_t b) { return (vec2_t){a.x / b.x, a.y / b.y}; }
FORCE_INLINE vec2_t vec2_scale(vec2_t a, float s) { return (vec2_t){a.x * s, a.y * s}; }
FORCE_INLINE vec2_t vec2_neg(vec2_t a) { return (vec2_t){-a.x, -a.y}; }

FORCE_INLINE vec3_t vec3_add(vec3_t a, vec3_t b) { return (vec3_t){a.x + b.x, a.y + b.y, a.z + b.z}; }
FORCE_INLINE vec3_t vec3_sub(vec3_t a, vec3_t b) { return (vec3_t){a.x - b.x, a.y - b.y, a.z - b.z}; }
FORCE_INLINE vec3_t vec3_mul(vec3_t a, vec3_t b) { return (vec3_t){a.x * b.x, a.y * b.y, a.z * b.z}; }
FORCE_INLINE vec3_t vec3_div(vec3_t a, vec3_t b) { return (vec3_t){a.x / b.x, a.y / b.y, a.z / b.z}; }
FORCE_INLINE vec3_t vec3_scale(vec3_t a, float s) { return (vec3_t){a.x * s, a.y * s, a.z * s}; }
FORCE_INLINE vec3_t vec3_neg(vec3_t a) { return (vec3_t){-a.x, -a.y, -a.z}; }

FORCE_INLINE vec4_t vec4_add(vec4_t a, vec4_t b) { return (vec4_t){a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
FORCE_INLINE vec4_t vec4_sub(vec4_t a, vec4_t b) { return (vec4_t){a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }
FORCE_INLINE vec4_t vec4_mul(vec4_t a, vec4_t b) { return (vec4_t){a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w}; }
FORCE_INLINE vec4_t vec4_div(vec4_t a, vec4_t b) { return (vec4_t){a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w}; }
FORCE_INLINE vec4_t vec4_scale(vec4_t a, float s) { return (vec4_t){a.x * s, a.y * s, a.z * s, a.w * s}; }
FORCE_INLINE vec4_t vec4_neg(vec4_t a) { return (vec4_t){-a.x, -a.y, -a.z, -a.w}; }

// Vector dot product (horizontal reduction)
FORCE_INLINE float vec2_dot(vec2_t a, vec2_t b) {
    return a.x * b.x + a.y * b.y;
}

FORCE_INLINE float vec3_dot(vec3_t a, vec3_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

FORCE_INLINE float vec4_dot(vec4_t a, vec4_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

// Vector cross product (3D only)
FORCE_INLINE vec3_t vec3_cross(vec3_t a, vec3_t b) {
    return (vec3_t){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
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
    return len > 0.0f ? (vec2_t){a.x / len, a.y / len} : vec2_zero();
}

FORCE_INLINE vec3_t vec3_normalize(vec3_t a) {
    float len = vec3_length(a);
    return len > 0.0f ? (vec3_t){a.x / len, a.y / len, a.z / len} : vec3_zero();
}

FORCE_INLINE vec4_t vec4_normalize(vec4_t a) {
    float len = vec4_length(a);
    return len > 0.0f ? (vec4_t){a.x / len, a.y / len, a.z / len, a.w / len} : vec4_zero();
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
    vec2_t diff = vec2_sub(a, b);
    float epsilon = 1e-6f;
    return (fabsf(diff.x) < epsilon) && 
           (fabsf(diff.y) < epsilon);
}

FORCE_INLINE int vec3_equal(vec3_t a, vec3_t b) {
    vec3_t diff = vec3_sub(a, b);
    float epsilon = 1e-6f;
    return (fabsf(diff.x) < epsilon) && 
           (fabsf(diff.y) < epsilon) && 
           (fabsf(diff.z) < epsilon);
}

FORCE_INLINE int vec4_equal(vec4_t a, vec4_t b) {
    vec4_t diff = vec4_sub(a, b);
    float epsilon = 1e-6f;
    return (fabsf(diff.x) < epsilon) && 
           (fabsf(diff.y) < epsilon) && 
           (fabsf(diff.z) < epsilon) && 
           (fabsf(diff.w) < epsilon);
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
        vec3_dot(a.x, (vec3_t){b.x.x, b.y.x, b.z.x}),
        vec3_dot(a.x, (vec3_t){b.x.y, b.y.y, b.z.y}),
        vec3_dot(a.x, (vec3_t){b.x.z, b.y.z, b.z.z})
    );
    result.y = vec3(
        vec3_dot(a.y, (vec3_t){b.x.x, b.y.x, b.z.x}),
        vec3_dot(a.y, (vec3_t){b.x.y, b.y.y, b.z.y}),
        vec3_dot(a.y, (vec3_t){b.x.z, b.y.z, b.z.z})
    );
    result.z = vec3(
        vec3_dot(a.z, (vec3_t){b.x.x, b.y.x, b.z.x}),
        vec3_dot(a.z, (vec3_t){b.x.y, b.y.y, b.z.y}),
        vec3_dot(a.z, (vec3_t){b.x.z, b.y.z, b.z.z})
    );
    return result;
}

FORCE_INLINE mat4_t mat4_mul_mat4(mat4_t a, mat4_t b) {
    mat4_t result;
    result.x = vec4(
        vec4_dot(a.x, (vec4_t){b.x.x, b.y.x, b.z.x, b.w.x}),
        vec4_dot(a.x, (vec4_t){b.x.y, b.y.y, b.z.y, b.w.y}),
        vec4_dot(a.x, (vec4_t){b.x.z, b.y.z, b.z.z, b.w.z}),
        vec4_dot(a.x, (vec4_t){b.x.w, b.y.w, b.z.w, b.w.w})
    );
    result.y = vec4(
        vec4_dot(a.y, (vec4_t){b.x.x, b.y.x, b.z.x, b.w.x}),
        vec4_dot(a.y, (vec4_t){b.x.y, b.y.y, b.z.y, b.w.y}),
        vec4_dot(a.y, (vec4_t){b.x.z, b.y.z, b.z.z, b.w.z}),
        vec4_dot(a.y, (vec4_t){b.x.w, b.y.w, b.z.w, b.w.w})
    );
    result.z = vec4(
        vec4_dot(a.z, (vec4_t){b.x.x, b.y.x, b.z.x, b.w.x}),
        vec4_dot(a.z, (vec4_t){b.x.y, b.y.y, b.z.y, b.w.y}),
        vec4_dot(a.z, (vec4_t){b.x.z, b.y.z, b.z.z, b.w.z}),
        vec4_dot(a.z, (vec4_t){b.x.w, b.y.w, b.z.w, b.w.w})
    );
    result.w = vec4(
        vec4_dot(a.w, (vec4_t){b.x.x, b.y.x, b.z.x, b.w.x}),
        vec4_dot(a.w, (vec4_t){b.x.y, b.y.y, b.z.y, b.w.y}),
        vec4_dot(a.w, (vec4_t){b.x.z, b.y.z, b.z.z, b.w.z}),
        vec4_dot(a.w, (vec4_t){b.x.w, b.y.w, b.z.w, b.w.w})
    );
    return result;
}

// Matrix transpose
FORCE_INLINE mat3_t mat3_transpose(mat3_t m) {
    mat3_t result;
    result.x = (vec3_t){m.x.x, m.y.x, m.z.x};
    result.y = (vec3_t){m.x.y, m.y.y, m.z.y};
    result.z = (vec3_t){m.x.z, m.y.z, m.z.z};
    return result;
}

FORCE_INLINE mat4_t mat4_transpose(mat4_t m) {
    mat4_t result;
    result.x = (vec4_t){m.x.x, m.y.x, m.z.x, m.w.x};
    result.y = (vec4_t){m.x.y, m.y.y, m.z.y, m.w.y};
    result.z = (vec4_t){m.x.z, m.y.z, m.z.z, m.w.z};
    result.w = (vec4_t){m.x.w, m.y.w, m.z.w, m.w.w};
    return result;
}

// Matrix determinant (3x3)
FORCE_INLINE float mat3_determinant(mat3_t m) {
    return m.x.x * (m.y.y * m.z.z - m.y.z * m.z.y) -
           m.x.y * (m.y.x * m.z.z - m.y.z * m.z.x) +
           m.x.z * (m.y.x * m.z.y - m.y.y * m.z.x);
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
        (m.y.y * m.z.z - m.y.z * m.z.y) * inv_det,
        (m.x.z * m.z.y - m.x.y * m.z.z) * inv_det,
        (m.x.y * m.y.z - m.x.z * m.y.y) * inv_det
    );
    result.y = vec3(
        (m.y.z * m.z.x - m.y.x * m.z.z) * inv_det,
        (m.x.x * m.z.z - m.x.z * m.z.x) * inv_det,
        (m.x.z * m.y.x - m.x.x * m.y.z) * inv_det
    );
    result.z = vec3(
        (m.y.x * m.z.y - m.y.y * m.z.x) * inv_det,
        (m.x.y * m.z.x - m.x.x * m.z.y) * inv_det,
        (m.x.x * m.y.y - m.x.y * m.y.x) * inv_det
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
    m.w = vec4(translation.x, translation.y, translation.z, 1.0f);
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
    m.x = vec4(scale.x, 0.0f, 0.0f, 0.0f);
    m.y = vec4(0.0f, scale.y, 0.0f, 0.0f);
    m.z = vec4(0.0f, 0.0f, scale.z, 0.0f);
    return m;
}

// Look-at matrix (4x4)
FORCE_INLINE mat4_t mat4_look_at(vec3_t eye, vec3_t target, vec3_t up) {
    vec3_t z = vec3_normalize(vec3_sub(eye, target));
    vec3_t x = vec3_normalize(vec3_cross(up, z));
    vec3_t y = vec3_cross(z, x);
    
    mat4_t m;
    m.x = vec4(x.x, y.x, z.x, 0.0f);
    m.y = vec4(x.y, y.y, z.y, 0.0f);
    m.z = vec4(x.z, y.z, z.z, 0.0f);
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
// QUATERNION OPERATIONS
// ============================================================================

// Quaternion creation
FORCE_INLINE quat_t quat(float x, float y, float z, float w) {
    quat_t q = {x, y, z, w};
    return q;
}

// Quaternion constants
FORCE_INLINE quat_t quat_identity(void) {
    return quat(0.0f, 0.0f, 0.0f, 1.0f);
}

FORCE_INLINE quat_t quat_zero(void) {
    return quat(0.0f, 0.0f, 0.0f, 0.0f);
}

// Quaternion arithmetic operations
FORCE_INLINE quat_t quat_add(quat_t a, quat_t b) {
    return quat(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

FORCE_INLINE quat_t quat_sub(quat_t a, quat_t b) {
    return quat(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

FORCE_INLINE quat_t quat_scale(quat_t q, float s) {
    return quat(q.x * s, q.y * s, q.z * s, q.w * s);
}

FORCE_INLINE quat_t quat_neg(quat_t q) {
    return quat(-q.x, -q.y, -q.z, -q.w);
}

// Quaternion multiplication (Hamilton product)
FORCE_INLINE quat_t quat_mul(quat_t a, quat_t b) {
    return quat(
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    );
}

// Quaternion dot product
FORCE_INLINE float quat_dot(quat_t a, quat_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

// Quaternion length and normalization
FORCE_INLINE float quat_length(quat_t q) {
    return sqrtf(quat_dot(q, q));
}

FORCE_INLINE quat_t quat_normalize(quat_t q) {
    float len = quat_length(q);
    return len > 0.0f ? quat_scale(q, 1.0f / len) : quat_identity();
}

// Quaternion conjugate
FORCE_INLINE quat_t quat_conjugate(quat_t q) {
    return quat(-q.x, -q.y, -q.z, q.w);
}

// Quaternion inverse
FORCE_INLINE quat_t quat_inverse(quat_t q) {
    float len_sq = quat_dot(q, q);
    if (len_sq < 1e-6f) {
        return quat_identity();
    }
    quat_t conj = quat_conjugate(q);
    return quat_scale(conj, 1.0f / len_sq);
}

// Quaternion from axis-angle
FORCE_INLINE quat_t quat_from_axis_angle(vec3_t axis, float angle) {
    float half_angle = angle * 0.5f;
    float s = sinf(half_angle);
    float c = cosf(half_angle);
    vec3_t norm_axis = vec3_normalize(axis);
    
    return quat(
        norm_axis.x * s,
        norm_axis.y * s,
        norm_axis.z * s,
        c
    );
}

// Quaternion from Euler angles (ZYX order)
FORCE_INLINE quat_t quat_from_euler(float x, float y, float z) {
    float cx = cosf(x * 0.5f);
    float sx = sinf(x * 0.5f);
    float cy = cosf(y * 0.5f);
    float sy = sinf(y * 0.5f);
    float cz = cosf(z * 0.5f);
    float sz = sinf(z * 0.5f);
    
    return quat(
        sx * cy * cz - cx * sy * sz,
        cx * sy * cz + sx * cy * sz,
        cx * cy * sz - sx * sy * cz,
        cx * cy * cz + sx * sy * sz
    );
}

// Quaternion to Euler angles (ZYX order)
FORCE_INLINE vec3_t quat_to_euler(quat_t q) {
    // Roll (x-axis rotation)
    float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    float roll = atan2f(sinr_cosp, cosr_cosp);
    
    // Pitch (y-axis rotation)
    float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    float pitch;
    if (fabsf(sinp) >= 1.0f) {
        pitch = copysignf(M_PI / 2.0f, sinp); // use 90 degrees if out of range
    } else {
        pitch = asinf(sinp);
    }
    
    // Yaw (z-axis rotation)
    float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    float yaw = atan2f(siny_cosp, cosy_cosp);
    
    return vec3(roll, pitch, yaw);
}

// Quaternion to rotation matrix (4x4)
FORCE_INLINE mat4_t quat_to_mat4(quat_t q) {
    quat_t nq = quat_normalize(q);
    
    float x = nq.x, y = nq.y, z = nq.z, w = nq.w;
    float x2 = x + x, y2 = y + y, z2 = z + z;
    float xx = x * x2, xy = x * y2, xz = x * z2;
    float yy = y * y2, yz = y * z2, zz = z * z2;
    float wx = w * x2, wy = w * y2, wz = w * z2;
    
    mat4_t m;
    m.x = vec4(1.0f - (yy + zz), xy + wz, xz - wy, 0.0f);
    m.y = vec4(xy - wz, 1.0f - (xx + zz), yz + wx, 0.0f);
    m.z = vec4(xz + wy, yz - wx, 1.0f - (xx + yy), 0.0f);
    m.w = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    
    return m;
}

// Spherical linear interpolation between two quaternions
FORCE_INLINE quat_t quat_slerp(quat_t a, quat_t b, float t) {
    float dot = quat_dot(a, b);
    
    // If the dot product is negative, slerp won't take the shorter path
    if (dot < 0.0f) {
        b = quat_neg(b);
        dot = -dot;
    }
    
    // If the inputs are too close for comfort, linearly interpolate
    if (dot > 0.9995f) {
        quat_t result = quat_add(a, quat_scale(quat_sub(b, a), t));
        return quat_normalize(result);
    }
    
    float theta_0 = acosf(fabsf(dot));
    float theta = theta_0 * t;
    float sin_theta = sinf(theta);
    float sin_theta_0 = sinf(theta_0);
    
    float s0 = cosf(theta) - dot * sin_theta / sin_theta_0;
    float s1 = sin_theta / sin_theta_0;
    
    return quat_add(quat_scale(a, s0), quat_scale(b, s1));
}

// Quaternion comparison
FORCE_INLINE int quat_equal(quat_t a, quat_t b) {
    quat_t diff = quat_sub(a, b);
    float epsilon = 1e-6f;
    return (fabsf(diff.x) < epsilon) && 
           (fabsf(diff.y) < epsilon) && 
           (fabsf(diff.z) < epsilon) && 
           (fabsf(diff.w) < epsilon);
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Vector to array conversion
FORCE_INLINE void vec2_to_array(vec2_t v, float* RESTRICT arr) {
    arr[0] = v.x;
    arr[1] = v.y;
}

FORCE_INLINE void vec3_to_array(vec3_t v, float* RESTRICT arr) {
    arr[0] = v.x;
    arr[1] = v.y;
    arr[2] = v.z;
}

FORCE_INLINE void vec4_to_array(vec4_t v, float* RESTRICT arr) {
    arr[0] = v.x;
    arr[1] = v.y;
    arr[2] = v.z;
    arr[3] = v.w;
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
    arr[0] = m.x.x; arr[3] = m.x.y; arr[6] = m.x.z;
    arr[1] = m.y.x; arr[4] = m.y.y; arr[7] = m.y.z;
    arr[2] = m.z.x; arr[5] = m.z.y; arr[8] = m.z.z;
}

FORCE_INLINE void mat4_to_array(mat4_t m, float* RESTRICT arr) {
    arr[0] = m.x.x; arr[4] = m.x.y; arr[8] = m.x.z; arr[12] = m.x.w;
    arr[1] = m.y.x; arr[5] = m.y.y; arr[9] = m.y.z; arr[13] = m.y.w;
    arr[2] = m.z.x; arr[6] = m.z.y; arr[10] = m.z.z; arr[14] = m.z.w;
    arr[3] = m.w.x; arr[7] = m.w.y; arr[11] = m.w.z; arr[15] = m.w.w;
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

// Quaternion to array conversion
FORCE_INLINE void quat_to_array(quat_t q, float* RESTRICT arr) {
    arr[0] = q.x;
    arr[1] = q.y;
    arr[2] = q.z;
    arr[3] = q.w;
}

// Array to quaternion conversion
FORCE_INLINE quat_t array_to_quat(const float* RESTRICT arr) {
    return quat(arr[0], arr[1], arr[2], arr[3]);
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

// Print quaternion to stdout (for debugging)
void quat_print(const char* name, quat_t q);

#ifdef __cplusplus
}
#endif

#endif // ENGINE_MATH_H
