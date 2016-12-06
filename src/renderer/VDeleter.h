// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#pragma once

#include <vulkan/vulkan.h>
#include <functional>

// This RAII class is used to hold Vulkan handles.
// It will call deletef upon destruction or & operator
template <typename T>
class VDeleter
{
public:
	VDeleter()
		: object(VK_NULL_HANDLE)
		, deleter( [](T obj) { std::runtime_error("VDeleter with empty deleter destroyed while resource was not VK_NULL_HANDLE");} )
	{}

	VDeleter(std::function<void(T, VkAllocationCallbacks*)> deletef)
		: object(VK_NULL_HANDLE)
		, deleter( [deletef](T obj) { deletef(obj, nullptr); } )
	{}

	VDeleter(const VDeleter<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef)
		: object(VK_NULL_HANDLE)
		, deleter( [&instance, deletef](T obj) { deletef(instance, obj, nullptr); } )
	{}

	VDeleter(const VDeleter<VkDevice>& device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef)
		: object(VK_NULL_HANDLE)
		, deleter( [&device, deletef](T obj) { deletef(device, obj, nullptr); } )
	{}

	~VDeleter()
	{
		cleanup();
	}

	T* operator &()
	{
		cleanup();
		return &object;
	}

	operator T() const
	{
		return object;
	}

	VDeleter(VDeleter<T>&& other)
		: object(VK_NULL_HANDLE) // to be swapped to "other"
		, deleter(other.deleter) // deleter will be copied in case there is still use for the old container
	{
		swap(*this, other);
	}
	VDeleter<T>& operator=(VDeleter<T>&& other)
	{
		swap(*this, other);
		return *this;
	}
	friend void swap(VDeleter<T>& first, VDeleter<T>& second)
	{
		using std::swap;
		swap(first.object, second.object);
	}

	VDeleter<T>& operator=(const VDeleter<T>&) = delete;
	VDeleter(const VDeleter<T>&) = delete;

private:
	T object;
	const std::function<void(T)> deleter;

	void cleanup()
	{
		if (object != VK_NULL_HANDLE)
		{
			deleter(object);
		}
		object = VK_NULL_HANDLE;
	}
};

/** 
* RAII for vulkan.hpp classes
*/
template <typename T>
class VRaii
{
public:
	using obj_t = T;

	VRaii()
		: object(VK_NULL_HANDLE)
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
		return obj;
	}

	//operator T() const
	//{
	//	return object;
	//}

	VRaii(VRaii<T>&& other)
		: object(VK_NULL_HANDLE) // to be swapped to "other"
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
