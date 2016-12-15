// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <string>
#include <vector>
#include <tuple>
#include <memory>

namespace util
{
	template <class T>
	void hash_combine(std::size_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 tex_coord;
		glm::vec3 normal;

		using index_t = uint32_t;

		bool operator==(const Vertex& other) const noexcept
		{
			return pos == other.pos && color == other.color && tex_coord == other.tex_coord && normal == other.normal;
		}

		size_t hash() const
		{
			size_t seed = 0;
			hash_combine(seed, pos);
			hash_combine(seed, color);
			hash_combine(seed, tex_coord);
			hash_combine(seed, normal);
			return seed;
		}
	};

	template <typename str_t>
	std::string getContentPath(str_t&& filename)
	{
		static std::string content_folder{ "../content/" };
		return content_folder + std::forward<str_t>(filename);
	}

	//const std::string MODEL_PATH = util::getContentPath("chalet.obj");
	//const std::string TEXTURE_PATH = util::getContentPath("chalet.jpg");
	//const std::string MODEL_PATH = util::getContentPath("sponza_full/sponza.obj");
	const std::string MODEL_PATH = util::getContentPath("rungholt/rungholt.obj");
	//const std::string TEXTURE_PATH = util::getContentPath("sponza/color.jpg");
	//const std::string NORMALMAP_PATH = util::getContentPath("sponza/normal.png");


	std::vector<char> readFile(const std::string& filename);

	constexpr glm::vec3 vec_up = glm::vec3(0.0f, 1.0f, 0.0f);
	constexpr glm::vec3 vec_right = glm::vec3(1.0f, 0.0f, 0.0f);
	constexpr glm::vec3 vec_forward = glm::vec3(0.0f, 0.0f, -1.0f);
	
	constexpr float SMALL_NUMBER = 1e-8f;

	inline bool isNearlyEqual(float a, float b, float tolerance = SMALL_NUMBER)
	{
		return glm::abs(a - b) <= SMALL_NUMBER;
	}

	inline std::string findFolderName(const std::string& str)
	{
		return str.substr(0, str.find_last_of("/\\"));
	}

}

