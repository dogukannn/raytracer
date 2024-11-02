#pragma once

#include <cmath>
#include <iostream>

#include "common.h"

struct vec3i
{
	int e[3];

	vec3i() : e{0,0,0} {} 
	vec3i(int e0, int e1, int e2) : e{e0, e1, e2} {}

	int x() const { return e[0]; }
	int y() const { return e[1]; }
	int z() const { return e[2]; }

	vec3i operator-() const { return vec3i(-e[0], -e[1], -e[2]); }
	int operator[](int i) const { return e[i]; }
	int& operator[](int i) { return e[i]; }

	vec3i& operator+=(const vec3i& v)
	{
		e[0] += v.e[0];
		e[1] += v.e[1];
		e[2] += v.e[2];
		return *this;
	}

	vec3i& operator*=(const int t)
	{
		e[0] *= t;
		e[1] *= t;
		e[2] *= t;
		return *this;
	}

	vec3i& operator/=(const int t)
	{
		return *this *= 1 / t;
	}

	vec3i& operator/=(const double t)
	{
		return *this *= 1.0 / t;
	}

};


struct vec3
{
	float e[3];

	vec3() : e{0,0,0} {} 
	vec3(float e0, float e1, float e2) : e{e0, e1, e2} {}
	vec3(float e0) : e{e0, e0, e0} {}

	float x() const { return e[0]; }
	float y() const { return e[1]; }
	float z() const { return e[2]; }

	vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
	float operator[](int i) const { return e[i]; }
	float& operator[](int i) { return e[i]; }

	vec3& operator+=(const vec3& v)
	{
		e[0] += v.e[0];
		e[1] += v.e[1];
		e[2] += v.e[2];
		return *this;
	}

	vec3& operator*=(const float t)
	{
		e[0] *= t;
		e[1] *= t;
		e[2] *= t;
		return *this;
	}

	vec3& operator/=(const float t)
	{
		return *this *= 1 / t;
	}

	float length() const
	{
		return std::sqrt(lengthSquared());
	}

	float lengthSquared() const
	{
		return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
	}

	inline static vec3 random()
	{
		return vec3(randomFloat(), randomFloat(), randomFloat());
	}

	inline static vec3 random(float min, float max)
	{
		return vec3(randomFloat(min, max), randomFloat(min, max), randomFloat(min, max));
	}

	bool nearZero() const
	{
		const auto s = 1e-8;
		return (fabs(e[0]) < s) && (fabs(e[1]) < s) && (fabs(e[2]) < s);
	}
};

using point3 = vec3;
using color = vec3;

inline std::ostream& operator<<(std::ostream &out, const vec3 &v)
{
	return out << v.e[0] << " " << v.e[1] << " " << v.e[2];
}

inline vec3 operator+(const vec3 &u, const vec3 &v)
{
	return vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}


inline vec3 operator-(const vec3 &u, const vec3 &v)
{
	return vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}


inline vec3 operator*(const vec3 &u, const vec3 &v)
{
	return vec3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}

inline vec3 operator*(float t, const vec3 &v)
{
	return vec3(t * v.e[0], t * v.e[1], t * v.e[2]);
}

inline vec3 operator*(const vec3 &v, float t)
{
	return t * v;
}

inline vec3 operator/(const vec3 &v, float t)
{
	return ( 1 / t ) * v;
}

inline double dot(const vec3 &u, const vec3 &v)
{
	return u.e[0] * v.e[0] +
		   u.e[1] * v.e[1] +
		   u.e[2] * v.e[2];
}

inline vec3 cross(const vec3 &u, const vec3 &v)
{
    return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
                u.e[2] * v.e[0] - u.e[0] * v.e[2],
                u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}

inline vec3 unit(vec3 v)
{
	return v / v.length();
}

inline vec3 randomInUnitSphere()
{
	while (true)
	{
		auto p = vec3::random(-1, 1);
		if (p.length() >= 1) continue;
		return p;
	}
}

