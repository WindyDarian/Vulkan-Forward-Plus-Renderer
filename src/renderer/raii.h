// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#pragma once

#include <vulkan/vulkan.h>
#include <functional>

/**
* This RAII class is used to hold Vulkan handles; just like a std::unique_ptr, but for Vulkan handles instead of pointers
* It will call deletef upon destruction
* It also support move operations, which is implemented by swapping, which will correctly destruct the old resource, since the right value argument get destructed right after the operation
* It does not support copy operation
*/
template <typename T>
class VRaii
{
public:
	using obj_t = T;

	VRaii()
		: object(nullptr)
		, deleter([](T&) {})
	{}

	VRaii(T obj, std::function<void(T&)> deleter)
		: object(obj)
		, deleter(deleter)
	{}

	~VRaii()
	{
		cleanup();
	}

	T& get()
	{
		return object;
	}

	const T& get() const
	{
		return object;
	}

	T* data()
	{
		return &object;
	}

	//operator T() const
	//{
	//	return object;
	//}

	VRaii(VRaii<T>&& other)
		: object(nullptr) // to be swapped to "other"
		, deleter([](T&) {}) // deleter will be copied in case there is still use for the old container
	{
		swap(*this, other);
	}
	VRaii<T>& operator=(VRaii<T>&& other)
	{
		swap(*this, other);
		return *this;
	}
	friend void swap(VRaii<T>& first, VRaii<T>& second)
	{
		using std::swap;
		swap(first.object, second.object);
		swap(first.deleter, second.deleter);
	}

	VRaii<T>& operator=(const VRaii<T>&) = delete;
	VRaii(const VRaii<T>&) = delete;

private:
	T object;
	std::function<void(T&)> deleter;

	void cleanup()
	{
		deleter(object);
	}
};

// Typical usage:
//
//	VRaii<vk::DescriptorSetLayout> camera_descriptor_set_layout
//
//	You can allocate VRaii as class member in advance and then when you are allocating the resource
//
//	camera_descriptor_set_layout = VRaii<vk::DescriptorSetLayout>(
//		device.createDescriptorSetLayout(create_info, nullptr),
//		[device = this->device](auto & layout){	device.destroyDescriptorSetLayout(layout);};
//	)
//	
//	this is for vulkan.hpp bindings. You can also use this for vulkan.h bindings but you need a temp 
//	  handle to pass in address for resource allocation functions to accept the handle value first.
//  Of course, you should guarantee the vk::device is released after
//	  this handle.