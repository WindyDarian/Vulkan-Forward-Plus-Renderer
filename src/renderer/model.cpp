// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#include "model.h"

#include "vulkan_util.h"
#include "context.h"
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

VModel VModel::loadModelFromFile(const VContext& vulkan_context, const std::string & path)
{
	VModel model;

	auto device = vulkan_context.getDevice();
	VUtility vulkan_utility{ vulkan_context };

	//std::vector<util::Vertex> vertices, std::vector<util::Vertex::index_t> vertex_indices;
	auto group = loadModel(path);
	vk::DeviceSize vertex_section_size = sizeof(group.vertices[0]) * group.vertices.size();
	vk::DeviceSize index_section_size = sizeof(group.vertex_indices[0]) * group.vertex_indices.size();

	auto buffer_size = vertex_section_size + index_section_size;

	std::tie(model.buffer, model.buffer_memory) = vulkan_utility.createBuffer(buffer_size
		, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
		, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vk::DeviceSize current_offset = 0;
	
	VBufferSection vertex_buffer_section = { model.buffer.get(), current_offset, vertex_section_size };
	// copy vertex data
	{
		auto staging_buffer_size = vertex_section_size;
		auto host_data = group.vertices.data();

		VRaii<VkBuffer> staging_buffer;
		VRaii<VkDeviceMemory> staging_buffer_memory;
		std::tie(staging_buffer, staging_buffer_memory) = vulkan_utility.createBuffer(staging_buffer_size
			, VK_BUFFER_USAGE_TRANSFER_SRC_BIT // to be transfered from
			, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);

		void* data = device.mapMemory(staging_buffer_memory.get(), 0, staging_buffer_size, vk::MemoryMapFlags());
		memcpy(data, host_data, static_cast<size_t>(staging_buffer_size)); // may not be immediate due to memory caching or write operation not visiable without VK_MEMORY_PROPERTY_HOST_COHERENT_BIT or explict flusing
		device.unmapMemory(staging_buffer_memory.get());

		vulkan_utility.copyBuffer(staging_buffer.get(), model.buffer.get(), staging_buffer_size, 0, current_offset);



		current_offset += staging_buffer_size;
	}
	
	VBufferSection index_buffer_section = { model.buffer.get(), current_offset, index_section_size };
	// copy index data
	{
		auto staging_buffer_size = index_section_size;
		auto host_data = group.vertex_indices.data();

		VRaii<VkBuffer> staging_buffer;
		VRaii<VkDeviceMemory> staging_buffer_memory;
		std::tie(staging_buffer, staging_buffer_memory) = vulkan_utility.createBuffer(staging_buffer_size
			, VK_BUFFER_USAGE_TRANSFER_SRC_BIT // to be transfered from
			, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);

		void* data = device.mapMemory(staging_buffer_memory.get(), 0, staging_buffer_size, vk::MemoryMapFlags());
		memcpy(data, host_data, static_cast<size_t>(staging_buffer_size)); // may not be immediate due to memory caching or write operation not visiable without VK_MEMORY_PROPERTY_HOST_COHERENT_BIT or explict flusing
		device.unmapMemory(staging_buffer_memory.get());

		vulkan_utility.copyBuffer(staging_buffer.get(), model.buffer.get(), staging_buffer_size, 0, current_offset);

		current_offset += staging_buffer_size;
	}

	VMeshPart part = { vertex_buffer_section, index_buffer_section, group.vertex_indices.size() };
	model.mesh_parts.push_back(part);
	return model;
}


//void _VulkanRenderer_Impl::createVertexBuffer()
//{
//	VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
//
//	// create staging buffer
//	VRaii<VkBuffer> staging_buffer;
//	VRaii<VkDeviceMemory> staging_buffer_memory;
//	std::tie(staging_buffer, staging_buffer_memory) = utility.createBuffer(buffer_size
//		, VK_BUFFER_USAGE_TRANSFER_SRC_BIT // to be transfered from
//		, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
//	);
//
//	// copy data to staging buffer
//	void* data;
//	vkMapMemory(graphics_device, staging_buffer_memory.get(), 0, buffer_size, 0, &data); // access the graphics memory using mapping
//	memcpy(data, vertices.data(), (size_t)buffer_size); // may not be immediate due to memory caching or write operation not visiable without VK_MEMORY_PROPERTY_HOST_COHERENT_BIT or explict flusing
//	vkUnmapMemory(graphics_device, staging_buffer_memory.get());
//
//	// create vertex buffer at optimized local memory which may not be directly accessable by memory mapping
//	// as copy destination of staging buffer
//	std::tie(vertex_buffer, vertex_buffer_memory) = utility.createBuffer(buffer_size
//		, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
//		, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//
//	// copy content of staging buffer to vertex buffer
//	utility.copyBuffer(staging_buffer.get(), vertex_buffer.get(), buffer_size);
//}
//
//void _VulkanRenderer_Impl::createIndexBuffer()
//{
//	VkDeviceSize buffer_size = sizeof(vertex_indices[0]) * vertex_indices.size();
//
//	// create staging buffer
//	VRaii<VkBuffer> staging_buffer;
//	VRaii<VkDeviceMemory> staging_buffer_memory;
//	std::tie(staging_buffer, staging_buffer_memory) = utility.createBuffer(buffer_size
//		, VK_BUFFER_USAGE_TRANSFER_SRC_BIT // to be transfered from
//		, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
//
//	void* data;
//	vkMapMemory(graphics_device, staging_buffer_memory.get(), 0, buffer_size, 0, &data); // access the graphics memory using mapping
//	memcpy(data, vertex_indices.data(), (size_t)buffer_size); // may not be immediate due to memory caching or write operation not visiable without VK_MEMORY_PROPERTY_HOST_COHERENT_BIT or explict flusing
//	vkUnmapMemory(graphics_device, staging_buffer_memory.get());
//
//	std::tie(index_buffer, index_buffer_memory) = utility.createBuffer(buffer_size
//		, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
//		, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//
//	// copy content of staging buffer to index buffer
//	utility.copyBuffer(staging_buffer.get(), index_buffer.get(), buffer_size);
//}


