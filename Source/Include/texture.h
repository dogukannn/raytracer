#pragma once

#include "common.h"


enum class sampler
{
	nearest,
	bilinear,
	trilinear
};

struct perlin;

struct texture
{
	sampler sampler_type;
	int width;
	int height;
	color* data;
	float normalizer = 255.0f;

	virtual perlin* get_perlin()
	{
		return nullptr;
	}

	color nearest(double u, double v)
	{
		int i = static_cast<int>(u * width);
		int j = static_cast<int>(v * height);

		i = clamp(i, 0, width - 1);
		j = clamp(j, 0, height - 1);

		return data[j * width + i];
	}

	color bilinear(float u, float v)
	{
		float i = u * width;
		float j = v * height;

		int i0 = static_cast<int>(i);
		int j0 = static_cast<int>(j);

		i0 = clamp(i0, 0, width - 1);
		j0 = clamp(j0, 0, height - 1);

		int i1 = std::min(i0 + 1, width - 1);
		int j1 = std::min(j0 + 1, height - 1);

		float s = i - i0;
		float t = j - j0;

		color c00 = data[j0 * width + i0];
		color c10 = data[j0 * width + i1];
		color c01 = data[j1 * width + i0];
		color c11 = data[j1 * width + i1];

		return (c00 * (1 - s) * (1 - t) + c10 * s * (1 - t) + c01 * (1 - s) * t + c11 * s * t );
        //return average
        //return c00;
		//return (c00 + c10 + c01 + c11) * 0.25f;
        

	}

	void area_values(float u, float v, vec3 p, color& top, color& bottom, color& left, color& right)
	{
		//float i = u * width;
		//float j = v * height;

		//int i0 = static_cast<int>(i);
		//int j0 = static_cast<int>(j);

		//i0 = std::clamp(i0, 0, width - 1);
		//j0 = std::clamp(j0, 0, height - 1);

		//int im = std::clamp(i0 - 1, 0, width - 1);
		//int jm = std::clamp(j0 - 1, 0, height - 1);

		//int ip = std::min(i0 + 1, width - 1);
		//int jp = std::min(j0 + 1, height - 1);

		//top = data[jp * width + i0];
		//bottom = data[jm * width + i0];
		//left = data[j0 * width + im];
		//right = data[j0 * width + ip];


		//get the valuies with value function
		top = value(u, v + 1.0f / height, p);
		bottom = value(u, v - 1.0f / height, p);
		left = value(u - 1.0f / width, v, p);
		right = value(u + 1.0f / width, v, p);
	}

	virtual color value(double u, double v, vec3 p)
	{
		if (u < 0.0f) u *= -1.0f;
		if (v < 0.0f) v *= -1.0f;

		u = fmod(u, 1.0);
		v = fmod(v, 1.0);
                
		if (data == nullptr)
		{
			throw std::runtime_error("Texture data is null");
		}

		if(sampler_type == sampler::nearest)
		{
			return nearest(u, v);
		}
		else if (sampler_type == sampler::bilinear)
		{
			return bilinear(u, v);
		}
		else
		{
			return bilinear(u, v);
			//throw std::runtime_error("Trilinear not implemented");
		}
	}
};


class PerlinNoiseGenerator {
private:
    std::vector<int> permutation;

