#pragma once

#include "io/anim_loader.h"
#include "io/scene_loader.h"
#include "io/logger.h"
#include "rendering/anim_evaluator.h"
#include "rendering/camera.h"
#include "geometry/hittable_list.h"

#include <filesystem>
#include <string>
#include <map>
#include <sstream>
#include <iomanip>

inline void patch_camera(scene_config& scn, const std::map<std::string, double>& values) {
    auto get = [&](const std::string& key, double fallback) -> double {
        auto it = values.find("camera." + key);
        return (it != values.end()) ? it->second : fallback;
    };

    auto patch_vec = [&](const std::string& base, vec3& v) {
        auto xi = values.find("camera." + base + ".x");
        auto yi = values.find("camera." + base + ".y");
        auto zi = values.find("camera." + base + ".z");
        if (xi != values.end()) v.e[0] = xi->second;
        if (yi != values.end()) v.e[1] = yi->second;
        if (zi != values.end()) v.e[2] = zi->second;
    };

    patch_vec("lookfrom", scn.lookfrom);
    patch_vec("lookat",   scn.lookat);

    auto vfov_it = values.find("camera.vfov");
    if (vfov_it != values.end()) scn.vfov = vfov_it->second;
}

struct ObjectAnim {
    std::string name;
    vec3  translate = {0, 0, 0};
    double rotate_y = 0.0;
    double scale    = 1.0;
};

inline void patch_object_anim(ObjectAnim& oa, const std::map<std::string, double>& values) {
    std::string pfx = oa.name + ".";

    auto patch = [&](const std::string& key, double& target) {
        auto it = values.find(pfx + key);
        if (it != values.end()) target = it->second;
    };

    patch("translate.x", oa.translate.e[0]);
    patch("translate.y", oa.translate.e[1]);
    patch("translate.z", oa.translate.e[2]);
    patch("rotate_y",    oa.rotate_y);
    patch("scale",       oa.scale);
}

inline std::string frame_output_path(const std::string& out_dir, int frame) {
    std::ostringstream ss;
    ss << out_dir << "/frame_" << std::setw(4) << std::setfill('0') << frame << ".png";
    return ss.str();
}

inline void run_animation(const AnimData& anim,
                          const render_config& cfg,
                          const std::string& out_dir)
{
    std::filesystem::create_directories(out_dir);
    auto anim_start = std::chrono::steady_clock::now();

    for (int f = 0; f < anim.frame_count; ++f) {
        auto values = evaluate_all(anim, f);

        hittable_list world, lights;
        scene_config  scn;
        scn = load_scene(cfg.scene_path, world, lights, values);

        camera cam(scn.skybox.c_str());
        cam.aspect_ratio      = cfg.aspect_ratio;
        cam.image_width       = cfg.image_width;
        cam.samples_per_pixel = cfg.samples_per_pixel;
        cam.max_depth         = cfg.max_depth;
        cam.background        = scn.background;
        cam.vfov              = scn.vfov;
        cam.lookfrom          = scn.lookfrom;
        cam.lookat            = scn.lookat;
        cam.vup               = scn.vup;
        cam.defocus_angle     = scn.defocus_angle;
        cam.focus_dist        = scn.focus_dist;
        cam.denoise           = cfg.denoise;
        cam.save_aov          = cfg.save_aov;

        std::ostringstream ss;
        ss << out_dir << "/frame_" << std::setw(4) << std::setfill('0') << f << ".png";
        std::string out_path = ss.str();

        cam.render(world, lights, out_path);
    }

    Logger::success("Frames saved to " + out_dir);
}