#!/usr/bin/env python3
import math

def generate_sphere(radius=1.0, latitude_segments=32, longitude_segments=32):
    """Generate a unit sphere with specified number of segments."""
    vertices = []
    indices = []
    
    # Generate vertices
    for lat in range(latitude_segments + 1):
        theta = lat * math.pi / latitude_segments
        sin_theta = math.sin(theta)
        cos_theta = math.cos(theta)
        
        for lon in range(longitude_segments + 1):
            phi = lon * 2 * math.pi / longitude_segments
            sin_phi = math.sin(phi)
            cos_phi = math.cos(phi)
            
            x = radius * cos_phi * sin_theta
            y = radius * cos_theta
            z = radius * sin_phi * sin_theta
            
            vertices.extend([x, y, z])
    
    # Generate indices
    for lat in range(latitude_segments):
        for lon in range(longitude_segments):
            first = lat * (longitude_segments + 1) + lon
            second = first + longitude_segments + 1
            
            # First triangle
            indices.extend([first, second, first + 1])
            # Second triangle  
            indices.extend([second, second + 1, first + 1])
    
    return vertices, indices

# Generate sphere data
vertices, indices = generate_sphere(radius=1.0, latitude_segments=16, longitude_segments=32)

print(f"Generated sphere with {len(vertices)//3} vertices and {len(indices)} indices ({len(indices)//3} triangles)")

# Output vertices for FBX
print("\nVertices:")
print("Vertices: *" + str(len(vertices)//3) + " {")
for i in range(0, len(vertices), 3):
    print(f"a: {vertices[i]:.6f}, {vertices[i+1]:.6f}, {vertices[i+2]:.6f},")
print("}")

# Output indices for FBX
print("\nPolygonVertexIndex:")
print("PolygonVertexIndex: *" + str(len(indices)) + " {")
for i in range(0, len(indices), 3):
    print(f"a: {indices[i]}, {indices[i+1]}, {indices[i+2]},")
print("}")

# Calculate normals (same as vertices for unit sphere)
print("\nNormals:")
print("Normals: *" + str(len(vertices)//3) + " {")
for i in range(0, len(vertices), 3):
    print(f"a: {vertices[i]:.6f}, {vertices[i+1]:.6f}, {vertices[i+2]:.6f},")
print("}")
