#include "engine_math.h"
#include <stdio.h>
#include <math.h>

// Test function declarations
void test_vectors(void);
void test_matrices(void);
void test_transformations(void);
void test_performance(void);

int main(void) {
    printf("=== Engine Math Library Test ===\n\n");
    
    test_vectors();
    printf("\n");
    
    test_matrices();
    printf("\n");
    
    test_transformations();
    printf("\n");
    
    test_performance();
    printf("\n");
    
    printf("=== All Tests Completed ===\n");
    return 0;
}

void test_vectors(void) {
    printf("--- Vector Tests ---\n");
    
    // Test vector creation
    vec3_t v1 = vec3(1.0f, 2.0f, 3.0f);
    vec3_t v2 = vec3(4.0f, 5.0f, 6.0f);
    
    printf("Vector creation:\n");
    vec3_print("v1", v1);
    vec3_print("v2", v2);
    
    // Test vector arithmetic
    vec3_t sum = vec3_add(v1, v2);
    vec3_t diff = vec3_sub(v2, v1);
    vec3_t scaled = vec3_scale(v1, 2.0f);
    
    printf("\nVector arithmetic:\n");
    vec3_print("v1 + v2", sum);
    vec3_print("v2 - v1", diff);
    vec3_print("v1 * 2", scaled);
    
    // Test vector operations
    float dot = vec3_dot(v1, v2);
    vec3_t cross = vec3_cross(v1, v2);
    float len = vec3_length(v1);
    vec3_t normalized = vec3_normalize(v1);
    
    printf("\nVector operations:\n");
    printf("v1 · v2 = %.6f\n", dot);
    vec3_print("v1 × v2", cross);
    printf("|v1| = %.6f\n", len);
    vec3_print("v1 normalized", normalized);
    
    // Test 4D vectors
    vec4_t v4_1 = vec4(1.0f, 2.0f, 3.0f, 4.0f);
    vec4_t v4_2 = vec4(5.0f, 6.0f, 7.0f, 8.0f);
    
    printf("\n4D Vector tests:\n");
    vec4_print("v4_1", v4_1);
    vec4_print("v4_2", v4_2);
    printf("v4_1 · v4_2 = %.6f\n", vec4_dot(v4_1, v4_2));
}

void test_matrices(void) {
    printf("--- Matrix Tests ---\n");
    
    // Test matrix creation
    mat3_t m3 = mat3_identity();
    mat4_t m4 = mat4_identity();
    
    printf("Identity matrices:\n");
    mat3_print("3x3 Identity", m3);
    mat4_print("4x4 Identity", m4);
    
    // Test 3x3 matrix operations
    vec3_t x_axis = vec3_unit_x();
    vec3_t y_axis = vec3_unit_y();
    vec3_t z_axis = vec3_unit_z();
    
    mat3_t m3_custom = mat3_from_vec3(x_axis, y_axis, z_axis);
    printf("\nCustom 3x3 matrix:\n");
    mat3_print("Custom 3x3", m3_custom);
    
    // Test matrix-vector multiplication
    vec3_t test_vec = vec3(1.0f, 2.0f, 3.0f);
    vec3_t result = mat3_mul_vec3(m3_custom, test_vec);
    
    printf("\nMatrix-vector multiplication:\n");
    vec3_print("Test vector", test_vec);
    vec3_print("Result", result);
    
    // Test matrix-matrix multiplication
    mat3_t m3_rot = mat3_from_vec3(
        vec3(cosf(0.5f), -sinf(0.5f), 0.0f),
        vec3(sinf(0.5f), cosf(0.5f), 0.0f),
        vec3(0.0f, 0.0f, 1.0f)
    );
    
    mat3_t m3_product = mat3_mul_mat3(m3_custom, m3_rot);
    printf("\nMatrix multiplication (3x3):\n");
    mat3_print("Rotation matrix", m3_rot);
    mat3_print("Product", m3_product);
    
    // Test matrix transpose and inverse
    mat3_t m3_transposed = mat3_transpose(m3_rot);
    mat3_t m3_inverse = mat3_inverse(m3_rot);
    
    printf("\nMatrix operations:\n");
    mat3_print("Transposed", m3_transposed);
    mat3_print("Inverse", m3_inverse);
    
    // Verify inverse
    mat3_t m3_verify = mat3_mul_mat3(m3_rot, m3_inverse);
    printf("\nVerification (should be identity):\n");
    mat3_print("m3 * m3_inverse", m3_verify);
}

