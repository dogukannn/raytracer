#pragma once

#include <cmath>
#include <iostream>


struct vec3
{
	double e[3];

	vec3() : e{0,0,0} {} 
	vec3(double e0, double e1, double e2) : e{e0, e1, e2} {}

	double x() const { return e[0]; }
	double y() const { return e[1]; }
	double z() const { return e[2]; }

	vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
	double operator[](int i) const { return e[i]; }
	double& operator[](int i) { return e[i]; }

	vec3& operator+=(const vec3& v)
	{
		e[0] += v.e[0];
		e[1] += v.e[1];
		e[2] += v.e[2];
		return *this;
	}

	vec3& operator*=(const double t)
	{
		e[0] *= t;
		e[1] *= t;
		e[2] *= t;
		return *this;
	}

	vec3& operator/=(const double t)
	{
		return *this *= 1 / t;
	}

	double length() const
	{
		return std::sqrt(lengthSquared());
	}

	double lengthSquared() const
	{
		return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
	}

	inline static vec3 random()
	{
		return vec3(randomDouble(), randomDouble(), randomDouble());
	}

	inline static vec3 random(double min, double max)
	{
		return vec3(randomDouble(min, max), randomDouble(min, max), randomDouble(min, max));
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

inline vec3 operator*(double t, const vec3 &v)
{
	return vec3(t * v.e[0], t * v.e[1], t * v.e[2]);
}

inline vec3 operator*(const vec3 &v, double t)
{
	return t * v;
}

inline vec3 operator/(const vec3 &v, double t)
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

vec3 randomInUnitSphere()
{
	while (true)
	{
		auto p = vec3::random(-1, 1);
		if (p.length() >= 1) continue;
		return p;
	}
}

vec3 randomUnitVector()
{
	return unit(randomInUnitSphere());
}

vec3 reflect(const vec3 &v, const vec3 &n)
{
	return v - 2 * dot(v, n) * n;
}

vec3 refract(const vec3 &uv, const vec3 &n, double etaiOverEtat)
{
	auto cosTheta = fmin(dot(-uv, n), 1.0);
	vec3 rOutPerp = etaiOverEtat * (uv + cosTheta * n);
	vec3 rOutParallel = -sqrt(fabs(1.0 - rOutPerp.lengthSquared())) * n;
	return rOutPerp + rOutParallel;
}












