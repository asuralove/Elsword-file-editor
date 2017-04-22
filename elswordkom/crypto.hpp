#pragma once

#include "generic.hpp"

namespace crypto
{
	bool decrypt_algorithm(std::string const& file_name, std::shared_ptr<unsigned char>& file_data, std::size_t file_size);
	bool has_special_header(std::shared_ptr<unsigned char>& file_data, std::size_t file_size);
}