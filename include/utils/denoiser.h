#pragma once
#include <vector>
#include <iostream>
#include <OpenImageDenoise/oidn.hpp>

inline void oidn_denoise(
    std::vector<float>& color_buffer,
    int width, int height,
    std::vector<float>& albedo_buffer,
    std::vector<float>& normal_buffer)
{
    oidn::DeviceRef device = oidn::newDevice(oidn::DeviceType::CPU);
    device.commit();

    oidn::FilterRef filter = device.newFilter("RT");

    filter.setImage("color",  color_buffer.data(),  oidn::Format::Float3, width, height);
    filter.setImage("output", color_buffer.data(),  oidn::Format::Float3, width, height);
    filter.set("hdr", true); 

    if (!albedo_buffer.empty()) {
        filter.setImage("albedo", albedo_buffer.data(), oidn::Format::Float3, width, height);
    }
    if (!normal_buffer.empty()) {
        filter.setImage("normal", normal_buffer.data(), oidn::Format::Float3, width, height);
    }

    filter.commit();
    filter.execute();

    const char* err = nullptr;
    if (device.getError(err) != oidn::Error::None) {
        std::cerr << "[OIDN] Error: " << err << "\n";
    }
}