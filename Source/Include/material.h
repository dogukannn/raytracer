#pragma once

#include "common.h"
#include "hittable.h"

struct hitRecord;

struct material
{
	virtual bool scatter(const ray& r_in, const hitRecord& rec, color& attenuation, ray& scattered) const = 0;
};

struct lambertian : public material
{
	color albedo;

	lambertian(const color &a) : albedo(a) {}

	bool scatter(const ray& r_in, const hitRecord& rec, color& attenuation, ray& scattered) const override
	{
		auto scatterDirection = rec.normal + randomUnitVector();
		if (scatterDirection.nearZero())
			scatterDirection = rec.normal;
		
		scattered = ray(rec.p, scatterDirection);
		attenuation = albedo;
		return true;
	}
};

struct metal : public material
{
	color albedo;
	double fuzz;

	metal(const color &a, double f) : albedo(a), fuzz(f < 1 ? f : 1) {}

	bool scatter(const ray& r_in, const hitRecord& rec, color& attenuation, ray& scattered) const override
	{
		vec3 reflected = reflect(unit(r_in.direction()), rec.normal);
		scattered = ray(rec.p, reflected + fuzz * randomInUnitSphere());
		attenuation = albedo;
		return true;
	}
};

struct dielectric : public material
{
	double ir;

	dielectric(double indexOfReclection) : ir(indexOfReclection) {}

	bool scatter(const ray& r_in, const hitRecord& rec, color& attenuation, ray& scattered) const override
	{
		attenuation = color(1.0, 1.0, 1.0);
		double refractionRatio = rec.frontFace ? (1.0 / ir) : ir;

		vec3 unitDirection = unit(r_in.direction());
		double cos_theta = fmin(dot(-unitDirection, rec.normal), 1.0);
		double sin_theta = sqrt(1.0 - cos_theta*cos_theta);

		bool cannot_refract = refractionRatio * sin_theta > 1.0;
		vec3 direction;

		if (cannot_refract || reflectance(cos_theta, refractionRatio) > randomDouble())
			direction = reflect(unitDirection, rec.normal);
		else
			direction = refract(unitDirection, rec.normal, refractionRatio);

            scattered = ray(rec.p, direction);

		scattered = ray(rec.p, direction);

		return true;
	}

	 private:
        static double reflectance(double cosine, double ref_idx) {
            auto r0 = (1-ref_idx) / (1+ref_idx);
            r0 = r0*r0;
            return r0 + (1-r0)*pow((1 - cosine),5);
        }
};




