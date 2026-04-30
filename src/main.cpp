#include "camera.h"
#include "scene_loader.h"
#include <filesystem>

int main (int argc, char* argv[]) {
    std::string config_path = "config.toml";

    int first_flag = 1;
    if (argc > 1 && argv[1][0] != '-') {
        config_path = argv[1];
        first_flag = 2;
    }

    std::clog << "Loading config: " << config_path << "\n";

    render_config cfg;

    try {
        cfg = load_config(config_path);
    } catch (const std::exception& e) {
        std::cerr << "Config error: " << e.what() << "\n";
        return 1;
    }

    for (int i = first_flag ; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--scene"  && i+1 < argc) cfg.scene_path         = argv[++i];
        if (arg == "--output" && i+1 < argc) cfg.output_file        = argv[++i];
        if (arg == "--spp"    && i+1 < argc) cfg.samples_per_pixel  = std::stoi(argv[++i]);
        if (arg == "--width"  && i+1 < argc) cfg.image_width        = std::stoi(argv[++i]);
        if (arg == "--depth"  && i+1 < argc) cfg.max_depth          = std::stoi(argv[++i]);
    }
    
    hittable_list world, lights;
    scene_config scn;

    std::clog << "Loading scene: " << cfg.scene_path << "\n";

    try {
        scn = load_scene(cfg.scene_path, world, lights);
    } catch (const std::exception& e) {
        std::cerr << "Scene error: " << e.what() << "\n";
        return 1;
    }

    std::clog << "Starting render...\n";

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

    std::filesystem::create_directories(
        std::filesystem::path(cfg.output_file).parent_path()
    );

    cam.render(world, lights, cfg.output_file);

    return 0;
}