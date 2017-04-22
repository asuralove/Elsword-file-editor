#include "output.hpp"

namespace output
{
	void hexdump(void* input, int length)
	{
		unsigned char* buffer = reinterpret_cast<unsigned char*>(input);

		for (int i = 0; i < length; i += 16)
		{
			printf("%06X: ", i);

			for (int j = 0; j < 16; j++)
			{
				if (i + j < length)
				{
					printf("%02X ", buffer[i + j]);
				}
				else
				{
					printf("   ");
				}
			}

			printf(" ");

			for (int j = 0; j < 16; j++)
			{
				if (i + j < length)
				{
					printf("%c", isprint(buffer[i + j]) ? buffer[i + j] : '.');
				}
			}

			printf("\n");
		}
	}
	
	void shexdump(char* output, void* input, int length)
	{
		unsigned char* buffer = reinterpret_cast<unsigned char*>(input);

		char temp_buffer[256];
		memset(temp_buffer, 0, sizeof(temp_buffer));

		for (int i = 0; i < length; i += 16)
		{
			memset(temp_buffer, 0, sizeof(temp_buffer));
			sprintf(temp_buffer, "%06X: ", i);
			strcat(output, temp_buffer);

			for (int j = 0; j < 16; j++)
			{
				if (i + j < length)
				{
					memset(temp_buffer, 0, sizeof(temp_buffer));
					sprintf(temp_buffer, "%02X ", buffer[i + j]);
					strcat(output, temp_buffer);
				}
				else
				{
					memset(temp_buffer, 0, sizeof(temp_buffer));
					sprintf(temp_buffer, "   ");
					strcat(output, temp_buffer);
				}
			}
			
			memset(temp_buffer, 0, sizeof(temp_buffer));
			sprintf(temp_buffer, " ");
			strcat(output, temp_buffer);

			for (int j = 0; j < 16; j++)
			{
				if (i + j < length)
				{
					memset(temp_buffer, 0, sizeof(temp_buffer));
					sprintf(temp_buffer, "%c", isprint(buffer[i + j]) ? buffer[i + j] : '.');
					strcat(output, temp_buffer);
				}
			}
			
			memset(temp_buffer, 0, sizeof(temp_buffer));
			sprintf(temp_buffer, "\r\n");
			strcat(output, temp_buffer);
		}
	}
	
	void fhexdump(FILE* file, void* input, int length)
	{
		unsigned char* buffer = reinterpret_cast<unsigned char*>(input);

		for (int i = 0; i < length; i += 16)
		{
			fprintf(file, "%06X: ", i);

			for (int j = 0; j < 16; j++)
			{
				if (i + j < length)
				{
					fprintf(file, "%02X ", buffer[i + j]);
				}
				else
				{
					fprintf(file, "   ");
				}
			}

			fprintf(file, " ");

			for (int j = 0; j < 16; j++)
			{
				if (i + j < length)
				{
					fprintf(file, "%c", isprint(buffer[i + j]) ? buffer[i + j] : '.');
				}
			}

			fprintf(file, "\n");
		}
	}
}