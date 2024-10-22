#include "Include/parser.h"
#include "Include/tinyxml2.h"
#include <sstream>
#include <stdexcept>
#include <cassert>
#include "Include/happly.h"
#include <cmath>

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

    //Get Cameras
    element = root->FirstChildElement("Cameras");
    element = element->FirstChildElement("Camera");
    Camera camera;
    while (element)
    {
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
			float t = near_distance * tan(fov_y / 2);
			float b = -t;
			float r = t * aspect_ratio;
			float l = -r;
			camera.near_plane = Vec4f{ l, r, b, t };
			camera.image_width = image_width;
			camera.image_height = image_height;
			camera.image_name = image_name;
			camera.near_distance = near_distance;

			cameras.push_back(camera);
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

			cameras.push_back(camera);
        }

		element = element->NextSiblingElement("Camera");
    }

    //Get Lights
    element = root->FirstChildElement("Lights");
    auto child = element->FirstChildElement("AmbientLight");
    stream << child->GetText() << std::endl;
    stream >> ambient_light.x >> ambient_light.y >> ambient_light.z;
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

        point_lights.push_back(point_light);
        element = element->NextSiblingElement("PointLight");
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

        materials.push_back(material);
        element = element->NextSiblingElement("Material");
    }

    //Get VertexData
    element = root->FirstChildElement("VertexData");
    stream << element->GetText() << std::endl;
    Vec3f vertex;
    while (!(stream >> vertex.x).eof())
    {
        stream >> vertex.y >> vertex.z;
        vertex_data.push_back(vertex);
    }
    stream.clear();


    //Get Meshes
    element = root->FirstChildElement("Objects");
    element = element->FirstChildElement("Mesh");
    Mesh mesh;
    auto mesh_offset = 0;
    while (element)
    {
        child = element->FirstChildElement("Material");
        stream << child->GetText() << std::endl;
        stream >> mesh.material_id;

        child = element->FirstChildElement("Faces");

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

			for (auto& v : vPos)
			{
				vertex_data.push_back(Vec3f{ (float)v[0], (float)v[1], (float)v[2] });
			}
			for (auto& f : fInd)
			{
				Face face;
				face.v0_id = (int)f[0] + 1 + mesh_offset;
				face.v1_id = (int)f[1] + 1 + mesh_offset;
				face.v2_id = (int)f[2] + 1 + mesh_offset;
				mesh.faces.push_back(face);
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


        meshes.push_back(mesh);
        mesh.faces.clear();
        element = element->NextSiblingElement("Mesh");
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

        triangles.push_back(triangle);
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

        spheres.push_back(sphere);
        element = element->NextSiblingElement("Sphere");
    }
}
