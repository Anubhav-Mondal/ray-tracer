#include "rendering/camera.h"
#include "io/scene_loader.h"
#include "io/logger.h"
#include "io/anim_loader.h"
#include "rendering/anim_evaluator.h"
#include "rendering/anim_driver.h"
#include <filesystem>

int main (int argc, char* argv[]) {
    std::string config_path = "config.toml";

    int first_flag = 1;
    if (argc > 1 && argv[1][0] != '-') {
        config_path = argv[1];
        first_flag = 2;
    }

    Logger::task_start("Loading config: " + config_path);

    render_config cfg;

    try {
        cfg = load_config(config_path);
    } catch (const std::exception& e) {
        Logger::error(std::string("Config error: ") + e.what());
        return 1;
    }

    Logger::task_end();

    for (int i = first_flag; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--denoise")       cfg.denoise     = true;
        if (arg == "--no-denoise")    cfg.denoise     = false;
        if (arg == "--save-aov")      cfg.save_aov    = true;
        if (arg == "--no-save-aov")   cfg.save_aov    = false;
        if (arg == "--anim")          cfg.anim        = true;
        if (arg == "--no-anim")       cfg.anim        = false;
        if (arg == "--keep-frame")    cfg.keep_frames = true;
        if (arg == "--no-keep-frame") cfg.keep_frames = false;

        if (arg == "--scene-path"  && i+1 < argc) cfg.scene_path         = argv[++i];
        if (arg == "--output"      && i+1 < argc) cfg.output_file        = argv[++i];
        if (arg == "--spp"         && i+1 < argc) cfg.samples_per_pixel  = std::stoi(argv[++i]);
        if (arg == "--width"       && i+1 < argc) cfg.image_width        = std::stoi(argv[++i]);
        if (arg == "--depth"       && i+1 < argc) cfg.max_depth          = std::stoi(argv[++i]);
        
        if (arg == "--anim-path"   && i+1 < argc) cfg.anim_path         = argv[++i];
        if (arg == "--anim-output" && i+1 < argc) cfg.anim_output       = argv[++i];
        if (arg == "--fps"         && i+1 < argc) cfg.anim_fps          = std::stoi(argv[++i]);
    }


    if (cfg.anim) {
        Logger::task_start("Loading animation: " + cfg.anim_path);
        AnimData anim;
        try {
            anim = load_anim(cfg.anim_path);
        } catch (const std::exception& e) {
            Logger::error(std::string("Animation error: ") + e.what());
            return 1;
        }

        if (cfg.anim_fps > 0) {
            anim.fps         = cfg.anim_fps;
            anim.frame_count = static_cast<int>(anim.fps * anim.duration);
        }

        Logger::task_end();

        Logger::info(
            std::to_string(anim.frame_count) + " frames | " +
            std::to_string(anim.fps) + " fps | " +
            std::to_string(static_cast<int>(anim.duration)) + "s"
        );
 
        std::string out_dir = std::filesystem::path(cfg.anim_output).parent_path().string() + "/frames";

        run_animation(anim, cfg, out_dir);
        return 0;
    }

    hittable_list world, lights;
    scene_config scn;

    Logger::task_start("Loading scene: " + cfg.scene_path);

    try {
        scn = load_scene(cfg.scene_path, world, lights);
    } catch (const std::exception& e) {
        Logger::error(std::string("Scene error: ") + e.what());
        return 1;
    }

    Logger::task_end();

    int count = world.objects.size();
    Logger::info("Scene initialized with " + std::to_string(count) + (count == 1 ? " object." : " objects."));
    std::clog << "\n";

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

    std::filesystem::create_directories(
        std::filesystem::path(cfg.output_file).parent_path()
    );

    cam.render(world, lights, cfg.output_file);

    return 0;
}