#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <future>

//project headers
#include "Include/camera.h"
#include "Include/common.h"
#include "Include/color.h"
#include "Include/hittableList.h"
#include "Include/material.h"
#include "Include/sphere.h"

color RayColor(const ray& r, const hittable& world, int depth)
{

	if(depth <= 0)
	{
		return color(0, 0, 0);
	}

	hitRecord rec;
	if(world.hit(r, 0.001, infinity, rec))
	{
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

hittableList random_scene() {
    hittableList world;

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


int main(int argc, char* argv[])
{
	auto start = std::chrono::steady_clock::now();
	//image
	constexpr int imageWidth = 1200;
	constexpr int imageHeight = static_cast<int>(imageWidth / aspectRatio);
	constexpr int samplesPerPixel = 500;
	constexpr int maxDepth = 50;

	//world
	hittableList world = random_scene();

	//camera
	point3 lookfrom(13, 2, 3);
	point3 lookat(0, 0, 0);
	vec3 vup(0, 1, 0);
	auto distToFocus = 10;
	auto aperture = 0.1;
	camera cam(lookfrom, lookat, vup, 20, aspectRatio, aperture, distToFocus);

	std::ofstream image("image.ppm");

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
			threads.emplace_back(std::async([i, j, cam, world, imageHeight, imageWidth, maxDepth, samplesPerPixel, &img]()
				{
					color pixelColor(0, 0, 0);
					for (int s = samplesPerPixel - 1; s >= 0; --s)
					{
						const auto u = (i + randomDouble()) / (imageWidth - 1);
						const auto v = (j + randomDouble()) / (imageHeight - 1);
						ray r = cam.getRay(u, v);
						pixelColor += RayColor(r, world, maxDepth);
					}
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
			write_color(image, img[j][i], samplesPerPixel);
		}
	}
	std::cerr << "\nDone.\n";

	auto end = std::chrono::steady_clock::now();
	std::cerr << "Elapsed time in milliseconds: "
			<< std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
			<< " ms" << std::endl;
}