#ifndef MATERIAL_H
#define MATERIAL_H

#include "hittable.h"
#include "texture.h"
#include "pdf.h"
#include "material_utility.h"

class scatter_record {
  public:
    color attenuation;
    shared_ptr<pdf> pdf_ptr;
    bool skip_pdf;
    ray skip_pdf_ray;
};

class material {
  public:
    virtual ~material() = default;

    virtual color emitted(const ray& r_in, const hit_record& rec, double u, double v, const point3& p
    ) const {
        return color(0,0,0);
    }

    virtual bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const {
        return false;
    }

    virtual double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered)
    const {
        return 0;
    }
};

class lambertian : public material {
  public:
    lambertian(const color& albedo) : tex(make_shared<solid_color>(albedo)) {}
    lambertian(shared_ptr<texture> tex) : tex(tex) {}

    lambertian& add_normal_map(shared_ptr<texture> t) { normalMap = t; return *this; }
    lambertian& remove_normal_map() { normalMap = nullptr; return *this; }

    bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const override {
        vec3 normal = (normalMap && !normalMap->is_empty())
            ? apply_normal_map(normalMap, rec)
            : rec.normal;

        srec.attenuation = tex->value(rec.u, rec.v, rec.p);
        srec.pdf_ptr = make_shared<cosine_pdf>(normal);
        srec.skip_pdf = false;
        return true;
    }

    double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered)
    const override {
        vec3 normal = (normalMap && !normalMap->is_empty())
            ? apply_normal_map(normalMap, rec)
            : rec.normal;

        auto cos_theta = dot(normal, unit_vector(scattered.direction()));
        return cos_theta < 0 ? 0 : cos_theta/pi;
    }

  private:
    shared_ptr<texture> tex;
    shared_ptr<texture> normalMap = nullptr;
};

class metal : public material {
  public:
    metal(const color& albedo, double fuzz = 0.0) 
        : albedo(albedo), fuzz(std::clamp(fuzz, 0.0, 1.0)) {}

    metal& add_albedo_map(shared_ptr<texture> tex) { albedoMap = tex; return *this; }
    metal& add_fuzz_map(shared_ptr<texture> tex)   { fuzzMap   = tex; return *this; }
    metal& add_normal_map(shared_ptr<texture> tex) { normalMap = tex; return *this; }
    metal& set_albedo(const color& c)              { albedo    = c;   return *this; }
    metal& set_fuzz(double f)                      { fuzz = std::clamp(f, 0.0, 1.0); return *this; }
    metal& remove_albedo_map()                     { albedoMap = nullptr; return *this; }
    metal& remove_fuzz_map()                       { fuzzMap   = nullptr; return *this; }
    metal& remove_normal_map()                     { normalMap = nullptr; return *this; }

    bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const override {
        vec3 normal = rec.normal;
        if (normalMap && !normalMap->is_empty()) {
            color normalColors = normalMap->value(rec.u, rec.v, rec.p);
            vec3 tangentNormal = vec3(
                normalColors.e[0] * 2.0 - 1.0,
                normalColors.e[1] * 2.0 - 1.0,
                normalColors.e[2] * 2.0 - 1.0
            );

            if (tangentNormal.length() > 0.001) {
                vec3 N = unit_vector(rec.normal);
                vec3 up = (std::abs(dot(N, vec3(0,1,0))) > 0.9) ? vec3(1,0,0) : vec3(0,1,0);
                vec3 T = unit_vector(cross(up, N));
                vec3 B = cross(N, T);
                normal = unit_vector(T * tangentNormal.x() +
                                     B * tangentNormal.y() +
                                     N * tangentNormal.z());
            }
        }

        vec3 reflected = reflect(r_in.direction(), normal);

        double effective_fuzz = (fuzzMap && !fuzzMap->is_empty())
            ? std::min(1.0, fuzzMap->value(rec.u, rec.v, rec.p).e[0])
            : fuzz;
        reflected = unit_vector(reflected) + (effective_fuzz * random_unit_vector());

        if (reflected.length_squared() < 1e-8)
            reflected = unit_vector(reflect(r_in.direction(), normal));

        srec.attenuation = (albedoMap && !albedoMap->is_empty())
            ? albedoMap->value(rec.u, rec.v, rec.p)
            : albedo;

        srec.pdf_ptr = nullptr;
        srec.skip_pdf = true;
        srec.skip_pdf_ray = ray(rec.p, reflected, r_in.time());

        return true;
    }

  private:
    color albedo;
    double fuzz;
    shared_ptr<texture> albedoMap = nullptr;
    shared_ptr<texture> fuzzMap   = nullptr;
    shared_ptr<texture> normalMap = nullptr;
};

