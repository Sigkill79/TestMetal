#include "engine_asset_fbx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Minimal FBX ASCII parser tailored for meshes with positions, normals, uvs, indices
// Supports loading of the provided UnitBox.fbx-like ASCII files. Binary FBX returns error for now.

typedef struct {
    float* positions;      // triplets
    uint32_t positions_count; // count of floats
    float* normals;        // triplets
    uint32_t normals_count;
    float* uvs;            // pairs
    uint32_t uvs_count;
    int* poly_indices;     // FBX polygon vertex indices (with negative to mark polygon end)
    uint32_t poly_indices_count;
} FBXParseData;

static void fbx_parse_data_free(FBXParseData* d) {
    if (!d) return;
    if (d->positions) free(d->positions);
    if (d->normals) free(d->normals);
    if (d->uvs) free(d->uvs);
    if (d->poly_indices) free(d->poly_indices);
    memset(d, 0, sizeof(*d));
}

static char* str_dup(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char* out = (char*)malloc(n);
    if (out) memcpy(out, s, n);
    return out;
}

static int is_binary_fbx(FILE* f) {
    fprintf(stderr, "=== FBX BINARY CHECK START ===\n");
    unsigned char header[23] = {0};
    size_t r = fread(header, 1, sizeof(header), f);
    fseek(f, 0, SEEK_SET);
    fprintf(stderr, "Read %zu bytes from file header\n", r);
    
    if (r < 23) {
        fprintf(stderr, "File too small (%zu bytes), not binary FBX\n", r);
        return 0;
    }
    
    // Print first 23 bytes as hex
    fprintf(stderr, "First 23 bytes (hex): ");
    for (int i = 0; i < 23; i++) {
        fprintf(stderr, "%02x ", header[i]);
    }
    fprintf(stderr, "\n");
    
    // Print first 23 bytes as characters (if printable)
    fprintf(stderr, "First 23 bytes (chars): ");
    for (int i = 0; i < 23; i++) {
        if (header[i] >= 32 && header[i] <= 126) {
            fprintf(stderr, "%c", header[i]);
        } else {
            fprintf(stderr, ".");
        }
    }
    fprintf(stderr, "\n");
    
    // Binary FBX starts with "Kaydara FBX Binary  \x00\x1a\x00"
    const char* expected = "Kaydara FBX Binary  \x00\x1a\x00";
    int is_binary = (memcmp(header, expected, 23) == 0);
    fprintf(stderr, "Is binary FBX: %s\n", is_binary ? "YES" : "NO");
    fprintf(stderr, "=== FBX BINARY CHECK END ===\n");
    return is_binary;
}

// Helper: check if token appears between p (inclusive) and line_end (exclusive)
static int has_token_in_line(const char* p, const char* line_end, const char* token) {
    const char* pos = strstr(p, token);
    return (pos && pos < line_end) ? 1 : 0;
}

