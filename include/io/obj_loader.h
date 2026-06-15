#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include "tiny_obj_loader.h"
#include "geometry/mesh_utils.h"
#include "geometry/hittable_list.h"
#include "geometry/bvh.h"
#include <map>
#include <tuple>

inline shared_ptr<hittable> load_obj(const std::string& path, shared_ptr<material> mat) {
    tinyobj::ObjReaderConfig cfg;
    cfg.mtl_search_path = "./";
    cfg.triangulate = true;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path, cfg)) {
        throw std::runtime_error("TinyObjReader: " + reader.Error());
    }
    if (!reader.Warning().empty())
        std::cerr << "TinyObjReader warning: " << reader.Warning();

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();

    auto mesh = make_shared<Mesh>();
    mesh->material   = mat;
    mesh->has_normal = false;
    mesh->has_uv     = !attrib.texcoords.empty();

    using Key = std::tuple<int,int,int>;
    std::map<Key, int> seen;

    for (const auto& shape : shapes) {
        for (const auto& idx : shape.mesh.indices) {
            Key key{ idx.vertex_index, idx.normal_index, idx.texcoord_index };
            auto [it, inserted] = seen.emplace(key, (int)mesh->vertices.size());

            if (inserted) {
                Vertex v{};
                v.position = point3(
                    attrib.vertices[3*idx.vertex_index + 0],
                    attrib.vertices[3*idx.vertex_index + 1],
                    attrib.vertices[3*idx.vertex_index + 2]
                );
                if (idx.normal_index >= 0) {
                    v.normal = vec3(
                        attrib.normals[3*idx.normal_index + 0],
                        attrib.normals[3*idx.normal_index + 1],
                        attrib.normals[3*idx.normal_index + 2]
                    );
                    mesh->has_normal = true;
                }
                if (idx.texcoord_index >= 0) {
                    v.uv = vec2(
                        attrib.texcoords[2*idx.texcoord_index + 0],
                        attrib.texcoords[2*idx.texcoord_index + 1]
                    );
                }
                mesh->vertices.push_back(v);
            }
            mesh->indices.push_back(it->second);
        }
    }

    if (!mesh->has_normal)
        compute_smooth_normals(mesh);

    return mesh_to_bvh(mesh);
}

#endif