#ifndef __HW1__PARSER__
#define __HW1__PARSER__

#include <string>
#include <unordered_map>
#include <map>
#include <vector>

namespace parser
{
    //Notice that all the structures are as simple as possible
    //so that you are not enforced to adopt any style or design.
    struct Vec3f
    {
        float x, y, z;
    };

    struct Vec3i
    {
        int x, y, z;
    };

    struct Vec4f
    {
        float x, y, z, w;
    };

    struct Camera
    {
        Vec3f position;
        Vec3f gaze;
        Vec3f up;
        Vec4f near_plane;
        float near_distance;
        int image_width, image_height;
		int num_samples = 1;
        std::string image_name;

		bool enable_dof = false;
		float focus_distance = 0.0f;
		float aperture = 0.0f;

		std::vector<std::string> transformations;
    };

    struct PointLight
    {
        Vec3f position;
        Vec3f intensity;

		std::vector<std::string> transformations;
    };


    struct AreaLight
    {
        Vec3f position;
        Vec3f normal;
        float size;
        Vec3f intensity;
		std::vector<std::string> transformations;
    };

    struct Material
    {
        bool is_mirror;
        bool is_conductor;
        bool is_dielectric;

        Vec3f ambient;
        Vec3f diffuse;
        Vec3f specular;
        Vec3f mirror;
        float phong_exponent = 1.0f;

        float refraction_index;
        float absorption_index;
        Vec3f absorption_coef;

        bool has_roughness = false;
		float roughness = 0.0f;
    };

    struct Face
    {
        int v0_id;
        int v1_id;
        int v2_id;
    };

    struct Mesh
    {
		bool is_instance = false;
		bool reset_transform = false;
        int base_mesh_id;

		bool has_motion_blur = false;
		Vec3f motion;

        int material_id;
        std::vector<Face> faces;

		std::vector<std::string> transformations;
    };

    struct Triangle
    {
        int material_id;
        Face indices;

		std::vector<std::string> transformations;
    };

    struct Sphere
    {
        int material_id;
        int center_vertex_id;
        float radius;

		std::vector<std::string> transformations;
    };

    struct Translation
    {
		Vec3f translation;
    };

    struct Scaling
    {
		Vec3f scaling;
    };

    struct Rotation
    {
		float angle;
		Vec3f rotation;
    };

    struct Scene
    {
        //Data
        Vec3i background_color;
        float shadow_ray_epsilon;
        int max_recursion_depth;
        std::vector<Camera> cameras;
        Vec3f ambient_light;
        std::vector<PointLight> point_lights;
        std::vector<AreaLight> area_lights;
        std::vector<Material> materials;
        std::vector<Vec3f> vertex_data;

        std::map<int, Mesh> meshes;

        std::vector<Triangle> triangles;
        std::vector<Sphere> spheres;

		std::unordered_map<std::string, Translation> translations;
		std::unordered_map<std::string, Scaling> scalings;
		std::unordered_map<std::string, Rotation> rotations;

        //Functions
        void loadFromXml(const std::string &filepath);
    };
}

#endif