void test_transformations(void) {
    printf("--- Transformation Tests ---\n");
    
    // Test translation matrix
    vec3_t translation = vec3(10.0f, 20.0f, 30.0f);
    mat4_t trans_matrix = mat4_translation(translation);
    
    printf("Translation matrix:\n");
    mat4_print("Translation", trans_matrix);
    
    // Test rotation matrices
    float angle = M_PI / 4.0f; // 45 degrees
    mat4_t rot_x = mat4_rotation_x(angle);
    mat4_t rot_y = mat4_rotation_y(angle);
    mat4_t rot_z = mat4_rotation_z(angle);
    
    printf("\nRotation matrices (45°):\n");
    mat4_print("Rotation X", rot_x);
    mat4_print("Rotation Y", rot_y);
    mat4_print("Rotation Z", rot_z);
    
    // Test scale matrix
    vec3_t scale = vec3(2.0f, 3.0f, 4.0f);
    mat4_t scale_matrix = mat4_scale(scale);
    
    printf("\nScale matrix:\n");
    mat4_print("Scale", scale_matrix);
    
    // Test look-at matrix
    vec3_t eye = vec3(0.0f, 0.0f, 5.0f);
    vec3_t target = vec3(0.0f, 0.0f, 0.0f);
    vec3_t up = vec3(0.0f, 1.0f, 0.0f);
    mat4_t look_at_matrix = mat4_look_at(eye, target, up);
    
    printf("\nLook-at matrix:\n");
    mat4_print("Look-at", look_at_matrix);
    
    // Test perspective projection
    float fov = M_PI / 3.0f; // 60 degrees
    float aspect = 16.0f / 9.0f; // 16:9 aspect ratio
    float near = 0.1f;
    float far = 100.0f;
    mat4_t persp_matrix = mat4_perspective(fov, aspect, near, far);
    
    printf("\nPerspective projection:\n");
    mat4_print("Perspective", persp_matrix);
    
    // Test orthographic projection
    mat4_t ortho_matrix = mat4_ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    
    printf("\nOrthographic projection:\n");
    mat4_print("Orthographic", ortho_matrix);
}

void test_performance(void) {
    printf("--- Performance Tests ---\n");
    
    const int iterations = 1000000;
    
    // Test vector operations performance
    vec3_t v1 = vec3(1.0f, 2.0f, 3.0f);
    vec3_t v2 = vec3(4.0f, 5.0f, 6.0f);
    vec3_t result = vec3_zero();
    
    printf("Testing %d vector operations...\n", iterations);
    
    // Vector addition performance
    for (int i = 0; i < iterations; i++) {
        result = vec3_add(v1, v2);
    }
    
    printf("Vector addition: %d operations completed\n", iterations);
    vec3_print("Final result", result);
    
    // Matrix multiplication performance
    mat4_t m1 = mat4_identity();
    mat4_t m2 = mat4_identity();
    mat4_t m_result = mat4_identity();
    
    printf("\nTesting %d matrix multiplications...\n", iterations);
    
    for (int i = 0; i < iterations; i++) {
        m_result = mat4_mul_mat4(m1, m2);
    }
    
    printf("Matrix multiplication: %d operations completed\n", iterations);
    
    // Test transformation chain
    vec3_t pos = vec3(1.0f, 2.0f, 3.0f);
    vec4_t pos4 = vec4(pos[0], pos[1], pos[2], 1.0f);
    
    printf("\nTesting transformation chain...\n");
    
    mat4_t transform = mat4_identity();
    transform = mat4_mul_mat4(transform, mat4_translation(vec3(10.0f, 0.0f, 0.0f)));
    transform = mat4_mul_mat4(transform, mat4_rotation_y(M_PI / 4.0f));
    transform = mat4_mul_mat4(transform, mat4_scale(vec3(2.0f, 2.0f, 2.0f)));
    
    vec4_t transformed = mat4_mul_vec4(transform, pos4);
    
    printf("Original position: ");
    vec3_print("", pos);
    printf("Transformed position: ");
    vec4_print("", transformed);
}
