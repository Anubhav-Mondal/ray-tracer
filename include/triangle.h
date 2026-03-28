#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "hittable.h"
#include "onb.h"
#include "raytracing.h"

class triangle : public hittable
{
    public:
        triangle(const point3 &v0, const point3 &v1, const point3 &v2, shared_ptr<material> mat)
            : vertex0(v0), vertex1(v1), vertex2(v2), mat(mat), uv0(0,0), uv1(1,0), uv2(0,1), has_normals(false), has_uvs(false) { 
                setup_triangle(); 
            }

        triangle(const point3 &v0, const point3 &v1, const point3 &v2,
                const vec2 &uv0, const vec2 &uv1, const vec2 &uv2,
                shared_ptr<material> mat)
            : vertex0(v0), vertex1(v1), vertex2(v2), mat(mat),
            uv0(uv0), uv1(uv1), uv2(uv2),
            has_normals(false), has_uvs(true)
        {
            setup_triangle();
        }

        triangle(const point3 &v0, const point3 &v1, const point3 &v2,
                const vec3 &n0, const vec3 &n1, const vec3 &n2,
                shared_ptr<material> mat)
            : vertex0(v0), vertex1(v1), vertex2(v2), mat(mat),
            normal0(n0), normal1(n1), normal2(n2),
            uv0(0, 0), uv1(1, 0), uv2(0, 1), // Default UVs
            has_normals(true), has_uvs(false)
        {
            setup_triangle();
        }

        triangle(const point3 &v0, const point3 &v1, const point3 &v2,
                const vec3 &n0, const vec3 &n1, const vec3 &n2,
                const vec2 &uv0, const vec2 &uv1, const vec2 &uv2,
                shared_ptr<material> mat)
            : vertex0(v0), vertex1(v1), vertex2(v2), mat(mat),
            normal0(n0), normal1(n1), normal2(n2),
            uv0(uv0), uv1(uv1), uv2(uv2),
            has_normals(true), has_uvs(true)
        {
            setup_triangle();
        }


        bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
            
            const double EPSILON = 1e-8;
            
            vec3 edge1 = vertex1 - vertex0;
            vec3 edge2 = vertex2 - vertex0;
            vec3 h = cross(r.direction(), edge2);
            double a = dot(edge1, h);
            
            if (a > -EPSILON && a < EPSILON) {
                return false;
            }
            
            double f = 1.0 / a;
            vec3 s = r.origin() - vertex0;
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
            
            vec3 outward_normal;
            if (has_normals) {
                double w = 1.0 - u - v;  
                outward_normal = unit_vector(w * normal0 + u * normal1 + v * normal2);
            } else {
                outward_normal = face_normal;
            }

            
            rec.set_face_normal(r, outward_normal);
            
            if (has_uvs) {
                double w = 1.0 - u - v;
                rec.u = w * uv0.x() + u * uv1.x() + v * uv2.x();
                rec.v = w * uv0.y() + u * uv1.y() + v * uv2.y();
            } else {
                rec.u = u;
                rec.v = v;
            }
            
            rec.mat = mat;
            
            return true;
        }

        aabb bounding_box() const override {
            return bbox;
        }
        
        // Utility methods
        double area() const {
            vec3 edge1 = vertex1 - vertex0;
            vec3 edge2 = vertex2 - vertex0;
            return 0.5 * cross(edge1, edge2).length();
        }
        
        point3 centroid() const {
            return (vertex0 + vertex1 + vertex2) / 3.0;
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
            point3 p = (1 - sqrt_r1) * vertex0
                    + sqrt_r1 * (1 - r2) * vertex1
                    + sqrt_r1 * r2       * vertex2;
            return p - origin;
        }

    private:
        point3 vertex0, vertex1, vertex2;
        vec3 normal0, normal1, normal2; // Vertex normals for smooth shading
        vec2 uv0, uv1, uv2;             // UV coordinates
        vec3 face_normal;               // Geometric normal of the triangle
        shared_ptr<material> mat;
        aabb bbox;
        bool has_normals;
        bool has_uvs;

        void setup_triangle()
        {
            // Calculate face normal
            vec3 edge1 = vertex1 - vertex0;
            vec3 edge2 = vertex2 - vertex0;
            face_normal = unit_vector(cross(edge1, edge2));

            // Calculate bounding box
            double min_x = std::min({vertex0.x(), vertex1.x(), vertex2.x()});
            double max_x = std::max({vertex0.x(), vertex1.x(), vertex2.x()});
            double min_y = std::min({vertex0.y(), vertex1.y(), vertex2.y()});
            double max_y = std::max({vertex0.y(), vertex1.y(), vertex2.y()});
            double min_z = std::min({vertex0.z(), vertex1.z(), vertex2.z()});
            double max_z = std::max({vertex0.z(), vertex1.z(), vertex2.z()});

            const double epsilon = 1e-8;
            if (max_x - min_x < epsilon)
            {
                min_x -= epsilon;
                max_x += epsilon;
            }
            if (max_y - min_y < epsilon)
            {
                min_y -= epsilon;
                max_y += epsilon;
            }
            if (max_z - min_z < epsilon)
            {
                min_z -= epsilon;
                max_z += epsilon;
            }

            bbox = aabb(point3(min_x, min_y, min_z), point3(max_x, max_y, max_z));
        }
};

#endif