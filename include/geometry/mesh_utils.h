#ifndef MESH_UTILS_H
#define MESH_UTILS_H

#include "geometry/mesh.h"
#include "geometry/hittable_list.h"
#include "geometry/bvh.h"

inline void compute_smooth_normals(shared_ptr<Mesh> mesh) {
    int num_positions = mesh->vertices.size();
    int num_faces     = mesh->indices.size() / 3;

    std::vector<vec3> accumulated(num_positions, vec3(0, 0, 0));
    for (int i = 0; i < num_faces; i++) {
        int i0 = mesh->indices[3*i + 0];
        int i1 = mesh->indices[3*i + 1];
        int i2 = mesh->indices[3*i + 2];
        vec3 wn = cross(
            mesh->vertices[i1].position - mesh->vertices[i0].position,
            mesh->vertices[i2].position - mesh->vertices[i0].position
        );
        accumulated[i0] += wn;
        accumulated[i1] += wn;
        accumulated[i2] += wn;
    }
    for (int i = 0; i < num_positions; i++)
        mesh->vertices[i].normal = unit_vector(accumulated[i]);
    mesh->has_normal = true;
}

inline shared_ptr<hittable> mesh_to_bvh(shared_ptr<Mesh> mesh) {
    auto list = make_shared<hittable_list>();
    int num_faces = mesh->indices.size() / 3;
    for (int i = 0; i < num_faces; i++)
        list->add(make_shared<meshTriangle>(mesh, i));
    return make_shared<bvh_node>(*list);
}

#endif