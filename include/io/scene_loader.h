#pragma once

#include "rendering/render_config.h"
#include "rendering/camera.h"
#include "geometry/hittable.h"
#include <string>

render_config load_config(const std::string& path);

scene_config load_scene(const std::string& path, hittable_list& world, hittable_list& lights);