<p align="center">
  <img src="images/demo/dragon_in_clouds_thumbnail.png" alt="Metal Stanford Dragon in Clouds" width="100%">
</p>

# Custom C++ CPU Ray Tracer

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17%2B-blue.svg" alt="C++ 17+">
  <img src="https://img.shields.io/badge/Build-CMake-orange.svg" alt="Build">
  <img src="https://img.shields.io/badge/License-MIT-green.svg" alt="License">
</p>

A CPU-based ray tracer built from scratch in C++. This project started as an implementation of Peter Shirley's Ray Tracing in One Weekend trilogy and has been expanded with advanced material systems, texture mapping, acceleration structures, and mesh loading, and a TOML-based scene file system.

## Key Features
### Primitives & Acceleration
- Supported Shapes: Spheres, Triangles, Quads, and OBJ Meshes.
- Mesh Loading: Load 3D models from OBJ files using TinyObjLoader. If vertex normals are not provided in the OBJ file, they are automatically generated based on face geometry.
- Efficiency: Uses BVH (Bounding Volume Hierarchy) to significantly speed up render times for complex scenes.
- Engine: Entirely CPU-based with no external graphics APIs (No OpenGL/DirectX).
- Multi-threading with OpenMP for faster rendering on multi-core processors.

### Scene File System
Scenes and render settings are defined in human-readable TOML files, no recompiling needed to change a scene.
- **`config.toml`** — render settings: resolution, samples, output path, and which scene to load
- **`scenes/*.toml`** — scene data: materials, objects, camera, skybox, and lights

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
> **Note**: HDR is preferred for much more realistic environmental lighting.

### Denoising & Debugging
#### AI Denoising:
Integrated [Intel Open Image Denoise (OIDN) 2.4](https://github.com/RenderKit/oidn) for superior image quality at low sample counts. Toggle denoising via config file or CLI.
#### AOV Saving:
Debug rendering with albedo and normal Arbitrary Output Variables (AOV). Save these outputs alongside the final render for analysis and troubleshooting.

## Build Guide
### Prerequisites
- [CMake](https://cmake.org/download/) (version 3.10 or higher)
- A C++17 compiler — [GCC](https://gcc.gnu.org/), [Clang](https://clang.llvm.org/), or [MSVC](https://visualstudio.microsoft.com/downloads/) *(recommended on Windows)*
### Steps to Run
#### 1. Clone the repo: 
``` 
git clone https://github.com/Anubhav-Mondal/ray-tracer
cd ray-tracer
```
#### 2. Make `build` folder:
```
mkdir build
cd build
```
#### 3. Generate build files:
```
cmake ..
```
#### 4. Build the project:
```
cmake --build . --config Release
```
#### 5. Go back to the project root:
```
cd ..
```
#### 6. Run the Renderer:
Once compiled, run the executable.
#### a. On Windows:
```
./build/Release/raytracer
```
#### b. On Linux/Mac:
```
./build/raytracer
```

## Customizing Renders
### Using config files
Edit `config.toml` to change render settings, and point it to any scene file in the `scenes/` folder:
```toml
scene  = "scenes/cornell_box.toml"
output = "renders/cornell_box.png"
width  = 1024
samples_per_pixel = 500
denoise = true
save_aov = false
```
 
### Using CLI overrides
Override any config value without editing files:
```
./build/Release/raytracer --spp 20 --width 400          # quick preview
./build/Release/raytracer --scene scenes/cornellbox.toml     # different scene
./build/Release/raytracer --output renders/final.png    # different output path
./build/Release/raytracer --depth 20 --width 1920       # high quality
./build/Release/raytracer --spp 50 --denoise            # denoise the output
./build/Release/raytracer --save-aov                     # save albedo and normal maps
```
Supported flags:
 
| Flag | Description |
|---|---|
| `--scene` | Path to a scene `.toml` file |
| `--output` | Output filename (`.png`, `.jpg`, or `.hdr`) |
| `--spp` | Samples per pixel |
| `--width` | Image width in pixels |
| `--depth` | Max ray bounce depth |
| `--denoise` | Enable OIDN denoising |
| `--no-denoise` | Disable OIDN denoising |
| `--save-aov` | Save albedo and normal AOVs |
| `--no-save-aov` | Disable AOV saving |
 
You can also pass a custom config file as the first argument:
```
./build/Release/raytracer my_config.toml
./build/Release/raytracer my_config.toml --spp 50
```
 
---

## Loading Meshes
Load OBJ files into your scenes via the scene file:
```toml
[[object]]
type      = "mesh"
path      = "models/filename.obj"
material  = "my_material"
scale     = 100.0
translate = [0, 0, 0]
rotate_y  = 45.0
```
---

## Results
<table>
  <tr>
    <td align="center"><img src="images/demo/stanford_bunny_smooth_metal.png" alt="Stanford Bunny"/></td>
    <td align="center"><img src="images/demo/utah_teapot_smooth_metal.png" alt="Utah Teapot"/></td>
  </tr>
  <tr>
    <td align="center"><img src="images/demo/frosted_glass.png" alt="Frosted Crystal Ball"/></td>
    <td align="center"><img src="images/demo/crystal_ball_at_resturant.png" alt="Crystal Ball at Restaurant"/></td>
  </tr>
  <tr>
    <td align="center"><img src="images/demo/gl_normal.png" alt="Foilwrap Sphere"/></td>
    <td align="center"><img src="images/demo/glossy_ball_indoor.png" alt="Glossy Ball Indoor"/></td>
  </tr>
</table>
<p>
Checkout the <a href="GALLERY.md">Gallery</a> for more renders and details on materials, textures, and scenes!
</p>

## Roadmap (To-Do)
- [ ] Animation System - for rendering path traced videos.
- [ ] More 3D format support.

## Acknowledgments
- Ray Tracing in One Weekend series for the foundational math.

- `stb_image` and `tinyobjloader` for header-only utility support.
---
<p align="center">Created by <b>Anubhav Mondal</b></p>