inline vec3 randomUnitVector()
{
	return unit(randomInUnitSphere());
}

inline vec3 randomInUnitDisk()
{
	while (true)
	{
		auto p = vec3(randomDouble(-1, 1), randomDouble(-1, 1), 0);
		if(p.lengthSquared() >= 1) continue;
		return p;
	}
}

inline vec3 reflect(const vec3 &v, const vec3 &n)
{
	return v - 2 * dot(v, n) * n;
}

inline vec3 refract(const vec3 &uv, const vec3 &n, double etaiOverEtat)
{
	auto cosTheta = fmin(dot(-uv, n), 1.0);
	vec3 rOutPerp = etaiOverEtat * (uv + cosTheta * n);
	vec3 rOutParallel = -sqrt(fabs(1.0 - rOutPerp.lengthSquared())) * n;
	return rOutPerp + rOutParallel;
}

inline vec3 fmin(vec3 a, vec3 b)
{
	return vec3(fmin(a[0], b[0]), fmin(a[1], b[1]), fmin(a[2], b[2]));
}

inline vec3 fmax(vec3 a, vec3 b)
{
	return vec3(fmax(a[0], b[0]), fmax(a[1], b[1]), fmax(a[2], b[2]));
}


struct vec4 {
    float e[4];
    vec4() : e{0,0,0,1} {}
    vec4(float e0, float e1, float e2, float e3) : e{e0, e1, e2, e3} {}
    vec4(const vec3& v, float e3) : e{v.x(), v.y(), v.z(), e3} {}
    //vec4(vec3 v, float e3) : e{v.x(), v.y(), v.z(), e3} {}


    float x() const { return e[0]; }
    float y() const { return e[1]; }
    float z() const { return e[2]; }
    float w() const { return e[3]; }
    float operator[](int i) const { return e[i]; }
    float& operator[](int i) { return e[i]; }
};

struct mat4 {
    float e[4][4];

    // Constructors
    mat4() {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                e[i][j] = (i == j) ? 1.0f : 0.0f;  // Identity matrix
    }

    mat4(float diagonal) {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                e[i][j] = (i == j) ? diagonal : 0.0f;
    }

    // Access operators
    float* operator[](int i) { return e[i]; }
    const float* operator[](int i) const { return e[i]; }

    // Helper function for 3x3 determinant (used in 4x4 determinant calculation)
    static float det3x3(float a00, float a01, float a02,
                       float a10, float a11, float a12,
                       float a20, float a21, float a22) {
        return a00 * (a11 * a22 - a12 * a21) -
               a01 * (a10 * a22 - a12 * a20) +
               a02 * (a10 * a21 - a11 * a20);
    }

    // Calculate determinant of 4x4 matrix
    float determinant() const {
        float det = e[0][0] * det3x3(e[1][1], e[1][2], e[1][3],
                                    e[2][1], e[2][2], e[2][3],
                                    e[3][1], e[3][2], e[3][3]) -
                    e[0][1] * det3x3(e[1][0], e[1][2], e[1][3],
                                    e[2][0], e[2][2], e[2][3],
                                    e[3][0], e[3][2], e[3][3]) +
                    e[0][2] * det3x3(e[1][0], e[1][1], e[1][3],
                                    e[2][0], e[2][1], e[2][3],
                                    e[3][0], e[3][1], e[3][3]) -
                    e[0][3] * det3x3(e[1][0], e[1][1], e[1][2],
                                    e[2][0], e[2][1], e[2][2],
                                    e[3][0], e[3][1], e[3][2]);
        return det;
    }

