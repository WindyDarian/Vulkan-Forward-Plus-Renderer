// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <string>
#include <vector>
#include <tuple>

namespace util
{

	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 tex_coord;

		using index_t = uint32_t;

		bool operator==(const Vertex& other) const noexcept
		{
			return pos == other.pos && color == other.color && tex_coord == other.tex_coord;
		}

		size_t hash() const
		{
			return ((std::hash<glm::vec3>()(pos) ^
				(std::hash<glm::vec3>()(color) << 1)) >> 1) ^
				(std::hash<glm::vec2>()(tex_coord) << 1);
		}
	};

	template <typename str_t>
	std::string getContentPath(str_t&& filename)
	{
		static std::string content_folder{ "../content/" };
		return content_folder + std::forward<str_t>(filename);
	}

	std::vector<char> readFile(const std::string& filename);

	std::tuple<std::vector<Vertex>, std::vector<Vertex::index_t>> loadModel();

	const std::string MODEL_PATH = util::getContentPath("chalet.obj");
	const std::string TEXTURE_PATH = util::getContentPath("chalet.jpg");
}

