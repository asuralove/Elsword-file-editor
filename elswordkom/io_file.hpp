#pragma once

#include "generic.hpp"

class io_file
{
public:
	io_file()
	{
		
	}
	
	~io_file()
	{
		if (this->file_handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(this->file_handle);
		}
	}
	
	unsigned int get_size()
	{
		return (this->file_handle != INVALID_HANDLE_VALUE ? GetFileSize(this->file_handle, NULL) : 0);
	}

	bool create(std::string const& file_path)
	{
		return ((this->file_handle = CreateFileA(file_path.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, 
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE);
	}

	bool create(std::wstring const& file_path)
	{
		return ((this->file_handle = CreateFileW(file_path.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, 
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE);
	}
	
	bool open(std::string const& file_path)
	{
		return ((this->file_handle = CreateFileA(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, 
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE);
	}

	bool open(std::wstring const& file_path)
	{
		return ((this->file_handle = CreateFileW(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, 
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE);
	}

	bool read_data(char* buffer, unsigned int size, unsigned int offset = 0)
	{
		if (SetFilePointer(this->file_handle, offset, NULL, 0) == INVALID_SET_FILE_POINTER)
		{
			return false;
		}

		DWORD bytes_read;
		return (ReadFile(this->file_handle, buffer, size, &bytes_read, 0) && bytes_read == size);
	}

	bool write_data(char* buffer, unsigned int size)
	{
		DWORD bytes_written;
		return (WriteFile(this->file_handle, buffer, size, &bytes_written, 0) && bytes_written == size);
	}

private:
	HANDLE file_handle;
};