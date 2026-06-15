#ifndef MESH_LOADER_H
#define MESH_LOADER_H

#include "geometry/mesh.h"
#include "geometry/hittable_list.h"
#include "geometry/bvh.h"

#include "io/obj_loader.h"
#include "io/gltf_loader.h"

#include <filesystem>

inline shared_ptr<hittable> load_mesh(const std::string& path, shared_ptr<material> mat) {
    std::string ext = std::filesystem::path(path).extension().string();

    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".obj")              return load_obj(path, mat);
    if (ext == ".gltf" || 
        ext == ".glb")              return load_gltf(path, mat);

    throw std::runtime_error("Unsupported mesh format: " + ext);
}

#endif