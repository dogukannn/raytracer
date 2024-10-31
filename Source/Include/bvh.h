#pragma once
#include "triangle.h"
#include <vector>

struct BVHNode
{
	vec3 aabb_min, aabb_max;
	uint32_t left_child;
	uint32_t first, count;
	bool is_leaf() const { return count > 0; }
};

struct BVH : public hittable
{
	std::vector<BVHNode> nodes;
	uint32_t rootNodeIdx = 0;
	uint32_t nodesUsed = 0;

	std::vector<triangle> triangles;
	std::vector<uint32_t> triangleIndices;


	BVH() = default;

	bool hit(const ray& r, double tMin, double tMax, hitRecord& rec) const override;

	void build(std::vector<triangle>&& _triangles);
	void UpdateNodeBounds(uint32_t nodeIdx);
	void Subdivide(uint32_t nodeIdx);
};

