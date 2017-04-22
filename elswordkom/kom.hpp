#pragma once

#include "generic.hpp"

namespace kom
{
	enum algorithm_type
	{
		compressed = 0,
		compressed_encrypted = 3
	};

	struct actual_file
	{
		actual_file()
		{
			this->file_data.reset();
			this->compressed_file_data.reset();
		}

		std::string name;
		std::string file_path;
		std::size_t size;
		std::size_t compressed_size;
		std::size_t checksum;
		std::size_t file_time;
		std::size_t algorithm;
		std::size_t offset;
		std::shared_ptr<unsigned char> file_data;
		std::shared_ptr<unsigned char> compressed_file_data;
	};

	bool decompress_files(std::string const& file_path, std::vector<actual_file>& decompressed_files);
	bool dump_decompressed_files(std::string const& file_path, std::vector<actual_file> decompressed_files);
	
	bool compress_file(actual_file& new_file);
	bool dump_compressed_files(std::string const& file_path, std::vector<actual_file> compressed_files);
}