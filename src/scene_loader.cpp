#include "io/scene_loader.h"
#include "io/mesh_loader.h"
#include "material/material.h"
#include "material/texture.h"
#include "geometry/sphere.h"
#include "geometry/quad.h"
#include "geometry/hittable_list.h"
#include "geometry/bvh.h"
#include "toml.hpp"

#include <map>
#include <string>
#include <stdexcept>

// ============= //
//    HELPERS    //
// ============= //

template <typename NodeView>
static color parse_color(const toml::node_view<NodeView>& node,
                         const color& fallback = color(1, 1, 1)) {
    if (auto* arr = node.as_array()) {
        return color(
            arr->get(0)->value_or(fallback.x()),
            arr->get(1)->value_or(fallback.y()),
            arr->get(2)->value_or(fallback.z())
        );
    }
    return fallback;
}

template <typename NodeView>
static vec3 parse_vec3(const toml::node_view<NodeView>& node,
                       const vec3& fallback = vec3(0, 0, 0)) {
    if (auto* arr = node.as_array()) {
        return vec3(
            arr->get(0)->value_or(fallback.x()),
            arr->get(1)->value_or(fallback.y()),
            arr->get(2)->value_or(fallback.z())
        );
    }
    return fallback;
}

// ===================== //
//    Texture Parsing    //
// ===================== //

static shared_ptr<texture> parse_texture(const toml::table& t) {
    std::string type = t["type"].value_or<std::string>("");

    if (type == "image") {
        std::string path = t["path"].value_or<std::string>("");
        if (path.empty()) throw std::runtime_error("image texture missing 'path'");
        return make_shared<image_texture>(path.c_str());
    }

    if (type == "checker") {
        double scale = t["scale"].value_or(10.0);
        color even   = parse_color(t["even"], color(0.9, 0.9, 0.9));
        color odd    = parse_color(t["odd"],  color(0.1, 0.1, 0.1));
        return make_shared<checker_texture>(scale, even, odd);
    }

    if (type == "solid") {
        color c = parse_color(t["color"]);
        return make_shared<solid_color>(c);
    }

    throw std::runtime_error("Unknown texture type: " + type);
}

static shared_ptr<texture> parse_albedo_field(const toml::table& t,
                                              const std::string& key = "albedo",
                                              const color& fallback = color(1, 1, 1)) {
    auto node = t[key];
    if (!node) return make_shared<solid_color>(fallback);

    if (node.is_array())
        return make_shared<solid_color>(parse_color(node, fallback));

    if (node.is_table())
        return parse_texture(*node.as_table());

    return make_shared<solid_color>(fallback);
}

// ==================== //
//   Material Parsing   //
// ==================== //

