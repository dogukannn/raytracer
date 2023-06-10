#include <iostream>
#include <fstream>

//project headers
#include "Include/camera.h"
#include "Include/common.h"
#include "Include/color.h"
#include "Include/hittableList.h"
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
		point3 target = rec.p + rec.normal + randomInUnitSphere();
		return 0.5 * RayColor(ray(rec.p, target - rec.p), world, depth - 1);
	}
	const vec3 unitDirection = unit(r.direction());
	auto t = 0.5 * (unitDirection.y() + 1.0);
	return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0);
}


int main(int argc, char* argv[]) 
{
	//image
	constexpr int imageWidth = 800;
	constexpr int imageHeight = static_cast<int>(imageWidth / aspectRatio);
	constexpr int samplesPerPixel = 200;
	constexpr int maxDepth = 50;

	//world
	hittableList world;
	world.add(std::make_shared<sphere>(point3(0, 0, -1), 0.5));
	world.add(std::make_shared<sphere>(point3(0, -100.5, -1), 100));

	//camera
	camera cam;

	std::ofstream image("image.ppm");

	image << "P3\n" << imageWidth << " " << imageHeight << "\n255\n";
	for(int j = imageHeight-1; j >= 0; j--)
	{
		std::cerr << "\r" << static_cast<int>((static_cast<double>(imageHeight - j) / imageHeight) * 100.0) << "% is completed         " << std::flush;
		for(int i = imageWidth-1; i >= 0; i --)
		{
			color pixelColor(0, 0, 0);
			for (int s = samplesPerPixel - 1; s >= 0; --s)
			{
				const auto u = (i + randomDouble()) / (imageWidth - 1);
				const auto v = (j + randomDouble()) / (imageHeight - 1);
				ray r = cam.getRay(u, v);
				pixelColor += RayColor(r, world, maxDepth);
			}
			write_color(image, pixelColor, samplesPerPixel);
		}
	}
	std::cerr << "\nDone.\n";
}