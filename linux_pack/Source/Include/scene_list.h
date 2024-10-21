#pragma once

#include "hittable.h"
#include "point_light.h"

#include <memory>
#include <vector>

struct scene_list : public hittable
{
	std::vector<std::shared_ptr<hittable>> objects;
	std::vector<std::shared_ptr<point_light>> point_lights;
	color ambient_light;
	color bg_color;

	scene_list() = default;
	scene_list(std::shared_ptr<hittable> object) { add(object); }

	void clear() { objects.clear(); }
	void add(std::shared_ptr<hittable> object) { objects.push_back(object); }

	bool hit(const ray& r, double tMin, double tMax, hitRecord& rec) const override;
};

inline bool scene_list::hit(const ray& r, double tMin, double tMax, hitRecord& rec) const
{
	hitRecord tmpRec;
	bool hitAnything = false;
	auto closestSoFar = tMax;

	for(auto&& object : objects)
	{
		if(object->hit(r, tMin, closestSoFar, tmpRec))
		{
			hitAnything = true;
			closestSoFar = tmpRec.t;
			rec = tmpRec;
		}
	}

	return hitAnything;
}
