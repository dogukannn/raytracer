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

color RayColor(const ray& r, const hittable& world, int depth)
{

	if(depth <= 0)
	{
		return color(0, 0, 0);
	}

	hitRecord rec;
	if(world.hit(r, 0.001, infinity, rec))
	{
		const vec3 unitDirection = unit(r.direction());
		return -dot(rec.normal, unitDirection) * color(1, 1, 0);
		ray scattered;
		color attenuation;
		if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
			return attenuation * RayColor(scattered, world, depth - 1);
		return color(0, 0, 0);
	}
	const vec3 unitDirection = unit(r.direction());
	auto t = 0.5 * (unitDirection.y() + 1.0);
	return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0);
}

scene_list random_scene() {
    scene_list world;

    auto ground_material = std::make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(std::make_shared<sphere>(point3(0,-1000,0), 1000, ground_material));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = randomDouble();
            point3 center(a + 0.9*randomDouble(), 0.2, b + 0.9*randomDouble());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                std::shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo = color::random() * color::random();
                    sphere_material = std::make_shared<lambertian>(albedo);
                    world.add(std::make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = randomDouble(0, 0.5);
                    sphere_material = std::make_shared<metal>(albedo, fuzz);
                    world.add(std::make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = std::make_shared<dielectric>(1.5);
                    world.add(std::make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = std::make_shared<dielectric>(1.5);
    world.add(std::make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = std::make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(std::make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = std::make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(std::make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    return world;
}


scene_list hittableListFromScene(const parser::Scene& scene)
{
	scene_list world;

	auto albedo = color::random() * color::random();
	auto random_material = std::make_shared<lambertian>(albedo);

	for(auto& face : scene.meshes[0].faces)
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

		world.add(std::make_shared<triangle>(p1, p2, p3, random_material));
	}
	return world;
}

point3 to_p(parser::Vec3f v)
{
	return { v.x, v.y, v.z };
}

vec3 to_v(parser::Vec3f v)
{
	return { v.x, v.y, v.z };
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
					pixelColor += RayColor(r, world, maxDepth);
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
			write_color(image, img[j][i], 1);
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
	scene.loadFromXml("../../Assets/bunny.xml");

	//image

	//world
	scene_list world = hittableListFromScene(scene);

	for(int i = 0; i < scene.cameras.size(); i++)
	{
		render_camera(scene, i, world);
	}
}