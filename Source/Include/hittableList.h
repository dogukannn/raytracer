#pragma once

#include "hittable.h"

#include <memory>
#include <vector>

struct hittableList : public hittable
{
	std::vector<std::shared_ptr<hittable>> objects;

	hittableList() = default;
	hittableList(std::shared_ptr<hittable> object) { add(object); }

	void clear() { objects.clear(); }
	void add(std::shared_ptr<hittable> object) { objects.push_back(object); }

	bool hit(const ray& r, double tMin, double tMax, hitRecord& rec) const override;
};

inline bool hittableList::hit(const ray& r, double tMin, double tMax, hitRecord& rec) const
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
