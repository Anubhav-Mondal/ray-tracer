#pragma once

#include "render_config.h"
#include "camera.h"
#include "hittable.h"
#include <string>

render_config load_config(const std::string& path);

scene_config load_scene(const std::string& path, hittable_list& world, hittable_list& lights);