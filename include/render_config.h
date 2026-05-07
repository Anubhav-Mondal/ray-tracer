#pragma once

#include <string>
#include "raytracing.h"

struct render_config {
    std::string scene_path = "scene.toml";
    std::string output_file = "output.png";
    int         image_width       = 800;
    double      aspect_ratio      = 1.0;
    int         samples_per_pixel = 100;
    int         max_depth         = 10;
    bool        denoise            = true;
    bool        save_aov           = false;
};

struct scene_config {
    std::string skybox        = "";
    color       background    = color(0, 0, 0);
    double      vfov          = 40;
    point3      lookfrom      = point3(0, 0, 1);
    point3      lookat        = point3(0, 0, 0);
    vec3        vup           = vec3(0, 1, 0);
    double      defocus_angle = 0.0;
    double      focus_dist    = 10.0;
};