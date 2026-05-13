#ifndef VEC2_H
#define VEC2_H

#include <cmath>
#include <iostream>

class vec2 {
  public:
    double e[2];

    vec2() : e{0,0} {}
    vec2(double e0, double e1) : e{e0, e1} {}

    double x() const { return e[0]; }
    double y() const { return e[1]; }

    vec2 operator-() const { return vec2(-e[0], -e[1]); }
    double operator[](int i) const { return e[i]; }
    double& operator[](int i) { return e[i]; }

    vec2& operator+=(const vec2& v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        return *this;
    }

    vec2& operator*=(double t) {
        e[0] *= t;
        e[1] *= t;
        return *this;
    }

    vec2& operator/=(double t) {
        return *this *= 1/t;
    }

    double length() const {
        return std::sqrt(length_squared());
    }

    double length_squared() const {
        return e[0]*e[0] + e[1]*e[1];
    }

    bool near_zero() const {
        auto s = 1e-8;
        return (std::fabs(e[0]) < s) && (std::fabs(e[1]) < s);
    }

    static vec2 random() {
        return vec2(random_double(), random_double());
    }

    static vec2 random(double min, double max) {
        return vec2(random_double(min,max), random_double(min,max));
    }
};

// point2 is just an alias for vec2, but useful for geometric clarity in the code.
using point2 = vec2;


// Vector Utility Functions

inline std::ostream& operator<<(std::ostream& out, const vec2& v) {
    return out << v.e[0] << ' ' << v.e[1];
}

inline vec2 operator+(const vec2& u, const vec2& v) {
    return vec2(u.e[0] + v.e[0], u.e[1] + v.e[1]);
}

inline vec2 operator-(const vec2& u, const vec2& v) {
    return vec2(u.e[0] - v.e[0], u.e[1] - v.e[1]);
}

inline vec2 operator*(const vec2& u, const vec2& v) {
    return vec2(u.e[0] * v.e[0], u.e[1] * v.e[1]);
}

inline vec2 operator*(double t, const vec2& v) {
    return vec2(t*v.e[0], t*v.e[1]);
}

inline vec2 operator*(const vec2& v, double t) {
    return t * v;
}

inline vec2 operator/(const vec2& v, double t) {
    return (1/t) * v;
}

inline double dot(const vec2& u, const vec2& v) {
    return u.e[0] * v.e[0] + u.e[1] * v.e[1];
}

// Cross product in 2D returns a scalar (the z-component of the 3D cross product)
inline double cross(const vec2& u, const vec2& v) {
    return u.e[0] * v.e[1] - u.e[1] * v.e[0];
}

inline vec2 unit_vector(const vec2& v) {
    return v / v.length();
}

inline vec2 random_in_unit_circle() {
    while (true) {
        auto p = vec2::random(-1,1);
        if (p.length_squared() < 1)
            return p;
    }
}

inline vec2 random_unit_vec2() {
    while (true) {
        auto p = vec2::random(-1,1);
        auto lensq = p.length_squared();
        if (1e-160 < lensq && lensq <= 1)
            return p / sqrt(lensq);
    }
}

// Reflect vector v off surface with normal n
inline vec2 reflect(const vec2& v, const vec2& n) {
    return v - 2*dot(v,n)*n;
}

// Refract vector uv through surface with normal n
inline vec2 refract(const vec2& uv, const vec2& n, double etai_over_etat) {
    auto cos_theta = std::fmin(dot(-uv, n), 1.0);
    vec2 r_out_perp =  etai_over_etat * (uv + cos_theta*n);
    vec2 r_out_parallel = -std::sqrt(std::fabs(1.0 - r_out_perp.length_squared())) * n;
    return r_out_perp + r_out_parallel;
}

// Rotate vector by angle (in radians)
inline vec2 rotate(const vec2& v, double angle) {
    double cos_a = std::cos(angle);
    double sin_a = std::sin(angle);
    return vec2(v.e[0] * cos_a - v.e[1] * sin_a,
                v.e[0] * sin_a + v.e[1] * cos_a);
}

// Get perpendicular vector (rotated 90 degrees counter-clockwise)
inline vec2 perpendicular(const vec2& v) {
    return vec2(-v.e[1], v.e[0]);
}

#endif