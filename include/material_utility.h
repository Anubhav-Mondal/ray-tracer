#ifndef MATERIAL_UTILIY_H
#define MATERIAL_UTILIY_H

#include "raytracing.h"
#include "texture.h"
#include "hittable.h"

inline vec3 apply_normal_map(const shared_ptr<texture>& normalMap, const hit_record& rec) {
    color normalColors = normalMap->value(rec.u, rec.v, rec.p);
    vec3 tangentNormal = vec3(
        normalColors.e[0] * 2.0 - 1.0,
        normalColors.e[1] * 2.0 - 1.0,
        normalColors.e[2] * 2.0 - 1.0
    );

    if (tangentNormal.length() <= 0.001) return rec.normal;

    vec3 N = unit_vector(rec.normal);
    vec3 up = (std::abs(dot(N, vec3(0,1,0))) > 0.9) ? vec3(1,0,0) : vec3(0,1,0);
    vec3 T = unit_vector(cross(up, N));
    vec3 B = cross(N, T);

    return unit_vector(T * tangentNormal.x() +
                       B * tangentNormal.y() +
                       N * tangentNormal.z());
}

#endif