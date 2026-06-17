#pragma once

#include "material/material.h"
#include "material/texture.h"
#include "material/material_utility.h"
#include "core/pdf.h"
#include "geometry/hittable.h"
#include <algorithm>

// Normal Distribution Function
inline double ggx_D(double NdotH, double alpha) {
    double a2 = alpha * alpha;
    double d  = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (pi * d * d);
}

inline double G1_schlick(double NdotV, double k) {
    return NdotV / (NdotV * (1.0 - k) + k);
}

inline double ggx_G(double NdotV, double NdotL, double roughness) {
    double k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    return G1_schlick(NdotV, k) * G1_schlick(NdotL, k);
}

inline color fresnel_schlick(double cos_theta, const color& F0) {
    double x = std::pow(1.0 - cos_theta, 5.0);
    return F0 + (color(1,1,1) - F0) * x;
}

inline vec3 sample_ggx(const vec3& normal, double alpha) {
    double r1 = random_double(), r2 = random_double();
    double theta = std::atan(alpha * std::sqrt(r1) / std::sqrt(std::max(1e-10, 1.0 - r1)));
    double phi   = 2.0 * pi * r2;

    vec3 w = normal;
    vec3 up = (std::abs(w.x()) > 0.1) ? vec3(0,1,0) : vec3(1,0,0);
    vec3 t  = unit_vector(cross(up, w));
    vec3 b  = cross(w, t);

    return unit_vector(
        std::sin(theta) * std::cos(phi) * t +
        std::sin(theta) * std::sin(phi) * b +
        std::cos(theta) * w
    );
}

// PBR Material

class pbr_material: public material {
    public:
        // Parameters
        color  baseColorFactor    = color(1, 1, 1);
        double metallicFactor     = 1.0;
        double roughnessFactor    = 1.0;
        color  emissiveFactor     = color(0, 0, 0);
        double emissiveStrength   = 1.0;
        double ior                = 1.5;
        double normalScale        = 1.0;
        double occlusionStrength  = 1.0;
        double alphaCutoff        = 0.5;
        bool   doubleSided        = false;

        enum class AlphaMode { Opaque, Mask, Blend };
        AlphaMode alphaMode = AlphaMode::Opaque;

        // Textures
        shared_ptr<texture> baseColorTexture          = nullptr;
        shared_ptr<texture> metallicRoughnessTexture  = nullptr;
        shared_ptr<texture> normalTexture             = nullptr;
        shared_ptr<texture> occlusionTexture          = nullptr;
        shared_ptr<texture> emissiveTexture           = nullptr;

        pbr_material& set_base_color(const color& c)          { baseColorFactor = c;          return *this; }
        pbr_material& set_metallic(double m)                  { metallicFactor  = m;          return *this; }
        pbr_material& set_roughness(double r)                 { roughnessFactor = r;          return *this; }
        pbr_material& set_emissive(const color& e)            { emissiveFactor  = e;          return *this; }
        pbr_material& set_emissive_strength(double s)         { emissiveStrength= s;          return *this; }
        pbr_material& set_ior(double i)                       { ior = i;                      return *this; }
        pbr_material& set_normal_scale(double s)              { normalScale = s;              return *this; }
        pbr_material& set_occlusion_strength(double s)        { occlusionStrength = s;        return *this; }
        pbr_material& set_double_sided(bool v)                { doubleSided = v;              return *this; }
        pbr_material& set_alpha_mode(AlphaMode m, double cut) { alphaMode=m; alphaCutoff=cut; return *this; }

        pbr_material& add_base_color_texture(shared_ptr<texture> t)         { baseColorTexture = t;         return *this; }
        pbr_material& add_metallic_roughness_texture(shared_ptr<texture> t) { metallicRoughnessTexture = t; return *this; }
        pbr_material& add_normal_texture(shared_ptr<texture> t)             { normalTexture = t;            return *this; }
        pbr_material& add_occlusion_texture(shared_ptr<texture> t)          { occlusionTexture = t;         return *this; }
        pbr_material& add_emissive_texture(shared_ptr<texture> t)           { emissiveTexture = t;          return *this; }

