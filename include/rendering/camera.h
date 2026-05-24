#ifndef CAMERA_H
#define CAMERA_H

#include "core/pdf.h"
#include "geometry/hittable.h"
#include "material/material.h"
#include "io/logger.h"
#include "utils/denoiser.h"
#include "stb_image_write.h"
#include <string>
#include <algorithm>
#include <omp.h>
#include <chrono>
#include <iomanip>
#include <functional>

class camera {
    public:
        double aspect_ratio = 1.0;         // Ratio of image width over height
        int image_width  = 100;            // Rendered image width in pixel count
        int samples_per_pixel = 10;        // Count of random samples for each pixel
        int max_depth = 10;                // Maximum number of ray bounces into scene
        color background;                  // Scene Background Color

        double vfov = 90;
        point3 lookfrom = point3(0,0,0);   // Point camera is looking from
        point3 lookat   = point3(0,0,-1);  // Point camera is looking at
        vec3   vup      = vec3(0,1,0);     // Camera-relative "up" direction

        double defocus_angle = 0;          // Variation angle of rays through each pixel
        double focus_dist = 10;            // Distance from camera lookfrom point to plane of perfect focus
        
        bool denoise = true;               // Whether to apply AI denoising to the rendered image
        bool save_aov = false;             // Whether to save Arbitrary Output Variables (AOVs) such as albedo and normal maps
        int jpg_quality = 90;

        bool silent = false;
        std::function<void(int,int)> on_progress = nullptr;

        camera() : skybox("") {}
        
        // Constructor with skybox filename
        camera(const char* skybox_filename) : skybox(skybox_filename) {}


        void render(const hittable& world, const hittable& lights, const std::string& output_file="output.png") {
            std::string ext = "";
            size_t dot = output_file.find_last_of('.');
            if (dot != std::string::npos) {
                ext = output_file.substr(dot + 1);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            }

            if (ext == "jpg" || ext == "jpeg")      save_ext = save_extension::jpg;
            else if (ext == "hdr")                  save_ext = save_extension::hdr;
            else                                    save_ext = save_extension::png;

            std::string output_name = (dot != std::string::npos) ? output_file.substr(0, dot) : output_file;

            initialize();

            auto start_time = std::chrono::steady_clock::now();
            std::atomic<int> completed_rows(0);

            if (!silent)
            Logger::stage("RENDERING (" + std::to_string(image_width) + "x" + 
                std::to_string(image_height) + ", " + 
                std::to_string(samples_per_pixel) + " SPP)");

            #pragma omp parallel for schedule(dynamic, 2)
            for (int j = 0; j < image_height; j++) {
                for (int i = 0; i < image_width; i++) {
                    color pixel_color(0, 0, 0);
                    color pixel_albedo(0, 0, 0);
                    vec3 pixel_normal(0, 0, 0);

                    for (int s_j = 0; s_j < sqrt_spp; s_j++) {
                        for (int s_i = 0; s_i < sqrt_spp; s_i++) {
                            ray r = get_ray(i, j, s_i, s_j);

                            if (s_i==0 && s_j==0) {
                                color first_sample(0, 0, 0);
                                first_sample = ray_color_aov(r, max_depth, world, lights,
                                                            pixel_albedo, pixel_normal);
                                pixel_color += first_sample;
                            } else {
                                pixel_color += ray_color(r, max_depth, world, lights);
                            }
                        }
                    }

                    int idx = (j * image_width + i) * 3;
                    write_hdr_pixel(hdr_buffer, pixel_color * pixel_samples_scale, (j * image_width + i) * 3);

                    albedo_buffer[idx+0] = static_cast<float> (pixel_albedo.x());
                    albedo_buffer[idx+1] = static_cast<float> (pixel_albedo.y());
                    albedo_buffer[idx+2] = static_cast<float> (pixel_albedo.z());
                    
                    normal_buffer[idx+0] = static_cast<float> (pixel_normal.x());
                    normal_buffer[idx+1] = static_cast<float> (pixel_normal.y());
                    normal_buffer[idx+2] = static_cast<float> (pixel_normal.z());
                }
                int done = ++completed_rows;

                if (done % (image_height / 100 + 1) == 0 || done == image_height) {
                    if (on_progress) {
                        #pragma omp critical
                        { on_progress(done, image_height); }
                    } else if (!silent) {
                        #pragma omp critical
                        { Logger::update_progress(done, image_height, start_time); }
                    }
                }
            }

            if (!silent){
                std::clog << "\n";
                Logger::finish_render(start_time);
            }

            if (denoise) {
                if (!silent) Logger::task_start("Running OIDN Denoiser");
                oidn_denoise(hdr_buffer, image_width, image_height, albedo_buffer, normal_buffer);
                if (!silent) Logger::task_end();
            }

            if (save_ext != save_extension::hdr) {
                hdr_to_ldr(hdr_buffer, ldr_buffer);
            }
            
            std::string out_ext = (save_ext == save_extension::jpg) ? ".jpg" 
            : (save_ext == save_extension::hdr) ? ".hdr" 
            : ".png";
            std::string out = output_name + out_ext;
            
            if (!silent) Logger::task_start("Saving image to " + out);

            if (save_ext == save_extension::hdr) {
                int result = stbi_write_hdr(out.c_str(), image_width, image_height, 3, hdr_buffer.data());
                if (!result) {
                    if (!silent) Logger::error("Failed to save " + out);
                    return;
                }
            }
            else if (save_ext == save_extension::jpg) {
                int result = stbi_write_jpg(out.c_str(), image_width, image_height, 3, ldr_buffer.data(), jpg_quality);
                if (!result) {
                    if (!silent) Logger::error("Failed to save " + out);
                    return;
                }
            } else {
                int result = stbi_write_png(out.c_str(), image_width, image_height, 3, ldr_buffer.data(), image_width * 3);
                if (!result) {
                    if (!silent) Logger::error("Failed to save " + out);
                    return;
                }
            }

            if (!silent) Logger::task_end();
            if (!silent) Logger::success("Saved " + out + " successfully.");

            if (save_aov) {
                if (!silent) Logger::task_start("Saving Albedo Buffer");
                save_debug_image(albedo_buffer, output_name + "_albedo.png", false);
                if (!silent) Logger::task_end();
                if (!silent) Logger::task_start("Saving Normal Buffer");
                save_debug_image(normal_buffer, output_name + "_normal.png", true);
                if (!silent) Logger::task_end();
            }
        }

