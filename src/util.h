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

	const std::string MODEL_PATH = util::getContentPath("chalet.obj");
	const std::string TEXTURE_PATH = util::getContentPath("chalet.jpg");

	std::vector<char> readFile(const std::string& filename);

	// returns tuple(vertex vector, index vector)
	std::tuple<std::vector<Vertex>, std::vector<Vertex::index_t>> loadModel();


}

