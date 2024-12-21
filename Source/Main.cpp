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

#define TINYEXR_IMPLEMENTATION
#include "Include/tinyexr.h"


bool SaveEXR(const float* rgb, int width, int height, const char* outfilename) {

    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage image;
    InitEXRImage(&image);

    image.num_channels = 3;

    std::vector<float> images[3];
    images[0].resize(width * height);
    images[1].resize(width * height);
    images[2].resize(width * height);

    // Split RGBRGBRGB... into R, G and B layer
    for (int i = 0; i < width * height; i++) {
      images[0][i] = rgb[3*i+0];
      images[1][i] = rgb[3*i+1];
      images[2][i] = rgb[3*i+2];
    }

    float* image_ptr[3];
    image_ptr[0] = &(images[2].at(0)); // B
    image_ptr[1] = &(images[1].at(0)); // G
    image_ptr[2] = &(images[0].at(0)); // R

    image.images = (unsigned char**)image_ptr;
    image.width = width;
    image.height = height;

    header.num_channels = 3;
    header.channels = new EXRChannelInfo[header.num_channels];
    // Must be (A)BGR order, since most of EXR viewers expect this channel order.
    strncpy(header.channels[0].name, "B", 255); header.channels[0].name[strlen("B")] = '\0';
    strncpy(header.channels[1].name, "G", 255); header.channels[1].name[strlen("G")] = '\0';
    strncpy(header.channels[2].name, "R", 255); header.channels[2].name[strlen("R")] = '\0';

    header.pixel_types = new int[header.num_channels];
    header.requested_pixel_types = new int[header.num_channels];
    for (int i = 0; i < header.num_channels; i++) {
      header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of input image
      header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF; // pixel type of output image to be stored in .EXR
    }

    const char* err = NULL; // or nullptr in C++11 or later.
    int ret = SaveEXRImageToFile(&image, &header, outfilename, &err);
    if (ret != TINYEXR_SUCCESS) {
      fprintf(stderr, "Save EXR err: %s\n", err);
      FreeEXRErrorMessage(err); // free's buffer for an error message
      return ret;
    }
    printf("Saved exr file. [ %s ] \n", outfilename);

    delete header.channels;
    delete header.pixel_types;
    delete header.requested_pixel_types;
  }

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
	return color(exp(-t * absorp.x), exp(-t * absorp.y), exp(-t * absorp.z));
}