        color emitted(const ray& r_in, const hit_record& rec, double u, double v, const point3& p) 
        const override {
            if (!rec.front_face && !doubleSided) return color(0,0,0);

            color e = emissiveFactor;
            if (emissiveTexture)
                e = e * emissiveTexture->value(u, v, p);
            return e * emissiveStrength;
        }

        bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) 
        const override {
            color baseColor = baseColorFactor;
            double alpha    = 1.0;
            if (baseColorTexture) {
                color s   = baseColorTexture->value(rec.u, rec.v, rec.p);
                baseColor = baseColor * color(s.x(), s.y(), s.z());
                alpha     = s.e[3];
            }

            if (alphaMode == AlphaMode::Mask && alpha < alphaCutoff)
                return false;

            double metallic  = metallicFactor;
            double roughness = roughnessFactor;
            if (metallicRoughnessTexture) {
                color mr = metallicRoughnessTexture->value(rec.u, rec.v, rec.p);
                roughness *= mr.e[1];
                metallic  *= mr.e[2];
            }
            roughness = std::clamp(roughness, 0.04, 1.0);
            metallic  = std::clamp(metallic,  0.0,  1.0);

            double occlusion = 1.0;
            if (occlusionTexture) {
                color occ = occlusionTexture->value(rec.u, rec.v, rec.p);
                occlusion = 1.0 + occlusionStrength * (occ.x() - 1.0);
            }

            vec3 N = rec.normal;
            if (!doubleSided && !rec.front_face) N = -N;
            if (normalTexture)
                N = apply_normal_map(normalTexture, rec, normalScale);

            double f0_dielectric = std::pow((ior - 1.0) / (ior + 1.0), 2.0);
            color  F0 = color(f0_dielectric, f0_dielectric, f0_dielectric);
            F0 = F0 * (1.0 - metallic) + baseColor * metallic;

            double F0_avg        = (F0.x() + F0.y() + F0.z()) / 3.0;
            double p_specular    = F0_avg + (1.0 - F0_avg) * metallic * 0.9;
            bool   do_specular   = random_double() < p_specular;

            if (do_specular) {
                double alpha_ggx = roughness * roughness;
                vec3 H  = sample_ggx(N, alpha_ggx);
                vec3 wo = unit_vector(r_in.direction());
                vec3 wi = reflect(wo, H);

                if (dot(wi, N) <= 0.0) {
                    srec.attenuation = baseColor * (1.0 - metallic) * occlusion;
                    srec.pdf_ptr     = make_shared<cosine_pdf>(N);
                    srec.skip_pdf    = false;
                    return true;
                }

                double NdotH = std::max(0.0, dot(N, H));
                double NdotL = std::max(0.0, dot(N, wi));
                double NdotV = std::max(1e-4, dot(N,-wo));
                double VdotH = std::max(0.0, dot(-wo, H));

                color  F = fresnel_schlick(VdotH, F0);
                double G = ggx_G(NdotV, NdotL, roughness);
                double D = ggx_D(NdotH, alpha_ggx);

                color weight = (NdotH > 1e-4 && VdotH > 1e-4)
                    ? F * G * VdotH / (NdotV * NdotH)
                    : color(0,0,0);

                srec.attenuation = weight * occlusion / p_specular;
                srec.pdf_ptr     = nullptr;
                srec.skip_pdf    = true;
                srec.skip_pdf_ray= ray(rec.p, wi, r_in.time());

            } else {
                color diffuse_albedo = baseColor * (1.0 - metallic);
                srec.attenuation = diffuse_albedo * occlusion / (1.0 - p_specular);
                srec.pdf_ptr     = make_shared<cosine_pdf>(N);
                srec.skip_pdf    = false;
            }

            return true;
        }

        double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) 
        const override {
            vec3 N = normalTexture
                ? apply_normal_map(normalTexture, rec, normalScale)
                : rec.normal;
            double cos_theta = dot(N, unit_vector(scattered.direction()));
            return cos_theta < 0 ? 0 : cos_theta / pi;
        }

};