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

// uniform buffer object for model transformation
struct MaterialUbo
{
	int has_albedo_map;
	int has_normal_map;
};


struct MeshMaterialGroup // grouped by material
{
	std::vector<util::Vertex> vertices = {};
	std::vector<util::Vertex::index_t> vertex_indices = {};
	
	std::string albedo_map_path = "";
	std::string normal_map_path = "";
};

std::vector<MeshMaterialGroup> loadModel(const std::string& path)
{
	using util::Vertex;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	std::string folder = util::findFolderName(path) + "/";
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str(), folder.c_str()))
	{
		throw std::runtime_error(err);  
	}

	bool has_vertex_normal = attrib.normals.size() > 0;
	assert(has_vertex_normal);

	std::vector<MeshMaterialGroup> groups(materials.size() + 1); // group parts of the same material together, +1 for unknown material

	for (size_t i = 0; i < materials.size(); i++)
	{
		if (materials[i].diffuse_texname != "")
		{
			groups[i + 1].albedo_map_path = folder + materials[i].diffuse_texname;
		}
		if (materials[i].normal_texname != "")
		{
			groups[i + 1].normal_map_path = folder + materials[i].normal_texname;
		}
		else if (materials[i].bump_texname != "")
		{
			// CryEngine sponza scene uses keyword "bump" to store normal
			groups[i + 1].normal_map_path = folder + materials[i].bump_texname;
		}
	}

	std::vector<std::unordered_map <util::Vertex, size_t >> unique_vertices_per_group(materials.size() + 1);

	auto appendVertex = [&unique_vertices_per_group, &groups](const Vertex& vertex, int material_id)
	{
		// 0 for unknown material
		auto& unique_vertices = unique_vertices_per_group[material_id + 1];
		auto& group = groups[material_id + 1];
		if (unique_vertices.count(vertex) == 0)
		{
			unique_vertices[vertex] = group.vertices.size(); // auto incrementing size
			group.vertices.push_back(vertex);
		}
		group.vertex_indices.push_back(static_cast<util::Vertex::index_t>(unique_vertices[vertex]));
	};

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

				appendVertex(vertex, material_id);

			}
			indexOffset += ngon;
		}
	}

	return groups;
}

