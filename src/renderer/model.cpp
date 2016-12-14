// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#include "model.h"

#include "../util.h"

#include <tiny_obj_loader.h>

#include <unordered_map>
#include <vector>
#include <string>

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

struct MeshMaterialGroup // grouped by material
{
	std::vector<util::Vertex> vertices = {};
	std::vector<util::Vertex::index_t> vertex_indices = {};
	
	std::string albedo_map_path = "";
	std::string normal_map_path = "";
};

MeshMaterialGroup loadModel(const std::string& path)
{
	using util::Vertex;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str()))
	{
		throw std::runtime_error(err);
	}
	bool has_vertex_normal = attrib.normals.size() > 0;
	assert(has_vertex_normal);

	//std::vector<MeshMaterialGroup> groups(materials.size()); // group parts of the same material together

	MeshMaterialGroup temp_group = {};
	std::vector<std::unordered_map <util::Vertex, size_t >> unique_vertices_per_group(1);

	auto append_vertex = [&vertices = temp_group.vertices, &vertex_indices = temp_group.vertex_indices, &unique_vertices = unique_vertices_per_group[0]](const Vertex& vertex)
	{
		if (unique_vertices.count(vertex) == 0)
		{
			unique_vertices[vertex] = vertices.size(); // auto incrementing size
			vertices.push_back(vertex);
		}

		vertex_indices.push_back(static_cast<util::Vertex::index_t>(unique_vertices[vertex]));
	};

	//std::array<Vertex, 3> current_tri;
	for (const auto& shape : shapes)
	{

		size_t indexOffset = 0;
		for (size_t n = 0; n < shape.mesh.num_face_vertices.size(); n++) 
		{
			// per face
			auto ngon = shape.mesh.num_face_vertices[n];
			auto material_id = shape.mesh.material_ids[n];
			for (size_t f = 0; f < ngon; f++) 
			{
				const auto& index = shape.mesh.indices[indexOffset + f];

				Vertex vertex;

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.tex_coord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};

				append_vertex(vertex);

			}
			indexOffset += ngon;
		}
	}

	return temp_group;
}

VModel VModel::loadModelFromFile(vk::Device device_handle, const std::string & path)
{
	VModel model;
	
	//std::vector<util::Vertex> vertices, std::vector<util::Vertex::index_t> vertex_indices;
	auto group = loadModel(path);


	return VModel();
}