    private:
        int    image_height;         // Rendered image height
        double pixel_samples_scale;  // Color scale factor for a sum of pixel samples
        int    sqrt_spp;             // Square root of number of samples per pixel
        double recip_sqrt_spp;  
        point3 center;               // Camera center
        point3 pixel00_loc;          // Location of pixel 0, 0
        vec3   pixel_delta_u;        // Offset to pixel to the right
        vec3   pixel_delta_v;        // Offset to pixel below
        vec3   u, v, w;
        vec3   defocus_disk_u;       // Defocus disk horizontal radius
        vec3   defocus_disk_v;       // Defocus disk vertical radius
        rtw_image skybox;

        std::vector<unsigned char> ldr_buffer;
        std::vector<float> hdr_buffer;

        std::vector<float> albedo_buffer;
        std::vector<float> normal_buffer;

        enum class save_extension { png, jpg, hdr};
        save_extension save_ext = save_extension::png;

        void initialize() {
            image_height = int(image_width / aspect_ratio);
            image_height = (image_height < 1) ? 1 : image_height;

            sqrt_spp = int(std::sqrt(samples_per_pixel));
            pixel_samples_scale = 1.0 / (sqrt_spp * sqrt_spp);
            recip_sqrt_spp = 1.0 / sqrt_spp;

            ldr_buffer.resize(image_width * image_height * 3, 0);
            hdr_buffer.resize(image_width * image_height * 3, 0);

            albedo_buffer.assign(image_width * image_height * 3, 0.0f);
            normal_buffer.assign(image_width * image_height * 3, 0.0f);

            center = lookfrom;

            // Determine viewport dimensions.
            auto theta = degrees_to_radians(vfov);
            auto h = std::tan(theta/2);
            auto viewport_height = 2 * h * focus_dist;
            auto viewport_width = viewport_height * (double(image_width)/image_height);

            w = unit_vector(lookfrom - lookat);
            u = unit_vector(cross(vup, w));
            v = cross(w, u);

            // Calculate the vectors across the horizontal and down the vertical viewport edges.
            vec3 viewport_u = viewport_width * u;
            vec3 viewport_v = viewport_height * -v;

            // Calculate the horizontal and vertical delta vectors from pixel to pixel.
            pixel_delta_u = viewport_u / image_width;
            pixel_delta_v = viewport_v / image_height;

            // Calculate the location of the upper left pixel.
            auto viewport_upper_left = center - (focus_dist * w) - viewport_u/2 - viewport_v/2;
            pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

            auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
            defocus_disk_u = u * defocus_radius;
            defocus_disk_v = v * defocus_radius;
        }