static int parse_fbx_ascii(const char* text, FBXParseData* out, char** out_error) {
    // Very simple line-based parsing for Vertices, PolygonVertexIndex, Normals, UV, UVIndex
    // This is not a complete FBX parser, but adequate for simple meshes.
    fprintf(stderr, "=== FBX ASCII PARSING START ===\n");
    fprintf(stderr, "Text length: %zu characters\n", strlen(text));
    
    memset(out, 0, sizeof(*out));

    const char* p = text;
    int line_number = 1;
    while (*p) {
        // Current line range
        const char* nl = strchr(p, '\n');
        const char* line_end = nl ? nl : (p + strlen(p));

        if (!out->positions && has_token_in_line(p, line_end, "Vertices:")) {
            fprintf(stderr, "Found 'Vertices:' on line %d\n", line_number);
            // Find data array start anywhere after this token (can be next line)
            const char* a = strstr(p, "a:");
            if (!a) {
                fprintf(stderr, "No 'a:' found after Vertices on line %d\n", line_number);
                goto next_line;
            }
            a += 2;
            const char* end = strchr(a, '}');
            if (!end) end = p + strlen(p);
            fprintf(stderr, "Vertices data range: %zu characters\n", end - a);
            
            uint32_t count = 0; const char* t = a;
            while (t < end) {
                char* e; strtod(t, &e);
                if (e == t) break; count++; t = e;
                while (t < end && (*t == ',' || isspace((unsigned char)*t))) t++;
            }
            fprintf(stderr, "Found %u position values\n", count);
            
            if (count) {
                out->positions = (float*)malloc(count * sizeof(float));
                out->positions_count = 0;
                t = a;
                while (t < end && out->positions_count < count) {
                    char* e; double v = strtod(t, &e);
                    if (e == t) break; out->positions[out->positions_count++] = (float)v; t = e;
                    while (t < end && (*t == ',' || isspace((unsigned char)*t))) t++;
                }
                fprintf(stderr, "Parsed %u position values successfully\n", out->positions_count);
            }
        } else if (!out->poly_indices && has_token_in_line(p, line_end, "PolygonVertexIndex:")) {
            fprintf(stderr, "Found 'PolygonVertexIndex:' on line %d\n", line_number);
            const char* a = strstr(p, "a:");
            if (!a) {
                fprintf(stderr, "No 'a:' found after PolygonVertexIndex on line %d\n", line_number);
                goto next_line;
            }
            a += 2;
            const char* end = strchr(a, '}');
            if (!end) end = p + strlen(p);
            fprintf(stderr, "PolygonVertexIndex data range: %zu characters\n", end - a);
            
            uint32_t count = 0; const char* t = a;
            while (t < end) {
                char* e; strtol(t, &e, 10);
                if (e == t) break; count++; t = e;
                while (t < end && (*t == ',' || isspace((unsigned char)*t))) t++;
            }
            fprintf(stderr, "Found %u polygon vertex indices\n", count);
            
            if (count) {
                out->poly_indices = (int*)malloc(count * sizeof(int));
                out->poly_indices_count = 0;
                t = a;
                while (t < end && out->poly_indices_count < count) {
                    char* e; long v = strtol(t, &e, 10);
                    if (e == t) break; out->poly_indices[out->poly_indices_count++] = (int)v; t = e;
                    while (t < end && (*t == ',' || isspace((unsigned char)*t))) t++;
                }
                fprintf(stderr, "Parsed %u polygon vertex indices successfully\n", out->poly_indices_count);
            }
        } else if (has_token_in_line(p, line_end, "LayerElementNormal:")) {
            fprintf(stderr, "Found 'LayerElementNormal:' on line %d\n", line_number);
            const char* normals_key = strstr(p, "Normals:");
            if (normals_key) {
                fprintf(stderr, "Found 'Normals:' within LayerElementNormal on line %d\n", line_number);
                const char* a = strstr(normals_key, "a:");
                if (a) {
                    a += 2;
                    const char* end = strchr(a, '}');
                    if (!end) end = p + strlen(p);
                    fprintf(stderr, "Normals data range: %zu characters\n", end - a);
                    
                    uint32_t count = 0; const char* t = a;
                    while (t < end) {
                        char* e; strtod(t, &e);
                        if (e == t) break; count++; t = e;
                        while (t < end && (*t == ',' || isspace((unsigned char)*t))) t++;
                    }
                    fprintf(stderr, "Found %u normal values\n", count);
                    
                    if (count) {
                        out->normals = (float*)malloc(count * sizeof(float));
                        out->normals_count = 0;
                        t = a;
                        while (t < end && out->normals_count < count) {
                            char* e; double v = strtod(t, &e);
                            if (e == t) break; out->normals[out->normals_count++] = (float)v; t = e;
                            while (t < end && (*t == ',' || isspace((unsigned char)*t))) t++;
                        }
                        fprintf(stderr, "Parsed %u normal values successfully\n", out->normals_count);
                    }
                } else {
                    fprintf(stderr, "No 'a:' found after Normals on line %d\n", line_number);
                }
            } else {
                fprintf(stderr, "No 'Normals:' found within LayerElementNormal on line %d\n", line_number);
            }
        } else if (has_token_in_line(p, line_end, "LayerElementUV:")) {
            fprintf(stderr, "Found 'LayerElementUV:' on line %d\n", line_number);
            const char* uv_key = strstr(p, "UV:");
            if (uv_key) {
                fprintf(stderr, "Found 'UV:' within LayerElementUV on line %d\n", line_number);
                const char* a = strstr(uv_key, "a:");
                if (a) {
                    a += 2;
                    const char* end = strchr(a, '}');
                    if (!end) end = p + strlen(p);
                    fprintf(stderr, "UV data range: %zu characters\n", end - a);
                    
                    uint32_t count = 0; const char* t = a;
                    while (t < end) {
                        char* e; strtod(t, &e);
                        if (e == t) break; count++; t = e;
                        while (t < end && (*t == ',' || isspace((unsigned char)*t))) t++;
                    }
                    fprintf(stderr, "Found %u UV values\n", count);
                    
                    if (count) {
                        out->uvs = (float*)malloc(count * sizeof(float));
                        out->uvs_count = 0;
                        t = a;
                        while (t < end && out->uvs_count < count) {
                            char* e; double v = strtod(t, &e);
                            if (e == t) break; out->uvs[out->uvs_count++] = (float)v; t = e;
                            while (t < end && (*t == ',' || isspace((unsigned char)*t))) t++;
                        }
                        fprintf(stderr, "Parsed %u UV values successfully\n", out->uvs_count);
                    }
                } else {
                    fprintf(stderr, "No 'a:' found after UV on line %d\n", line_number);
                }
            } else {
                fprintf(stderr, "No 'UV:' found within LayerElementUV on line %d\n", line_number);
            }
        }

    next_line:
        if (!nl) break;
        p = nl + 1;
        line_number++;
    }

    fprintf(stderr, "=== FBX ASCII PARSING SUMMARY ===\n");
    fprintf(stderr, "Positions: %s (%u values)\n", out->positions ? "FOUND" : "MISSING", out->positions_count);
    fprintf(stderr, "PolygonVertexIndex: %s (%u values)\n", out->poly_indices ? "FOUND" : "MISSING", out->poly_indices_count);
    fprintf(stderr, "Normals: %s (%u values)\n", out->normals ? "FOUND" : "MISSING", out->normals_count);
    fprintf(stderr, "UVs: %s (%u values)\n", out->uvs ? "FOUND" : "MISSING", out->uvs_count);
    fprintf(stderr, "=== FBX ASCII PARSING END ===\n");

    if (!out->positions || !out->poly_indices) {
        if (out_error) *out_error = str_dup("FBX ASCII parse failed: missing positions or indices");
        return 0;
    }
    return 1;
}

