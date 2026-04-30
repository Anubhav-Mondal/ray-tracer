#ifndef MESH_LOADER_H
#define MESH_LOADER_H

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

    int num_positions = attrib.vertices.size() / 3;
    mesh->vertices.resize(num_positions);

    for (int i = 0; i < num_positions; i++) {
        mesh->vertices[i].position = point3(
            attrib.vertices[3*i + 0],
            attrib.vertices[3*i + 1],
            attrib.vertices[3*i + 2]
        );
    }

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            mesh->indices.push_back(index.vertex_index);

            if (mesh->has_normal && index.normal_index >= 0) {
                mesh->vertices[index.vertex_index].normal = vec3(
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                );
            }

            if (mesh->has_uv && index.texcoord_index >= 0) {
                mesh->vertices[index.vertex_index].uv = vec2(
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1]
                );
            }
        }
    }

    if (!mesh->has_normal) {
        int num_faces = mesh->indices.size() / 3;

        std::vector<vec3> accumulated(num_positions, vec3(0, 0, 0));

        for (int i = 0; i < num_faces; i++) {
            int i0 = mesh->indices[3*i + 0];
            int i1 = mesh->indices[3*i + 1];
            int i2 = mesh->indices[3*i + 2];

            const vec3& p0 = mesh->vertices[i0].position;
            const vec3& p1 = mesh->vertices[i1].position;
            const vec3& p2 = mesh->vertices[i2].position;

            vec3 weighted_normal = cross(p1 - p0, p2 - p0);

            accumulated[i0] += weighted_normal;
            accumulated[i1] += weighted_normal;
            accumulated[i2] += weighted_normal;
        }

        for (int i = 0; i < num_positions; i++) {
            mesh->vertices[i].normal = unit_vector(accumulated[i]);
        }

        mesh->has_normal = true;
    }

    auto list = make_shared<hittable_list>();
    int num_faces = mesh->indices.size() / 3;

    for (int i = 0; i < num_faces; i++) {
        list->add(make_shared<meshTriangle>(mesh, i));
    }

    return make_shared<bvh_node>(*list);
}

#endif