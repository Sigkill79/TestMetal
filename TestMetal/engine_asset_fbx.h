#ifndef ENGINE_ASSET_FBX_H
#define ENGINE_ASSET_FBX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "engine_model.h"

// Load an FBX file (ASCII or Binary) into a Model3D.
// Returns a heap-allocated Model3D* on success; NULL on failure.
// On failure, if out_error is non-NULL, it will receive a heap-allocated error message that the caller must free.
Model3D* fbx_load_model(const char* filepath, char** out_error);

// Convenience: free error strings returned by fbx_load_model
void fbx_free_error(char* err);

#ifdef __cplusplus
}
#endif

#endif // ENGINE_ASSET_FBX_H


