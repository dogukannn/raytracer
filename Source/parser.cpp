#include "Include/parser.h"
#include "Include/tinyxml2.h"
#include <sstream>
#include <stdexcept>
#include <cassert>
#include "Include/happly.h"
#include <cmath>

#include "Include/common.h"

#define STB_IMAGE_IMPLEMENTATION
#include "Include/stb_image.h"

#include "Include/tinyexr.h"

void parser::Scene::loadFromXml(const std::string &filepath)
{
    tinyxml2::XMLDocument file;
    std::stringstream stream;

    auto res = file.LoadFile(filepath.c_str());
    if (res)
    {
        throw std::runtime_error("Error: The xml file cannot be loaded.");
    }

    //get directory of the file
	std::string directory;
	auto pos = filepath.find_last_of('/');
	if (pos == std::string::npos)
		pos = filepath.find_last_of('\\');
	if (pos != std::string::npos)
	{
		directory = filepath.substr(0, pos + 1);
	}

    auto root = file.FirstChild();
    if (!root)
    {
        throw std::runtime_error("Error: Root is not found.");
    }

    //Get BackgroundColor
    auto element = root->FirstChildElement("BackgroundColor");
    if (element)
    {
        stream << element->GetText() << std::endl;
    }
    else
    {
        stream << "0 0 0" << std::endl;
    }
    stream >> background_color.x >> background_color.y >> background_color.z;

    //Get ShadowRayEpsilon
    element = root->FirstChildElement("ShadowRayEpsilon");
    if (element)
    {
        stream << element->GetText() << std::endl;
    }
    else
    {
        stream << "0.001" << std::endl;
    }
    stream >> shadow_ray_epsilon;

    //Get MaxRecursionDepth
    element = root->FirstChildElement("MaxRecursionDepth");
    if (element)
    {
        stream << element->GetText() << std::endl;
    }
    else
    {
        stream << "0" << std::endl;
    }
    stream >> max_recursion_depth;


	element = root->FirstChildElement("Transformations");
    if(element)
    {
		if(auto translation = element->FirstChildElement("Translation"))
		{
			while(translation)
			{
				auto translation_id = (translation->Attribute("id"));
				stream << translation->GetText() << std::endl;
				Vec3f translation_vec;
				stream >> translation_vec.x >> translation_vec.y >> translation_vec.z;

				translation = translation->NextSiblingElement("Translation");

				translations["t" + std::string(translation_id)] = Translation{translation_vec};
			}
		}
		//get scalings
		if (auto scaling = element->FirstChildElement("Scaling"))
		{
			while(scaling)
			{
				auto scaling_id = (scaling->Attribute("id"));
				stream << scaling->GetText() << std::endl;
				Vec3f scaling_vec;
				stream >> scaling_vec.x >> scaling_vec.y >> scaling_vec.z;

				scaling = scaling->NextSiblingElement("Scaling");

				scalings["s" + std::string(scaling_id)] = Scaling{ scaling_vec };
			}
		}
		//get rotations
		if (auto rotation = element->FirstChildElement("Rotation"))
		{
			while (rotation)
			{
				auto rotation_id = (rotation->Attribute("id"));
				stream << rotation->GetText() << std::endl;
				float angle;
				Vec3f axis;
				stream >> angle >> axis.x >> axis.y >> axis.z;

				rotation = rotation->NextSiblingElement("Rotation");

				rotations["r" + std::string(rotation_id)] = Rotation{ angle, axis };
			}
		}

		//get composite transformations
		if (auto composite = element->FirstChildElement("Composite"))
		{
			while (composite)
			{
				auto composite_id = (composite->Attribute("id"));
				stream << composite->GetText() << std::endl;

				//get 16 values

				float values[16];
				for (int i = 0; i < 16; i++)
				{
					stream >> values[i];
				}

				composite = composite->NextSiblingElement("Composite");

				Composite c;
				for (int i = 0; i < 16; i++)
				{
					c.matrix[i] = values[i];
				}

				composites["c" + std::string(composite_id)] = c;
			}
		}
	    
    }

    //Get Cameras
    element = root->FirstChildElement("Cameras");
    element = element->FirstChildElement("Camera");
    Camera camera;
    while (element)
	{
		camera.enable_tonemap = false;
		camera.tmo = "";
		camera.key_value = 0.18f;
		camera.burn_percent = 0.0f;
		camera.saturation = 1.0f;
		camera.gamma = 1.0f;

		if(auto child = element->FirstChildElement("Tonemap"))
		{
			auto tmo = child->FirstChildElement("TMO");
			auto tmo_options = child->FirstChildElement("TMOOptions");
			auto saturation = child->FirstChildElement("Saturation");
			auto gamma = child->FirstChildElement("Gamma");

			if (tmo)
			{
				stream << tmo->GetText() << std::endl;
				stream >> camera.tmo;
			}

			if (tmo_options)
			{
				stream << tmo_options->GetText() << std::endl;
				stream >> camera.key_value >> camera.burn_percent;
			}

			if (saturation)
			{
				stream << saturation->GetText() << std::endl;
				stream >> camera.saturation;
			}

			if (gamma)
			{
				stream << gamma->GetText() << std::endl;
				stream >> camera.gamma;
			}
			camera.enable_tonemap = true;
		}

        auto is_lookat= (element->Attribute("type", "lookAt") != NULL);

        if (is_lookat)
        {
            Vec3f pos;
            Vec3f gaze_point;
            Vec3f up;
			float fov_y;
			float near_distance;
			int image_width;
			int image_height;
			std::string image_name;

			auto child = element->FirstChildElement("Position");
			stream << child->GetText() << std::endl;
			child = element->FirstChildElement("GazePoint");
			stream << child->GetText() << std::endl;
			child = element->FirstChildElement("FovY");
			stream << child->GetText() << std::endl;
			child = element->FirstChildElement("Up");
			stream << child->GetText() << std::endl;
			child = element->FirstChildElement("NearDistance");
			stream << child->GetText() << std::endl;
			child = element->FirstChildElement("ImageResolution");
			stream << child->GetText() << std::endl;
			child = element->FirstChildElement("ImageName");
			stream << child->GetText() << std::endl;

			stream >> pos.x >> pos.y >> pos.z;
			stream >> gaze_point.x >> gaze_point.y >> gaze_point.z;
			stream >> fov_y;
			stream >> up.x >> up.y >> up.z;
			stream >> near_distance;
			stream >> image_width >> image_height;
			stream >> image_name;

			camera.position = pos;
            camera.gaze = Vec3f{ gaze_point.x - pos.x, gaze_point.y - pos.y, gaze_point.z - pos.z };
			camera.up = up;
			float aspect_ratio = (float)image_width / image_height;
			float t = near_distance * tan(degreesToRadians(fov_y) / 2.0f);
			float b = -t;
			float r = t * aspect_ratio;
			float l = -r;
			camera.near_plane = Vec4f{ l, r, b, t };
			camera.image_width = image_width;
			camera.image_height = image_height;
			camera.image_name = image_name;
			camera.near_distance = near_distance;


			if(child = element->FirstChildElement("NumSamples"))
			{
				stream << child->GetText() << std::endl;
				stream >> camera.num_samples;
			}

			if(child = element->FirstChildElement("ApertureSize"))
			{
				stream << child->GetText() << std::endl;
				stream >> camera.aperture;
				camera.enable_dof = true;

				if(child = element->FirstChildElement("FocusDistance"))
				{
					stream << child->GetText() << std::endl;
					stream >> camera.focus_distance;
				}
			}

			auto transformations = element->FirstChildElement("Transformations");
			if (transformations)
			{
				std::string transformation;
				stream << transformations->GetText() << std::endl;
				while (!(stream >> transformation).eof())
				{
					camera.transformations.push_back(transformation);
					transformation.clear();
				}
				stream.clear();
			}

			cameras.push_back(camera);
			camera.transformations.clear();
        }
        else
        {
			auto child = element->FirstChildElement("Position");
			stream << child->GetText() << std::endl;
			child = element->FirstChildElement("Gaze");
			stream << child->GetText() << std::endl;
			child = element->FirstChildElement("Up");
			stream << child->GetText() << std::endl;
			child = element->FirstChildElement("NearPlane");
			stream << child->GetText() << std::endl;
			child = element->FirstChildElement("NearDistance");
			stream << child->GetText() << std::endl;
			child = element->FirstChildElement("ImageResolution");
			stream << child->GetText() << std::endl;
			child = element->FirstChildElement("ImageName");
			stream << child->GetText() << std::endl;

			stream >> camera.position.x >> camera.position.y >> camera.position.z;
			stream >> camera.gaze.x >> camera.gaze.y >> camera.gaze.z;
			stream >> camera.up.x >> camera.up.y >> camera.up.z;
			stream >> camera.near_plane.x >> camera.near_plane.y >> camera.near_plane.z >> camera.near_plane.w;
			stream >> camera.near_distance;
			stream >> camera.image_width >> camera.image_height;
			stream >> camera.image_name;

			if(child = element->FirstChildElement("ApertureSize"))
			{
				stream << child->GetText() << std::endl;
				stream >> camera.aperture;
				camera.enable_dof = true;

				if(child = element->FirstChildElement("FocusDistance"))
				{
					stream << child->GetText() << std::endl;
					stream >> camera.focus_distance;
				}
			}

			if(child = element->FirstChildElement("NumSamples"))
			{
				stream << child->GetText() << std::endl;
				stream >> camera.num_samples;
			}

			auto transformations = element->FirstChildElement("Transformations");
			if (transformations)
			{
				std::string transformation;
				stream << transformations->GetText() << std::endl;
				while (!(stream >> transformation).eof())
				{
					camera.transformations.push_back(transformation);
					transformation.clear();
				}
				stream.clear();
			}

			cameras.push_back(camera);
			camera.transformations.clear();
			camera.num_samples = 1;
			camera.enable_dof = false;
        }

		element = element->NextSiblingElement("Camera");
    }

	//Get Textures
	element = root->FirstChildElement("Textures");
	if(element)
		element = element->FirstChildElement("Images");
	if(element)
		element = element->FirstChildElement("Image");
	Image image;
    while (element)
    {
		auto image_id= (element->Attribute("id"));
		stream << element->GetText() << std::endl;
		std::string image_path;
		stream >> image_path;
        //load image
		image_path = directory + image_path;

		//check if the image is png or exr

		//check if the image is png
		std::string extension = image_path.substr(image_path.find_last_of(".") + 1);

		if (extension == "png")
		{
			int width, height, channels;
			unsigned char* data = stbi_load(image_path.c_str(), &width, &height, &channels, 0);
			if (!data)
			{
				throw std::runtime_error("Error: The image cannot be loaded.");
			}
			if (channels != 3 && channels != 4 && channels != 1)
			{
				throw std::runtime_error("Error: The image must have 1, 3 or 4 channels.");
			}
			if (channels == 3)
			{
				image.width = width;
				image.height = height;
				image.channels = channels;
				image.data = new float[width * height * 4];
				for (int i = 0; i < width * height; i++)
				{
					image.data[4 * i] = data[3 * i] / 255.0f;
					image.data[4 * i + 1] = data[3 * i + 1] / 255.0f;
					image.data[4 * i + 2] = data[3 * i + 2] / 255.0f;
				}
				STBI_FREE(data);
			}
			else if (channels == 1)
			{
				image.width = width;
				image.height = height;
				image.channels = channels;
				image.data = new float[width * height * 4];
				for (int i = 0; i < width * height; i++)
				{
					image.data[4 * i] = data[i] / 255.0f;
					image.data[4 * i + 1] = data[i] / 255.0f;
					image.data[4 * i + 2] = data[i] / 255.0f;
				}
				STBI_FREE(data);
			}
			else if (channels == 4)
			{
				image.width = width;
				image.height = height;
				image.channels = 3;
				image.data = new float[width * height * 4];
				for (int i = 0; i < width * height * channels; i++)
				{
					image.data[i] = data[i] / 255.0f;
				}
				STBI_FREE(data);
			}

		}
		else if (extension == "exr")
		{
			const char* err;
			int ret = LoadEXR(&image.data, &image.width, &image.height, image_path.c_str(), &err);
			if (ret != TINYEXR_SUCCESS)
			{
				throw std::runtime_error("Error: The image cannot be loaded.");
			}
			image.channels = 4;
		}
		else
		{
			//throw std::runtime_error("Error: The image format is not supported.");
		}

		images.push_back(image);
		element = element->NextSiblingElement("Image");
    }


    //Get Lights
    element = root->FirstChildElement("Lights");
    auto child = element->FirstChildElement("AmbientLight");
	if(child)
	{
		stream << child->GetText() << std::endl;
		stream >> ambient_light.x >> ambient_light.y >> ambient_light.z;
	}
	else
	{
		ambient_light = Vec3f{ 0.0f, 0.0f, 0.0f };
	}
    element = element->FirstChildElement("PointLight");
    PointLight point_light;
    while (element)
    {
        child = element->FirstChildElement("Position");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("Intensity");
        stream << child->GetText() << std::endl;

        stream >> point_light.position.x >> point_light.position.y >> point_light.position.z;
        stream >> point_light.intensity.x >> point_light.intensity.y >> point_light.intensity.z;

		auto transformations = element->FirstChildElement("Transformations");
		if (transformations)
		{
			std::string transformation;
			stream << transformations->GetText() << std::endl;
			while (!(stream >> transformation).eof())
			{
				point_light.transformations.push_back(transformation);
				transformation.clear();
			}
			stream.clear();
		}

        point_lights.push_back(point_light);
		point_light.transformations.clear();
        element = element->NextSiblingElement("PointLight");
    }
    element = root->FirstChildElement("Lights");
    element = element->FirstChildElement("AreaLight");
    AreaLight area_light;
    while (element)
    {
        child = element->FirstChildElement("Position");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("Normal");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("Size");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("Radiance");
        stream << child->GetText() << std::endl;

        stream >> area_light.position.x >> area_light.position.y >> area_light.position.z;
        stream >> area_light.normal.x >> area_light.normal.y >> area_light.normal.z;
        stream >> area_light.size;
        stream >> area_light.intensity.x >> area_light.intensity.y >> area_light.intensity.z;

		auto transformations = element->FirstChildElement("Transformations");
		if (transformations)
		{
			std::string transformation;
			stream << transformations->GetText() << std::endl;
			while (!(stream >> transformation).eof())
			{
				area_light.transformations.push_back(transformation);
				transformation.clear();
			}
			stream.clear();
		}

        area_lights.push_back(area_light);
		area_light.transformations.clear();
        element = element->NextSiblingElement("AreaLight");
    }

	element = root->FirstChildElement("Lights");
    element = element->FirstChildElement("SpotLight");
    SpotLight spot_light;
    while (element)
    {
        child = element->FirstChildElement("Position");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("Direction");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("CoverageAngle");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("FalloffAngle");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("Intensity");
        stream << child->GetText() << std::endl;

		stream >> spot_light.position.x >> spot_light.position.y >> spot_light.position.z;
		stream >> spot_light.direction.x >> spot_light.direction.y >> spot_light.direction.z;
		stream >> spot_light.coverage_angle;
		stream >> spot_light.falloff_angle;
		stream >> spot_light.intensity.x >> spot_light.intensity.y >> spot_light.intensity.z;
       
		auto transformations = element->FirstChildElement("Transformations");
		if (transformations)
		{
			std::string transformation;
			stream << transformations->GetText() << std::endl;
			while (!(stream >> transformation).eof())
			{
				area_light.transformations.push_back(transformation);
				transformation.clear();
			}
			stream.clear();
		}

        spot_lights.push_back(spot_light);
		spot_light.transformations.clear();
        element = element->NextSiblingElement("AreaLight");
    }

	//get directional lights
	element = root->FirstChildElement("Lights");
	element = element->FirstChildElement("DirectionalLight");
	DirectionalLight directional_light;
	while (element)
	{
		child = element->FirstChildElement("Direction");
		stream << child->GetText() << std::endl;
		child = element->FirstChildElement("Radiance");
		stream << child->GetText() << std::endl;

		stream >> directional_light.direction.x >> directional_light.direction.y >> directional_light.direction.z;
		stream >> directional_light.intensity.x >> directional_light.intensity.y >> directional_light.intensity.z;

		directional_lights.push_back(directional_light);
		element = element->NextSiblingElement("DirectionalLight");
	}

	//get spherical directional lights
	element = root->FirstChildElement("Lights");
	element = element->FirstChildElement("SphericalDirectionalLight");
	SphericalDirectionalLight spherical_directional_light;
	while (element)
	{
		child = element->FirstChildElement("ImageId");
		stream << child->GetText() << std::endl;
		
		stream >> spherical_directional_light.image_id;

		spherical_directional_light.type = (element->Attribute("type", "probe") != NULL) ? sphere_light_type::spherical : sphere_light_type::latlong;

		spherical_directional_lights.push_back(spherical_directional_light);
		element = element->NextSiblingElement("SphericalDirectionalLight");
	}

    //Get Materials
    element = root->FirstChildElement("Materials");
    element = element->FirstChildElement("Material");
    Material material;
    while (element)
    {
        material.is_mirror = (element->Attribute("type", "mirror") != NULL);
        material.is_conductor = (element->Attribute("type", "conductor") != NULL);
        material.is_dielectric = (element->Attribute("type", "dielectric") != NULL);

        child = element->FirstChildElement("AmbientReflectance");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("DiffuseReflectance");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("SpecularReflectance");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("MirrorReflectance");



        bool has_mirror = false;
        if (child)
        {
            stream << child->GetText() << std::endl;
            has_mirror = true;
        }
        child = element->FirstChildElement("PhongExponent");
        bool has_phong_exponent = false;
        if(child)
        {
			stream << child->GetText() << std::endl;
            has_phong_exponent = true;
        }

        child = element->FirstChildElement("RefractionIndex");
        bool has_refraction_index = false;
        if(child)
        {
            stream << child->GetText() << std::endl;
            has_refraction_index = true;
        }

        child = element->FirstChildElement("AbsorptionIndex");
        bool has_absorption_index = false;
        if(child)
        {
            stream << child->GetText() << std::endl;
            has_absorption_index = true;
        }

        child = element->FirstChildElement("AbsorptionCoefficient");
        bool has_absorption_coef = false;
        if(child)
        {
            stream << child->GetText() << std::endl;
            has_absorption_coef = true;
        }


        stream >> material.ambient.x >> material.ambient.y >> material.ambient.z;
        stream >> material.diffuse.x >> material.diffuse.y >> material.diffuse.z;
        stream >> material.specular.x >> material.specular.y >> material.specular.z;
        if (has_mirror)
            stream >> material.mirror.x >> material.mirror.y >> material.mirror.z;
        if(has_phong_exponent)
			stream >> material.phong_exponent;
        if (has_refraction_index)
            stream >> material.refraction_index;
        if (has_absorption_index)
            stream >> material.absorption_index;
        if (has_absorption_coef)
			stream >> material.absorption_coef.x >> material.absorption_coef.y >> material.absorption_coef.z;

		if(child = element->FirstChildElement("Roughness"))
		{
			material.has_roughness = true;
			stream << child->GetText() << std::endl;
            stream >> material.roughness;
		}


        bool degamma = (element->Attribute("degamma", "true") != NULL);
		if(degamma)
		{
			material.diffuse.x = pow(material.diffuse.x, 2.2f);
			material.diffuse.y = pow(material.diffuse.y, 2.2f);
			material.diffuse.z = pow(material.diffuse.z, 2.2f);

			material.ambient.x = pow(material.ambient.x, 2.2f);
			material.ambient.y = pow(material.ambient.y, 2.2f);
			material.ambient.z = pow(material.ambient.z, 2.2f);

			material.specular.x = pow(material.specular.x, 2.2f);
			material.specular.y = pow(material.specular.y, 2.2f);
			material.specular.z = pow(material.specular.z, 2.2f);

			material.mirror.x = pow(material.mirror.x, 2.2f);
			material.mirror.y = pow(material.mirror.y, 2.2f);
			material.mirror.z = pow(material.mirror.z, 2.2f);
		}

        materials.push_back(material);
        element = element->NextSiblingElement("Material");
		material.has_roughness = false;
    }


	element = root->FirstChildElement("Textures");
	if(element)
		element = element->FirstChildElement("TextureMap");
	Texture texture;
	while (element)
	{
		auto texture_id = (element->Attribute("id"));
		auto type = (element->Attribute("type"));
		if (strcmp(type, "image") == 0)
		{

			if(child = element->FirstChildElement("ImageId"))
			{
				stream << child->GetText() << std::endl;
				stream >> texture.image_id;
			}

			if(child = element->FirstChildElement("DecalMode"))
			{
				stream << child->GetText() << std::endl;
				std::string decal_mode;
				stream >> decal_mode;
				if (decal_mode == "replace_kd")
					texture.type = texture_type::replace_kd;
				else if (decal_mode == "blend_kd")
					texture.type = texture_type::blend_kd;
				else if (decal_mode == "replace_ks")
					texture.type = texture_type::replace_ks;
				else if (decal_mode == "replace_background")
					texture.type = texture_type::replace_background;
				else if (decal_mode == "replace_normal")
					texture.type = texture_type::replace_normal;
				else if (decal_mode == "bump_normal")
					texture.type = texture_type::bump_normal;
				else if (decal_mode == "replace_all")
					texture.type = texture_type::replace_all;
			}

			if(child = element->FirstChildElement("BumpFactor"))
			{
				stream << child->GetText() << std::endl;
				stream >> texture.bump_factor;
			}

			if(child = element->FirstChildElement("Normalizer"))
			{
				stream << child->GetText() << std::endl;
				stream >> texture.normalizer;
			}


			if (child = element->FirstChildElement("Interpolation"))
			{
				stream << child->GetText() << std::endl;
				std::string interpolation;
				stream >> interpolation;
				if (interpolation == "nearest")
					texture.interpolation = interpolation_type::nearest;
				else if (interpolation == "bilinear")
					texture.interpolation = interpolation_type::bilinear;
				else if (interpolation == "trilinear")
					texture.interpolation = interpolation_type::trilinear;
			}
		}

		if (strcmp(type, "perlin") == 0 || strcmp(type, "checkerboard") == 0 )
		{
			texture.is_perlin = true;

			if(child = element->FirstChildElement("DecalMode"))
			{
				stream << child->GetText() << std::endl;
				std::string decal_mode;
				stream >> decal_mode;
				if (decal_mode == "replace_kd")
					texture.type = texture_type::replace_kd;
				else if (decal_mode == "blend_kd")
					texture.type = texture_type::blend_kd;
				else if (decal_mode == "replace_ks")
					texture.type = texture_type::replace_ks;
				else if (decal_mode == "replace_background")
					texture.type = texture_type::replace_background;
				else if (decal_mode == "replace_normal")
					texture.type = texture_type::replace_normal;
				else if (decal_mode == "bump_normal")
					texture.type = texture_type::bump_normal;
				else if (decal_mode == "replace_all")
					texture.type = texture_type::replace_all;
			}

			if (child = element->FirstChildElement("NoiseConversion"))
			{
				stream << child->GetText() << std::endl;
				std::string noiseConversion;
				stream >> noiseConversion;
				if (noiseConversion == "linear")
					texture.is_linear = true;
			}

			if (child = element->FirstChildElement("NoiseScale"))
			{
				stream << child->GetText() << std::endl;
				stream >> texture.perlin_freq;
			}

			if (child = element->FirstChildElement("NumOctaves"))
			{
				stream << child->GetText() << std::endl;
				stream >> texture.perlin_octave;
			}

		}
		textures.push_back(texture);
		texture.interpolation = interpolation_type::nearest;
		texture.bump_factor = 1.0f;
		texture.normalizer = 255.0f;

		texture.is_perlin = false;
		texture.is_linear = false;
		texture.perlin_octave = 1;

		element = element->NextSiblingElement("TextureMap");
	}

    //Get VertexData
    element = root->FirstChildElement("VertexData");
	if(element)
	{
		stream << element->GetText() << std::endl;
		Vec3f vertex;
		while (!(stream >> vertex.x).eof())
		{
			stream >> vertex.y >> vertex.z;
			vertex_data.push_back(vertex);
		}
		stream.clear();
	}

	//Get TexCoordData
    element = root->FirstChildElement("TexCoordData");
	if(element)
	{
		stream << element->GetText() << std::endl;
		Vec2f uv;
		while (!(stream >> uv.x).eof())
		{
			stream >> uv.y;
			vertex_uv_data.push_back(uv);
		}
		stream.clear();
	}
	

    //Get Meshes
    element = root->FirstChildElement("Objects");
    element = element->FirstChildElement("Mesh");
    Mesh mesh;
    auto mesh_offset = 0;
    while (element)
    {

        if (child = element->FirstChildElement("MotionBlur"))
        {
			mesh.has_motion_blur = true;
			stream << child->GetText() << std::endl;
			stream >> mesh.motion.x >> mesh.motion.y >> mesh.motion.z;
        }

        child = element->FirstChildElement("Material");
        stream << child->GetText() << std::endl;
        stream >> mesh.material_id;

        child = element->FirstChildElement("Faces");

        auto mesh_id = (element->Attribute("id"));

		auto plyFile = (child->Attribute("plyFile"));
		if (plyFile)
		{
			mesh_offset = vertex_data.size();
			std::string plyFilePath = plyFile;
			//add directory to the file path
			plyFilePath = directory + plyFilePath;

			// Construct the data object by reading from file
			happly::PLYData plyIn(plyFilePath);

			// Get mesh-style data from the object
			std::vector<std::array<double, 3>> vPos = plyIn.getVertexPositions();
			std::vector<std::vector<size_t>> fInd = plyIn.getFaceIndices<size_t>();
			std::vector<std::array<double, 2>> vTex;
			if(plyIn.getElement("vertex").hasProperty("u"))
				vTex = plyIn.getTextureCoordinates();

			mesh.uv_offset = -mesh_offset + vertex_uv_data.size();

			mesh.is_ply = true;

			for (auto& uv : vTex)
			{
				vertex_uv_data.push_back(Vec2f{ fabs((float)uv[0]), fabs((float)uv[1])});
			}
			for (auto& v : vPos)
			{
				vertex_data.push_back(Vec3f{ (float)v[0], (float)v[1], (float)v[2] });
			}
			for (auto& f : fInd)
			{
                if(f.size() == 3)
                {
					Face face;
					face.v0_id = (int)f[0] + 1 + mesh_offset;
					face.v1_id = (int)f[1] + 1 + mesh_offset;
					face.v2_id = (int)f[2] + 1 + mesh_offset;
					mesh.faces.push_back(face);
                }
                if(f.size() == 4)
                {
					Face face;
					face.v0_id = (int)f[0] + 1 + mesh_offset;
					face.v1_id = (int)f[1] + 1 + mesh_offset;
					face.v2_id = (int)f[2] + 1 + mesh_offset;
					mesh.faces.push_back(face);

					Face face2;
					face2.v0_id = (int)f[0] + 1 + mesh_offset;
					face2.v1_id = (int)f[2] + 1 + mesh_offset;
					face2.v2_id = (int)f[3] + 1 + mesh_offset;
					mesh.faces.push_back(face2);
                }
			}
		}
		else
		{
			stream << child->GetText() << std::endl;
			Face face;
			while (!(stream >> face.v0_id).eof())
			{
				stream >> face.v1_id >> face.v2_id;
				mesh.faces.push_back(face);
			}
			stream.clear();
		}

		auto vertexOffset = (child->Attribute("vertexOffset"));
		if (vertexOffset)
		{
			mesh.vertex_offset = std::stoi(vertexOffset);
		}
		auto textureOffset = (child->Attribute("textureOffset"));
		if (textureOffset)
		{
			mesh.uv_offset = std::stoi(textureOffset);
		}

		auto transformations = element->FirstChildElement("Transformations");
        if (transformations)
        {
			std::string transformation;
            stream << transformations->GetText() << std::endl;
            while (!(stream >> transformation).eof())
            {
                mesh.transformations.push_back(transformation);
                transformation.clear();
            }
			stream.clear();
        }

		auto textures = element->FirstChildElement("Textures");
        if (textures)
        {
			std::string texture_id;
            stream << textures->GetText() << std::endl;
            while (!(stream >> texture_id).eof())
            {
				mesh.texture_ids.push_back(std::stoi(texture_id));
                texture_id.clear();
            }
			stream.clear();
        }

        //if(std::stoi(mesh_id) != 6)
			meshes[std::stoi(mesh_id)] = mesh;
   //     else{
			//std::cout << "cxklc" << std::endl;
   //     }

        mesh.faces.clear();
        mesh.transformations.clear();
		mesh.has_motion_blur = false;
		mesh.texture_ids.clear();
		mesh.vertex_offset = 0;
		mesh.uv_offset = 0;
		mesh.is_ply = false;
        element = element->NextSiblingElement("Mesh");
    }
    stream.clear();

    //Get Mesh Instances
    mesh.faces.clear();
	mesh.transformations.clear();
    element = root->FirstChildElement("Objects");
    element = element->FirstChildElement("MeshInstance");
    while (element)
    {
		if (child = element->FirstChildElement("MotionBlur"))
        {
			mesh.has_motion_blur = true;
			stream << child->GetText() << std::endl;
			stream >> mesh.motion.x >> mesh.motion.y >> mesh.motion.z;
        }

        mesh.material_id = -1;
        child = element->FirstChildElement("Material");
        if (child)
        {
			stream << child->GetText() << std::endl;
			stream >> mesh.material_id;
        }
        

        auto instance_id = (element->Attribute("id"));

		//this is an instanced mesh
		mesh.is_instance = true;
		auto base_mesh_id = (element->Attribute("baseMeshId"));
		mesh.base_mesh_id = std::stoi(base_mesh_id);
		if (element->Attribute("resetTransform", "true"))
		{
			mesh.reset_transform = true;
		}


		auto transformations = element->FirstChildElement("Transformations");
        if (transformations)
        {
			std::string transformation;
            stream << transformations->GetText() << std::endl;
            while (!(stream >> transformation).eof())
            {
                mesh.transformations.push_back(transformation);
                transformation.clear();
            }
			stream.clear();
        }

		auto textures = element->FirstChildElement("Textures");
        if (textures)
        {
			std::string texture_id;
            stream << textures->GetText() << std::endl;
            while (!(stream >> texture_id).eof())
            {
				mesh.texture_ids.push_back(std::stoi(texture_id));
                texture_id.clear();
            }
			stream.clear();
        }


		meshes[std::stoi(instance_id)] = mesh;
        mesh.faces.clear();
        mesh.transformations.clear();
        mesh.reset_transform = false;
		mesh.has_motion_blur = false;
		mesh.texture_ids.clear();
        element = element->NextSiblingElement("MeshInstance");
    }
    stream.clear();


    //Get Triangles
    element = root->FirstChildElement("Objects");
    element = element->FirstChildElement("Triangle");
    Triangle triangle;
    while (element)
    {
        child = element->FirstChildElement("Material");
        stream << child->GetText() << std::endl;
        stream >> triangle.material_id;

        child = element->FirstChildElement("Indices");
        stream << child->GetText() << std::endl;
        stream >> triangle.indices.v0_id >> triangle.indices.v1_id >> triangle.indices.v2_id;

		auto transformations = element->FirstChildElement("Transformations");
        if (transformations)
        {
			std::string transformation;
            stream << transformations->GetText() << std::endl;
            while (!(stream >> transformation).eof())
            {
                triangle.transformations.push_back(transformation);
                transformation.clear();
            }
			stream.clear();
        }

		auto textures = element->FirstChildElement("Textures");
        if (textures)
        {
			std::string texture_id;
            stream << textures->GetText() << std::endl;
            while (!(stream >> texture_id).eof())
            {
				triangle.texture_ids.push_back(std::stoi(texture_id));
                texture_id.clear();
            }
			stream.clear();
        }



        triangles.push_back(triangle);
        triangle.transformations.clear();
		triangle.texture_ids.clear();
        element = element->NextSiblingElement("Triangle");
    }

    //Get Spheres
    element = root->FirstChildElement("Objects");
    element = element->FirstChildElement("Sphere");
    Sphere sphere;
    while (element)
    {
        child = element->FirstChildElement("Material");
        stream << child->GetText() << std::endl;
        stream >> sphere.material_id;

        child = element->FirstChildElement("Center");
        stream << child->GetText() << std::endl;
        stream >> sphere.center_vertex_id;

        child = element->FirstChildElement("Radius");
        stream << child->GetText() << std::endl;
        stream >> sphere.radius;

		auto transformations = element->FirstChildElement("Transformations");
        if (transformations)
        {
			std::string transformation;
            stream << transformations->GetText() << std::endl;
            while (!(stream >> transformation).eof())
            {
                sphere.transformations.push_back(transformation);
                transformation.clear();
            }
			stream.clear();
        }

		auto textures = element->FirstChildElement("Textures");
        if (textures)
        {
			std::string texture_id;
            stream << textures->GetText() << std::endl;
            while (!(stream >> texture_id).eof())
            {
				sphere.texture_ids.push_back(std::stoi(texture_id));
                texture_id.clear();
            }
			stream.clear();
        }

        spheres.push_back(sphere);
        element = element->NextSiblingElement("Sphere");
        sphere.transformations.clear();
		sphere.texture_ids.clear();
    }
}
