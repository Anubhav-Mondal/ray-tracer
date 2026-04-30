#pragma once
#include "hittable.h"
#include "material.h"
#include <string>
#include <memory>

shared_ptr<hittable> load_obj(const std::string& path, shared_ptr<material> mat);