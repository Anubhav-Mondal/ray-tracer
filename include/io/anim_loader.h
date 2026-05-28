#pragma once

#include "toml.hpp"
#include <algorithm>
#include <string>
#include <vector>
#include <stdexcept>

enum class Easing { linear, ease_in, ease_out, ease_in_out };

struct Keyframe {
    int    frame;
    double value;
    Easing easing = Easing::linear;
};

struct AnimTrack {
    std::string           target;
    std::string           property;
    std::vector<Keyframe> keyframes;
};

struct AnimData {
    int    fps;
    double duration;
    int    frame_count;
    std::vector<AnimTrack> tracks;
};

inline Easing parse_easing(const std::string& s) {
    if (s == "ease_in")      return Easing::ease_in;
    if (s == "ease_out")     return Easing::ease_out;
    if (s == "ease_in_out")  return Easing::ease_in_out;
    return Easing::linear;
}

inline AnimData load_anim(const std::string& path) {
    toml::table tbl;
    try {
        tbl = toml::parse_file(path);
    } catch (const toml::parse_error& e) {
        throw std::runtime_error(std::string("Failed to parse anim: ") + e.what());
    }

    AnimData anim;
    anim.fps         = tbl["timeline"]["fps"].value_or(24);
    anim.duration    = tbl["timeline"]["duration"].value_or(1.0);
    anim.frame_count = static_cast<int>(anim.fps * anim.duration);

    const auto* tracks = tbl.get_as<toml::array>("track");
    if (!tracks) return anim;

    for (auto& entry : *tracks) {
        const auto* tt = entry.as_table();
        if (!tt) continue;

        AnimTrack track;
        track.target   = (*tt)["target"].value_or<std::string>("");
        track.property = (*tt)["property"].value_or<std::string>("");

        const auto* kfs = (*tt).get_as<toml::array>("keyframe");
        if (kfs) {
            for (auto& ke : *kfs) {
                const auto* kt = ke.as_table();
                if (!kt) continue;
                Keyframe kf;
                kf.frame  = (*kt)["frame"].value_or(0);
                kf.value  = (*kt)["value"].value_or(0.0);
                kf.easing = parse_easing((*kt)["easing"].value_or<std::string>("linear"));
                track.keyframes.push_back(kf);
            }
            std::sort(track.keyframes.begin(), track.keyframes.end(),
                      [](const Keyframe& a, const Keyframe& b){ return a.frame < b.frame; });
        }

        anim.tracks.push_back(std::move(track));
    }

    return anim;
}
