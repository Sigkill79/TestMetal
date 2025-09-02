#include "engine_asset_fbx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* assets_dir = "assets";

static void assert_true(int cond, const char* msg) {
    if (!cond) {
        fprintf(stderr, "Assertion failed: %s\n", msg);
        exit(1);
    }
}

int main(void) {
    printf("🧪 FBX Loader Test\n");
    printf("===================\n\n");

    char path[512];
    snprintf(path, sizeof(path), "%s/%s", assets_dir, "UnitBox.fbx");

    char* err = NULL;
    Model3D* model = fbx_load_model(path, &err);
    if (!model) {
        fprintf(stderr, "Failed to load FBX: %s\n", err ? err : "(no error)");
        fbx_free_error(err);
        return 1;
    }

    // Basic sanity checks
    assert_true(model->mesh_count == 1, "Model should have 1 mesh");
    assert_true(model->meshes[0].vertex_count > 0, "Mesh should have vertices");
    assert_true(model->meshes[0].index_count > 0, "Mesh should have indices");
    assert_true(model->meshes[0].triangle_count == model->meshes[0].index_count / 3, "Triangle count matches indices/3");

    // Bounds should be sensible for a unit box around [-0.5, 0.5]
    model3d_calculate_bounds(model);
    printf("Bounds min: [%.3f, %.3f, %.3f], max: [%.3f, %.3f, %.3f]\n",
           model->bounding_min.x, model->bounding_min.y, model->bounding_min.z,
           model->bounding_max.x, model->bounding_max.y, model->bounding_max.z);

    assert_true(model->bounding_min.x <= -0.5f + 1e-2f, "Min x near -0.5");
    assert_true(model->bounding_max.x >= 0.5f - 1e-2f, "Max x near 0.5");

    printf("✅ FBX load sanity passed!\n");
    model3d_free(model);
    return 0;
}


