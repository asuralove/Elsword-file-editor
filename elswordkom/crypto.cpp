#include "crypto.hpp"
#include "crc32.hpp"

#include "output.hpp"

namespace crypto
{	
	bool decrypt_algorithm(std::string const& file_name, std::shared_ptr<unsigned char>& file_data, std::size_t file_size)
	{
		if (!file_data.get() || file_size <= 0)
			return false;
		
		std::wstring wide_name = std::wstring(L"").assign(file_name.begin(), file_name.end());
		wchar_t* wide_file_name = const_cast<wchar_t*>(wide_name.c_str());
		
		if (!wide_file_name || !*wide_file_name)
			return false;
		
		unsigned char* data_buffer = file_data.get();
		
		unsigned char* seed_buffer = new unsigned char[(wcslen(wide_file_name) * 2) + 4];
		unsigned int seed_offset = 0;
		
		unsigned char crc_keys[] = { 0xFD, 0xE1, 0x08, 0xEC, 0x0D, 0x09, 0x02, 0xDC, 0xDE, 0x4C, 0x8C, 0x64, 0x16, 0x62, 0x7D, 0xE4, 0x97, 0x16, 0xFF, 0xDC };
		unsigned int crc_key_offset = 0;
		unsigned int crc_init_seed_1 = 0xFFFFFFFF;
	
		std::size_t file_name_length = wcslen(wide_file_name);

		if (file_name_length > 260)
			file_name_length = 260;

		if (file_name_length > 0)
		{
			for (std::size_t i = 0; i < file_name_length; i++)
			{
				unsigned short lowercase_char = tolower(wide_file_name[i]);

				unsigned char low_byte = static_cast<unsigned char>(lowercase_char);
				unsigned char high_byte = (lowercase_char >> 8);
					
				seed_buffer[seed_offset++] = low_byte;

				if (high_byte)
				{
					seed_buffer[seed_offset++] = high_byte;
				}
			}
		}

		for (int i = 0, seed = file_size; i < 4 && seed; i++, seed >>= 8)
			seed_buffer[seed_offset++] = static_cast<unsigned char>(seed);
			
		unsigned int* crc32_dword_table = crc32::get_table();

		for (unsigned int i = 0; i < seed_offset; i++)
		{
			unsigned int crc_key_seed = crc_keys[crc_key_offset++] ^ static_cast<unsigned char>(crc_init_seed_1);

			if (crc_key_offset == 20)
				crc_key_offset = 0;

			unsigned int crc_key = static_cast<unsigned char>(~(seed_buffer[i])) ^ crc_key_seed;
			crc_init_seed_1 = (crc_init_seed_1 >> 8) ^ ((crc32_dword_table[crc_key] & 0xFFFFFF00) | crc_key);
		}

		unsigned int crc_init_seed_2 = crc_init_seed_1;

		if (crc_key_offset >= 20)
			crc_key_offset %= 20;
			
		for (unsigned int i = 0; i < file_size; i++, data_buffer++)
		{
			unsigned int crc_key_seed = crc_keys[crc_key_offset++] ^ static_cast<unsigned char>(crc_init_seed_1);

			if (crc_key_offset == 20)
				crc_key_offset = 0;

			unsigned int crc_key = static_cast<unsigned char>(~(data_buffer[0])) ^ crc_key_seed;
			data_buffer[0] = static_cast<unsigned char>(crc32_dword_table[crc_key]) ^ static_cast<unsigned char>(crc_key_seed);
			crc_init_seed_1 = ((crc32_dword_table[crc_key] & 0xFFFFFF00) | crc_key) ^ (crc_init_seed_1 >> 8);
		}

		if (file_size >= 4 && data_buffer)
		{
			if (crc_key_offset >= 20)
				crc_key_offset %= 20;
		
			unsigned char* crc32_byte_table = reinterpret_cast<unsigned char*>(crc32_dword_table);

			unsigned int crc32_key_1 = crc32_byte_table[1024 + (crc_init_seed_1 >> 24)];
			unsigned int crc32_seed_1 = (((crc32_dword_table[crc32_key_1] & 0xFFFFFF00) | crc32_key_1) ^ crc_init_seed_1) << 8;

			unsigned int crc32_key_2 = crc32_byte_table[1024 + (crc32_seed_1 >> 24)];
			unsigned int crc32_seed_2 = (((crc32_dword_table[crc32_key_2] & 0xFFFFFF00) | crc32_key_2) ^ crc32_seed_1) << 8;

			unsigned int crc32_key_3 = crc32_byte_table[1024 + (crc32_seed_2 >> 24)];
			unsigned int crc32_seed_3 = (((crc32_dword_table[crc32_key_3] & 0xFFFFFF00) | crc32_key_3) ^ crc32_seed_2) << 8;
				
			unsigned int crc32_key_4 = crc32_byte_table[1024 + (crc32_seed_3 >> 24)];
			unsigned int crc32_seed_4 = crc_keys[crc_key_offset++] ^ static_cast<unsigned char>(crc_init_seed_2);

			if (crc_key_offset >= 20)
				crc_key_offset %= 20;

			data_buffer[0] = static_cast<unsigned char>(crc32_dword_table[crc32_key_4]) ^ static_cast<unsigned char>(crc32_seed_4);
				
			unsigned int crc32_key_5 = (((crc32_dword_table[crc32_key_4] & 0xFFFFFF00) | crc32_key_4) ^ (crc_init_seed_2 >> 8));
			unsigned int crc32_seed_5 = crc_keys[crc_key_offset++] ^ static_cast<unsigned char>(crc32_key_5);
				
			if (crc_key_offset >= 20)
				crc_key_offset %= 20;
				
			data_buffer[1] = static_cast<unsigned char>(crc32_dword_table[crc32_key_3]) ^ static_cast<unsigned char>(crc32_seed_5);

			unsigned int crc32_key_6 = (((crc32_dword_table[crc32_key_3] & 0xFFFFFF00) | crc32_key_3) ^ (crc32_key_5 >> 8));
			unsigned int crc32_seed_6 = crc_keys[crc_key_offset++] ^ static_cast<unsigned char>(crc32_key_6);
				
			if (crc_key_offset >= 20)
				crc_key_offset %= 20;
				
			data_buffer[2] = static_cast<unsigned char>(crc32_dword_table[crc32_key_2]) ^ static_cast<unsigned char>(crc32_seed_6);

			unsigned int crc32_key_7 = (((crc32_dword_table[crc32_key_2] & 0xFFFFFF00) | crc32_key_2) ^ (crc32_key_6 >> 8));
			unsigned int crc32_seed_7 = crc_keys[crc_key_offset++] ^ static_cast<unsigned char>(crc32_key_7);

			data_buffer[3] = static_cast<unsigned char>(crc32_dword_table[crc32_key_1]) ^ static_cast<unsigned char>(crc32_seed_7);
		}
		
		delete[] seed_buffer;
		return true;
	}
	
	bool has_special_header(std::shared_ptr<unsigned char>& file_data, std::size_t file_size)
	{
		unsigned char* data_buffer = file_data.get();

		if (file_size >= 3 && data_buffer[0] == 0xEF && data_buffer[1] == 0xBB && data_buffer[2] == 0xBF)
		{
			return true;
		}
		else if (data_buffer[0] != 0x1B)
		{
			//return false;
		}
		
		return false;
	}
}