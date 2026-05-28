#pragma once

#include "io/anim_loader.h"
#include <cmath>
#include <map>
#include <string>

inline double apply_easing(double t, Easing e) {
    switch (e) {
        case Easing::ease_in:      return t * t;
        case Easing::ease_out:     return 1.0 - (1.0 - t) * (1.0 - t);
        case Easing::ease_in_out:  return t < 0.5 ? 2*t*t : 1 - std::pow(-2*t+2, 2)/2;
        default:                   return t;    // Linear
    }
}

inline double evaluate_track(const AnimTrack& track, int frame) {
    const auto& kfs = track.keyframes;
    if (kfs.empty()) return 0.0;
    if (frame <= kfs.front().frame) return kfs.front().value;
    if (frame >= kfs.back().frame) return kfs.back().value;

    for (size_t i = 0; i + 1 < kfs.size(); ++i) {
        const auto& a = kfs[i];
        const auto& b = kfs[i + 1];
        if (frame >= a.frame && frame <= b.frame) {
            double t = static_cast<double>(frame - a.frame)
                     / static_cast<double>(b.frame - a.frame);
            t = apply_easing(t, b.easing);
            return a.value + t * (b.value - a.value);
        }
    }
    return kfs.back().value;
}

inline std::map<std::string, double> evaluate_all(const AnimData& anim, int frame) {
    std::map<std::string, double> result;
    for (const auto& track : anim.tracks) {
        std::string key = track.target + "." + track.property;
        result[key] = evaluate_track(track, frame);
    }
    return result;
}