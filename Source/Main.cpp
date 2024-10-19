#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <future>

//project headers
#include "Include/camera.h"
#include "Include/common.h"
#include "Include/color.h"
#include "Include/scene_list.h"
#include "Include/material.h"
#include "Include/sphere.h"
#include "Include/parser.h"
#include "Include/triangle.h"

color beerslaw(double t, color absorp)
{
	return color(exp(-t * absorp.x()), exp(-t * absorp.y()), exp(-t * absorp.z()));
}

color RayColor(const ray& r, const scene_list& world, const camera& cam, int depth)
{

	if(depth <= 0)
	{
		return color(0, 0, 0);
	}

	hitRecord rec;
	if(world.hit(r, 0.001, infinity, rec))
	{
		if(auto basicmat = rec.mat_ptr->as_basic())
		{
			return basicmat->calc_color(r, rec, world, cam);
		}
		else if (auto mirrormat = rec.mat_ptr->as_mirror())
		{
			return mirrormat->calc_color(r, rec, world, cam) + mirrormat->mirror_reflectance * RayColor(mirrormat->reflected_ray(r, rec), world, cam, depth - 1);
		}
		else if (auto diemat = rec.mat_ptr->as_dielectric())
		{
			double refractionRatio = rec.frontFace ? (1.0 / diemat->refraction_index) : diemat->refraction_index;
			auto d = unit(r.direction());
			double cosi = dot(-d, rec.normal);
			double k = 1.0 - refractionRatio * refractionRatio * (1 - cosi * cosi);
			if (k < 0)
			{
				color ber = color(1,1,1);
				hitRecord refrec;
				if(world.hit(ray(rec.p, unit(diemat->reflected_ray(r, rec).direction())), 0.001, infinity, refrec))
				{
					ber = beerslaw(refrec.t, diemat->absorption_coef);
				}
				return RayColor(diemat->reflected_ray(r, rec), world, cam, depth - 1) * ber;
			}
			double cosph = sqrt(k);
			//vec3 refractdir = d * refractionRatio + rec.normal * (refractionRatio * cosi - sqrt(k));
			vec3 refractdir = (d + rec.normal * cosi) * refractionRatio - rec.normal * cosph;
			refractdir = unit(refractdir);

			double n1 = rec.frontFace ? (1.0) : diemat->refraction_index;
			double n2 = rec.frontFace ? (diemat->refraction_index) : 1.0f;
			double r2 = (n2 * cosi - n1 * cosph) / (n2 * cosi + n1 * cosph);
			double r1 = (n1 * cosi - n2 * cosph) / (n1 * cosi + n2 * cosph);

			double frefl = 0.5 * (r1 * r1 + r2 * r2);
			double frefr = 1 - frefl;

			if(!rec.frontFace)
			{
				color col = color(0,0,0);

				col += frefr * RayColor(ray(rec.p, refractdir), world, cam, depth - 1);

				color ber = color(1,1,1);
				hitRecord refrec;
				if(world.hit(ray(rec.p, unit(diemat->reflected_ray(r, rec).direction())), 0.001, infinity, refrec))
				{
					ber = beerslaw(refrec.t, diemat->absorption_coef);
				}

				col += frefl * RayColor(diemat->reflected_ray(r, rec), world, cam, depth - 1) * ber;

				return col;
			}

			auto col = diemat->calc_color(r, rec, world, cam);

			color ber = color(1,1,1);
			hitRecord refrec;
			if(world.hit(ray(rec.p, refractdir), 0.001, infinity, refrec))
			{
				ber = beerslaw(refrec.t, diemat->absorption_coef);
			}
			col += frefr * RayColor(ray(rec.p, refractdir), world, cam, depth - 1) * ber;

			col += frefl * RayColor(diemat->reflected_ray(r, rec), world, cam, depth - 1);
			
			return col;

			return diemat->calc_color(r, rec, world, cam)
				+ frefr * RayColor(ray(rec.p, refractdir), world, cam, depth - 1)
				+ frefl * RayColor(diemat->reflected_ray(r, rec), world, cam, depth - 1);
		}
		else if (auto condmat = rec.mat_ptr->as_conductor())
		{
			auto d = unit(r.direction());
			double cosi = dot(-d, rec.normal);

			double n2 = condmat->refraction_index;
			double k2 = condmat->absorption_index;

			double rs = ((n2 * n2 + k2 * k2) - 2 * n2 * cosi + cosi * cosi) / ((n2 * n2 + k2 * k2) + 2 * n2 * cosi + cosi * cosi);
			double rp = ((n2 * n2 + k2 * k2) * cosi * cosi - 2 * n2 * cosi + 1) / ((n2 * n2 + k2 * k2) * cosi * cosi + 2 * n2 * cosi + 1);

			double frefl = 0.5 * (rs + rp);

			return condmat->calc_color(r, rec, world, cam)
				+ condmat->mirror_reflectance * frefl * RayColor(condmat->reflected_ray(r, rec), world, cam, depth - 1);
		}
	}

	return world.bg_color;
}