class dielectric : public material {
  public:
    dielectric(double refraction_index, const color& albedo=color(1.0, 1.0, 1.0)) : refraction_index(refraction_index), albedo(albedo) {}

    dielectric& add_albedo_map(shared_ptr<texture> tex) { albedoMap = tex; return *this; }
    dielectric& add_normal_map(shared_ptr<texture> tex) { normalMap = tex; return *this; }

    bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const override {
        srec.pdf_ptr = nullptr;
        srec.skip_pdf = true;
        double ri = rec.front_face ? (1.0/refraction_index) : refraction_index;

        vec3 normal = (normalMap && !normalMap->is_empty())
            ? apply_normal_map(normalMap, rec)
            : rec.normal;

        vec3 unit_direction = unit_vector(r_in.direction());
        double cos_theta = std::fmin(dot(-unit_direction, normal), 1.0);
        double sin_theta = std::sqrt(1.0 - cos_theta*cos_theta);

        bool cannot_refract = ri * sin_theta > 1.0;
        vec3 direction;

        if (cannot_refract || reflectance(cos_theta, ri) > random_double()) {
            direction = reflect(unit_direction, normal);
            srec.attenuation = color(1.0, 1.0, 1.0);
        } else {
            direction = refract(unit_direction, normal, ri);
            srec.attenuation = (albedoMap && !albedoMap->is_empty())
                ? albedoMap->value(rec.u, rec.v, rec.p)
                : albedo;
        }

        srec.skip_pdf_ray = ray(rec.p, direction, r_in.time());
        return true;
    }

  private:
    double refraction_index;
    color albedo;
    shared_ptr<texture> albedoMap = nullptr;
    shared_ptr<texture> normalMap = nullptr;

    static double reflectance(double cosine, double refraction_index) {
        auto r0 = (1 - refraction_index) / (1 + refraction_index);
        r0 = r0*r0;
        return r0 + (1-r0)*std::pow((1 - cosine),5);
    }
};

class diffuse_light : public material {
  public:
    diffuse_light(shared_ptr<texture> tex) : tex(tex) {}
    diffuse_light(const color& emit) : tex(make_shared<solid_color>(emit)) {}

    color emitted(const ray& r_in, const hit_record& rec, double u, double v, const point3& p)
    const override {
        if (!rec.front_face)
            return color(0,0,0);
        return tex->value(u, v, p);
    }

  private:
    shared_ptr<texture> tex;
};

class isotropic : public material {
  public:
    isotropic(const color& albedo) : tex(make_shared<solid_color>(albedo)) {}
    isotropic(shared_ptr<texture> tex) : tex(tex) {}

    bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const override {
        srec.attenuation = tex->value(rec.u, rec.v, rec.p);
        srec.pdf_ptr = make_shared<sphere_pdf>();
        srec.skip_pdf = false;
        return true;
    }

    double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered)
    const override {
        return 1 / (4 * pi);
    }

  private:
    shared_ptr<texture> tex;
};

class glossy : public material {
  public:
    glossy(const color& albedo, double roughness = 0.1, double specular_strength = 0.8) 
        : albedo(albedo), roughness(roughness), specular_strength(specular_strength) {}

    bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const override {
        double fresnel = schlick_approximation(r_in, rec.normal);
        
        if (random_double() < fresnel * specular_strength) {
            vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
            reflected = unit_vector(reflected) + (roughness * random_unit_vector());

            if (reflected.length_squared() < 1e-8)
                reflected = unit_vector(reflect(unit_vector(r_in.direction()), rec.normal));

            srec.skip_pdf_ray = ray(rec.p, reflected, r_in.time());
            
            srec.attenuation = color(1.0, 1.0, 1.0); // White specular highlights 
            srec.pdf_ptr = nullptr;
            srec.skip_pdf = true; 
            srec.skip_pdf_ray = ray(rec.p, reflected, r_in.time());
        } else {
            srec.attenuation = albedo;
            srec.pdf_ptr = make_shared<cosine_pdf>(rec.normal);
            srec.skip_pdf = false;
        }
        
        return true;
    }

    double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const override {
        auto cos_theta = dot(rec.normal, unit_vector(scattered.direction()));
        return cos_theta < 0 ? 0 : cos_theta / pi;
    }

  private:
    color albedo;
    double roughness;
    double specular_strength;
    
    double schlick_approximation(const ray& r_in, const vec3& normal) const {
        auto cos_theta = fmin(dot(-unit_vector(r_in.direction()), normal), 1.0);
        auto r0 = 0.04;
        return r0 + (1-r0)*pow((1 - cos_theta), 5);
    }
};

class frosted_glass : public material {
  public:
    frosted_glass(double refraction_index, double roughness = 0.1, const color& tint = color(1.0, 1.0, 1.0)) 
        : refraction_index(refraction_index), roughness(roughness), tint(tint) {}

    bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const override {
        srec.attenuation = tint;
        srec.pdf_ptr = nullptr;
        srec.skip_pdf = true;
        
        double ri = rec.front_face ? (1.0/refraction_index) : refraction_index;
        vec3 unit_direction = unit_vector(r_in.direction());
        
        vec3 perturbed_normal = rec.normal + (roughness * random_unit_vector());
        if (perturbed_normal.length_squared() < 1e-8)
            perturbed_normal = rec.normal;
        else
            perturbed_normal = unit_vector(perturbed_normal);
        
        double cos_theta = fmin(dot(-unit_direction, perturbed_normal), 1.0);
        double sin_theta = sqrt(1.0 - cos_theta*cos_theta);
        
        bool cannot_refract = ri * sin_theta > 1.0;
        vec3 direction;
        
        if (cannot_refract || reflectance(cos_theta, ri) > random_double()) {
            direction = reflect(unit_direction, perturbed_normal);
            direction = unit_vector(direction) + (roughness * 0.5 * random_unit_vector());
        } else {
            direction = refract(unit_direction, perturbed_normal, ri);
            direction = unit_vector(direction) + (roughness * 0.3 * random_unit_vector());
        }
        if (direction.length_squared() < 1e-8)
            direction = perturbed_normal;
        else
            direction = unit_vector(direction);

        srec.skip_pdf_ray = ray(rec.p, direction, r_in.time());
        return true;
    }

  private:
    double refraction_index;
    double roughness;
    color tint;
    
    static double reflectance(double cosine, double refraction_index) {
        auto r0 = (1 - refraction_index) / (1 + refraction_index);
        r0 = r0*r0;
        return r0 + (1-r0)*pow((1 - cosine), 5);
    }
};

class advanced_frosted_glass : public material {
  public:
    advanced_frosted_glass(double refraction_index, double roughness = 0.1, 
                          double subsurface_scattering = 0.2, const color& tint = color(1.0, 1.0, 1.0)) 
        : refraction_index(refraction_index), roughness(roughness), 
          subsurface_scattering(subsurface_scattering), tint(tint) {}

    bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const override {
        srec.attenuation = tint;
        
        if (random_double() < subsurface_scattering) {
            srec.pdf_ptr = make_shared<cosine_pdf>(rec.normal);
            srec.skip_pdf = false;
            return true;
        } else {
            srec.pdf_ptr = nullptr;
            srec.skip_pdf = true;
            
            double ri = rec.front_face ? (1.0/refraction_index) : refraction_index;
            vec3 unit_direction = unit_vector(r_in.direction());
            
            vec3 microfacet_normal = sample_microfacet_normal(rec.normal, roughness);
            
            double cos_theta = fmin(dot(-unit_direction, microfacet_normal), 1.0);
            double sin_theta = sqrt(1.0 - cos_theta*cos_theta);
            
            bool cannot_refract = ri * sin_theta > 1.0;
            vec3 direction;
            
            if (cannot_refract || reflectance(cos_theta, ri) > random_double()) {
                direction = reflect(unit_direction, microfacet_normal);
            } else {
                direction = refract(unit_direction, microfacet_normal, ri);
            }

            if (direction.length_squared() < 1e-8)
                direction = microfacet_normal;

            srec.skip_pdf_ray = ray(rec.p, direction, r_in.time());
            return true;
        }
    }

    double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const override {
        auto cos_theta = dot(rec.normal, unit_vector(scattered.direction()));
        return cos_theta < 0 ? 0 : cos_theta / pi;
    }

  private:
    double refraction_index;
    double roughness;
    double subsurface_scattering;
    color tint;
    
    vec3 sample_microfacet_normal(const vec3& normal, double alpha) const {
        double r1 = random_double();
        double r2 = random_double();

        r1 = std::clamp(r1, 0.0, 1.0 - 1e-10);  
        
        double theta = atan(alpha * sqrt(r1) / sqrt(1.0 - r1));
        double phi = 2.0 * pi * r2;
        
        // Convert to world space
        vec3 w = normal;
        vec3 u = ((abs(w.x()) > 0.1) ? vec3(0, 1, 0) : vec3(1, 0, 0));
        u = unit_vector(cross(u, w));
        vec3 v = cross(w, u);
        
        vec3 sample_dir = sin(theta) * cos(phi) * u + 
                         sin(theta) * sin(phi) * v + 
                         cos(theta) * w;
        
        return unit_vector(sample_dir);
    }
    
    static double reflectance(double cosine, double refraction_index) {
        auto r0 = (1 - refraction_index) / (1 + refraction_index);
        r0 = r0*r0;
        return r0 + (1-r0)*pow((1 - cosine), 5);
    }
};

#endif