    // Smooth interpolation function
    float fade(float t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    // Linear interpolation
    float lerp(float a, float b, float x) {
        return a + x * (b - a);
    }

    // Dot product of gradient and distance vector
    float grad(int hash, float x, float y, float z) {
        switch(hash & 15) {
            case  0: return  x + y;
            case  1: return -x + y;
            case  2: return  x - y;
            case  3: return -x - y;
            case  4: return  x + z;
            case  5: return -x + z;
            case  6: return  x - z;
            case  7: return -x - z;
            case  8: return  y + z;
            case  9: return -y + z;
            case 10: return  y - z;
            case 11: return -y - z;
            case 12: return  y + x;
            case 13: return -y + z;
            case 14: return  y - x;
            case 15: return -y - z;
            default: return 0;
        }
    }

public:
    PerlinNoiseGenerator() {
        // Seed random number generator
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(0, 255);

        // Generate permutation table
        permutation.resize(512);
        for(int i = 0; i < 256; ++i) {
            permutation[i] = i;
            permutation[i + 256] = i;
        }
        
        // Shuffle permutation table
        for(int i = 0; i < 512; ++i) {
            int j = distrib(gen) % 512;
            std::swap(permutation[i], permutation[j]);
        }
    }

    float noise(float x, float y, float z, int octaves = 4, float frequency = 1.0f, bool is_linear = true) {
        float amplitude = 1.0f;
        float noise = 0.0f;
        float totalAmplitude = 0.0f;

        for(int octave = 0; octave < octaves; ++octave) {
            // Scale coordinates based on frequency
            float scaledX = x * frequency;
            float scaledY = y * frequency;
            float scaledZ = z * frequency;

            // Find unit grid cell containing point
            int xi = static_cast<int>(std::floor(scaledX)) & 255;
            int yi = static_cast<int>(std::floor(scaledY)) & 255;
            int zi = static_cast<int>(std::floor(scaledZ)) & 255;

            // Relative coordinates within the cell
            float xf = scaledX - std::floor(scaledX);
            float yf = scaledY - std::floor(scaledY);
            float zf = scaledZ - std::floor(scaledZ);

            // Interpolation weights
            float u_fade = fade(xf);
            float v_fade = fade(yf);
            float w_fade = fade(zf);

            // Hash coordinates of the 8 cell corners
            int aaa = permutation[permutation[permutation[xi    ] + yi    ] + zi    ];
            int aab = permutation[permutation[permutation[xi    ] + yi    ] + zi + 1];
            int aba = permutation[permutation[permutation[xi    ] + yi + 1] + zi    ];
            int abb = permutation[permutation[permutation[xi    ] + yi + 1] + zi + 1];
            int baa = permutation[permutation[permutation[xi + 1] + yi    ] + zi    ];
            int bab = permutation[permutation[permutation[xi + 1] + yi    ] + zi + 1];
            int bba = permutation[permutation[permutation[xi + 1] + yi + 1] + zi    ];
            int bbb = permutation[permutation[permutation[xi + 1] + yi + 1] + zi + 1];

            // Interpolate along x
            float x1 = lerp(grad(aaa, xf, yf, zf),
                            grad(baa, xf - 1, yf, zf),
                            u_fade);
            float x2 = lerp(grad(aba, xf, yf - 1, zf),
                            grad(bba, xf - 1, yf - 1, zf),
                            u_fade);
            float y1 = lerp(x1, x2, v_fade);

            // Interpolate along x for the next z level
            float x3 = lerp(grad(aab, xf, yf, zf - 1),
                            grad(bab, xf - 1, yf, zf - 1),
                            u_fade);
            float x4 = lerp(grad(abb, xf, yf - 1, zf - 1),
                            grad(bbb, xf - 1, yf - 1, zf - 1),
                            u_fade);
            float y2 = lerp(x3, x4, v_fade);

            // Final interpolation along z
            float currentNoise = lerp(y1, y2, w_fade);

            // Accumulate noise with decreasing amplitude
            noise += currentNoise * amplitude;
            totalAmplitude += amplitude;

            // Increase frequency and decrease amplitude for next octave
            frequency *= 2.0f;
            amplitude *= 0.5f;
        }

		if (is_linear)
			return noise / totalAmplitude; 

        return clamp(fabs(noise), 0, 1); 
    }
};
struct perlin : public texture
{
	bool is_linear = true;
	int num_octaves = 1;
	float freq = 1.0f;

	PerlinNoiseGenerator generator;

	virtual perlin* get_perlin() override
	{
		return this;
	}

	virtual color value(double u, double v, vec3 p) override
	{
		float noise = generator.noise(p.x, p.y, p.z, num_octaves, freq, is_linear);
        if(is_linear)
			noise = 1.0f - noise;
		return color(noise, noise, noise);
	}
};

