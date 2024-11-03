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

struct BVH
{
	std::vector<BVHNode> nodes;
	uint32_t rootNodeIdx = 0;
	uint32_t nodesUsed = 0;

	std::vector<triangle> triangles;
	std::vector<uint32_t> triangleIndices;


	BVH() = default;

	void build(std::vector<triangle>&& _triangles);
	void UpdateNodeBounds(uint32_t nodeIdx);
	void Subdivide(uint32_t nodeIdx);
};

struct BVHInstance : public hittable
{
	std::shared_ptr<BVH> bvh;
	std::shared_ptr<material> mat_ptr;

	BVHInstance(std::shared_ptr<BVH> _bvh) : bvh(_bvh) {}
	BVHInstance(std::shared_ptr<BVH> _bvh, mat4 _model) : bvh(_bvh) { model = _model; }

	bool hit(const ray& r, double tMin, double tMax, hitRecord& rec, mat4* model) const override;
};

