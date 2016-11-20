// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#pragma once

#include <string>
#include <vector>

namespace util
{
	template <typename str_t>
	std::string getContentPath(str_t&& filename)
	{
		static std::string content_folder{ "../content/" };
		return content_folder + std::forward<str_t>(filename);
	}

	std::vector<char> readFile(const std::string& filename);

}
