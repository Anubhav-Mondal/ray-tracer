#ifndef MESH_LOADER_H
#define MESH_LOADER_H

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include "mesh.h"
#include "hittable_list.h"
#include "bvh.h"

shared_ptr<hittable> load_obj(const std::string& path, shared_ptr<material> mat) {
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "./";

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(path, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader Error: " << reader.Error();
        }
        exit(1);
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader Warning: " << reader.Warning();
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();

    auto mesh = make_shared<Mesh>();
    mesh->material = mat;
    mesh->has_normal = !attrib.normals.empty();
    mesh->has_uv = !attrib.texcoords.empty();

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex;

            vertex.position = point3(
                attrib.vertices[3*index.vertex_index + 0],
                attrib.vertices[3*index.vertex_index + 1],
                attrib.vertices[3*index.vertex_index + 2]
            );

            if (mesh->has_normal && index.normal_index >= 0) {
                vertex.normal = vec3(
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                );
            }

            if (mesh->has_uv && index.texcoord_index >= 0) {
                vertex.uv = vec2(
                    attrib.texcoords[2*index.texcoord_index + 0],
                    attrib.texcoords[2*index.texcoord_index + 1]
                );
            }

            mesh->vertices.push_back(vertex);
            mesh->indices.push_back(mesh->indices.size());
        }
    }

    auto list = make_shared<hittable_list>();
    int num_faces = mesh->indices.size() / 3;

    for (int i=0; i<num_faces; i++) {
        list->add(make_shared<meshTriangle>(mesh, i));
    }

    return make_shared<bvh_node>(*list);
}

#endif