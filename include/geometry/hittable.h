#ifndef HITTABLE_H
#define HITTABLE_H

#include "rendering/raytracing.h"
#include "geometry/aabb.h"

class material; 

class hit_record {
  public:
    point3 p;
    vec3 normal;
    shared_ptr<material> mat;
    double t;
    double u;
    double v;
    bool front_face;

    void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class hittable {
  public:
    virtual ~hittable() = default;

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;

    virtual aabb bounding_box() const = 0;

    virtual double pdf_value(const point3& origin, const vec3& direction) const {
        return 0.0;
    }

    virtual vec3 random(const point3& origin) const {
        return vec3(1,0,0);
    }
};

class translate : public hittable {
  public:
    translate(shared_ptr<hittable> object, const vec3& offset)
      : object(object), offset(offset)
    {
        bbox = object->bounding_box() + offset;
    }
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        ray offset_r(r.origin() - offset, r.direction(), r.time());

        if (!object->hit(offset_r, ray_t, rec))
            return false;

        rec.p += offset;

        return true;
    }
    aabb bounding_box() const override { return bbox; }


  private:
    shared_ptr<hittable> object;
    vec3 offset;
    aabb bbox;
};

class rotate_y : public hittable {
  public:

    rotate_y(shared_ptr<hittable> object, double angle) : object(object) {
        auto radians = degrees_to_radians(angle);
        sin_theta = std::sin(radians);
        cos_theta = std::cos(radians);
        bbox = object->bounding_box();

        point3 min( infinity,  infinity,  infinity);
        point3 max(-infinity, -infinity, -infinity);

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                for (int k = 0; k < 2; k++) {
                    auto x = i*bbox.x.max + (1-i)*bbox.x.min;
                    auto y = j*bbox.y.max + (1-j)*bbox.y.min;
                    auto z = k*bbox.z.max + (1-k)*bbox.z.min;

                    auto newx =  cos_theta*x + sin_theta*z;
                    auto newz = -sin_theta*x + cos_theta*z;

                    vec3 tester(newx, y, newz);

                    for (int c = 0; c < 3; c++) {
                        min[c] = std::fmin(min[c], tester[c]);
                        max[c] = std::fmax(max[c], tester[c]);
                    }
                }
            }
        }

        bbox = aabb(min, max);
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {

        // Transform the ray from world space to object space.

        auto origin = point3(
            (cos_theta * r.origin().x()) - (sin_theta * r.origin().z()),
            r.origin().y(),
            (sin_theta * r.origin().x()) + (cos_theta * r.origin().z())
        );

        auto direction = vec3(
            (cos_theta * r.direction().x()) - (sin_theta * r.direction().z()),
            r.direction().y(),
            (sin_theta * r.direction().x()) + (cos_theta * r.direction().z())
        );

        ray rotated_r(origin, direction, r.time());

        // Determine whether an intersection exists in object space (and if so, where).

        if (!object->hit(rotated_r, ray_t, rec))
            return false;

        // Transform the intersection from object space back to world space.

        rec.p = point3(
            (cos_theta * rec.p.x()) + (sin_theta * rec.p.z()),
            rec.p.y(),
            (-sin_theta * rec.p.x()) + (cos_theta * rec.p.z())
        );

        rec.normal = vec3(
            (cos_theta * rec.normal.x()) + (sin_theta * rec.normal.z()),
            rec.normal.y(),
            (-sin_theta * rec.normal.x()) + (cos_theta * rec.normal.z())
        );

        return true;
    }

    aabb bounding_box() const override { return bbox; }

  private:
    shared_ptr<hittable> object;
    double sin_theta;
    double cos_theta;
    aabb bbox;
};

class scale : public hittable {
  public:
    // Uniform scale
    scale(shared_ptr<hittable> object, double s)
      : scale(object, vec3(s, s, s)) {}

    // Per-axis scale
    scale(shared_ptr<hittable> object, const vec3& s)
      : object(object), scale_factor(s), inv_scale(1.0/s.x(), 1.0/s.y(), 1.0/s.z())
    {
        aabb ob = object->bounding_box();
        bbox = aabb(
            point3(ob.x.min * s.x(), ob.y.min * s.y(), ob.z.min * s.z()),
            point3(ob.x.max * s.x(), ob.y.max * s.y(), ob.z.max * s.z())
        );
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        ray scaled_r(
            point3(r.origin().x() * inv_scale.x(),
                   r.origin().y() * inv_scale.y(),
                   r.origin().z() * inv_scale.z()),
            vec3(r.direction().x() * inv_scale.x(),
                 r.direction().y() * inv_scale.y(),
                 r.direction().z() * inv_scale.z()),
            r.time()
        );

        if (!object->hit(scaled_r, ray_t, rec))
            return false;

        rec.p = point3(
            rec.p.x() * scale_factor.x(),
            rec.p.y() * scale_factor.y(),
            rec.p.z() * scale_factor.z()
        );

        rec.normal = unit_vector(vec3(
            rec.normal.x() * inv_scale.x(),
            rec.normal.y() * inv_scale.y(),
            rec.normal.z() * inv_scale.z()
        ));

        return true;
    }

    aabb bounding_box() const override { return bbox; }

  private:
    shared_ptr<hittable> object;
    vec3 scale_factor;
    vec3 inv_scale;
    aabb bbox;
};

#endif