static shared_ptr<material> parse_material(
        const toml::table& t,
        const std::string& owner_name,
        const std::map<std::string, double>& overrides)
{
    std::string type = t["type"].value_or<std::string>("");

    auto get = [&](const std::string& prop, double toml_val) -> double {
        if (owner_name.empty()) return toml_val;
        auto it = overrides.find(owner_name + "." + prop);
        return (it != overrides.end()) ? it->second : toml_val;
    };

    auto patch_color = [&](color c, const std::string& prefix) -> color {
        if (owner_name.empty()) return c;
        auto ri = overrides.find(owner_name + "." + prefix + ".r");
        auto gi = overrides.find(owner_name + "." + prefix + ".g");
        auto bi = overrides.find(owner_name + "." + prefix + ".b");
        if (ri != overrides.end()) c.e[0] = ri->second;
        if (gi != overrides.end()) c.e[1] = gi->second;
        if (bi != overrides.end()) c.e[2] = bi->second;
        return c;
    };

    if (type == "lambertian") {
        color base = parse_color(t["albedo"], color(0.8, 0.8, 0.8));
        base = patch_color(base, "material.color");
        auto tex = make_shared<solid_color>(base);
        auto mat = make_shared<lambertian>(tex);
        if (auto nm = t["normal_map"].value<std::string>())
            mat->add_normal_map(make_shared<image_texture>(nm->c_str()));
        return mat;
    }

    if (type == "metal") {
        color  albedo = parse_color(t["albedo"], color(0.8, 0.8, 0.8));
        albedo = patch_color(albedo, "material.color");
        double fuzz = get("material.fuzz", t["fuzz"].value_or(0.0));
        auto   mat  = make_shared<metal>(albedo, fuzz);
        if (auto p = t["albedo_map"].value<std::string>())
            mat->add_albedo_map(make_shared<image_texture>(p->c_str()));
        if (auto p = t["fuzz_map"].value<std::string>())
            mat->add_fuzz_map(make_shared<image_texture>(p->c_str()));
        if (auto p = t["normal_map"].value<std::string>())
            mat->add_normal_map(make_shared<image_texture>(p->c_str()));
        return mat;
    }

    if (type == "dielectric") {
        double ior  = get("material.ior", t["ior"].value_or(1.5));
        color  tint = parse_color(t["tint"], color(1, 1, 1));
        tint = patch_color(tint, "material.tint");
        auto mat = make_shared<dielectric>(ior, tint);
        if (auto s = t["absorption_strength"].value<double>())
            mat->set_absorption_strength(get("material.absorption_strength", *s));
        if (auto p = t["albedo_map"].value<std::string>())
            mat->add_albedo_map(make_shared<image_texture>(p->c_str()));
        if (auto p = t["normal_map"].value<std::string>())
            mat->add_normal_map(make_shared<image_texture>(p->c_str()));
        return mat;
    }

    if (type == "frosted_glass") {
        double ior        = get("material.ior",        t["ior"].value_or(1.5));
        double roughness  = get("material.roughness",  t["roughness"].value_or(0.1));
        double subsurface = get("material.subsurface", t["subsurface"].value_or(0.0));
        color  tint       = patch_color(parse_color(t["tint"], color(1, 1, 1)), "material.tint");
        auto   mat        = make_shared<frosted_glass>(ior, roughness, subsurface, tint);
        if (auto s = t["absorption_strength"].value<double>())
            mat->set_absorption_strength(get("material.absorption_strength", *s));
        if (auto b = t["two_sided"].value<bool>())
            mat->set_two_sided(*b);
        if (auto p = t["albedo_map"].value<std::string>())
            mat->add_albedo_map(make_shared<image_texture>(p->c_str()));
        if (auto p = t["normal_map"].value<std::string>())
            mat->add_normal_map(make_shared<image_texture>(p->c_str()));
        return mat;
    }

    if (type == "glossy") {
        color  albedo    = patch_color(parse_color(t["color"], color(0.8, 0.8, 0.8)), "material.color");
        double roughness = get("material.roughness", t["roughness"].value_or(0.1));
        double specular  = get("material.specular_strength", t["specular_strength"].value_or(0.8));
        auto   mat       = make_shared<glossy>(albedo, roughness, specular);
        if (auto p = t["albedo_map"].value<std::string>())
            mat->add_albedo_map(make_shared<image_texture>(p->c_str()));
        if (auto p = t["normal_map"].value<std::string>())
            mat->add_normal_map(make_shared<image_texture>(p->c_str()));
        return mat;
    }

    if (type == "diffuse_light") {
        color emit = patch_color(parse_color(t["color"], color(15, 15, 15)), "material.color");
        auto  mat  = make_shared<diffuse_light>(emit);
        if (auto i = t["intensity"].value<double>())
            mat->set_intensity(get("material.intensity", *i));
        return mat;
    }

    if (type == "isotropic") {
        auto tex = parse_albedo_field(t, "albedo", color(1, 1, 1));
        return make_shared<isotropic>(tex);
    }

    throw std::runtime_error("Unknown material type: '" + type + "'");
}

// ==================== //
//    OBJECT Parsing    //
// ==================== //

