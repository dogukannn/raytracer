#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <future>
#include <queue>

//project headers
#include "Include/camera.h"
#include "Include/common.h"
#include "Include/color.h"
#include "Include/scene_list.h"
#include "Include/material.h"
#include "Include/sphere.h"
#include "Include/parser.h"
#include "Include/triangle.h"


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Include/bvh.h"
#include "Include/stb_image_write.h"

//#define OLD_THREADING

class ThreadPool {
public:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable task_condition;
    std::condition_variable finished_condition;
    std::atomic<bool> stop{false};
    std::atomic<size_t> active_tasks{0};

public:
    ThreadPool(size_t threads) {
        for(size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while(true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        task_condition.wait(lock, [this] {
                            return stop || !tasks.empty();
                        });
                        
                        if(stop && tasks.empty()) {
                            return;
                        }
                        
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    
                    active_tasks++;
                    task();
                    
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        active_tasks--;
                    }
                    finished_condition.notify_all();
                }
            });
        }
    }

    template<class F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::forward<F>(f));
        }
        task_condition.notify_one();
    }

    void wait_all() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        finished_condition.wait(lock, [this] {
            return tasks.empty() && active_tasks == 0;
        });
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        task_condition.notify_all();
        for(std::thread& worker : workers) {
            worker.join();
        }
    }
};    


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
	if(world.hit(r, 0.0001, infinity, rec, nullptr))
	{
		if(auto basicmat = rec.mat_ptr->as_basic())
		{
			return basicmat->calc_color(r, rec, world, cam);
		}
		else if (auto mirrormat = rec.mat_ptr->as_mirror())
		{
			return mirrormat->calc_color(r, rec, world, cam) + mirrormat->mirror_reflectance * RayColor(ray(rec.p + rec.normal * 0.001f, unit(mirrormat->reflected_ray(r, rec).direction())), world, cam, depth - 1);
		}
		else if (auto diemat = rec.mat_ptr->as_dielectric())
		{

			if (!rec.frontFace)
			{
				rec.normal = -rec.normal;
			}

			double refractionRatio = rec.frontFace ? (1.0 / diemat->refraction_index) : diemat->refraction_index;
			auto d = unit(r.direction());
			double cosi = dot(-d, rec.normal);
			double k = 1.0 - refractionRatio * refractionRatio * (1 - cosi * cosi);
			if (k < 0)
			{
				color ber = color(1,1,1);
				hitRecord refrec;
				if(world.hit(ray(rec.p, unit(diemat->reflected_ray(r, rec).direction())), 0.001, infinity, refrec, nullptr))
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
				if(world.hit(ray(rec.p, unit(diemat->reflected_ray(r, rec).direction())), 0.001, infinity, refrec, nullptr))
				{
					ber = beerslaw(refrec.t, diemat->absorption_coef);
				}

				col += frefl * RayColor(diemat->reflected_ray(r, rec), world, cam, depth - 1) * ber;

				return col;
			}

			auto col = diemat->calc_color(r, rec, world, cam);

			color ber = color(1,1,1);
			hitRecord refrec;
			if(world.hit(ray(rec.p, refractdir), 0.001, infinity, refrec, nullptr))
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
	return { (float)v.x, (float)v.y, (float)v.z };
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

mat4 model_matrix_from_transforms(const std::vector<std::string>& transforms, parser::Scene& scene)
{
	mat4 model = mat4(1.0f);
	for(auto transform : transforms)
	{
		if (transform[0] == 't')
		{
			assert(scene.translations.count(transform) > 0);
			auto translation = scene.translations[transform];
			model = mat4::translate(vec3(translation.translation.x, translation.translation.y, translation.translation.z)) * model;
		}
		else if (transform[0] == 's')
		{
			assert(scene.scalings.count(transform) > 0);
			auto scaling = scene.scalings[transform];
			model = mat4::scale(vec3(scaling.scaling.x, scaling.scaling.y, scaling.scaling.z)) * model;
		}
		else if (transform[0] == 'r')
		{
			assert(scene.rotations.count(transform) > 0);
			auto rotation = scene.rotations[transform];
			model = mat4::rotate_degrees(to_v(rotation.rotation), rotation.angle) * model;
		}
	}
	return model;
}

scene_list hittableListFromScene(parser::Scene& scene)
{
	scene_list world;

	std::vector<triangle> triangles;

	std::unordered_map<int, std::shared_ptr<BVH>> bvh_map;
	std::unordered_map<int, mat4> mesh_model_map;
	std::unordered_map<int, std::shared_ptr<material>> mat_map;

	std::vector<std::shared_ptr<BVHInstance>> bvh_instances;

	for(auto& [mesh_id, mesh] : scene.meshes)
	{
		
		
		if (!mesh.is_instance)
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


				triangle t(p1, p2, p3, mesh_material);
				triangles.push_back(t);
				//world.add(std::make_shared<triangle>(p1, p2, p3, mesh_material));
			}

			if(triangles.size() > 0)
			{
				//build bvh
				auto bvh = std::make_shared<BVH>();
				bvh->build(std::move(triangles));
				bvh_map[mesh_id] = bvh;
				auto bvh_instance = std::make_shared<BVHInstance>(bvh);
				bvh_instance->model = model_matrix_from_transforms(mesh.transformations, scene);
				mesh_model_map[mesh_id] = bvh_instance->model;
				bvh_instance->mat_ptr = mesh_material;
				mat_map[mesh_id] = mesh_material;
				//world.add(bvh_instance);
				bvh_instances.push_back(bvh_instance);
			}
			triangles.clear();
		}
		else
		{
			auto bvh_instance = std::make_shared<BVHInstance>(bvh_map[mesh.base_mesh_id]);
			if (mesh.reset_transform)
			{
				bvh_instance->model = model_matrix_from_transforms(mesh.transformations, scene);
			}
			else
			{
				bvh_instance->model = model_matrix_from_transforms(mesh.transformations, scene) * mesh_model_map[mesh.base_mesh_id];
			}
			bvh_map[mesh_id] = bvh_map[mesh.base_mesh_id];
			mesh_model_map[mesh_id] = bvh_instance->model;

			if(mesh.material_id == -1)
			{
				bvh_instance->mat_ptr = mat_map[mesh.base_mesh_id];
				mat_map[mesh_id] = mat_map[mesh.base_mesh_id];
			}
			else
			{
				auto& mat = scene.materials[mesh.material_id-1];
				std::shared_ptr<material> mesh_material = convert_material(mat);
				bvh_instance->mat_ptr = mesh_material;
			}
			//world.add(bvh_instance);
			bvh_instances.push_back(bvh_instance);
		}
		}

	if (bvh_instances.size() > 0)
	{
		auto tl_bvh = std::make_shared<TLBVH>();
		tl_bvh->build(std::move(bvh_instances));
		world.add(tl_bvh);
	}

	for(auto& sp : scene.spheres)
	{
		point3 c = to_p(scene.vertex_data[sp.center_vertex_id-1]);

		auto& mat = scene.materials[sp.material_id-1];
		std::shared_ptr<material> mesh_material = convert_material(mat);

		auto sph = std::make_shared<sphere>(c, sp.radius, mesh_material);

		sph->model = model_matrix_from_transforms(sp.transformations, scene);

		world.add(sph);
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
		triangle t(p1, p2, p3, mesh_material);

		t.model = model_matrix_from_transforms(tr.transformations, scene);

		//triangles.push_back(t);
		world.add(std::make_shared<triangle>(t));
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

	std::vector<std::vector<color>> img;
	img.resize(imageHeight);
	for(auto& vec : img)
	{
		vec.resize(imageWidth);
	}

	//debug specific pixel
	//{
	//	color pixelColor(0, 0, 0);
	//	const auto u = (475 + 0.5f) / (imageWidth - 1);
	//	const auto v = (800 - 267 + 0.5f) / (imageHeight - 1);
	//	ray r = cam.getRay(u, v);
	//	pixelColor += RayColor(r, world, cam, maxDepth);
	//	return;
	//}

#ifdef OLD_THREADING
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
#else
	std::vector<std::future<void>> threads;

	const size_t num_threads = std::thread::hardware_concurrency();
    ThreadPool pool(num_threads);

	const int TILE_SIZE_X = 32;
    const int TILE_SIZE_Y = 32;

    // Calculate number of tiles
    const int num_tiles_x = (imageWidth + TILE_SIZE_X - 1) / TILE_SIZE_X;
    const int num_tiles_y = (imageHeight + TILE_SIZE_Y - 1) / TILE_SIZE_Y;
    const int total_tiles = num_tiles_x * num_tiles_y;

	for (int ty = 0; ty < num_tiles_y; ty++)
	{
		for (int tx = 0; tx < num_tiles_x; tx++)
		{
            int startX = tx * TILE_SIZE_X;
            int startY = ty * TILE_SIZE_Y;
            int endX = std::min(startX + TILE_SIZE_X, imageWidth);
            int endY = std::min(startY + TILE_SIZE_Y, imageHeight);

			pool.enqueue([total_tiles, startX, startY, endX, endY, &cam, &world, imageHeight, imageWidth, maxDepth, &img]()
				{
					for (int j = endY - 1; j >= startY; j--)
					{
						for (int i = startX; i < endX; i++)
						{
							color pixelColor(0, 0, 0);
							const auto u = (i + 0.5f) / (imageWidth);
							const auto v = (j + 0.5f) / (imageHeight);
							ray r = cam.getRay(u, v);
							pixelColor += RayColor(r, world, cam, maxDepth - 1);
							img[j][i] = pixelColor;
						}
					}
				});
		}
	}

	pool.wait_all();
#endif
	
	//convert img data to raw for saving as png using stb (r g b floats range in 0.0f - 255.99f)
	std::vector<unsigned char> raw;
	raw.resize(imageWidth * imageHeight * 3);

	for (int j = imageHeight - 1; j >= 0; j--)
	{
		for (int i = 0; i < imageWidth; i++)
		{
			auto& pixelColor = img[j][i];
			int index = ((imageHeight - j - 1) * imageWidth + i) * 3;
			raw[index] = static_cast<unsigned char>(clamp(pixelColor.x(), 0.0, 255.999));
			raw[index + 1] = static_cast<unsigned char>(clamp(pixelColor.y(), 0.0, 255.999));
			raw[index + 2] = static_cast<unsigned char>(clamp(pixelColor.z(), 0.0, 255.999));
		}
	}
	
	if (!stbi_write_png((scene_cam.image_name).c_str(), imageWidth, imageHeight, 3, raw.data(), 0))
	{
		std::cerr << "Failed to save image" << std::endl;
	}

	auto end = std::chrono::steady_clock::now();
	std::cerr << scene_cam.image_name << " Elapsed time in milliseconds: "
			<< std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
			<< " ms" << std::endl;

}

int main(int argc, char* argv[])
{
	parser::Scene scene;
	//parse args and load scene
	if (argc > 1)
	{
		scene.loadFromXml(argv[1]);
	}
	else
	{
		std::cerr << "No scene file provided" << std::endl;
		return 1;
	}

	//adjust cameras and point lights according to transformations
	for (auto& cam : scene.cameras)
	{
		auto model = model_matrix_from_transforms(cam.transformations, scene);
		vec3 pos = { cam.position.x, cam.position.y, cam.position.z };
		vec3 gaze = { cam.gaze.x, cam.gaze.y, cam.gaze.z };
		vec3 up = { cam.up.x, cam.up.y, cam.up.z };

		pos = to_vec3(model * vec4(pos, 1.0f));
		gaze = to_vec3(model * vec4(gaze, 0.0f));
		up = cross(gaze, cross(up, gaze));
		//up = to_vec3(model * vec4(up, 0.0f));

		gaze = unit(gaze);
		up = unit(up);

		cam.position = { pos.x(), pos.y(), pos.z() };
		cam.gaze = { gaze.x(), gaze.y(), gaze.z() };
		cam.up = { up.x(), up.y(), up.z() };
	}

	for (auto& pl : scene.point_lights)
	{
		auto model = model_matrix_from_transforms(pl.transformations, scene);

		vec3 pos = { pl.position.x, pl.position.y, pl.position.z };
		pos = to_vec3(model * vec4(pos, 1.0f));

		pl.position = { pos.x(), pos.y(), pos.z() };
	}

	//world
	scene_list world = hittableListFromScene(scene);



	for(int i = 0; i < scene.cameras.size(); i++)
	{
		render_camera(scene, i, world);
	}
}