color to_c(parser::Vec3i v)
{
	return { (double)v.x, (double)v.y, (double)v.z };
}

color to_c(parser::Vec3f v)
{
	return { v.x, v.y, v.z };
}

point3 to_p(parser::Vec3f v)
{
	return { v.x, v.y, v.z };
}

vec3 to_v(parser::Vec3f v)
{
	return { v.x, v.y, v.z };
}

std::shared_ptr<material> convert_material(const parser::Material& mat)
{
	std::shared_ptr<material> mesh_material;
	if(mat.is_mirror)
	{
		mesh_material = std::make_shared<mirror>(to_c(mat.ambient), to_c(mat.diffuse), to_c(mat.specular), to_c(mat.mirror));
	}
	else if(mat.is_dielectric)
	{
		mesh_material = std::make_shared<dielectric>(to_c(mat.ambient), to_c(mat.diffuse), to_c(mat.specular), to_c(mat.absorption_coef), mat.refraction_index);
	}
	else if(mat.is_conductor)
	{
		mesh_material = std::make_shared<conductor>(to_c(mat.ambient), to_c(mat.diffuse), to_c(mat.specular), to_c(mat.mirror), mat.refraction_index, mat.absorption_index);
	}
	else
	{
		mesh_material = std::make_shared<basic>(to_c(mat.ambient), to_c(mat.diffuse), to_c(mat.specular));
	}

	mesh_material->phong_exponent = mat.phong_exponent;
	return mesh_material;
}

scene_list hittableListFromScene(const parser::Scene& scene)
{
	scene_list world;

	for(auto& mesh : scene.meshes)
	{
		auto& mat = scene.materials[mesh.material_id-1];

		std::shared_ptr<material> mesh_material = convert_material(mat);
				
		for(auto& face : mesh.faces)
		{
			point3 p1 = { scene.vertex_data[face.v0_id-1].x,
							scene.vertex_data[face.v0_id-1].y,
							scene.vertex_data[face.v0_id-1].z };

			point3 p2 = { scene.vertex_data[face.v1_id-1].x,
							scene.vertex_data[face.v1_id-1].y,
							scene.vertex_data[face.v1_id-1].z };

			point3 p3 = { scene.vertex_data[face.v2_id-1].x,
							scene.vertex_data[face.v2_id-1].y,
							scene.vertex_data[face.v2_id-1].z };

			world.add(std::make_shared<triangle>(p1, p2, p3, mesh_material));
		}
	}

	for(auto& sp : scene.spheres)
	{
		point3 c = to_p(scene.vertex_data[sp.center_vertex_id-1]);

		auto& mat = scene.materials[sp.material_id-1];
		std::shared_ptr<material> mesh_material = convert_material(mat);

		world.add(std::make_shared<sphere>(c, sp.radius, mesh_material));
	}

	for(auto& tr : scene.triangles)
	{
		point3 p1 = { scene.vertex_data[tr.indices.v0_id-1].x,
						scene.vertex_data[tr.indices.v0_id-1].y,
						scene.vertex_data[tr.indices.v0_id-1].z };

		point3 p2 = { scene.vertex_data[tr.indices.v1_id-1].x,
						scene.vertex_data[tr.indices.v1_id-1].y,
						scene.vertex_data[tr.indices.v1_id-1].z };

		point3 p3 = { scene.vertex_data[tr.indices.v2_id-1].x,
						scene.vertex_data[tr.indices.v2_id-1].y,
						scene.vertex_data[tr.indices.v2_id-1].z };

		auto& mat = scene.materials[tr.material_id-1];
		std::shared_ptr<material> mesh_material = convert_material(mat);
		world.add(std::make_shared<triangle>(p1, p2, p3, mesh_material));
	}
	

	world.ambient_light = to_c(scene.ambient_light);
	for(auto& pl : scene.point_lights)
	{
		point_light p;
		p.intensity = to_c(pl.intensity);
		p.position = to_p(pl.position);
		world.point_lights.push_back(std::make_shared<point_light>(p));
	}

	world.bg_color = to_c(scene.background_color);
	return world;
}


