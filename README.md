# TestMetal Game Engine

A high-performance game engine built with Metal 3.0, combining Objective-C for OS integration with C for engine core performance.

## 🎯 **Programming Philosophy**

This project synthesizes performance-first data-oriented design with strategic abstraction layers, balancing Casey Muratori's performance insights with maintainable code practices.

### **Core Principles**

1. **Performance-First Data Layout** - Data-oriented design over object hierarchies
2. **SIMD-Optimized Operations** - Direct SIMD intrinsics for vector computations
3. **Strategic Abstraction** - Abstraction only where performance cost is acceptable
4. **Compiler-Friendly Code** - Functions designed for inlining and optimization

## 🏗️ **Architecture Overview**

### **Layer Structure**
```
┌─────────────────────────────────────┐
│           Objective-C Layer         │  ← OS Integration (Cocoa, Metal)
├─────────────────────────────────────┤
│              C Engine Core          │  ← Performance-Critical Engine Logic
├─────────────────────────────────────┤
│            SIMD Math Layer          │  ← Vector Operations & Math
└─────────────────────────────────────┘
```

### **Technology Stack**
- **OS Layer**: Objective-C, Cocoa, Metal 3.0
- **Engine Core**: C99 with SIMD intrinsics
- **Math Library**: `simd` framework + custom SIMD optimizations
- **Build System**: Xcode with Metal shader compilation

## 📁 **Project Structure**

```
TestMetal/
├── README.md                    # This file
├── METAL_3_0_UPDATES.md        # Metal 3.0 upgrade documentation
├── TestMetal.xcodeproj/        # Xcode project
├── TestMetal/                   # Source code
│   ├── AppDelegate.h/m         # OS integration (Objective-C)
│   ├── GameViewController.h/m   # Game loop management (Objective-C)
│   ├── Renderer.h/m            # Metal rendering (Objective-C)
│   ├── Shaders.metal           # Metal shaders (MSL)
│   ├── ShaderTypes.h           # Shared types (C)
│   └── main.m                  # Entry point
└── Engine/                      # Future engine core (C)
    ├── Math/                   # SIMD math library
    ├── Core/                   # Engine systems
    ├── Rendering/              # Render pipeline
    └── Physics/                # Physics system
```

## 🚀 **Performance Guidelines**

### **Hot Path Optimization**
- **Inline functions** for frequently called code
- **SIMD intrinsics** for vector operations
- **Data-oriented design** over object hierarchies
- **Switch statements** over virtual function calls

### **Cold Path Optimization**
- **Higher-level abstractions** acceptable where performance isn't critical
- **Polymorphism** only where performance cost is acceptable
- **File I/O and asset loading** can use more abstraction

### **Memory Layout**
- **Structure of Arrays (SoA)** for SIMD operations
- **Cache-friendly data organization**
- **Minimal pointer indirection** in hot paths

## 💻 **Coding Standards**

### **Objective-C (OS Layer)**
```objc
// ✅ Use for OS integration, UI, Metal setup
@interface MetalRenderer : NSObject
@property (nonatomic, strong) id<MTLDevice> device;
- (void)renderFrame:(const GameState*)gameState;
@end
```

### **C (Engine Core)**
```c
// ✅ Use for performance-critical engine logic
typedef struct {
    float3 position;
    float2 texCoord;
    float3 normal;
} Vertex;

static inline float3 TransformVertex(float3 vertex, const Transform* transform) {
    return simd_mul(transform->matrix, vertex) + transform->position;
}
```

### **SIMD Operations**
```c
// ✅ Direct SIMD intrinsics for performance
#include <immintrin.h>

void ProcessVertices_AVX2(Vertex* vertices, size_t count) {
    for (size_t i = 0; i < count; i += 8) {
        __m256 pos = _mm256_load_ps(&vertices[i].position.x);
        __m256 transformed = _mm256_fmadd_ps(pos, transformMatrix, translation);
        _mm256_store_ps(&vertices[i].position.x, transformed);
    }
}
```

## 🔧 **Development Workflow**

### **1. Performance Profiling**
- Use Instruments to identify bottlenecks
- Profile hot paths (rendering, physics, game logic)
- Apply SIMD optimizations where beneficial

### **2. Code Organization**
- **Hot paths**: Minimal abstraction, SIMD optimizations
- **Cold paths**: Higher-level abstractions acceptable
- **OS integration**: Objective-C with Cocoa patterns
- **Engine core**: C with clear data structures

### **3. Testing Strategy**
- Unit tests for math library and core systems
- Performance benchmarks for critical paths
- Integration tests for Metal rendering pipeline

## 📚 **Key References**

- [Casey Muratori: "Clean" Code, Horrible Performance](https://www.computerenhance.com/p/clean-code-horrible-performance)
- [Codef00: Casey Muratori is Wrong About Clean Code](https://blog.codef00.com/2023/04/13/casey-muratori-is-wrong-about-clean-code)
- [Metal 3.0 Documentation](https://developer.apple.com/metal/)
- [SIMD Programming Guide](https://developer.apple.com/documentation/accelerate/simd)

## 🎮 **Current Status**

### **✅ Completed**
- Metal 3.0 integration with enhanced features
- Basic 3D rendering pipeline
- Texture loading and application
- Metal Performance Shaders integration
- Performance-optimized vertex descriptor setup

### **🚧 In Progress**
- Engine architecture design
- SIMD math library implementation
- Entity-Component-System architecture

### **📋 Planned**
- Physics system with SIMD optimizations
- Asset management system
- Game loop and timing system
- Input handling and event system

## 🛠️ **Building and Running**

### **Prerequisites**
- macOS 15.0+
- Xcode 15.0+
- Metal 3.0 capable GPU

### **Build Commands**
```bash
# Debug build
xcodebuild -project TestMetal.xcodeproj -scheme TestMetal -configuration Debug build

# Release build
xcodebuild -project TestMetal.xcodeproj -scheme TestMetal -configuration Release build

# Run from command line
./build/Debug/TestMetal.app/Contents/MacOS/TestMetal
```

## 🤝 **Contributing**

When contributing to this project:

1. **Follow the performance guidelines** outlined above
2. **Use appropriate language** for each layer (Objective-C for OS, C for engine)
3. **Profile performance** before and after changes
4. **Document performance characteristics** of new features
5. **Maintain the hybrid architecture** approach

## 📝 **Code Generation Guidelines**

**IMPORTANT**: When generating code for this project, always reference this README.md and follow these principles:

1. **Performance-critical code** → Use C with SIMD intrinsics
2. **OS integration code** → Use Objective-C with Cocoa patterns  
3. **Data structures** → Prefer structs over classes for performance
4. **Hot paths** → Minimize abstraction, use inline functions
5. **Cold paths** → Higher-level abstractions acceptable
6. **Memory layout** → Optimize for cache locality and SIMD operations

---

*This project demonstrates that performance and maintainability can coexist through thoughtful architecture and strategic use of appropriate technologies for each layer.*
