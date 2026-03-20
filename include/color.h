#ifndef COLOR_H
#define COLOR_H

#include "interval.h"
#include "vec3.h"

using color = vec3;

inline double linear_to_gamma(double linear_component)
{
    if (linear_component > 0)
        return std::sqrt(linear_component);

    return 0;
}

void write_ldr_pixel(std::vector<unsigned char>& ldr_buffer, const color& pixel_color, int i) {
    if (i+2 >= ldr_buffer.size()) {
        std::cerr << "Error: Attempting to write pixel data beyond the end of the image data buffer!" << std::endl;
        return;
    }

    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    if (r != r) r = 0.0;
    if (g != g) g = 0.0;
    if (b != b) b = 0.0;

    r = linear_to_gamma(r);
    g = linear_to_gamma(g);
    b = linear_to_gamma(b);

    static const interval intensity(0.000, 0.999);

    ldr_buffer[i]   = static_cast<unsigned char>(256 * intensity.clamp(r));
    ldr_buffer[i+1] = static_cast<unsigned char>(256 * intensity.clamp(g));
    ldr_buffer[i+2] = static_cast<unsigned char>(256 * intensity.clamp(b));
}

void write_hdr_pixel(std::vector<float>& hdr_buffer, const color& pixel_color, int i) {
    if (i+2 >= hdr_buffer.size()) {
        std::cerr << "Error: Attempting to write pixel data beyond the end of the image data buffer!" << std::endl;
        return;
    }

    hdr_buffer[i]   = static_cast<float>(pixel_color.x());
    hdr_buffer[i+1] = static_cast<float>(pixel_color.y());
    hdr_buffer[i+2] = static_cast<float>(pixel_color.z());
}

#endif