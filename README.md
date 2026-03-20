# Custom C++ CPU Ray Tracer
A CPU-based ray tracer built from scratch in C++. This project started as an implementation of Peter Shirley's Ray Tracing in One Weekend trilogy and has been expanded with advanced material systems, texture mapping, and acceleration structures.

## Key Features
### Primitives & Acceleration
- Supported Shapes: Spheres, Triangles, and Quads.
- Efficiency: Uses BVH (Bounding Volume Hierarchy) to significantly speed up render times for complex scenes.
- Engine: Entirely CPU-based with no external graphics APIs (No OpenGL/DirectX).
### Materials & Rendering
#### Material Library: 
- Dielectric: Realistic glass and water.
- Frosted Glass: Rough refraction effects.
- Metal: Polished reflective surfaces.
- Glossy (Clear Coat): Multi-layered materials with a shiny finish.
- Diffuse Light: For area lights and glowing objects.
- Image Textured: Mapping 2D images onto 3D geometry.
#### Advanced Maps: 
Full support for Albedo, Normal, and Roughness maps to add surface detail.
#### Camera: 
Supports Depth of Field (defocus blur) for a cinematic look.

### Textures & Environment
#### Procedural Textures: 
Checker patterns, Solid colors, and Perlin Noise.
#### Skybox: 
Supports PNG, JPG, and HDR (High Dynamic Range).
> Note: HDR is preferred for much more realistic environmental lighting.

## Results
![Frosted Crystal Ball](<images/demo/frosted_glass.png>)

![Crystal Ball at Resturant](<images/demo/crystal_ball_at_resturant.png>)

![Foilwrap sphere](<images/demo/gl_normal.png>)

![Metal Ball at Street](<images/demo/metal_ball_at_street.png>)

![Glossy Ball indoor](<images/demo/glossy_ball_indoor.png>)

![Demo](<images/demo/raytraced emmisive sphere.png>)


## Build Guide
### Prerequisites
- CMake (version 3.10 or higher).
- A C++ compiler (GCC/Clang/MSVC) supporting C++17 or higher.

### Steps to Run
#### 1. Clone the repo: 
``` 
clone https://github.com/Anubhav-Mondal/ray-tracer
cd ray-tracer
```
#### 2. Make `build` folder:
```
mkdir build
cd build
```
#### 3. Generate build files:
```
cmake -DCMAKE_BUILD_TYPE=Release ..
```
#### 4. Build the project:
```
cmake --build .
```
#### 5. Run the Renderer:
Once compiled, run the executable.
#### a. On Windows:
```
./Release/raytracer
```
#### b. On Linux/Mac:
```
./raytracer
```
#### Customizing Output
You can change the output filename and format directly in `main.cpp`:
```cpp
cam.output_name = "my_render";       // default: "output"
cam.save_ext = camera::save_extension::hdr;  // png | jpg | hdr
```
The file will be saved in the `build` folder.

## Roadmap (To-Do)
- Mesh Loading: Implement an .obj file parser to render complex 3D models.
- Multi-threading: Further optimize the CPU rendering using SIMD or OpenMP.

## Acknowledgments
This project was inspired by and built upon the excellent Ray Tracing in One Weekend book series. It served as the foundation for the core math, which I have since expanded with custom features like advanced material coats and HDR support.

