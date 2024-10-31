#include "Include/bvh.h"

#include <cmath>

bool hitAABB(const ray& r, const vec3& bmin, const vec3& bmax)
{
	// Slab method
	float tx1 = (bmin.x() - r.origin().x()) * -r.direction().x();
	float tx2 = (bmax.x() - r.origin().x()) * -r.direction().x();
	float tmin = fmin(tx1, tx2);
	float tmax = fmax(tx1, tx2);

	float ty1 = (bmin.y() - r.origin().y()) * -r.direction().y();
	float ty2 = (bmax.y() - r.origin().y()) * -r.direction().y();
	tmin = fmax(tmin, fmin(ty1, ty2));
	tmax = fmin(tmax, fmax(ty1, ty2));

	float tz1 = (bmin.z() - r.origin().z()) * -r.direction().z();
	float tz2 = (bmax.z() - r.origin().z()) * -r.direction().z();
	tmin = fmax(tmin, fmin(tz1, tz2));
	tmax = fmin(tmax, fmax(tz1, tz2));

	return tmax >= tmin && tmax > 0;

}

bool BVH::hit(const ray& r, double tMin, double tMax, hitRecord& rec) const
{
	bool hit = false;
	std::vector<uint32_t> stack;
	stack.push_back(rootNodeIdx);

	while (!stack.empty())
	{
		uint32_t nodeIdx = stack.back();
		stack.pop_back();

		const BVHNode& node = nodes[nodeIdx];

		if (hitAABB(r, node.aabb_min, node.aabb_max))
		{
			if (node.is_leaf())
			{
				for (uint32_t i = node.first; i < node.first + node.count; i++)
				{
					const triangle& triangle = triangles[triangleIndices[i]];
					hitRecord temp_rec;
					if (triangle.hit(r, tMin, tMax, temp_rec))
					{
						hit = true;
						tMax = temp_rec.t;
						rec = temp_rec;
					}
				}
			}
			else
			{
				stack.push_back(node.left_child);
				stack.push_back(node.left_child + 1);
			}
		}
	}

	return hit;
}

void BVH::build(std::vector<triangle>&& _triangles)
{

	triangles = std::move(_triangles);

	// initialize indices
	triangleIndices.resize(triangles.size());
	for (uint32_t i = 0; i < triangles.size(); i++)
		triangleIndices[i] = i;


	for (auto& triangle : triangles)
	{
		triangle.centroid = (triangle.p1 + triangle.p2 + triangle.p3) * 0.3333f;
	}

	nodes.resize(2 * triangles.size() - 1);

	nodesUsed++;
	BVHNode& root = nodes[rootNodeIdx];
	root.left_child = 0;
	root.first = 0;
	root.count = triangles.size();

	UpdateNodeBounds(rootNodeIdx);
	Subdivide(rootNodeIdx);
}

void BVH::UpdateNodeBounds(uint32_t nodeIdx)
{
	BVHNode& node = nodes[nodeIdx];
	node.aabb_min = vec3(FLT_MAX);
	node.aabb_max = vec3(-FLT_MAX);

	for (uint32_t i = node.first; i < node.first + node.count; i++)
	{
		const triangle& triangle = triangles[triangleIndices[i]];
		node.aabb_min = fmin(node.aabb_min, fmin(triangle.p1, fmin(triangle.p2, triangle.p3)));
		node.aabb_max = fmax(node.aabb_max, fmax(triangle.p1, fmax(triangle.p2, triangle.p3)));
	}
}

void BVH::Subdivide(uint32_t nodeIdx)
{
	BVHNode& node = nodes[nodeIdx];

	if (node.count <= 2) 		
		return;

	vec3 extents = node.aabb_max - node.aabb_min;
	int axis = 0;
	if (extents.y() > extents.x())
		axis = 1;
	if (extents.z() > extents[axis])
		axis = 2;

	//float split = 0.5f * (node.aabb_min[axis] + node.aabb_max[axis]);
	float split = node.aabb_min[axis] + extents[axis] * 0.5f;

	int i = node.first;
	int j = node.first + node.count - 1;

	while (i <= j)
	{
		if (triangles[triangleIndices[i]].centroid[axis] < split)
			i++;
		else 
			std::swap(triangleIndices[i], triangleIndices[j--]);
	}

	int left_count = i - node.first;
	if (left_count == 0 || left_count == node.count)
	{
		node.left_child = 0;
		return;
	}

	uint32_t left_idx = nodesUsed++;
	uint32_t right_idx = nodesUsed++;

	node.left_child = left_idx;
	BVHNode& left = nodes[left_idx];
	left.first = node.first;
	left.count = left_count;

	BVHNode& right = nodes[right_idx];
	right.first = i;
	right.count = node.count - left_count;

	node.count = 0;

	UpdateNodeBounds(left_idx);
	UpdateNodeBounds(right_idx);

	Subdivide(left_idx);
	Subdivide(right_idx);
}
