#include "Include/bvh.h"

//#define BASIC_SPLIT_EXP

#include <cmath>

bool hitAABB(const ray& r, const vec3& bmin, const vec3& bmax)
{
	// Slab method
	double tmin = (bmin.x() - r.orig.x()) / r.dir.x();
	double tmax = (bmax.x() - r.orig.x()) / r.dir.x();

	if (tmin > tmax)
		std::swap(tmin, tmax);

	double tymin = (bmin.y() - r.orig.y()) / r.dir.y();
	double tymax = (bmax.y() - r.orig.y()) / r.dir.y();

	if (tymin > tymax)
		std::swap(tymin, tymax);

	if ((tmin > tymax) || (tymin > tmax))
		return false;

	if (tymin > tmin)
		tmin = tymin;

	if (tymax < tmax)
		tmax = tymax;

	double tzmin = (bmin.z() - r.orig.z()) / r.dir.z();
	double tzmax = (bmax.z() - r.orig.z()) / r.dir.z();

	if (tzmin > tzmax)
		std::swap(tzmin, tzmax);

	if ((tmin > tzmax) || (tzmin > tmax))
		return false;

	if (tzmin > tmin)
		tmin = tzmin;

	if (tzmax < tmax)
		tmax = tzmax;

	return true;
}

bool BVHInstance::hit(const ray& r, double tMin, double tMax, hitRecord& rec, mat4* _model) const
{
	bool hit = false;
	std::vector<uint32_t> stack;
	stack.push_back(bvh->rootNodeIdx);

	while (!stack.empty())
	{
		uint32_t nodeIdx = stack.back();
		stack.pop_back();

		const BVHNode& node = bvh->nodes[nodeIdx];

		if (hitAABB(r, node.aabb_min, node.aabb_max))
		{
			if (node.is_leaf())
			{
				for (uint32_t i = node.first; i < node.first + node.count; i++)
				{
					const triangle& triangle = bvh->triangles[bvh->triangleIndices[i]];
					hitRecord temp_rec;
					if (triangle.hit(r, tMin, tMax, temp_rec, _model))
					{
						hit = true;
						tMax = temp_rec.t;
						rec = temp_rec;
						rec.mat_ptr = mat_ptr;
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
		triangle.centroid = (triangle.p1 + triangle.p2 + triangle.p3) * 0.333333f;
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

	float split = 0.5f * (node.aabb_min[axis] + node.aabb_max[axis]);

#ifdef BASIC_SPLIT_EXP
	//check each axis split count to choose  the best one
	int left_counts[10][3];
	int axis_bins[3] = {0, 0, 0};
	for(int j = 0; j < 10; j++)
	{
		for (int i = 0; i < 3; i++)
		{
			left_counts[j][i] = 0;
			float temp_split = (0.1f * j) * (node.aabb_min[i] + node.aabb_max[i]);
			for (int k = node.first; k < node.first + node.count; k++)
			{
				if (triangles[triangleIndices[k]].centroid[i] < temp_split)
					left_counts[j][i]++;
			}
		}
	}

	int axis_one_min_diff = INT_MAX;
	for (int j = 0; j < 10; j++)
	{
		int axis_one_diff = abs(left_counts[j][0] - ((int)node.count - left_counts[j][0]));
		if (axis_one_diff < axis_one_min_diff)
		{
			axis_one_min_diff = axis_one_diff;
			axis_bins[0] = j;
		}
	}

	int axis_two_min_diff = INT_MAX;
	int axis_two_bin = 0;
	for (int j = 0; j < 10; j++)
	{
		int axis_two_diff = abs(left_counts[j][1] - ((int)node.count - left_counts[j][1]));
		if (axis_two_diff < axis_two_min_diff)
		{
			axis_two_min_diff = axis_two_diff;
			axis_bins[1] = j;
		}
	}

	int axis_three_min_diff = INT_MAX;
	int axis_three_bin = 0;
	for (int j = 0; j < 10; j++)
	{
		int axis_three_diff = abs(left_counts[j][2] - ((int)node.count - left_counts[j][2]));
		if (axis_three_diff < axis_three_min_diff)
		{
			axis_three_min_diff = axis_three_diff;
			axis_bins[2] = j;
		}
	}

	int axis_one_diff = abs(left_counts[axis_bins[0]][0] - ((int)node.count - left_counts[axis_bins[0]][0]));
	int axis_two_diff = abs(left_counts[axis_bins[1]][1] - ((int)node.count - left_counts[axis_bins[1]][1]));
	int axis_three_diff = abs(left_counts[axis_bins[2]][2] - ((int)node.count - left_counts[axis_bins[2]][2]));

	//get the smallest diff
	if (axis_one_diff <= axis_two_diff && axis_one_diff <= axis_three_diff)
		axis = 0;
	else if (axis_two_diff <= axis_one_diff && axis_two_diff <= axis_three_diff)
		axis = 1;
	else
		axis = 2;

	split = (0.1f * axis_bins[axis]) * (node.aabb_min[axis] + node.aabb_max[axis]);
#endif 
	
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