void render_camera(parser::Scene& scene, int camera_idx, scene_list& world)
{
	auto start = std::chrono::steady_clock::now();

	auto& scene_cam = scene.cameras[camera_idx];
	//camera
	int maxDepth = std::max(5, scene.max_recursion_depth);
	int imageWidth = scene_cam.image_width;
	int imageHeight = scene_cam.image_height;
	point3 lookfrom = to_p(scene_cam.position);
	point3 lookat = to_p(scene_cam.gaze) + to_p(scene_cam.position);
	vec3 vup = to_v(scene_cam.up);
	auto distToFocus = scene_cam.near_distance;
	auto aperture = 0.1;
	double aspectRatio = imageWidth / (float)imageHeight;

	camera cam(lookfrom, lookat, vup, scene_cam.near_plane, aspectRatio, aperture, distToFocus);

	std::ofstream image(scene_cam.image_name + ".ppm");

	image << "P3\n" << imageWidth << " " << imageHeight << "\n255\n";
	std::vector<std::vector<color>> img;
	img.resize(imageHeight);
	for(auto& vec : img)
	{
		vec.resize(imageWidth);
	}

	//{
	//	color pixelColor(0, 0, 0);
	//	const auto u = (751 + 0.5f) / (imageWidth - 1);
	//	const auto v = (720 - 454 + 0.5f) / (imageHeight - 1);
	//	ray r = cam.getRay(u, v);
	//	pixelColor += RayColor(r, world, cam, maxDepth);
	//return;
	//}


	std::vector<std::future<void>> threads;
	for (int j = imageHeight - 1; j >= 0; j--)
	{
		for (int i = imageWidth - 1; i >= 0; i--)
		{
			threads.emplace_back(std::async([i, j, &cam, &world, imageHeight, imageWidth, maxDepth, &img]()
				{
					color pixelColor(0, 0, 0);
					const auto u = (i + 0.5f) / (imageWidth - 1);
					const auto v = (j + 0.5f) / (imageHeight - 1);
					ray r = cam.getRay(u, v);
					pixelColor += RayColor(r, world, cam, maxDepth-1);
					img[j][i] = pixelColor;
				}));

			if(threads.size() > 256)
			{
				for (auto& thread : threads)
				{
					thread.wait();
				}
				threads.clear();
				std::cerr << "\r" << static_cast<int>((((imageWidth-i) + (imageHeight - j) * imageWidth) / static_cast<double>(imageHeight * imageWidth)) * 100.0) << "% of rendering is completed         " << std::flush;
			}
		}
	}
	for (auto& thread : threads)
	{
		thread.get();
	}
	std::cerr << std::endl;
	for (int j = imageHeight - 1; j >= 0; j--)
	{
		std::cerr << "\r" << static_cast<int>((static_cast<double>(imageHeight - j) / imageHeight) * 100.0) << "% of file write is completed         " << std::flush;
		for (int i = 0; i < imageWidth; i++)
		{
			write_color(image, img[j][i]);
		}
	}
	std::cerr << "\nDone.\n";

	auto end = std::chrono::steady_clock::now();
	std::cerr << "Elapsed time in milliseconds: "
			<< std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
			<< " ms" << std::endl;

}

int main(int argc, char* argv[])
{

	parser::Scene scene;
	scene.loadFromXml("../../Assets/scienceTree_glass.xml");

	//image

	//world
	scene_list world = hittableListFromScene(scene);

	for(int i = 0; i < scene.cameras.size(); i++)
	{
		render_camera(scene, i, world);
	}
}