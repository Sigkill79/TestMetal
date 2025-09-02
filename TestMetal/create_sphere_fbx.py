#!/usr/bin/env python3
import math

def generate_sphere(radius=1.0, latitude_segments=16, longitude_segments=32):
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
vertex_count = len(vertices) // 3
index_count = len(indices)
triangle_count = index_count // 3

print(f"Generated sphere with {vertex_count} vertices and {index_count} indices ({triangle_count} triangles)")

# Create the FBX file
with open('assets/UnitSphere.fbx', 'w') as f:
    # FBX Header
    f.write("; FBX 7.7.0 project file\n")
    f.write("; ----------------------------------------------------\n\n")
    
    # FBX Version
    f.write("FBXHeaderExtension:  {\n")
    f.write("    FBXHeaderVersion: 1003\n")
    f.write("    FBXVersion: 7700\n")
    f.write("    CreationTimeStamp:  {\n")
    f.write("        Version: 1000\n")
    f.write("        Year: 2024\n")
    f.write("        Month: 1\n")
    f.write("        Day: 1\n")
    f.write("        Hour: 0\n")
    f.write("        Minute: 0\n")
    f.write("        Second: 0\n")
    f.write("        Millisecond: 0\n")
    f.write("    }\n")
    f.write("    Creator: \"UnitSphere Generator\"\n")
    f.write("    OtherFlags:  {\n")
    f.write("        FlagPLE: 0\n")
    f.write("    }\n")
    f.write("}\n\n")
    
    # Global Settings
    f.write("GlobalSettings:  {\n")
    f.write("    Version: 1000\n")
    f.write("    Properties70:  {\n")
    f.write("        P: \"UpAxis\", \"int\", \"Integer\", \"\",1\n")
    f.write("        P: \"UpAxisSign\", \"int\", \"Integer\", \"\",1\n")
    f.write("        P: \"FrontAxis\", \"int\", \"Integer\", \"\",2\n")
    f.write("        P: \"FrontAxisSign\", \"int\", \"Integer\", \"\",1\n")
    f.write("        P: \"CoordAxis\", \"int\", \"Integer\", \"\",0\n")
    f.write("        P: \"CoordAxisSign\", \"int\", \"Integer\", \"\",1\n")
    f.write("        P: \"OriginalUpAxis\", \"int\", \"Integer\", \"\",1\n")
    f.write("        P: \"OriginalUpAxisSign\", \"int\", \"Integer\", \"\",1\n")
    f.write("        P: \"UnitScaleFactor\", \"double\", \"Number\", \"\",1\n")
    f.write("        P: \"OriginalUnitScaleFactor\", \"double\", \"Number\", \"\",1\n")
    f.write("        P: \"AmbientColor\", \"ColorRGB\", \"Color\", \"\",0.4,0.4,0.4\n")
    f.write("        P: \"DefaultCamera\", \"KString\", \"\", \"\", \"Producer Perspective\"\n")
    f.write("        P: \"TimeMode\", \"enum\", \"\", \"\",6\n")
    f.write("        P: \"TimeProtocol\", \"enum\", \"\", \"\",2\n")
    f.write("        P: \"SnapOnFrameMode\", \"enum\", \"\", \"\",0\n")
    f.write("        P: \"TimeSpanStart\", \"KTime\", \"Time\", \"\",0\n")
    f.write("        P: \"TimeSpanStop\", \"KTime\", \"Time\", \"\",153953860000\n")
    f.write("        P: \"CustomFrameRate\", \"double\", \"Number\", \"\",-1\n")
    f.write("        P: \"TimeMarker\", \"Compound\", \"\", \"\"\n")
    f.write("        P: \"CurrentTimeMarker\", \"int\", \"Integer\", \"\",-1\n")
    f.write("    }\n")
    f.write("}\n\n")
    
    # Documents
    f.write("Documents:  {\n")
    f.write("    Count: 1\n")
    f.write("    Document: *1 {\n")
    f.write("        \"Document::UnitSphere\", \"Scene\"\n")
    f.write("        Properties70:  {\n")
    f.write("            P: \"SourceObject\", \"object\", \"\", \"\"\n")
    f.write("            P: \"ActiveAnimStackName\", \"KString\", \"\", \"\", \"\"\n")
    f.write("        }\n")
    f.write("        RootNode: 0\n")
    f.write("    }\n")
    f.write("}\n\n")
    
    # References
    f.write("References:  {\n")
    f.write("}\n\n")
    
    # Definitions
    f.write("Definitions:  {\n")
    f.write("    Version: 100\n")
    f.write("    Count: 2\n")
    f.write("    ObjectType: \"GlobalSettings\" {\n")
    f.write("        Count: 1\n")
    f.write("    }\n")
    f.write("    ObjectType: \"Model\" {\n")
    f.write("        Count: 1\n")
    f.write("        PropertyTemplate: \"FbxNode\" {\n")
    f.write("            Properties70:  {\n")
    f.write("                P: \"DefaultAttributeIndex\", \"int\", \"Integer\", \"\",0\n")
    f.write("                P: \"DefaultAttributeIndex\", \"int\", \"Integer\", \"\",0\n")
    f.write("            }\n")
    f.write("        }\n")
    f.write("    }\n")
    f.write("}\n\n")
    
    # Objects
    f.write("Objects:  {\n")
    f.write("    GlobalSettings: *0 {\n")
    f.write("        Version: 1000\n")
    f.write("        Properties70:  {\n")
    f.write("            P: \"UpAxis\", \"int\", \"Integer\", \"\",1\n")
    f.write("            P: \"UpAxisSign\", \"int\", \"Integer\", \"\",1\n")
    f.write("            P: \"FrontAxis\", \"int\", \"Integer\", \"\",2\n")
    f.write("            P: \"FrontAxisSign\", \"int\", \"Integer\", \"\",1\n")
    f.write("            P: \"CoordAxis\", \"int\", \"Integer\", \"\",0\n")
    f.write("            P: \"CoordAxisSign\", \"int\", \"Integer\", \"\",1\n")
    f.write("            P: \"OriginalUpAxis\", \"int\", \"Integer\", \"\",1\n")
    f.write("            P: \"OriginalUpAxisSign\", \"int\", \"Integer\", \"\",1\n")
    f.write("            P: \"UnitScaleFactor\", \"double\", \"Number\", \"\",1\n")
    f.write("            P: \"OriginalUnitScaleFactor\", \"double\", \"Number\", \"\",1\n")
    f.write("            P: \"AmbientColor\", \"ColorRGB\", \"Color\", \"\",0.4,0.4,0.4\n")
    f.write("            P: \"DefaultCamera\", \"KString\", \"\", \"\", \"Producer Perspective\"\n")
    f.write("            P: \"TimeMode\", \"enum\", \"\", \"\",6\n")
    f.write("            P: \"TimeProtocol\", \"enum\", \"\", \"\",2\n")
    f.write("            P: \"SnapOnFrameMode\", \"enum\", \"\", \"\",0\n")
    f.write("            P: \"TimeSpanStart\", \"KTime\", \"Time\", \"\",0\n")
    f.write("            P: \"TimeSpanStop\", \"KTime\", \"Time\", \"\",153953860000\n")
    f.write("            P: \"CustomFrameRate\", \"double\", \"Number\", \"\",-1\n")
    f.write("            P: \"TimeMarker\", \"Compound\", \"\", \"\"\n")
    f.write("            P: \"CurrentTimeMarker\", \"int\", \"Integer\", \"\",-1\n")
    f.write("        }\n")
    f.write("    }\n")
    f.write("    Model: \"Model::UnitSphere\", \"Mesh\" {\n")
    f.write("        Version: 232\n")
    f.write("        Properties70:  {\n")
    f.write("            P: \"DefaultAttributeIndex\", \"int\", \"Integer\", \"\",0\n")
    f.write("            P: \"DefaultAttributeIndex\", \"int\", \"Integer\", \"\",0\n")
    f.write("        }\n")
    f.write("        MultiLayer: 0\n")
    f.write("        MultiTake: 0\n")
    f.write("        Shading: Y\n")
    f.write("        Culling: \"CullingOff\"\n")
    f.write("    }\n")
    f.write("}\n\n")
    
    # Connections
    f.write("Connections:  {\n")
    f.write("    C: \"OO\",0,1\n")
    f.write("}\n\n")
    
    # Write the geometry data
    f.write("Geometry: \"Geometry::UnitSphere\" {\n")
    f.write("    Properties70:  {\n")
    f.write("        P: \"Color\", \"ColorRGB\", \"Color\", \"\",0.8,0.8,0.8\n")
    f.write("    }\n")
    f.write("    Vertices: *" + str(vertex_count) + " {\n")
    for i in range(0, len(vertices), 3):
        f.write(f"        a: {vertices[i]:.6f}, {vertices[i+1]:.6f}, {vertices[i+2]:.6f},\n")
    f.write("    }\n")
    f.write("    PolygonVertexIndex: *" + str(index_count) + " {\n")
    for i in range(0, len(indices), 3):
        f.write(f"        a: {indices[i]}, {indices[i+1]}, {indices[i+2]},\n")
    f.write("    }\n")
    f.write("    Edges: *0 {\n")
    f.write("    }\n")
    f.write("    GeometryVersion: 124\n")
    f.write("    LayerElementNormal: 0 {\n")
    f.write("        Normals: *" + str(vertex_count) + " {\n")
    for i in range(0, len(vertices), 3):
        # For a sphere, normals are the same as vertex positions (normalized)
        x, y, z = vertices[i], vertices[i+1], vertices[i+2]
        length = math.sqrt(x*x + y*y + z*z)
        if length > 0:
            x, y, z = x/length, y/length, z/length
        f.write(f"            a: {x:.6f}, {y:.6f}, {z:.6f},\n")
    f.write("        }\n")
    f.write("    }\n")
    f.write("}\n")

print("UnitSphere.fbx created successfully!")
