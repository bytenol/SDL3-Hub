#ifndef __SDL3_HUB_VEC2_H__
#define __SDL3_HUB_VEC2_H__

#include <cmath>

namespace phy {

    struct vec2 {
        float x = 0.0f; 
        float y = 0.0f;

        vec2 operator+(const vec2& v) const {
            return { x + v.x, y + v.y };
        }

        vec2 operator-(const vec2& v) const {
            return { x - v.x, y - v.y };
        }

        vec2 operator*(const float& s) const {
            return { x * s, y * s };
        }

        vec2& operator+=(const vec2& v) {
            x += v.x;
            y += v.y;
            return *this;
        }
    
        vec2& operator-=(const vec2& v) {
            x -= v.x;
            y -= v.y;
            return *this;
        }

        vec2& operator*=(const float& s) {
            x *= s;
            y *= s;
            return *this;
        }

        vec2 rotate(const float& angle) {
            return {
                x * std::cos(angle) - y * std::sin(angle),
                x * std::sin(angle) + y * std::cos(angle)
            };
        }

        vec2 perp(const float& u, const bool anticlockwise = true) const {
            auto len = std::hypot(x, y);
            vec2 vec{ y, -x };
            if (len > 0) {
                if (anticlockwise){ // anticlockwise with respect to canvas coordinate system
                    vec = vec * (u/len);
                }else{
                    vec = vec * (-u/len);				
                }
            }else{
                vec = {0, 0};
            }	
            return vec;
        }

        inline float dotProduct(const vec2& v) {
            return x * v.x + y * v.y;
        }

        inline float crossProduct(const vec2& v) {
            return x*v.y - y*v.x;
        }

        float length() {
            return std::hypot(x, y);
        }

        vec2 normalize() {
            const float l = length();
            if(l == 0) return { 0, 0 };
            return { x / l, y / l };
        }

    };

}

#endif 