/**
* Load model from file and allocate vulkan resources needed
*/
VModel VModel::loadModelFromFile(const VContext& vulkan_context, const std::string & path, const vk::Sampler& texture_sampler, const vk::DescriptorPool& descriptor_pool,
	const vk::DescriptorSetLayout& material_descriptor_set_layout)
{
	VModel model;

	auto device = vulkan_context.getDevice();
	VUtility vulkan_utility{ vulkan_context };

	//std::vector<util::Vertex> vertices, std::vector<util::Vertex::index_t> vertex_indices;
	auto groups = loadModel(path);
	vk::DeviceSize buffer_size = 0;
	for (const auto& group : groups)
	{
		if (group.vertex_indices.size() <= 0)
		{
			continue;
		}
		vk::DeviceSize vertex_section_size = sizeof(group.vertices[0]) * group.vertices.size();
		vk::DeviceSize index_section_size = sizeof(group.vertex_indices[0]) * group.vertex_indices.size();
		buffer_size += vertex_section_size;
		buffer_size += index_section_size;
	}

	std::tie(model.buffer, model.buffer_memory) = vulkan_utility.createBuffer(buffer_size
		, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
		, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vk::DeviceSize current_offset = 0;
	
	for (const auto& group : groups)
	{
		if (group.vertex_indices.size() <= 0)
		{
			continue;
		}

		vk::DeviceSize vertex_section_size = sizeof(group.vertices[0]) * group.vertices.size();
		vk::DeviceSize index_section_size = sizeof(group.vertex_indices[0]) * group.vertex_indices.size();

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

		if (!group.albedo_map_path.empty())
		{
			model.images.emplace_back();
			model.image_memories.emplace_back();
			model.imageviews.emplace_back();
			std::tie(model.images.back(), model.image_memories.back(), model.imageviews.back()) = vulkan_utility.loadImageFromFile(group.albedo_map_path);
			part.albedo_map = model.imageviews.back().get();
		}
		if (!group.normal_map_path.empty())
		{
			model.images.emplace_back();
			model.image_memories.emplace_back();
			model.imageviews.emplace_back();
			std::tie(model.images.back(), model.image_memories.back(), model.imageviews.back()) = vulkan_utility.loadImageFromFile(group.normal_map_path);
			part.normal_map = model.imageviews.back().get();
		}

		model.mesh_parts.push_back(part);
	}

	auto createMaterialDescriptorSet = [&vulkan_utility, &device, &texture_sampler, &descriptor_pool, &material_descriptor_set_layout, &uniform_buffer_memory = model.uniform_buffer_memory.get()](
		VMeshPart& mesh_part
		, VBufferSection uniform_buffer_section
	)
	{
		VkDescriptorSetLayout layouts[] = { material_descriptor_set_layout };
		VkDescriptorSetAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = descriptor_pool;
		alloc_info.descriptorSetCount = 1; 
		alloc_info.pSetLayouts = layouts;

		auto descriptor_set = device.allocateDescriptorSets(alloc_info)[0];
		MaterialUbo ubo{0, 0};

		std::vector<vk::WriteDescriptorSet> descriptor_writes = {};

		// refer to the uniform object buffer
		vk::DescriptorBufferInfo uniform_buffer_info = {};
		{
			uniform_buffer_info.buffer = uniform_buffer_section.buffer;
			uniform_buffer_info.offset = uniform_buffer_section.offset;
			uniform_buffer_info.range = uniform_buffer_section.size;
			// ubo
			descriptor_writes.emplace_back(
				descriptor_set,  //dstSet
				0,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCOunt
				vk::DescriptorType::eUniformBuffer,  // descriptorType
				nullptr,  // pImageInfo
				&uniform_buffer_info,  // pBufferInfo
				nullptr  // pTexelBufferView
			);
		}

		vk::DescriptorImageInfo albedo_map_info = {};
		if (mesh_part.albedo_map)
		{
			ubo.has_albedo_map = 1;
			albedo_map_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			albedo_map_info.imageView = mesh_part.albedo_map;
			albedo_map_info.sampler = texture_sampler;

			descriptor_writes.emplace_back(
				descriptor_set,  //dstSet
				1,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCOunt
				vk::DescriptorType::eCombinedImageSampler,  // descriptorType
				&albedo_map_info,  // pImageInfo
				nullptr,  // pBufferInfo
				nullptr  // pTexelBufferView
			);
		}

		vk::DescriptorImageInfo normalmap_info = {};
		if (mesh_part.normal_map)
		{
			ubo.has_normal_map = 1;
			normalmap_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			normalmap_info.imageView = mesh_part.normal_map;
			normalmap_info.sampler = texture_sampler;

			descriptor_writes.emplace_back(
				descriptor_set,  //dstSet
				2,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCOunt
				vk::DescriptorType::eCombinedImageSampler,  // descriptorType
				&normalmap_info,  // pImageInfo
				nullptr,  // pBufferInfo
				nullptr  // pTexelBufferView
			);
		}

		device.updateDescriptorSets(descriptor_writes, std::array<vk::CopyDescriptorSet, 0>());

		mesh_part.material_descriptor_set = descriptor_set;

		vk::DeviceSize staging_buffer_size = uniform_buffer_section.size;
		VRaii<VkBuffer> staging_buffer;
		VRaii<VkDeviceMemory> staging_buffer_memory;
		std::tie(staging_buffer, staging_buffer_memory) = vulkan_utility.createBuffer(staging_buffer_size
			, VK_BUFFER_USAGE_TRANSFER_SRC_BIT // to be transfered from
			, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);

		void* data = device.mapMemory(staging_buffer_memory.get(), 0, staging_buffer_size, vk::MemoryMapFlags());
		memcpy(data, &ubo, static_cast<size_t>(staging_buffer_size)); // may not be immediate due to memory caching or write operation not visiable without VK_MEMORY_PROPERTY_HOST_COHERENT_BIT or explict flusing
		device.unmapMemory(staging_buffer_memory.get());

		vulkan_utility.copyBuffer(staging_buffer.get(), uniform_buffer_info.buffer, staging_buffer_size, 0, uniform_buffer_info.offset);

	};


	auto min_alignment = vulkan_context.getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
	vk::DeviceSize alignment_offset = ((sizeof(MaterialUbo) - 1) / min_alignment + 1) * min_alignment;

	vk::DeviceSize uniform_buffer_size = alignment_offset * model.mesh_parts.size();
	std::tie(model.uniform_buffer, model.uniform_buffer_memory) = vulkan_utility.createBuffer(uniform_buffer_size
		, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
		, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vk::DeviceSize uniform_buffer_total_offset = 0;
	for (auto& part : model.mesh_parts)
	{
		createMaterialDescriptorSet(part, VBufferSection(model.uniform_buffer.get(), uniform_buffer_total_offset, sizeof(MaterialUbo)));
		uniform_buffer_total_offset += alignment_offset;
	}

	return model;
}