static shared_ptr<hittable> apply_transforms(
        shared_ptr<hittable> obj,
        const toml::table& t,
        const std::string& name,
        const std::map<std::string, double>& overrides)
{
    auto get = [&](const std::string& prop, double toml_val) -> double {
        if (name.empty()) return toml_val;
        auto it = overrides.find(name + "." + prop);
        return (it != overrides.end()) ? it->second : toml_val;
    };

    if (auto s = t["scale"].value<double>()) {
        double sv = get("scale", *s);
        obj = make_shared<scale>(obj, sv);
    }

    if (auto ry = t["rotate_y"].value<double>()) {
        double rv = get("rotate_y", *ry);
        obj = make_shared<rotate_y>(obj, rv);
    }

    if (t["translate"].is_array()) {
        vec3 base = parse_vec3(t["translate"]);
        base.e[0] = get("translate.x", base.e[0]);
        base.e[1] = get("translate.y", base.e[1]);
        base.e[2] = get("translate.z", base.e[2]);
        obj = make_shared<translate>(obj, base);
    }

    return obj;
}

static shared_ptr<hittable> parse_object(
        const toml::table& t,
        const std::map<std::string, shared_ptr<material>>& mat_map,
        const std::map<std::string, double>& overrides)
{
    std::string type = t["type"].value_or<std::string>("");
    std::string name = t["name"].value_or<std::string>("");

    shared_ptr<material> mat = nullptr;
    if (auto mat_name = t["material"].value<std::string>()) {
        auto it = mat_map.find(*mat_name);
        if (it == mat_map.end())
            throw std::runtime_error("Material not found: '" + *mat_name + "'");

        mat = it->second;
    }

    if (type == "mesh") {
        std::string path = t["path"].value_or<std::string>("");
        if (path.empty()) throw std::runtime_error("mesh object missing 'path'");
        auto obj = load_mesh(path, mat);
        return apply_transforms(obj, t, name, overrides);
    }

    if (type == "sphere") {
        point3 center = parse_vec3(t["center"]);
        double radius = t["radius"].value_or(1.0);

        if (!name.empty()) {
            auto it = overrides.find(name + ".radius");
            if (it != overrides.end()) radius = it->second;
        }

        auto obj = make_shared<sphere>(center, radius, mat);
        return apply_transforms(obj, t, name, overrides);
    }

    if (type == "quad") {
        point3 origin = parse_vec3(t["origin"]);
        vec3   u      = parse_vec3(t["u"]);
        vec3   v      = parse_vec3(t["v"]);
        auto obj = make_shared<quad>(origin, u, v, mat);
        return apply_transforms(obj, t, name, overrides);
    }

    if (type == "box") {
        point3 p_min = parse_vec3(t["min"]);
        point3 p_max = parse_vec3(t["max"]);
        auto obj = box(p_min, p_max, mat);
        return apply_transforms(obj, t, name, overrides);
    }

    throw std::runtime_error("Unknown object type: '" + type + "'");
}

// ===================== //
//  PUBLIC: load_config  //
// ===================== //

render_config load_config(const std::string& path) {
    toml::table tbl;
    try {
        tbl = toml::parse_file(path);
    } catch (const toml::parse_error& e) {
        throw std::runtime_error(std::string("Failed to parse config: ") + e.what());
    }

    render_config cfg;
    cfg.scene_path        = tbl["scene_path"].value_or<std::string>("scene.toml");
    cfg.output_file       = tbl["output"].value_or<std::string>("output.png");
    cfg.image_width       = tbl["width"].value_or(800);
    cfg.samples_per_pixel = tbl["samples_per_pixel"].value_or(100);
    cfg.max_depth         = tbl["max_depth"].value_or(10);
    cfg.aspect_ratio      = tbl["aspect"].value_or(1.0);
    cfg.denoise           = tbl["denoise"].value_or(true);
    cfg.save_aov          = tbl["save_aov"].value_or(false);

    cfg.anim              = tbl["anim"].value_or(false);
    cfg.anim_fps          = tbl["anim_fps"].value_or(0);
    cfg.anim_path         = tbl["anim_path"].value_or<std::string>("scenes/scene.anim.toml");
    cfg.anim_output       = tbl["anim_output"].value_or<std::string>("renders/output.mp4");
    cfg.keep_frames       = tbl["keep_frames"].value_or(false);

    return cfg;
}

