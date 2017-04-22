#pragma once

#include "generic.hpp"

namespace crc32
{
	void make_table(unsigned int* crc32_table)
	{
		static const unsigned char polynomial_terms[] = { 0, 1, 2, 4, 5, 7, 8, 10, 11, 12, 16, 22, 23, 26 };
		
		unsigned int polynomial = 0;

		for (unsigned int i = 0; i < sizeof(polynomial_terms); i++)
			polynomial |= (1 << (31 - polynomial_terms[i]));

		for (unsigned int i = 0, crc = 0; i < 256; i++, crc = i)
		{
			for (unsigned int j = 0; j < 8; j++)
				crc = (crc & 1 ? (polynomial ^ (crc >> 1)) : (crc >> 1));

			crc32_table[i] = crc;
		}

		unsigned char* crc32_byte_table = reinterpret_cast<unsigned char*>(crc32_table);

		for (unsigned int i = 0; i < 256; i++)
		{
			unsigned int offset = crc32_byte_table[(i * 4) + 3];
			crc32_byte_table[1024 + offset] = static_cast<unsigned char>(i);
		}
		
		for (unsigned int i = 0; i < 256; i++)
		{
			unsigned int crc = ((crc32_table[i] & 0xFFFFFF00) | i);
			crc32_table[320 + (crc32_table[i] & 0x000000FF)] = crc;
		}
	}

	unsigned int* get_table()
	{
		static bool crc_table_empty = true;
		static unsigned int crc32_table[575 + 1] = { 0 };

		if (crc_table_empty)
		{
			crc_table_empty = false;
			make_table(crc32_table);
		}

		return crc32_table;
	}
}