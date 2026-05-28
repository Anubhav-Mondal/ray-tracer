#pragma once

#include "rendering/render_config.h"
#include "rendering/camera.h"
#include "geometry/hittable.h"
#include "rendering/raytracing.h"

#include <string>
#include <map>

render_config load_config(const std::string& path);

scene_config load_scene(const std::string& path, hittable_list& world, hittable_list& lights, const std::map<std::string, double>& overrides = {});