        ray get_ray(int i, int j, int s_i, int s_j) const {
            auto offset = sample_square_stratified(s_i, s_j);
            auto pixel_sample = pixel00_loc
                            + ((i + offset.x()) * pixel_delta_u)
                            + ((j + offset.y()) * pixel_delta_v);

            auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
            auto ray_direction = pixel_sample - ray_origin;
            auto ray_time = random_double();

            return ray(ray_origin, ray_direction, ray_time);
        }
        vec3 sample_square_stratified(int s_i, int s_j) const {
            auto px = ((s_i + random_double()) * recip_sqrt_spp) - 0.5;
            auto py = ((s_j + random_double()) * recip_sqrt_spp) - 0.5;

            return vec3(px, py, 0);
        }

        vec3 sample_square() const {
            // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
            return vec3(random_double() - 0.5, random_double() - 0.5, 0);
        }

        point3 defocus_disk_sample() const {
            // Returns a random point in the camera defocus disk.
            auto p = random_in_unit_disk();
            return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
        }

        color sample_skybox(const ray& r) const {
            constexpr double PI = 3.14159265358979323846;

            if (skybox.height() <= 0)
                return background;

            vec3 dir = r.direction();

            if (dir.length_squared() < 1e-8) {
                return background;
            }

            dir = unit_vector(dir);

            double dy = std::clamp(dir.y(), -1.0, 1.0);

            double u = 0.5f + atan2(dir.z(), dir.x()) / (2 * PI);
            double v = 0.5f - asin(dy) / PI;

            if (!std::isfinite(u) || !std::isfinite(v))
                return background;
            
            auto i = std::min(std::max(int(u * skybox.width()),  0), skybox.width()  - 1);
            auto j = std::min(std::max(int(v * skybox.height()), 0), skybox.height() - 1);

            try {
                if (skybox.is_hdr_image()) {
                    auto pixel = skybox.pixel_data_float(i, j); 
                    return color(pixel[0], pixel[1], pixel[2]);
                } else {
                    auto pixel = skybox.pixel_data(i, j);
                    auto color_scale = 1.0 / 255.0;
                    return color(color_scale*pixel[0], color_scale*pixel[1], color_scale*pixel[2]);
                }
            } catch (...) {
                if (!silent) std::cout << "Exception caught in skybox access!" << std::endl;
                return background;
            }
        }

