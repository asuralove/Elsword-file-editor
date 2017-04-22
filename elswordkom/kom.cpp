#include "kom.hpp"
#include "crypto.hpp"
#include "io_file.hpp"

#include <rapidxml.hpp>
#include <rapidxml_print.hpp>

#include "output.hpp"

#include <zlib.h>
#pragma comment(lib, "zdll")

#pragma pack(push, 1)
struct kom_header
{
	char file_description[52];			// + 0x00
	unsigned int file_entries;			// + 0x34
	unsigned int always_one;			// + 0x38
	unsigned int file_time_sum;			// + 0x3C
	unsigned int checksum;				// + 0x40
	unsigned int data_size;				// + 0x44
};
#pragma pack(pop)

namespace kom
{	
	bool decompress_files(std::string const& file_path, std::vector<actual_file>& decompressed_files)
	{
		io_file data_file;

		if (file_path.empty() || !data_file.open(file_path) || data_file.get_size() < 0x48)
			return false;

		char raw_header[0x48];
		memset(raw_header, 0, sizeof(raw_header));

		if (!data_file.read_data(raw_header, 0x48))
			return false;
		
		kom_header* header = reinterpret_cast<kom_header*>(raw_header);
		
		if (strcmp(header->file_description, "KOG GC TEAM MASSFILE V.0.3.") || !header->file_entries || data_file.get_size() < (header->data_size + 0x48))
			return false;

		char* data_buffer = new char[header->data_size + 1];
		
		if (!data_file.read_data(data_buffer, header->data_size, 0x48))
		{
			delete[] data_buffer;
			return false;
		}
		
		data_buffer[header->data_size] = 0;

		if (!header->file_entries || !data_buffer || (adler32(adler32(0, Z_NULL, 0), reinterpret_cast<unsigned char*>(data_buffer), header->data_size) != header->checksum))
		{
			delete[] data_buffer;
			return false;
		}

		unsigned int data_read = header->data_size + 0x48;
		unsigned int file_time_sum = 0;

		decompressed_files.resize(header->file_entries);
		decompressed_files.clear();

		rapidxml::xml_document<> doc;
		doc.parse<0>(data_buffer);
	
		rapidxml::xml_node<>* files_node = doc.first_node();
	
		if (files_node == nullptr && strcmp(files_node->name(), "Files"))
		{
			delete[] data_buffer;
			return false;
		}

		for (rapidxml::xml_node<>* file_node = files_node->first_node(); file_node != nullptr; file_node = file_node->next_sibling())
		{
			if (strcmp(file_node->name(), "File"))
				continue;

			actual_file new_file;
			memset(&new_file, 0, sizeof(actual_file));

			new_file.offset = data_read;

			for (rapidxml::xml_attribute<>* file_attribute = file_node->first_attribute(); file_attribute != nullptr; file_attribute = file_attribute->next_attribute())
			{
				if (!strcmp(file_attribute->name(), "Name"))
				{
					new_file.name = std::string(file_attribute->value());
				}
				else if (!strcmp(file_attribute->name(), "Size"))
				{
					sscanf_s(file_attribute->value(), "%d", &new_file.size);
				}
				else if (!strcmp(file_attribute->name(), "CompressedSize"))
				{
					sscanf_s(file_attribute->value(), "%d", &new_file.compressed_size);
				}
				else if (!strcmp(file_attribute->name(), "Checksum"))
				{
					sscanf_s(file_attribute->value(), "%x", &new_file.checksum);
				}
				else if (!strcmp(file_attribute->name(), "FileTime"))
				{
					sscanf_s(file_attribute->value(), "%x", &new_file.file_time);
				}
				else if (!strcmp(file_attribute->name(), "Algorithm"))
				{
					sscanf_s(file_attribute->value(), "%d", &new_file.algorithm);
				}
			}
				
			new_file.compressed_file_data.reset(new unsigned char[new_file.compressed_size + (new_file.algorithm ? 4 : 0)]);
			
			if (!data_file.read_data(reinterpret_cast<char*>(new_file.compressed_file_data.get()),
				new_file.compressed_size, new_file.offset))
			{
				delete[] data_buffer;
				return false;
			}
				
			new_file.file_data.reset(new unsigned char[new_file.size]);
				
			if (new_file.algorithm == algorithm_type::compressed_encrypted)
			{
				if (!crypto::decrypt_algorithm(new_file.name, new_file.compressed_file_data, new_file.compressed_size))
				{
					delete[] data_buffer;
					return false;
				}
			}
				
			unsigned long decompressed_file_size = new_file.size;

			int z_err = uncompress(new_file.file_data.get(), &decompressed_file_size, new_file.compressed_file_data.get() + 
				((new_file.algorithm == algorithm_type::compressed_encrypted) && (new_file.compressed_size >= 4) ? 4 : 0), new_file.compressed_size);
				
			new_file.size = decompressed_file_size;

			if (z_err != Z_OK)
			{
				delete[] data_buffer;
				return false;
			}
			
			decompressed_files.push_back(new_file);
			data_read += new_file.compressed_size;
			file_time_sum += new_file.file_time;
		}
		
		delete[] data_buffer;
		return (header->file_time_sum == file_time_sum && data_read == data_file.get_size());
	}
	
