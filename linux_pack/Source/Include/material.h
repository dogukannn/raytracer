#pragma once

#include "common.h"
#include "hittable.h"

struct hitRecord;

struct basic;
struct mirror;
struct dielectric;
struct conductor;

struct material
{
	color ambient_reflectance;
	color diffuse_reflectance;
	color specular_reflectance;
	double phong_exponent = 1.0;


	ray reflected_ray(const ray& r_in, const hitRecord& rec)
	{
		vec3 reflected = reflect(unit(r_in.direction()), rec.normal);
		return ray(rec.p, reflected);
	}

	material(const color &a, const color &d, const color &s) : ambient_reflectance(a), diffuse_reflectance(d), specular_reflectance(s) {}

	virtual basic* as_basic() { return nullptr; }
	virtual mirror* as_mirror() { return nullptr; }
	virtual dielectric* as_dielectric() { return nullptr; }
	virtual conductor* as_conductor() { return nullptr; }
	virtual color calc_color(const ray& r_in, const hitRecord& rec, const scene_list& scene, const camera& cam) const
	{
		color res;
		for(auto& light : scene.point_lights)
		{

			auto shadow_ray = ray(rec.p + rec.normal * 0.001f, unit(light->position - rec.p + rec.normal * 0.001f));
			//shadow_ray.orig += rec.normal * 0.001;

			//shadow_ray.dir = unit(shadow_ray.dir);

			hitRecord srec;
			if (scene.hit(shadow_ray, 0.000001, (light->position - rec.p).length(), srec, nullptr))
			{
				continue;
			}

			auto cost = std::max(0.0, dot(unit(light->position - rec.p), rec.normal));
			res += diffuse_reflectance * cost * light->intensity * (1 / (light->position - rec.p).lengthSquared());

			auto wi = unit(light->position - rec.p);
			auto w0 = -unit(r_in.direction());
			auto h = unit(wi + w0);
			auto cosa = std::max(0.0, dot(rec.normal, h));
			res += specular_reflectance * pow(cosa, phong_exponent) * light->intensity * (1 / (light->position - rec.p).lengthSquared());
		}
		res += ambient_reflectance * scene.ambient_light;
		return res;
	}
};

struct conductor : public material
{
	color mirror_reflectance;
	double refraction_index;
	double absorption_index;

	conductor* as_conductor() override { return this; }
	conductor(const color &a, const color &d, const color &s, const color &mir, const double& rf, const double& ab)
		: material(a, d, s), mirror_reflectance(mir), refraction_index(rf), absorption_index(ab) {}

};


struct dielectric : public material
{
	color absorption_coef;
	double refraction_index;

	dielectric* as_dielectric() override { return this; }
	dielectric(const color &a, const color &d, const color &s, const color &abs, const double& rf) : material(a, d, s), absorption_coef(abs), refraction_index(rf) {}

};

struct mirror : public material
{
	color mirror_reflectance;

	mirror* as_mirror() override { return this; }
	mirror(const color &a, const color &d, const color &s, const color &m) : material(a, d, s), mirror_reflectance(m) {}

};

struct basic : public material
{
	basic* as_basic() override { return this; }

	basic(const color &a, const color &d, const color &s) : material(a, d, s) {}
};



