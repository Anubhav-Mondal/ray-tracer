#ifndef MESH_H
#define MESH_H

#include "hittable.h"
#include "onb.h"
#include "raytracing.h"

struct Vertex {
    point3 position;
    vec3 normal;
    vec2 uv;
};

class Mesh {
    public:
        std::vector<Vertex> vertices;
        std::vector<int> indices;

        bool has_normal;
        bool has_uv;

        shared_ptr<material> material;      
};

class meshTriangle: public hittable {
    public:
        meshTriangle(shared_ptr<Mesh> mesh, int face_index) : mesh(mesh) {
            i0 = mesh->indices[face_index*3 + 0];
            i1 = mesh->indices[face_index*3 + 1];
            i2 = mesh->indices[face_index*3 + 2];

            const vec3& p0 = mesh->vertices[i0].position;
            const vec3& p1 = mesh->vertices[i1].position;
            const vec3& p2 = mesh->vertices[i2].position;

            double min_x = std::min({p0.x(), p1.x(), p2.x()});
            double min_y = std::min({p0.y(), p1.y(), p2.y()});
            double min_z = std::min({p0.z(), p1.z(), p2.z()});

            double max_x = std::max({p0.x(), p1.x(), p2.x()});
            double max_y = std::max({p0.y(), p1.y(), p2.y()});
            double max_z = std::max({p0.z(), p1.z(), p2.z()});

            const double pad = 1e-8;
            bbox = aabb(
                point3(min_x - pad, min_y - pad, min_z - pad),
                point3(max_x + pad, max_y + pad, max_z + pad)
            );
        }

        bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
            const Vertex& v0 = mesh->vertices[i0];
            const Vertex& v1 = mesh->vertices[i1];
            const Vertex& v2 = mesh->vertices[i2];

            const double EPSILON = 1e-8;

            vec3 edge1 = v1.position - v0.position;
            vec3 edge2 = v2.position - v0.position;

            vec3 h = cross(r.direction(), edge2);
            double a = dot(edge1, h);

            if (a > -EPSILON && a < EPSILON) {
                return false;
            }

            double f = 1.0 / a;
            vec3 s = r.origin() - v0.position;
            double u = f * dot(s, h);

            if (u < 0.0 || u > 1.0) {
                return false;
            }

            vec3 q = cross(s, edge1);
            double v = f * dot(r.direction(), q);

            if (v < 0.0 || u + v > 1.0) {
                return false;
            }
            
            double t = f * dot(edge2, q);
            
            if (!ray_t.contains(t)) {
                return false;
            }

            rec.t = t;
            rec.p = r.at(t);

            double w = 1.0 - u - v;
            
            vec3 outward_normal;
            if (mesh->has_normal) {
                outward_normal = unit_vector(w * v0.normal + u * v1.normal + v * v2.normal);
            } else {
                outward_normal = unit_vector(cross(edge1, edge2));
            }
            rec.set_face_normal(r, outward_normal);
            
            if (mesh->has_uv) {
                rec.u = w * v0.uv.x() + u * v1.uv.x() + v * v2.uv.x();
                rec.v = w * v0.uv.y() + u * v1.uv.y() + v * v2.uv.y();
            } else {
                rec.u = u;
                rec.v = v;
            }

            rec.mat = mesh->material;

            return true;
        }

        aabb bounding_box() const override {
            return bbox;
        }

        double area() const {
            vec3 edge1 = mesh->vertices[i1].position - mesh->vertices[i0].position;
            vec3 edge2 = mesh->vertices[i2].position - mesh->vertices[i0].position;
            return 0.5 * cross(edge1, edge2).length();
        }

        point3 centroid() const {
            return (mesh->vertices[i0].position + mesh->vertices[i1].position + mesh->vertices[i2].position) / 3.0;
        }

        double pdf_value(const point3& origin, const vec3& direction) const override {
            hit_record rec;
            if (!this->hit(ray(origin, direction), interval(0.001, infinity), rec))
                return 0;

            auto distance_squared = rec.t * rec.t * direction.length_squared();
            auto cosine = std::fabs(dot(direction, rec.normal) / direction.length());
            return distance_squared / (cosine * area());
        }

        vec3 random(const point3& origin) const override {
            double r1 = random_double();
            double r2 = random_double();
            double sqrt_r1 = std::sqrt(r1);
            point3 p = (1 - sqrt_r1) * mesh->vertices[i0].position
                    + sqrt_r1 * (1 - r2) * mesh->vertices[i1].position
                    + sqrt_r1 * r2       * mesh->vertices[i2].position;
            return p - origin;
        }

    private:
        shared_ptr<Mesh> mesh;
        int i0, i1, i2;
        aabb bbox;
};

#endif