    // Calculate inverse of 4x4 matrix
    mat4 inverse() const {
        mat4 result;
        float det = determinant();
        
        if (abs(det) < 1e-8f) {
            // Matrix is not invertible
            return mat4(0.0f);
        }

        float inv_det = 1.0f / det;

        // Calculate cofactors and adjugate matrix
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                // Get 3x3 submatrix for cofactor
                float sub[9];
                int sub_idx = 0;
                for (int k = 0; k < 4; k++) {
                    if (k == i) continue;
                    for (int l = 0; l < 4; l++) {
                        if (l == j) continue;
                        sub[sub_idx++] = e[k][l];
                    }
                }
                
                float cofactor = det3x3(sub[0], sub[1], sub[2],
                                      sub[3], sub[4], sub[5],
                                      sub[6], sub[7], sub[8]);
                
                // Apply sign based on position
                if ((i + j) % 2 == 1) {
                    cofactor = -cofactor;
                }
                
                // Transpose while building the inverse
                result.e[j][i] = cofactor * inv_det;
            }
        }

        return result;
    }

    // Static helper functions for transformations
    static mat4 scale(float sx, float sy, float sz) {
        mat4 m;
        m[0][0] = sx;
        m[1][1] = sy;
        m[2][2] = sz;
        return m;
    }

    static mat4 scale(const vec3& s) {
        return scale(s[0], s[1], s[2]);
    }

    static mat4 translate(float tx, float ty, float tz) {
        mat4 m;
        m[0][3] = tx;
        m[1][3] = ty;
        m[2][3] = tz;
        return m;
    }

    static mat4 translate(const vec3& t) {
        return translate(t[0], t[1], t[2]);
    }

    static float deg_to_rad(float degrees) {
        return degrees * 0.0174532925f;  // pi/180
    }

    static mat4 rotate(const vec3& axis, float angle_radians) {
        mat4 m;
        float c = cos(angle_radians);
        float s = sin(angle_radians);
        float t = 1.0f - c;
        
        float x = axis[0];
        float y = axis[1];
        float z = axis[2];
        
        // Normalize axis
        float len = sqrt(x*x + y*y + z*z);
        if (len != 0) {
            x /= len;
            y /= len;
            z /= len;
        }

        m[0][0] = t*x*x + c;
        m[0][1] = t*x*y - s*z;
        m[0][2] = t*x*z + s*y;
        
        m[1][0] = t*x*y + s*z;
        m[1][1] = t*y*y + c;
        m[1][2] = t*y*z - s*x;
        
        m[2][0] = t*x*z - s*y;
        m[2][1] = t*y*z + s*x;
        m[2][2] = t*z*z + c;

        return m;
    }
    // New rotation function using degrees
    static mat4 rotate_degrees(const vec3& axis, float angle_degrees) {
        return rotate(axis, deg_to_rad(angle_degrees));
    }

    // Convenience functions for rotation around primary axes using degrees
    static mat4 rotate_x_degrees(float angle_degrees) {
        return rotate_degrees(vec3(1, 0, 0), angle_degrees);
    }

    static mat4 rotate_y_degrees(float angle_degrees) {
        return rotate_degrees(vec3(0, 1, 0), angle_degrees);
    }

    static mat4 rotate_z_degrees(float angle_degrees) {
        return rotate_degrees(vec3(0, 0, 1), angle_degrees);
    }

    // Matrix multiplication
    mat4 operator*(const mat4& other) const {
        mat4 result(0.0f);
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                for (int k = 0; k < 4; k++) {
                    result.e[i][j] += e[i][k] * other.e[k][j];
                }
            }
        }
        return result;
    }

    // Vector multiplication
    vec4 operator*(const vec4& v) const {
        vec4 result;
        for (int i = 0; i < 4; i++) {
            result.e[i] = 0;
            for (int j = 0; j < 4; j++) {
                result.e[i] += e[i][j] * v.e[j];
            }
        }
        return result;
    }

    //transpose
	mat4 transpose() const {
		mat4 result;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				result.e[i][j] = e[j][i];
			}
		}
		return result;
	}
};

inline vec3 to_vec3(const vec4& v) {
	return vec3(v.x(), v.y(), v.z());
}









