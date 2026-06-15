#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H

#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE

#include "tiny_gltf.h"
#include "geometry/mesh_utils.h"
#include "geometry/bvh.h"
#include "geometry/hittable_list.h"

template<typename T>
std::vector<T> gltf_buffer_as(const tinygltf::Model& model, int accessor_idx) {
    const auto& acc  = model.accessors[accessor_idx];
    const auto& view = model.bufferViews[acc.bufferView];
    const auto& buf  = model.buffers[view.buffer];

    const unsigned char* base = buf.data.data() + view.byteOffset + acc.byteOffset;

    size_t stride = view.byteStride != 0 ? view.byteStride : sizeof(T);

    std::vector<T> result(acc.count);
    for (size_t i = 0; i < acc.count; i++) {
        std::memcpy(&result[i], base + i * stride, sizeof(T));
    }
    return result;
}

inline shared_ptr<hittable> load_gltf(const std::string& path, shared_ptr<material> mat) {
    tinygltf::Model    model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    loader.SetImageLoader(nullptr, nullptr);

    bool ok = (path.size() >= 4 && path.substr(path.size() - 4) == ".glb")
        ? loader.LoadBinaryFromFile(&model, &err, &warn, path)
        : loader.LoadASCIIFromFile (&model, &err, &warn, path);

    if (!warn.empty()) std::cerr << "tinygltf warning: " << warn;
    if (!ok)           throw std::runtime_error("tinygltf: " + err);

    auto combined_list = make_shared<hittable_list>();

    for (const auto& gltf_mesh : model.meshes) {
        for (const auto& prim : gltf_mesh.primitives) {
            if (prim.mode != TINYGLTF_MODE_TRIANGLES) continue;

            auto mesh      = make_shared<Mesh>();
            mesh->material = mat;
            mesh->has_normal = prim.attributes.count("NORMAL");
            mesh->has_uv     = prim.attributes.count("TEXCOORD_0");

            // Positions
            struct vec3f { float x, y, z; };
            auto positions = gltf_buffer_as<vec3f>(model, prim.attributes.at("POSITION"));
            int nv = (int)positions.size();
            mesh->vertices.resize(nv);
            for (int i = 0; i < nv; i++)
                mesh->vertices[i].position = point3(positions[i].x, positions[i].y, positions[i].z);

            // Normals
            if (mesh->has_normal) {
                auto normals = gltf_buffer_as<vec3f>(model, prim.attributes.at("NORMAL"));
                for (int i = 0; i < nv; i++)
                    mesh->vertices[i].normal = vec3(normals[i].x, normals[i].y, normals[i].z);
            }

            // UVs
            struct vec2f { float x, y; };
            if (mesh->has_uv) {
                auto uvs = gltf_buffer_as<vec2f>(model, prim.attributes.at("TEXCOORD_0"));
                for (int i = 0; i < nv; i++)
                    mesh->vertices[i].uv = vec2(uvs[i].x, uvs[i].y);
            }

            // Indices
            if (prim.indices < 0) {
                for (int i=0; i<nv; i++) mesh->indices.push_back(i);
            } else {
                const auto& idx_acc = model.accessors[prim.indices];
                switch (idx_acc.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                        for (auto x : gltf_buffer_as<uint8_t>(model, prim.indices))
                            mesh->indices.push_back(x); break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        for (auto x : gltf_buffer_as<uint16_t>(model, prim.indices))
                            mesh->indices.push_back(x); break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                        for (auto x : gltf_buffer_as<uint32_t>(model, prim.indices))
                            mesh->indices.push_back(x); break;
                    default:
                        throw std::runtime_error("Unsupported index type: " + std::to_string(idx_acc.componentType));
                }

                int min_idx = *std::min_element(mesh->indices.begin(), mesh->indices.end());
                if (min_idx > 0)
                    for (int& idx : mesh->indices) idx -= min_idx;
            }

            int max_idx = *std::max_element(mesh->indices.begin(), mesh->indices.end());
            if (max_idx >= nv)
                throw std::runtime_error("Index still out of range after rebasing: max=" + std::to_string(max_idx) + " nv=" + std::to_string(nv));

            if (!mesh->has_normal) compute_smooth_normals(mesh);
            combined_list->add(mesh_to_bvh(mesh));
        }
    }

    return make_shared<bvh_node>(*combined_list);
}

#endif