#pragma once

#include "hittable.h"
#include "point_light.h"

#include <memory>
#include <vector>

struct scene_list : public hittable
{
	std::vector<std::shared_ptr<hittable>> objects;
	std::vector<std::shared_ptr<light>> lights;
	color ambient_light;
	color bg_color;

	scene_list() = default;
	scene_list(std::shared_ptr<hittable> object) { add(object); }

	void clear() { objects.clear(); }
	void add(std::shared_ptr<hittable> object) { objects.push_back(object); }

	bool hit(const ray& r, double tMin, double tMax, hitRecord& rec, mat4* model) const override;
};

inline bool scene_list::hit(const ray& r, double tMin, double tMax, hitRecord& rec, mat4* model) const
{
	hitRecord tmpRec;
	bool hitAnything = false;
	auto closestSoFar = tMax;

	for(auto&& object : objects)
	{
		//convert ray accoring to the model matrix
		auto omodel = object->model;

		if(object->has_motion_blur)
		{
			vec3 random_motion = object->motion * motion_blur_mp;
			mat4 translation = mat4::translate(random_motion.x(), random_motion.y(), random_motion.z());
			omodel = translation * object->model;
		}

		auto invModel = omodel.inverse();
		ray newRay = r;
		vec4 origin = vec4(newRay.origin(), 1.0f);
		vec4 direction = vec4(newRay.direction(), 0.0f);

		vec4 newOrigin = invModel * origin;
		vec4 newDirection = invModel * direction;

		newRay = ray(vec3(newOrigin.x(), newOrigin.y(), newOrigin.z()), vec3(newDirection.x(), newDirection.y(), newDirection.z()));

		//newRay.dir = unit(newRay.direction());
		
		if(object->hit(newRay, tMin, closestSoFar, tmpRec, &omodel))
		{
			hitAnything = true;
			closestSoFar = tmpRec.t;
			rec = tmpRec;
		}
	}

	return hitAnything;
}
