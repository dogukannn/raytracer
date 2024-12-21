#pragma once

#include "common.h"
#include "hittable.h"
#include "texture.h"

struct hitRecord;

struct basic;
struct mirror;
struct dielectric;
struct conductor;

//vec3 create_non_colinear_vector(vec3 v)
//{
//	if(abs(v.x) < abs(v.y) && abs(v.x) < abs(v.z))
//	{
//		return vec3(1.0f, 0.0f, 0.0f);
//	}
//	else if (abs(v.y) < abs(v.x) && abs(v.y) < abs(v.z))
//	{
//		return vec3(0.0f, 1.0f, 0.0f);
//	}
//	else
//	{
//		return vec3(0.0f, 0.0f, 1.0f);
//	}
//}

enum material_type
{
	BASIC,
	MIRROR,
	DIELECTRIC,
	CONDUCTOR
};





struct material
{
	color m_ambient_reflectance;
	color m_diffuse_reflectance;
	color m_specular_reflectance;
	float phong_exponent = 1.0;

	bool has_roughness = false;
	float roughness = 0.0f;

	bool blend_diffuse = false;
	std::shared_ptr<texture> diffuse_map;
	std::shared_ptr<texture> specular_map;
	std::shared_ptr<texture> ambient_map;
	std::shared_ptr<texture> normal_map;
	std::shared_ptr<texture> bump_map;
	float bump_factor = 1.0f;

	ray reflected_ray(const ray& r_in, const hitRecord& rec)
	{
		vec3 reflected = reflect(glm::normalize(r_in.direction()), rec.normal);

		if(has_roughness)
		{

			auto r = glm::normalize(reflected);
			auto rp = create_non_colinear_vector(r);
			auto u = glm::normalize(cross(r, rp));
			auto v = glm::normalize(cross(r, u));

			//auto rr = glm::normalize(r + u * roughness * roughness_u_offset + v * roughness * roughness_v_offset);
			auto rr = glm::normalize(r + u * roughness * ((frandom() - 0.5f) * 1.0f) + v * roughness * ((frandom() - 0.5f) * 1.0f));
			return ray(rec.p, rr);
		}

		return ray(rec.p, reflected);
	}

	material(const color &a, const color &d, const color &s) : m_ambient_reflectance(a), m_diffuse_reflectance(d), m_specular_reflectance(s) {}

	virtual basic* as_basic() { return nullptr; }
	virtual mirror* as_mirror() { return nullptr; }
	virtual dielectric* as_dielectric() { return nullptr; }
	virtual conductor* as_conductor() { return nullptr; }
	virtual color calc_color(const ray& r_in, const hitRecord& rec, const scene_list& scene, const camera& cam) const
	{
		color diffuse_reflectance = m_diffuse_reflectance;
		if (diffuse_map)
		{
			auto u = rec.uv.x;
			auto v = rec.uv.y;
			diffuse_reflectance = diffuse_map->value(u, v, rec.p);
			if (blend_diffuse)
			{
				diffuse_reflectance = (diffuse_reflectance + m_diffuse_reflectance) * 0.5f;
			}
		}
		color specular_reflectance = m_specular_reflectance;
		if (specular_map)
		{
			auto u = rec.uv.x;
			auto v = rec.uv.y;
			specular_reflectance = specular_map->value(u, v, rec.p);
		}
		color ambient_reflectance = m_ambient_reflectance;
		if (ambient_map)
		{
			auto u = rec.uv.x;
			auto v = rec.uv.y;
			ambient_reflectance = ambient_map->value(u, v, rec.p);
			return ambient_reflectance;
		}

		color res;
		for(auto& light : scene.lights)
		{

			auto shadow_ray = ray(rec.p + rec.normal * 0.001f, glm::normalize(light->get_position() - (rec.p + rec.normal * 0.001f)));
			if(bump_map)
			{
				shadow_ray = ray(rec.nobp + rec.normal * 0.001f, glm::normalize(light->get_position() - rec.nobp - rec.normal * 0.01f));
			}
			//shadow_ray.orig += rec.normal * 0.001;

			//shadow_ray.dir = glm::normalize(shadow_ray.dir);

			hitRecord srec;
			if (scene.hit(shadow_ray, 0.001, glm::length(light->get_position() - rec.p), srec, nullptr))
			{
				continue;
			}

			auto wi = glm::normalize(light->get_position() - rec.p);

			auto cost = std::max(0.0f, dot(glm::normalize(light->get_position() - rec.p), rec.normal));
			res += diffuse_reflectance * cost * light->get_intensity(wi) * (1 / glm::dot(light->get_position() - rec.p, light->get_position() - rec.p));

			auto w0 = -glm::normalize(r_in.direction());
			auto h = glm::normalize(wi + w0);
			auto cosa = std::max(0.0f, dot(rec.normal, h));
			res += specular_reflectance * pow(cosa, phong_exponent) * light->get_intensity(wi) * (1 / glm::dot(light->get_position() - rec.p, light->get_position() - rec.p));
		}

		//directional lights
		for (auto& light : scene.directional_lights)
		{
			//shadow ray
			auto shadow_ray = ray(rec.p + rec.normal * 0.001f, glm::normalize(-light->direction));
			if (bump_map)
			{
				shadow_ray = ray(rec.nobp + rec.normal * 0.001f, glm::normalize(-light->direction));
			}

			hitRecord srec;
			if (scene.hit(shadow_ray, 0.001, 1000000, srec, nullptr))
			{
				continue;
			}

			auto wi = -light->direction;
			auto cost = std::max(0.0f, dot(wi, rec.normal));
			res += diffuse_reflectance * cost * light->intensity;

			auto w0 = -glm::normalize(r_in.direction());
			auto h = glm::normalize(wi + w0);
			auto cosa = std::max(0.0f, dot(rec.normal, h));
			res += specular_reflectance * pow(cosa, phong_exponent) * light->intensity;
		}

		//spherical directional lights
		for (auto& sdl : scene.spherical_directional_lights)
		{
			//get a sample from spherical light
			vec3 dir;
			vec3 intensity;
			sdl->sample(rec.normal, intensity, dir);

			//shadow ray
			auto shadow_ray = ray(rec.p + rec.normal * 0.001f, glm::normalize(dir));
			if (bump_map)
			{
				shadow_ray = ray(rec.nobp + rec.normal * 0.001f, glm::normalize(dir));
			}

			hitRecord srec;
			if (scene.hit(shadow_ray, 0.001, 1000000, srec, nullptr))
			{
				continue;
			}
			auto wi = dir;

			auto w0 = -glm::normalize(r_in.direction());
			auto cosf = std::max(0.0f, dot(w0, wi));

			//if(cosf > 0.000001f)
			//	intensity = intensity * (2.0f - cosf);

			auto cost = std::max(0.0f, dot(wi, rec.normal));

			
			res += diffuse_reflectance * cost * intensity;

			auto h = glm::normalize(wi + w0);
			auto cosa = std::max(0.0f, dot(rec.normal, h));
			res += specular_reflectance * pow(cosa, phong_exponent) * intensity;
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