// ==================== //
//  PUBLIC: load_scene  //
// ==================== //

scene_config load_scene(const std::string& path,
                        hittable_list& world,
                        hittable_list& lights,
                        const std::map<std::string, double>& overrides)
{
    toml::table tbl;
    try {
        tbl = toml::parse_file(path);
    } catch (const toml::parse_error& e) {
        throw std::runtime_error(std::string("Failed to parse scene: ") + e.what());
    }

    std::map<std::string, shared_ptr<material>>    mat_map;
    std::map<std::string, const toml::table*>      mat_raw;

    const auto* mat_arr = tbl.get_as<toml::array>("material");
    if (mat_arr) {
        for (auto& entry : *mat_arr) {
            const auto* mt = entry.as_table();
            if (mt) {
                std::string mat_name = (*mt)["name"].value_or<std::string>("");
                if (mat_name.empty())
                    throw std::runtime_error("A [[material]] is missing a 'name' field");
                mat_map[mat_name] = parse_material(*mt, "", overrides);
                mat_raw[mat_name] = mt;
            }
        }
    }

    const auto* obj_arr = tbl.get_as<toml::array>("object");

    if (obj_arr) {
        for (auto& entry : *obj_arr) {
            if (auto* ot = entry.as_table()) {
                std::string obj_name = (*ot)["name"].value_or<std::string>("");

                std::map<std::string, shared_ptr<material>> mat_map_frame = mat_map;
                if (auto mat_ref = (*ot)["material"].value<std::string>()) {
                    auto raw_it = mat_raw.find(*mat_ref);
                    if (raw_it != mat_raw.end() && !obj_name.empty()) {
                        mat_map_frame[*mat_ref] =
                            parse_material(*raw_it->second, obj_name, overrides);
                    }
                }

                auto obj = parse_object(*ot, mat_map_frame, overrides);
                world.add(obj);

                bool is_light = (*ot)["is_light"].value_or(false);
                if (is_light) {
                    std::string type = (*ot)["type"].value_or<std::string>("");
                    auto empty_mat   = shared_ptr<material>();
                    if (type == "quad") {
                        point3 origin = parse_vec3((*ot)["origin"]);
                        vec3   u      = parse_vec3((*ot)["u"]);
                        vec3   v      = parse_vec3((*ot)["v"]);
                        lights.add(make_shared<quad>(origin, u, v, empty_mat));
                    }
                }
            }
        }
    }

    // Camera
    scene_config scn;
    if (auto* cam_tbl = tbl["camera"].as_table()) {
        auto& c   = *cam_tbl;
        scn.vfov  = c["vfov"].value_or(40.0);

        scn.lookfrom      = parse_vec3(c["lookfrom"], point3(0, 0, 1));
        scn.lookat        = parse_vec3(c["lookat"],   point3(0, 0, 0));
        scn.vup           = parse_vec3(c["vup"],      vec3(0, 1, 0));
        scn.defocus_angle = c["defocus_angle"].value_or(0.0);
        scn.focus_dist    = c["focus_dist"].value_or(10.0);

        auto patch_vec = [&](const std::string& base, vec3& v) {
            auto xi = overrides.find("camera." + base + ".x");
            auto yi = overrides.find("camera." + base + ".y");
            auto zi = overrides.find("camera." + base + ".z");
            if (xi != overrides.end()) v.e[0] = xi->second;
            if (yi != overrides.end()) v.e[1] = yi->second;
            if (zi != overrides.end()) v.e[2] = zi->second;
        };
        patch_vec("lookfrom", scn.lookfrom);
        patch_vec("lookat",   scn.lookat);

        auto vfov_it = overrides.find("camera.vfov");
        if (vfov_it != overrides.end()) scn.vfov = vfov_it->second;
    }

    // Environment
    if (auto* env_tbl = tbl["environment"].as_table()) {
        scn.skybox     = (*env_tbl)["skybox"].value_or<std::string>("");
        scn.background = parse_color((*env_tbl)["background"], color(0, 0, 0));
    }

    return scn;
}