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

			auto shadow_ray = ray(rec.p, (light->position - rec.p));
			//shadow_ray.orig += shadow_ray.direction() * 0.0001;

			hitRecord srec;
			if (scene.hit(shadow_ray, 0.001, 1.0, srec))
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

	//color calc_color(const ray& r_in, const hitRecord& rec, const scene_list& scene, const camera& cam) const override
	//{
	//}
};

//struct lambertian : public material
//{
//	color albedo;
//
//	lambertian(const color &a) : albedo(a) {}
//
//	bool scatter(const ray& r_in, const hitRecord& rec, color& attenuation, ray& scattered) const override
//	{
//		auto scatterDirection = rec.normal + randomUnitVector();
//		if (scatterDirection.nearZero())
//			scatterDirection = rec.normal;
//		
//		scattered = ray(rec.p, scatterDirection);
//		attenuation = albedo;
//		return true;
//	}
//};
//
//struct metal : public material
//{
//	color albedo;
//	double fuzz;
//
//	metal(const color &a, double f) : albedo(a), fuzz(f < 1 ? f : 1) {}
//
//	bool scatter(const ray& r_in, const hitRecord& rec, color& attenuation, ray& scattered) const override
//	{
//		vec3 reflected = reflect(unit(r_in.direction()), rec.normal);
//		scattered = ray(rec.p, reflected + fuzz * randomInUnitSphere());
//		attenuation = albedo;
//		return true;
//	}
//};
//
//struct dielectric : public material
//{
//	double ir;
//
//	dielectric(double indexOfReclection) : ir(indexOfReclection) {}
//
//	bool scatter(const ray& r_in, const hitRecord& rec, color& attenuation, ray& scattered) const override
//	{
//		attenuation = color(1.0, 1.0, 1.0);
//		double refractionRatio = rec.frontFace ? (1.0 / ir) : ir;
//
//		vec3 unitDirection = unit(r_in.direction());
//		double cos_theta = fmin(dot(-unitDirection, rec.normal), 1.0);
//		double sin_theta = sqrt(1.0 - cos_theta*cos_theta);
//
//		bool cannot_refract = refractionRatio * sin_theta > 1.0;
//		vec3 direction;
//
//		if (cannot_refract || reflectance(cos_theta, refractionRatio) > randomDouble())
//			direction = reflect(unitDirection, rec.normal);
//		else
//			direction = refract(unitDirection, rec.normal, refractionRatio);
//
//            scattered = ray(rec.p, direction);
//
//		scattered = ray(rec.p, direction);
//
//		return true;
//	}
//
//	 private:
//        static double reflectance(double cosine, double ref_idx) {
//            auto r0 = (1-ref_idx) / (1+ref_idx);
//            r0 = r0*r0;
//            return r0 + (1-r0)*pow((1 - cosine),5);
//        }
//};




