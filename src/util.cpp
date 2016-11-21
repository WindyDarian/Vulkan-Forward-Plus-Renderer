// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#include "util.h"

#include <tiny_obj_loader.h>

#include <fstream>
#include <unordered_map>
#include <tuple>
#include <array>

// TODO

std::vector<char> util::readFile(const std::string & filename)
{
	std::ifstream file_stream(filename, std::ios::ate | std::ios::binary);

	if (!file_stream.is_open())
	{
		throw std::runtime_error("failed to open file!");
	}

	// starts reading at the end of file to determine file size (ate)
	size_t file_size = (size_t)file_stream.tellg();
	std::vector<char> buffer(file_size);

	file_stream.seekg(0);
	file_stream.read(buffer.data(), file_size);

	file_stream.close();
	return buffer;
}

namespace std {
	// hash function for Vertex
	template<> struct hash<util::Vertex>
	{
		size_t operator()(util::Vertex const& vertex) const
		{
			return vertex.hash();
		}
	};
}

std::tuple<std::vector<util::Vertex>, std::vector<util::Vertex::index_t>> util::loadModel()
{
	// TODO: I need a normal index for flat shading models
	std::vector<Vertex> vertices;
	std::vector<util::Vertex::index_t> vertex_indices;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, MODEL_PATH.c_str()))
	{
		throw std::runtime_error(err);
	}

	bool has_vertex_normal = attrib.normals.size() > 0;

	std::unordered_map<Vertex, size_t> unique_vertices = {};
	auto append_vertex = [&vertices, &vertex_indices, &unique_vertices](const Vertex& vertex)
	{
		if (unique_vertices.count(vertex) == 0)
		{
			unique_vertices[vertex] = vertices.size(); // auto incrementing size
			vertices.push_back(vertex);
		}

		vertex_indices.push_back(static_cast<util::Vertex::index_t>(unique_vertices[vertex]));
	};

	std::array<Vertex, 3> current_tri;
	for (const auto& shape : shapes)
	{
		for (auto i = 0; i < shape.mesh.indices.size(); i++)
		{
			const auto& index = shape.mesh.indices[i];
			auto & vertex = current_tri[i % 3];

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			// since the y axis of obj's texture coordinate points up
			vertex.tex_coord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			if (has_vertex_normal)
			{
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};
			}

			if (has_vertex_normal)
			{
				append_vertex(vertex);
			}
			else
			{
				// we need to calculate the normal for the triangle
				if (i % 3 == 2)
				{
					// last one in triangle
					auto normal = glm::normalize(glm::cross(current_tri[1].pos - current_tri[0].pos, current_tri[2].pos - current_tri[0].pos));
					for (auto& v : current_tri)
					{
						v.normal = normal;
						append_vertex(v);
					}
				}
			}
		}
	}

	return std::make_tuple(std::move(vertices), std::move(vertex_indices));
}
