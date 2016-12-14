// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#pragma once

#include <vulkan/vulkan.h>
#include <functional>

/**
* This RAII class is used to hold Vulkan handles.
* It will call deletef upon destruction or & operator
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