color RayColor(const ray& r, const scene_list& world, const camera& cam, int depth, float* dist = nullptr, bool* hit = nullptr)
{
	if(depth <= 0)
	{
		return color(0, 0, 0);
	}

	hitRecord rec;
	if(world.hit(r, dist ? *dist - 0.001f : 0.001f, infinity, rec, nullptr))
	{
		//return color(1, 1, 1);
		if(rec.mat_ptr->normal_map)
		{
			auto normal = rec.normal;
			auto tangent = glm::normalize(rec.tangent);
			auto bitangent = glm::normalize(rec.bitangent);

			auto uv = rec.uv;

			auto normal_map = rec.mat_ptr->normal_map->value(uv.x, uv.y, rec.p);
			normal_map = glm::normalize(2.0f * normal_map - color(1, 1, 1));
			normal_map = glm::normalize(normal_map.x * tangent + normal_map.y * bitangent + normal_map.z * normal);

			rec.normal = normal_map;
		}

		if(rec.mat_ptr->bump_map && !rec.mat_ptr->bump_map->get_perlin())
		{
			auto normal = glm::normalize(rec.normal);
			auto tangent = rec.dpdu;
			auto bitangent = rec.dpdv;
			auto uv = rec.uv;

			auto bump_value = rec.mat_ptr->bump_map->value(uv.x, uv.y, rec.p);
			//rec.mat_ptr->bump_factor = 0.5f;

			color bump_bottom, bump_left, bump_top, bump_right;
			rec.mat_ptr->bump_map->area_values(uv.x, uv.y, rec.p, bump_top, bump_bottom, bump_left, bump_right);
			auto k = rec.negate ? -1.0f : 1.0f;

			
			float l0 = half_average(bump_value);
			float l1 = half_average(bump_right);
			float l2 = half_average(bump_top);

			rec.nobp = rec.p;
			//rec.p = rec.p + (rec.frontface ? normal : -normal) * l0;

			rec.p = rec.p  + rec.normal * l0 * rec.mat_ptr->bump_factor;

			auto delu = 1.0f / (float)rec.mat_ptr->bump_map->width;
			auto delv = 1.0f / (float)rec.mat_ptr->bump_map->height;
			//delu = delv;

			//rec.normal = vec3(l1 - l0, l2 - l0, 0.0f) * rec.mat_ptr->bump_factor * normal;
			//rec.normal = glm::normalize(rec.normal);
			auto dqdu = tangent + (-(l1 - l0)) * rec.mat_ptr->bump_factor * normal / delu;
			auto dqdv = bitangent + (1.0f * (l2 - l0)) * rec.mat_ptr->bump_factor * normal / delv;

			rec.normal = glm::normalize(cross(dqdv, dqdu));
			//rec.normal = glm::normalize(normal);

			//if (!rec.frontFace)
			//{
			//	rec.normal = -rec.normal;
			//}

			//if (rec.negate_normal)
			//{
			//	rec.normal = -rec.normal;
			//}

			//rec.normal = -glm::normalize(cross(tangent, bitangent));

			//return bitangent * 128.0f + 128.0f;
			
		}

		if(rec.mat_ptr->bump_map && rec.mat_ptr->bump_map->get_perlin())
		{
			auto perlin_texture = rec.mat_ptr->bump_map->get_perlin();

			auto pv = perlin_texture->value(rec.uv.x, rec.uv.y, rec.p).x;

			float eps = 0.0001f;
			auto dx = perlin_texture->value(rec.uv.x, rec.uv.y, rec.p + glm::vec3(eps, 0, 0)) - perlin_texture->value(rec.uv.x, rec.uv.y, rec.p);
			auto dy = perlin_texture->value(rec.uv.x, rec.uv.y, rec.p + glm::vec3(0, eps, 0)) - perlin_texture->value(rec.uv.x, rec.uv.y, rec.p);
			auto dz = perlin_texture->value(rec.uv.x, rec.uv.y, rec.p + glm::vec3(0, 0, eps)) - perlin_texture->value(rec.uv.x, rec.uv.y, rec.p);

			auto normal = glm::normalize(rec.normal);

			rec.nobp = rec.p;
			//rec.p = rec.p + normal * pv;

			auto gradient = glm::vec3(dx.x, dy.x, dz.x) / eps;
	
			auto g2 = dot(gradient, normal) * normal;
			auto g1 = gradient - g2;

			rec.normal = glm::normalize(normal - g1);
		}



		if(auto basicmat = rec.mat_ptr->as_basic())
		{
			return basicmat->calc_color(r, rec, world, cam);
		}
		else if (auto mirrormat = rec.mat_ptr->as_mirror())
		{
			auto c =  mirrormat->calc_color(r, rec, world, cam) + mirrormat->mirror_reflectance * RayColor(ray(rec.p + rec.normal * 0.001f, glm::normalize(mirrormat->reflected_ray(r, rec).direction())), world, cam, depth - 1);
			//check if any param of c is nan or - nan
			if (c.r != c.r || c.g != c.g || c.b != c.b)
			{
				return color(0, 0, 0);
			}
			return c;
		}
		else if (auto diemat = rec.mat_ptr->as_dielectric())
		{

			if (!rec.frontFace)
			{
				rec.normal = -rec.normal;
			}

			float refractionRatio = rec.frontFace ? (1.0 / diemat->refraction_index) : diemat->refraction_index;
			auto d = glm::normalize(r.direction());
			float cosi = dot(-d, rec.normal);
			float k = 1.0 - refractionRatio * refractionRatio * (1 - cosi * cosi);
			if (k < 0)
			{

				color ber = color(1,1,1);
				hitRecord refrec;
				if(world.hit(ray(rec.p, glm::normalize(diemat->reflected_ray(r, rec).direction())), 0.001, infinity, refrec, nullptr))
				{
					ber = beerslaw(refrec.t, diemat->absorption_coef);
				}
				return RayColor(diemat->reflected_ray(r, rec), world, cam, depth - 1) * ber;
			}
			float cosph = sqrt(k);
			//vec3 refractdir = d * refractionRatio + rec.normal * (refractionRatio * cosi - sqrt(k));
			vec3 refractdir = (d + rec.normal * cosi) * refractionRatio - rec.normal * cosph;
			refractdir = glm::normalize(refractdir);

			if(diemat->has_roughness)
			{

				auto r = glm::normalize(refractdir);
				auto rp = create_non_colinear_vector(r);
				auto u = glm::normalize(cross(r, rp));
				auto v = glm::normalize(cross(r, u));

				//auto rr = glm::normalize(r + u * roughness * roughness_u_offset + v * roughness * roughness_v_offset);
				auto rr = glm::normalize(r + u * diemat->roughness * ((frandom() - 0.5f) * 1.0f) + v * diemat->roughness * ((frandom() - 0.5f) * 1.0f));
				refractdir = rr;
			}

			float n1 = rec.frontFace ? (1.0) : diemat->refraction_index;
			float n2 = rec.frontFace ? (diemat->refraction_index) : 1.0f;
			float r2 = (n2 * cosi - n1 * cosph) / (n2 * cosi + n1 * cosph);
			float r1 = (n1 * cosi - n2 * cosph) / (n1 * cosi + n2 * cosph);

			float frefl = 0.5 * (r1 * r1 + r2 * r2);
			float frefr = 1 - frefl;

			if(!rec.frontFace)
			{
				color col = color(0,0,0);

				col += frefr * RayColor(ray(rec.p, refractdir), world, cam, depth - 1);

				color ber = color(1,1,1);
				hitRecord refrec;
				if(world.hit(ray(rec.p, glm::normalize(diemat->reflected_ray(r, rec).direction())), 0.001, infinity, refrec, nullptr))
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

			//return diemat->calc_color(r, rec, world, cam)
			//	+ frefr * RayColor(ray(rec.p, refractdir), world, cam, depth - 1)
			//	+ frefl * RayColor(diemat->reflected_ray(r, rec), world, cam, depth - 1);
		}
		else if (auto condmat = rec.mat_ptr->as_conductor())
		{
			auto d = glm::normalize(r.direction());
			float cosi = dot(-d, rec.normal);

			float n2 = condmat->refraction_index;
			float k2 = condmat->absorption_index;

			float rs = ((n2 * n2 + k2 * k2) - 2 * n2 * cosi + cosi * cosi) / ((n2 * n2 + k2 * k2) + 2 * n2 * cosi + cosi * cosi);
			float rp = ((n2 * n2 + k2 * k2) * cosi * cosi - 2 * n2 * cosi + 1) / ((n2 * n2 + k2 * k2) * cosi * cosi + 2 * n2 * cosi + 1);

			float frefl = 0.5 * (rs + rp);

			return condmat->calc_color(r, rec, world, cam)
				+ condmat->mirror_reflectance * frefl * RayColor(condmat->reflected_ray(r, rec), world, cam, depth - 1);
		}
	}

	if(hit)
		*hit = false;
        //<Composite id="1">0.9885 -0.1515 0 0.7262 0.1515 0.9885 0 -4.2062 0 0 1 0 0 0 0 1</Composite>
        //<Composite id="1">1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1</Composite>

	if(world.bg_texture)
	{
		switch (world.bg_type)
		{
		case background_type::spherical:
		{
			vec3 dir = r.dir;
			float r = (1.0f / pi) * ((acos(-dir.z) / sqrtf(dir.x * dir.x + dir.y * dir.y)));
			float ub = (r * dir.x + 1) / 2.0f;
			float vb = (r * -dir.y + 1) / 2.0f;


			return world.bg_texture->value(ub, vb, { 0,0,0 });
		}
		case background_type::latlong:
		{
			vec3 dir = r.dir;
			float ub = (1.0f + (atan2(dir.x, -dir.z) / pi)) / 2.0f;
			float vb = acos(dir.y) / pi;
			return world.bg_texture->value(ub, vb, { 0,0,0 });
		}
		default:
		{
			
		}
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

vec2 to_v(parser::Vec2f v)
{
	return { v.x, v.y };
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

	if(mat.has_roughness)
	{
		mesh_material->has_roughness = true;
		mesh_material->roughness = mat.roughness;
	}
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
			model = glm::translate(glm::identity<mat4>(), vec3(translation.translation.x, translation.translation.y, translation.translation.z)) * model;
		}
		else if (transform[0] == 's')
		{
			assert(scene.scalings.count(transform) > 0);
			auto scaling = scene.scalings[transform];
			model = glm::scale(glm::identity<mat4>(),vec3(scaling.scaling.x, scaling.scaling.y, scaling.scaling.z)) * model;
		}
		else if (transform[0] == 'r')
		{
			assert(scene.rotations.count(transform) > 0);
			auto rotation = scene.rotations[transform];
			model = glm::rotate(glm::identity<mat4>(), glm::radians(rotation.angle), to_v(rotation.rotation)) * model;
		}
		else if (transform[0] == 'c')
		{
			assert(scene.composites.count(transform) > 0);
			auto composite = scene.composites[transform];
			glm::mat4 cm;
			for (int i = 0; i < 16; i++)
			{
				cm[i % 4][i / 4] = composite.matrix[i];
			}
			model = cm * model;
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

	//find replace_background typed texture
	for(auto& tex : scene.textures)
	{
		if(tex.type != parser::replace_background)
			continue;

		std::shared_ptr<texture> bg_tex = std::make_shared<texture>();
		switch (tex.interpolation)
		{
		case parser::interpolation_type::nearest:
			bg_tex->sampler_type = sampler::nearest;
			break;
		case parser::interpolation_type::bilinear:
			bg_tex->sampler_type = sampler::bilinear;
			break;
		case parser::interpolation_type::trilinear:
			bg_tex->sampler_type = sampler::trilinear;
			break;
		default:
			bg_tex->sampler_type = sampler::bilinear;
			break;
		}

		parser::Image& img = scene.images[tex.image_id - 1];
		bg_tex->width = img.width;
		bg_tex->height = img.height;
		bg_tex->data = (color*)img.data;
		bg_tex->normalizer = tex.normalizer;

		world.bg_texture = bg_tex;
	}

	for(auto& [mesh_id, mesh] : scene.meshes)
	{
		if (!mesh.is_instance)
		{
			auto& mat = scene.materials[mesh.material_id-1];
			std::shared_ptr<material> mesh_material = convert_material(mat);

			for (auto& texture_id : mesh.texture_ids)
			{
				
				auto& tex = scene.textures[texture_id - 1];

				std::shared_ptr<texture> mesh_texture = std::make_shared<texture>();

				if(!tex.is_perlin)
				{
					switch (tex.interpolation)
					{
					case parser::interpolation_type::nearest:
						mesh_texture->sampler_type = sampler::nearest;
						break;
					case parser::interpolation_type::bilinear:
						mesh_texture->sampler_type = sampler::bilinear;
						break;
					case parser::interpolation_type::trilinear:
						mesh_texture->sampler_type = sampler::trilinear;
						break;
					default:
						mesh_texture->sampler_type = sampler::bilinear;
						break;
					}

					parser::Image& img = scene.images[tex.image_id - 1];
					mesh_texture->width = img.width;
					mesh_texture->height = img.height;
					mesh_texture->data = (color*)img.data;
					mesh_texture->normalizer = tex.normalizer;
					
				}
				else
				{
					auto p = std::make_shared<perlin>();
					p->num_octaves = tex.perlin_octave;
					p->freq = tex.perlin_freq;
					p->is_linear = tex.is_linear;
					mesh_texture = p;
				}

				switch (tex.type)
				{
				case parser::texture_type::replace_kd:
					mesh_material->diffuse_map = mesh_texture;
					break;
				case parser::texture_type::replace_ks:
					mesh_material->specular_map = mesh_texture;
					break;
				case parser::texture_type::replace_all:
					mesh_material->diffuse_map = mesh_texture;
					mesh_material->specular_map = mesh_texture;
					mesh_material->ambient_map = mesh_texture;
					break;
				case parser::texture_type::replace_normal:
					mesh_material->normal_map = mesh_texture;
					break;
				case parser::texture_type::blend_kd:
					mesh_material->blend_diffuse = true;
					mesh_material->diffuse_map = mesh_texture;
					break;
				case parser::texture_type::bump_normal:
					mesh_material->bump_map = mesh_texture;
					mesh_material->bump_factor = tex.bump_factor;
					break;
				}

			}

			for(auto& face : mesh.faces)
			{
				auto v0_id = face.v0_id + mesh.vertex_offset;
				auto v1_id = face.v1_id + mesh.vertex_offset;
				auto v2_id = face.v2_id + mesh.vertex_offset;

				point3 p1 = { scene.vertex_data[v0_id - 1].x,
								scene.vertex_data[v0_id - 1].y,
								scene.vertex_data[v0_id - 1].z };

				point3 p2 = { scene.vertex_data[v1_id - 1].x,
							scene.vertex_data[v1_id - 1].y,
								scene.vertex_data[v1_id - 1].z };

				point3 p3 = { scene.vertex_data[v2_id - 1].x,
								scene.vertex_data[v2_id - 1].y,
								scene.vertex_data[v2_id - 1].z };

				triangle t(p1, p2, p3, mesh_material);

				t.is_ply = mesh.is_ply;

				auto uv0_id = face.v0_id - 1 + mesh.uv_offset;
				auto uv1_id = face.v1_id - 1 + mesh.uv_offset;
				auto uv2_id = face.v2_id - 1 + mesh.uv_offset;

				//check all textures if one of them is not perlin
				bool should_have_uv = false;
				for (auto& texture_id : mesh.texture_ids)
				{
					auto& tex = scene.textures[texture_id - 1];
					if (!tex.is_perlin)
					{
						should_have_uv = true;
					}
				}

				auto normal = glm::normalize(glm::cross(p2 - p1, p3 - p1));

				if (mesh.texture_ids.size() > 0 && should_have_uv)
				{
					glm::vec2 uv1 = { scene.vertex_uv_data[uv0_id].x
								, scene.vertex_uv_data[uv0_id].y };

					glm::vec2 uv2 = { scene.vertex_uv_data[uv1_id].x
						, scene.vertex_uv_data[uv1_id].y };

					glm::vec2 uv3 = { scene.vertex_uv_data[uv2_id].x
						, scene.vertex_uv_data[uv2_id].y };

					//check for negative uv
					if (uv1.x < 0 || uv1.y < 0 || uv1.x > 1 || uv1.y > 1)
					{
						uv1 = glm::vec2(0, 0);
					}


					t.uv1 = uv1;
					t.uv2 = uv2;
					t.uv3 = uv3;

					vec3 tv0v1 = p2 - p1;
					vec3 tv0v2 = p3 - p1;

					vec2 deltaUV1 = uv2 - uv1;
					vec2 deltaUV2 = uv3 - uv1;

					auto tangent = (tv0v1 * deltaUV2.y - tv0v2 * deltaUV1.y) / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
					auto bitangent = (tv0v2 * deltaUV1.x - tv0v1 * deltaUV2.x) / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);


					t.dpdu = tangent;
					t.dpdv = bitangent;
				}
				t.normal = glm::normalize(normal);
				//precompute normals, tangent and bitangent





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
				if (mesh.has_motion_blur)
				{
					bvh_instance->has_motion_blur = true;
					bvh_instance->motion = to_v(mesh.motion);
				}
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

			if (mesh.has_motion_blur)
			{
				bvh_instance->has_motion_blur = true;
				bvh_instance->motion = to_v(mesh.motion);
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

		for (auto& texture_id : sp.texture_ids)
		{
			auto& tex = scene.textures[texture_id - 1];

				std::shared_ptr<texture> mesh_texture = std::make_shared<texture>();

				if(!tex.is_perlin)
				{
					switch (tex.interpolation)
					{
					case parser::interpolation_type::nearest:
						mesh_texture->sampler_type = sampler::nearest;
						break;
					case parser::interpolation_type::bilinear:
						mesh_texture->sampler_type = sampler::bilinear;
						break;
					case parser::interpolation_type::trilinear:
						mesh_texture->sampler_type = sampler::trilinear;
						break;
					default:
						mesh_texture->sampler_type = sampler::bilinear;
						break;
					}

					parser::Image& img = scene.images[tex.image_id - 1];
					mesh_texture->width = img.width;
					mesh_texture->height = img.height;
					mesh_texture->data = (color*)img.data;
					mesh_texture->normalizer = tex.normalizer;
					
				}
				else
				{
					auto p = std::make_shared<perlin>();
					p->num_octaves = tex.perlin_octave;
					p->freq = tex.perlin_freq;
					p->is_linear = tex.is_linear;
					mesh_texture = p;
				}

				switch (tex.type)
				{
				case parser::texture_type::replace_kd:
					mesh_material->diffuse_map = mesh_texture;
					break;
				case parser::texture_type::replace_ks:
					mesh_material->specular_map = mesh_texture;
					break;
				case parser::texture_type::replace_all:
					mesh_material->diffuse_map = mesh_texture;
					mesh_material->specular_map = mesh_texture;
					mesh_material->ambient_map = mesh_texture;
					break;
				case parser::texture_type::replace_normal:
					mesh_material->normal_map = mesh_texture;
					break;
				case parser::texture_type::blend_kd:
					mesh_material->blend_diffuse = true;
					mesh_material->diffuse_map = mesh_texture;
					break;
				case parser::texture_type::bump_normal:
					mesh_material->bump_map = mesh_texture;
					mesh_material->bump_factor = tex.bump_factor;
					break;
				}

		}

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
		point_light p(to_c(pl.intensity), to_p(pl.position));
		world.lights.push_back(std::make_shared<point_light>(p));
	}

	//area lights
	for(auto& al : scene.area_lights)
	{
		area_light a(to_c(al.intensity), to_p(al.position), to_v(al.normal), al.size);
		world.lights.push_back(std::make_shared<area_light>(a));
	}

	//spot lights
	for (auto& sl : scene.spot_lights)
	{
		spot_light s(to_c(sl.intensity), to_p(sl.position), to_v(sl.direction), sl.falloff_angle, sl.coverage_angle);
		world.lights.push_back(std::make_shared<spot_light>(s));
	}

	//directional lights
	for (auto& dl : scene.directional_lights)
	{
		directional_light d(to_c(dl.intensity), glm::normalize(to_v(dl.direction)));
		world.directional_lights.push_back(std::make_shared<directional_light>(d));
	}

	//spherical directional lights
	for (auto& sdl : scene.spherical_directional_lights)
	{
		auto& img = scene.images[sdl.image_id - 1];
		std::shared_ptr<texture> env_texture = std::make_shared<texture>();
		env_texture->width = img.width;
		env_texture->height = img.height;
		env_texture->data = (color*)img.data;
		env_texture->normalizer = 255.0f;
		env_texture->sampler_type = sampler::bilinear;
		envmap_type type = sdl.type == parser::sphere_light_type::spherical ? envmap_type::spherical : envmap_type::latlong;
		world.spherical_directional_lights.push_back(std::make_shared<spherical_directional_light>(env_texture, type));

		world.bg_texture = env_texture;
		world.bg_type = type == envmap_type::spherical ? background_type::spherical : background_type::latlong;
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
	glm::vec3 vup = to_v(scene_cam.up);
	auto distToFocus = scene_cam.near_distance;
	double aspectRatio = imageWidth / (float)imageHeight;

	camera cam(lookfrom, lookat, vup, scene_cam.near_plane, scene_cam.enable_dof, scene_cam.aperture, scene_cam.focus_distance, distToFocus);

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

	//scene_cam.num_samples = 100;

	for (int ns = 0; ns < scene_cam.num_samples; ns++)
	{
		motion_blur_mp = frandom();
		sample_u_offset = frandom();
		sample_v_offset = frandom();
		lens_x_offset = frandom();
		lens_y_offset = frandom();
		roughness_u_offset = (frandom() - 0.5f) * 2.0f;
		roughness_v_offset = (frandom() - 0.5f) * 2.0f;
		area_light_u_offset = frandom();
		area_light_v_offset = frandom();

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
								const auto u = (i + sample_u_offset) / (float)(imageWidth);
								const auto v = (j + sample_v_offset) / (float)(imageHeight);
								ray r = cam.getRay(u, v);
								float length = glm::length(r.direction());
								r.dir = glm::normalize(r.direction());
								bool does_hit = true;
								pixelColor += RayColor(r, world, cam, maxDepth - 1, cam.dof_enabled ? nullptr : &length, &does_hit);
								if(!does_hit && world.bg_texture)
								{
									switch (world.bg_type)
									{
									case background_type::spherical:
									{
										vec3 dir = r.dir;
										float r = (1.0f / pi) * ((acos(-dir.z) / sqrtf(dir.x * dir.x + dir.y * dir.y)));
										float ub = (r * dir.x + 1) / 2.0f;
										float vb = (r * -dir.y + 1) / 2.0f;

										//pixelColor += world.bg_texture->value(ub, vb, { 0,0,0 }) * 255.99f * 2.0f * pi;
										break;
									}
									case background_type::latlong:
									{
										vec3 dir = r.dir;
										float ub = (1.0f + (atan2(dir.x, -dir.z) / pi)) / 2.0f;
										float vb = acos(dir.y) / pi;
										//pixelColor += world.bg_texture->value(ub, vb, { 0,0,0 }) * 255.99f * 2.0f * pi;
										break;
									}
									case background_type::color:
									{
										pixelColor += world.bg_texture->value(u, 1.0f - v, { 0,0,0 }) * 255.99f;
										break;
									}
									}
								}

								img[j][i] += pixelColor;
							}
						}
					});
			}
		}
		pool.wait_all();
	}
#endif

	//normalize pixel values
	for (int j = imageHeight - 1; j >= 0; j--)
	{
		for (int i = 0; i < imageWidth; i++)
		{
			color& c = img[j][i];
			c = c / (float)scene_cam.num_samples;
			//c = c / 255.99f;
			//check if any param of c is nan or - nan
			if (c.r != c.r || c.g != c.g || c.b != c.b)
			{
				std::cerr << "nan detected" << std::endl;
			}

		}

	}

	//tonemap
	if(scene_cam.enable_tonemap)
	{
		int pixel_count = imageWidth * imageHeight;
		float epsilon = 1e-5;

		std::vector<float> luminance_values;
		luminance_values.reserve(pixel_count);

		float max_luminance = 0.0f;
		float min_luminance = std::numeric_limits<float>::max();
		float total_log_luminance = 0.0f;

		for (int j = imageHeight - 1; j >= 0; j--)
		{
			for (int i = 0; i < imageWidth; i++)
			{
				color c = img[j][i];
				float luminance_world = luminance(c);
				if (luminance_world > max_luminance)
				{
					max_luminance = luminance_world;
				}
				if (luminance_world < min_luminance)
				{
					min_luminance = luminance_world;
				}
				//luminance_values[j * imageWidth + i] = luminance_world;
				//luminance_values.push_back(luminance_world);
				total_log_luminance += log(epsilon + luminance_world);
			}
		}

		float lwp = exp((1.0f / pixel_count) * total_log_luminance);


		for (int j = imageHeight - 1; j >= 0; j--)
		{
			for (int i = 0; i < imageWidth; i++)
			{
				color c = img[j][i];
				float l = (scene_cam.key_value / lwp) * luminance(c);
				luminance_values.push_back(l);
			}
		}

		std::sort(luminance_values.begin(), luminance_values.end());

		//get the burnout percentage
		
		float burnout_percentage = scene_cam.burn_percent / 100.0f;
		float lwhite = 0.0f;
		if (scene_cam.burn_percent != 0.0f)
		{
			int burnout_index = (int)(pixel_count * (1.0f - burnout_percentage));
			lwhite = luminance_values[burnout_index];
		}

		//std::vector<std::vector<color>> img_ld;
		//img.resize(imageHeight);
		//for(auto& vec : img_ld)
		//{
		//	vec.resize(imageWidth);
		//}

		for (int j = imageHeight - 1; j >= 0; j--)
		{
			for (int i = 0; i < imageWidth; i++)
			{
				color& c = img[j][i];

				float ld = 1.0f;
				if(scene_cam.burn_percent != 0.0f)
				{
					//eq 4
					float l = (scene_cam.key_value / lwp) * luminance(c);
					ld = (l * (1.0f + (l / (lwhite * lwhite)))) / (1.0f + l);
				}
				else
				{
					float l = (scene_cam.key_value * luminance(c)) / lwp;
					ld = l / (l + 1.0f);
				}

				auto tmpc = c;
				if(luminance(c) < 0.00001f)
				{
					continue;
				}

				float lum = luminance(c);
				c.x = ld * (pow(c.x / lum, scene_cam.saturation));
				c.y = ld * (pow(c.y / lum, scene_cam.saturation));
				c.z = ld * (pow(c.z / lum, scene_cam.saturation));

			}
		}

	}

	//remove extension from image name
	auto pos = scene_cam.image_name.find_last_of(".");
	if (pos != std::string::npos)
	{
		scene_cam.image_name = scene_cam.image_name.substr(0, pos);
	}

	
	//convert img data to raw for saving as png using stb (r g b floats range in 0.0f - 255.99f)
	std::vector<unsigned char> raw;
	raw.resize(imageWidth * imageHeight * 3);

	for (int j = imageHeight - 1; j >= 0; j--)
	{
		for (int i = 0; i < imageWidth; i++)
		{
			auto pixelColor = img[j][i];
			int index = ((imageHeight - j - 1) * imageWidth + i) * 3;
			pixelColor = clamp(pixelColor, 0.0f, 1.0f);
			//pixel values are in 0.0f to 1.0f
			float red = 255.99f * powf(pixelColor.x, 1.0f / scene_cam.gamma);
			float green = 255.99f * powf(pixelColor.y, 1.0f / scene_cam.gamma);
			float blue = 255.99f * powf(pixelColor.z, 1.0f / scene_cam.gamma);
			
			raw[index] = static_cast<unsigned char>(clamp(red, 0.0, 255.999));
			raw[index + 1] = static_cast<unsigned char>(clamp(green, 0.0, 255.999));
			raw[index + 2] = static_cast<unsigned char>(clamp(blue, 0.0, 255.999));

			/*raw[index] = static_cast<unsigned char>(clamp(pixelColor.x, 0.0f, 1.0f) * 255.99f);
			raw[index + 1] = static_cast<unsigned char>(clamp(pixelColor.y, 0.0f, 1.0f) * 255.99f);
			raw[index + 2] = static_cast<unsigned char>(clamp(pixelColor.z, 0.0f, 1.0f) * 255.99f);*/
		}
	}
	
	if (!stbi_write_png((scene_cam.image_name + ".png").c_str(), imageWidth, imageHeight, 3, raw.data(), 0))
	{
		std::cerr << "Failed to save image" << std::endl;
	}

	std::vector<float> rawf;
	rawf.resize(imageWidth * imageHeight * 3);

	for (int j = imageHeight - 1; j >= 0; j--)
	{
		for (int i = 0; i < imageWidth; i++)
		{
			auto& pixelColor = img[j][i];
			int index = ((imageHeight - j - 1) * imageWidth + i) * 3;
			rawf[index] = pixelColor.x;
			rawf[index + 1] = pixelColor.y;
			rawf[index + 2] = pixelColor.z;
		}
	}

	//write as hdr
	//bool SaveEXR(const float* rgb, int width, int height, const char* outfilename)
	if(!SaveEXR(rawf.data(), imageWidth, imageHeight, (scene_cam.image_name + ".exr").c_str()))
	{
		std::cerr << "Failed to save image as exr" << std::endl;
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

		pos = glm::vec3(model * vec4(pos, 1.0f));
		gaze = glm::vec3(model * vec4(gaze, 0.0f));
		up = cross(gaze, cross(up, gaze));
		//up = glm::vec3(model * vec4(up, 0.0f));

		gaze = glm::normalize(gaze);
		up = glm::normalize(up);

		cam.position = { pos.x, pos.y, pos.z };
		cam.gaze = { gaze.x, gaze.y, gaze.z };
		cam.up = { up.x, up.y, up.z };
	}

	for (auto& pl : scene.point_lights)
	{
		auto model = model_matrix_from_transforms(pl.transformations, scene);

		vec3 pos = { pl.position.x, pl.position.y, pl.position.z };
		pos = glm::vec3(model * vec4(pos, 1.0f));

		pl.position = { pos.x, pos.y, pos.z };
	}

	//world
	scene_list world = hittableListFromScene(scene);



	for(int i = 0; i < scene.cameras.size(); i++)
	{
		render_camera(scene, i, world);
	}
}