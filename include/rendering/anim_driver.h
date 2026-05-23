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
#include <cstdlib>

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


inline bool assemble_video(const std::string& frames_dir, const std::string& out_path, int fps) {
    std::ostringstream cmd;
    cmd << "ffmpeg -y"
        << " -r " << fps
        << " -i \"" << frames_dir << "/frame_%04d.png\""
        << " -c:v libx264"
        << " -pix_fmt yuv420p"
        << " -crf 18"
        << " \"" << out_path << "\""
        << " -loglevel error";

        int ret = std::system(cmd.str().c_str());
        return ret == 0;
}

inline void delete_frames(const std::string& frames_dir)
{
    std::filesystem::remove_all(frames_dir);
}

inline void run_animation(const AnimData& anim,
                          const render_config& cfg,
                          const std::string& out_dir,
                          bool keep_frames = false)
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

    std::string video_path = std::filesystem::path(out_dir).parent_path().string() + "/output.mp4";
    Logger::task_start("Assembling video: " + video_path);
    if (assemble_video(out_dir, video_path, anim.fps)) {
        Logger::task_end();
        Logger::success("Saved " + video_path);
    } else {
        Logger::task_end("Failed.");
        Logger::error("ffmpeg failed — frames kept in " + out_dir);
        return;
    }

    if (!keep_frames) {
        delete_frames(out_dir);
        Logger::info("Frame cache deleted.");
    } else {
        Logger::info("Frames kept at " + out_dir);
    }

    Logger::success("Frames saved to " + out_dir);
}