        color ray_color(const ray& r, int depth, const hittable& world, const hittable& lights) const {
            if (depth <= 0)
                return color(0,0,0);

            hit_record rec;

            if (!world.hit(r, interval(0.001, infinity), rec))
                return sample_skybox(r);

            scatter_record srec;
            color color_from_emission = rec.mat->emitted(r, rec, rec.u, rec.v, rec.p);

            if (!rec.mat->scatter(r, rec, srec))
                return color_from_emission;

            if (srec.skip_pdf) {
                if (srec.skip_pdf_ray.direction().length_squared() < 1e-8)
                    return color_from_emission;
                return srec.attenuation * ray_color(srec.skip_pdf_ray, depth-1, world, lights);
            }

            auto light_ptr = make_shared<hittable_pdf>(lights, rec.p);
            mixture_pdf p(light_ptr, srec.pdf_ptr);

            ray scattered;
            double pdf_value;

            if (dynamic_cast<const hittable_list*>(&lights) != nullptr 
                && static_cast<const hittable_list&>(lights).empty()) {
                scattered = ray(rec.p, srec.pdf_ptr->generate(), r.time());
                if (scattered.direction().length_squared() < 1e-8)
                    return color_from_emission;
                pdf_value = srec.pdf_ptr->value(scattered.direction());
            } else {
                auto light_ptr = make_shared<hittable_pdf>(lights, rec.p);
                mixture_pdf p(light_ptr, srec.pdf_ptr);
                scattered = ray(rec.p, p.generate(), r.time());
                if (scattered.direction().length_squared() < 1e-8)
                    return color_from_emission;
                pdf_value = p.value(scattered.direction());
            }

            if (pdf_value < 1e-8)
                return color_from_emission;

            double scattering_pdf = rec.mat->scattering_pdf(r, rec, scattered);
            color sample_color = ray_color(scattered, depth-1, world, lights);
            color color_from_scatter = (srec.attenuation * scattering_pdf * sample_color) / pdf_value;

            return color_from_emission + color_from_scatter;
        }
        
        color ray_color_aov(const ray& r, int depth, const hittable& world, const hittable& lights, color& out_albedo, vec3& out_normal) const {
            if (depth <= 0) return color(0, 0, 0);

            hit_record rec;

            if (!world.hit(r, interval(0.001, infinity), rec)) {
                out_albedo = sample_skybox(r);
                out_normal = vec3(0, 0, 0);
                return out_albedo;
            }

            out_normal = rec.normal;

            scatter_record srec;
            color color_from_emission = rec.mat->emitted(r, rec, rec.u, rec.v, rec.p);

            if (!rec.mat->scatter(r, rec, srec)) {
                out_albedo = color_from_emission;
                return color_from_emission;
            }

            out_albedo = srec.attenuation;

            if (srec.skip_pdf) {
                if (srec.skip_pdf_ray.direction().length_squared() < 1e-8)
                    return color_from_emission;
                return srec.attenuation * ray_color(srec.skip_pdf_ray, depth - 1, world, lights);
            }

            ray scattered;
            double pdf_value;

            if (dynamic_cast<const hittable_list*>(&lights) != nullptr
                && static_cast<const hittable_list&>(lights).empty()) {
                scattered = ray(rec.p, srec.pdf_ptr->generate(), r.time());
                if (scattered.direction().length_squared() < 1e-8)
                    return color_from_emission;
                pdf_value = srec.pdf_ptr->value(scattered.direction());
            } else {
                auto light_ptr = make_shared<hittable_pdf>(lights, rec.p);
                mixture_pdf p(light_ptr, srec.pdf_ptr);
                scattered = ray(rec.p, p.generate(), r.time());
                if (scattered.direction().length_squared() < 1e-8)
                    return color_from_emission;
                pdf_value = p.value(scattered.direction());
            }

            if (pdf_value < 1e-8)
                return color_from_emission;

            double scattering_pdf = rec.mat->scattering_pdf(r, rec, scattered);
            color sample_color = ray_color(scattered, depth - 1, world, lights);
            color color_from_scatter = (srec.attenuation * scattering_pdf * sample_color) / pdf_value;

            return color_from_emission + color_from_scatter;
        }

        void save_debug_image(const std::vector<float>& buffer, const std::string& filename, bool is_normal) {
            std::vector<unsigned char> char_buffer(image_width * image_height * 3);

            for (int i = 0; i < buffer.size(); i++) {
                float val = buffer[i];

                if (is_normal) {
                    val = (val + 1.0f) * 0.5f;
                }

                char_buffer[i] = static_cast<unsigned char>(256 * std::clamp(val, 0.0f, 0.999f));
            }

            stbi_write_png(filename.c_str(), image_width, image_height, 3, char_buffer.data(), image_width * 3);
        }
};

#endif