static Model3D* build_model_from_parsed(const FBXParseData* d) {
    // Triangulate polygon indices. FBX uses negative index to mark end-of-polygon, and indices refer to position list.
    // We'll generate a triangle list via fan triangulation per polygon.
    // Also, normals/uvs mapping in FBX can be complex; for this minimal loader, we will duplicate vertices per polygon vertex if needed.

    fprintf(stderr, "=== BUILD MODEL FROM PARSED START ===\n");
    fprintf(stderr, "Input data: positions=%u, poly_indices=%u, normals=%u, uvs=%u\n", 
            d->positions_count, d->poly_indices_count, d->normals_count, d->uvs_count);

    // First pass: count triangles and vertex occurrences
    uint32_t tri_count = 0;
    uint32_t poly_vert_in_poly = 0;
    for (uint32_t i = 0; i < d->poly_indices_count; i++) {
        int idx = d->poly_indices[i];
        poly_vert_in_poly++;
        if (idx < 0) { // end of polygon
            if (poly_vert_in_poly >= 3) tri_count += (poly_vert_in_poly - 2);
            poly_vert_in_poly = 0;
        }
    }
    fprintf(stderr, "Calculated triangle count: %u\n", tri_count);
    if (tri_count == 0) {
        fprintf(stderr, "No triangles found, returning NULL\n");
        return NULL;
    }

    // We'll produce a flat triangle list with unique vertices for each polygon vertex.
    uint32_t out_vertex_count = tri_count * 3;
    fprintf(stderr, "Output vertex count: %u\n", out_vertex_count);

    Model3D* model = model3d_allocate(1);
    if (!model) {
        fprintf(stderr, "Failed to allocate Model3D\n");
        return NULL;
    }
    model->name = str_dup("FBXModel");
    Mesh* mesh = &model->meshes[0];
    Mesh* alloc_mesh = mesh_allocate(out_vertex_count, tri_count * 3);
    if (!alloc_mesh) { 
        fprintf(stderr, "Failed to allocate mesh\n");
        model3d_free(model); 
        return NULL; 
    }
    *mesh = *alloc_mesh;
    fprintf(stderr, "Allocated mesh with %u vertices and %u indices\n", out_vertex_count, tri_count * 3);

    // Second pass: fill vertices and indices
    uint32_t out_vi = 0;
    uint32_t out_ii = 0;
    uint32_t poly_start_vi = 0;
    uint32_t poly_vcount = 0;
    // We'll collect polygon vertex position indices as positive values
    int* poly_pos_idx = (int*)malloc(sizeof(int) * d->poly_indices_count);
    uint32_t poly_pos_count = 0;
    for (uint32_t i = 0; i < d->poly_indices_count; i++) {
        int raw = d->poly_indices[i];
        int is_last = raw < 0;
        int idx = is_last ? (-raw - 1) : raw;
        poly_pos_idx[poly_pos_count++] = idx;
        poly_vcount++;
        if (is_last) {
            // fan triangles: (0, k, k+1) for k in [1..poly_vcount-2]
            for (uint32_t k = 1; k + 1 < poly_vcount; k++) {
                int i0 = poly_pos_idx[poly_start_vi + 0];
                int i1 = poly_pos_idx[poly_start_vi + k];
                int i2 = poly_pos_idx[poly_start_vi + k + 1];

                // positions
                float px0 = d->positions[i0 * 3 + 0];
                float py0 = d->positions[i0 * 3 + 1];
                float pz0 = d->positions[i0 * 3 + 2];
                float px1 = d->positions[i1 * 3 + 0];
                float py1 = d->positions[i1 * 3 + 1];
                float pz1 = d->positions[i1 * 3 + 2];
                float px2 = d->positions[i2 * 3 + 0];
                float py2 = d->positions[i2 * 3 + 1];
                float pz2 = d->positions[i2 * 3 + 2];

                mesh->vertices[out_vi + 0] = vertex_create_components(px0, py0, pz0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
                mesh->vertices[out_vi + 1] = vertex_create_components(px1, py1, pz1, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
                mesh->vertices[out_vi + 2] = vertex_create_components(px2, py2, pz2, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);

                mesh->indices[out_ii + 0] = out_vi + 0;
                mesh->indices[out_ii + 1] = out_vi + 1;
                mesh->indices[out_ii + 2] = out_vi + 2;
                out_vi += 3;
                out_ii += 3;
            }
            poly_start_vi = poly_pos_count;
            poly_vcount = 0;
        }
    }
    free(poly_pos_idx);

    fprintf(stderr, "Generated %u vertices and %u indices\n", out_vi, out_ii);
    
    model3d_calculate_bounds(model);
    model3d_calculate_center_and_radius(model);
    
    fprintf(stderr, "Model bounds calculated successfully\n");
    fprintf(stderr, "=== BUILD MODEL FROM PARSED END ===\n");
    return model;
}

Model3D* fbx_load_model(const char* filepath, char** out_error) {
    fprintf(stderr, "=== FBX LOAD MODEL START ===\n");
    fprintf(stderr, "Loading FBX file: %s\n", filepath);
    
    if (out_error) *out_error = NULL;
    FILE* f = fopen(filepath, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open file: %s\n", filepath);
        if (out_error) {
            size_t n = strlen(filepath) + 64;
            char* msg = (char*)malloc(n);
            snprintf(msg, n, "Failed to open file: %s", filepath);
            *out_error = msg;
        }
        return NULL;
    }
    fprintf(stderr, "File opened successfully\n");
    
    Model3D* model = NULL;
    if (is_binary_fbx(f)) {
        // For now, return error for binary; extend later with a real parser
        fprintf(stderr, "Binary FBX detected, not supported yet\n");
        fclose(f);
        if (out_error) *out_error = str_dup("Binary FBX not supported yet");
        return NULL;
    }
    fprintf(stderr, "ASCII FBX detected, proceeding with parsing\n");
    
    // Read entire file as ASCII
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    fprintf(stderr, "File size: %ld bytes\n", sz);
    
    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { 
        fprintf(stderr, "Out of memory allocating %ld bytes\n", sz);
        fclose(f); 
        if (out_error) *out_error = str_dup("Out of memory"); 
        return NULL; 
    }
    size_t bytes_read = fread(buf, 1, (size_t)sz, f);
    buf[sz] = '\0';
    fclose(f);
    fprintf(stderr, "Read %zu bytes from file\n", bytes_read);

    FBXParseData parsed; memset(&parsed, 0, sizeof(parsed));
    if (!parse_fbx_ascii(buf, &parsed, out_error)) {
        fprintf(stderr, "FBX ASCII parsing failed\n");
        free(buf);
        fbx_parse_data_free(&parsed);
        return NULL;
    }
    fprintf(stderr, "FBX ASCII parsing succeeded\n");
    free(buf);

    model = build_model_from_parsed(&parsed);
    fbx_parse_data_free(&parsed);
    if (!model && out_error && !*out_error) {
        fprintf(stderr, "Failed to build model from parsed data\n");
        *out_error = str_dup("Failed to build model from parsed data");
    } else if (model) {
        fprintf(stderr, "Model built successfully\n");
    }
    
    fprintf(stderr, "=== FBX LOAD MODEL END ===\n");
    return model;
}

void fbx_free_error(char* err) { if (err) free(err); }


