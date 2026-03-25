#ifndef __SDL3_HUB_vec3_H__
#define __SDL3_HUB_vec3_H__

#include <cmath>

namespace phy {

    struct vec3 {
        float x = 0.0f; 
        float y = 0.0f;
        float z = 0.0f;

        vec3 operator+(const vec3& v) const {
            return { x + v.x, y + v.y, z + v.z };
        }

        vec3 operator-(const vec3& v) const {
            return { x - v.x, y - v.y, z - v.z };
        }

        vec3 operator*(const float& s) const {
            return { x * s, y * s, z * s };
        }

        vec3& operator+=(const vec3& v) {
            x += v.x;
            y += v.y;
            z += v.z;
            return *this;
        }
    
        vec3& operator-=(const vec3& v) {
            x -= v.x;
            y -= v.y;
            z -= v.z;
            return *this;
        }

        vec3& operator*=(const float& s) {
            x *= s;
            y *= s;
            z *= s;
            return *this;
        }


        vec3 operator*(const vec3& v) const 
        {
            return {
                y * v.z - z * v.y,
                z * v.x - x * v.z,
                x * v.y - y * v.x
            };
        }

        vec3 rotate(const float& angle) const {
            return {
                x * std::cos(angle) - y * std::sin(angle),
                x * std::sin(angle) + y * std::cos(angle),
                z
            };
        }

        vec3 perp(const float& u, const bool anticlockwise = true) const {
            auto len = std::hypot(x, y);
            vec3 vec{ y, -x };
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

        inline float dotProduct(const vec3& v) const {
            return x * v.x + y * v.y + z * v.z;
        }

        inline float cross(const vec3& v) {
            return x*v.y - y*v.x;
        }

        float length() const {
            return std::hypot(x, y, z);
        }

        vec3 normalize() const {
            const float l = length();
            if(l == 0) return { 0, 0 };
            return { x / l, y / l, z / l };
        }

        vec3 para(const float& u, const bool& positive = true) {
            float l = length();
            auto r = *this * (u / l);
            return r * (positive ? 1: -1);
        }

        vec3 project(const vec3& v) {
            return para(projection(v));
        }

        float projection(const vec3& vec) {
            auto  l = this->length();
            auto lv = std::hypot(vec.x, vec.y);
            if(l == 0 || lv == 0) return 0.0f;
            return dotProduct(vec) / lv;
        }
  
        static vec3 fromPolarCoord(const float& angleInRadians, const float& scale = 1) {
            return { std::cos(angleInRadians) * scale, std::sin(angleInRadians) * scale };
        }

    };

}

#endif 