	bool dump_decompressed_files(std::string const& file_path, std::vector<actual_file> decompressed_files)
	{
		if (!CreateDirectory(file_path.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
			return false;
		
		for (kom::actual_file x : decompressed_files)
		{
			if (x.file_data.get())
			{
				io_file file;
		
				if (!file.create(file_path + "\\" + x.name))
					return false;
			
				if (!file.write_data(reinterpret_cast<char*>(x.file_data.get()), x.size))
					return false;
			}
		}
		
		return true;
	}
	
	bool compress_file(actual_file& new_file)
	{
		const char* file_extension = strrchr(new_file.name.c_str(), '.');
		
		if (file_extension == nullptr)
			return false;

		if (!strcmp(file_extension, ".lua"))
		{
			/* Encrypt LUA */
		}
		
		unsigned long compressed_file_size = new_file.size;
		new_file.compressed_file_data.reset(new unsigned char[new_file.size]);

		int z_err = compress(new_file.compressed_file_data.get(), &compressed_file_size, new_file.file_data.get(), new_file.size);

		if (z_err != Z_OK)
			return false;
		
		new_file.compressed_size = compressed_file_size;

		if (!strcmp(file_extension, ".lua") || !strcmp(file_extension, ".txt") || !strcmp(file_extension, ".kim"))
		{
			/* Encrypt algorithm */
			new_file.algorithm = static_cast<std::size_t>(algorithm_type::compressed_encrypted);
		}

		return true;
	}
	
	bool dump_compressed_files(std::string const& file_path, std::vector<actual_file> compressed_files)
	{	
		rapidxml::xml_document<> doc;

		rapidxml::xml_node<>* declaration = doc.allocate_node(rapidxml::node_declaration);
		declaration->append_attribute(doc.allocate_attribute("version", "1.0"));
		doc.append_node(declaration);

		rapidxml::xml_node<>* root = doc.allocate_node(rapidxml::node_element, "Files");
		doc.append_node(root);

		unsigned int file_time_sum = 0;

		for (kom::actual_file& x : compressed_files)
		{
			std::string size_string = std::to_string(x.size);
			std::string compressed_size_string = std::to_string(x.compressed_size);
			std::string algorithm_string = std::to_string(x.algorithm);

			char checksum[(sizeof(unsigned long) * 2) + 1];
			memset(checksum, 0, sizeof(checksum));
			sprintf(checksum, "%08x", x.checksum);

			file_time_sum += x.file_time;

			rapidxml::xml_node<>* child_note = doc.allocate_node(rapidxml::node_element, "File");
			child_note->append_attribute(doc.allocate_attribute("Name", doc.allocate_string(x.name.c_str(), x.name.length() + 1)));
			child_note->append_attribute(doc.allocate_attribute("Size", doc.allocate_string(size_string.c_str(), size_string.length() + 1)));
			child_note->append_attribute(doc.allocate_attribute("CompressedSize", doc.allocate_string(compressed_size_string.c_str(), compressed_size_string.length() + 1)));					
			child_note->append_attribute(doc.allocate_attribute("Checksum", doc.allocate_string(checksum, (sizeof(unsigned long) * 2) + 1)));
			child_note->append_attribute(doc.allocate_attribute("FileTime", doc.allocate_string(checksum, (sizeof(unsigned long) * 2) + 1)));
			child_note->append_attribute(doc.allocate_attribute("Algorithm", doc.allocate_string(algorithm_string.c_str(), algorithm_string.length() + 1)));
			root->append_node(child_note);
		}

		std::string xml_header_string;
		rapidxml::print(std::back_inserter(xml_header_string), doc, rapidxml::print_no_indenting);

		unsigned char* xml_header_data = reinterpret_cast<unsigned char*>(const_cast<char*>(xml_header_string.c_str()));

		kom_header file_header;
		memset(&file_header, 0, sizeof(kom_header));

		strcpy(file_header.file_description, "KOG GC TEAM MASSFILE V.0.3.");
		file_header.file_entries = compressed_files.size();
		file_header.always_one = 1;
		file_header.file_time_sum = file_time_sum;
		file_header.checksum = adler32(adler32(0, Z_NULL, 0), xml_header_data, xml_header_string.length());
		file_header.data_size = xml_header_string.length();

		io_file file;

		if (!file.create(file_path))
			return false;
		
		if (!file.write_data(reinterpret_cast<char*>(&file_header), sizeof(kom_header)))
			return false;

		if (!file.write_data(const_cast<char*>(xml_header_string.c_str()), xml_header_string.length()))
			return false;
		
		for (kom::actual_file& x : compressed_files)
		{
			if (!file.write_data(reinterpret_cast<char*>(x.compressed_file_data.get()), x.compressed_size))
				return false;
		}
		
		return true;
	}
}