#pragma once
#include "geometry/hittable.h"
#include "material/material.h"
#include <string>
#include <memory>

shared_ptr<hittable> load_obj(const std::string& path, shared_ptr<material> mat);