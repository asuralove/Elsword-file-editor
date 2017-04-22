#include "main_form.hpp"

#include <fstream>
#include <regex>

#include "output.hpp"

namespace elswordkom
{
	WPARAM execute()
	{
		MSG message;
		while (GetMessage(&message, 0, 0, 0) > 0)
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		return message.wParam;
	}

	namespace gui
	{
		bool is_interpretable_image_file(std::string const& file_name)
		{
			std::string file_type = file_name.substr(file_name.find_last_of('.'));

			return (file_type.compare(".tga") == 0);
		}

		bool is_interpretable_text_file(std::string const& file_name)
		{
			std::string file_type = file_name.substr(file_name.find_last_of('.'));
			
			return (file_type.compare(".txt") == 0 || file_type.compare(".ini") == 0 || file_type.compare(".xml") == 0);
		}

		main_form::main_form(std::string const& class_name, std::string const& window_name, const char* icon_name) 
			: widget(0, 0, true), menu_bar(NULL), mode(load_mode::custom)
		{
			if (!this->register_class(class_name, icon_name))
			{
				throw std::exception("Failed to register main window class!");
			}

			if (!this->create_window(class_name, window_name, rectangle(0, 0, 900, 500)))
			{
				throw std::exception("Failed to create main window!");
			}
			
			this->set_message_handlers();

			if (!this->initialize())
			{
				throw std::exception("Failed to initialize main window!");
			}

			this->initialize_menu();
		}

		main_form::~main_form()
		{

		}

		bool main_form::initialize()
		{
			/* file list */
			this->internal_file_list.assemble(*this, rectangle(5, 5, 330, 490), [this](std::size_t index) -> void
			{
				kom::actual_file& new_file = this->current_files.at(index);

				if (new_file.file_data.get())
				{
					this->internal_file_name.set_text(new_file.name);

					if (is_interpretable_image_file(new_file.name))
					{
						this->internal_file_content.show(false);
						this->internal_file_image.set_targa_image(reinterpret_cast<unsigned char*>(new_file.file_data.get()), new_file.size);
						this->internal_file_image.show(true);
					}
					else if (bool b = is_interpretable_text_file(new_file.name))
					{
						this->internal_file_image.show(false);
						this->internal_file_content.set_text_rich(new_file);
						this->internal_file_content.show(true);
					}
					else
					{
						char* p = new char[new_file.size * 6];
						memset(p, 0, new_file.size * 6);
						output::shexdump(p, new_file.file_data.get(), new_file.size);

						this->internal_file_image.show(false);
						this->internal_file_content.set_text(std::string(p));
						this->internal_file_content.show(true);

						delete[] p;
					}
				}
			});
			
			this->internal_file_list.add_column("File name", 245);
			this->internal_file_list.add_column("File size", 60);
			
			/* file content */
			this->internal_file_content.assemble(*this, rectangle(340, 25, 555, 470));
			this->internal_file_content.set_font("Consolas");
			this->internal_file_content.show(false);

			this->internal_file_image.assemble(*this, rectangle(340, 25, 555, 470));
			this->internal_file_image.show(false);

			/* file names */
			this->widget_container.push_back(std::make_unique<label>(*this, rectangle(345, 5, 60, 15), "External file:"));
			
			this->external_file_name.assemble(*this, rectangle(410, 5, 80, 15));
			this->external_file_name.set_foreground_color(RGB(24, 180, 46));
			this->external_file_name.set_text("N/A");
			
			this->widget_container.push_back(std::make_unique<label>(*this, rectangle(495, 5, 60, 15), "Internal file:"));
							
			this->internal_file_name.assemble(*this, rectangle(560, 5, 335, 15));
			this->internal_file_name.set_foreground_color(RGB(200, 42, 42));
			this->internal_file_name.set_text("N/A");
			
			this->set_load_mode(load_mode::none);

			/* temp */
			//this->set_load_mode(load_mode::kom);

			//if (!kom::decompress_files("G:\\Games\\Elsword (North America)\\data\\data036.kom", this->current_files))
			//{
			//	this->set_load_mode(load_mode::none);
			//	MessageBox(this->get_handle(), "Could not load .kom file.", "Error!", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
			//	return false;
			//}

			//this->external_file_name.set_text("data036.kom");

			//for (kom::actual_file x : this->current_files)
			//{
			//	if (is_interpretable_image_file(x.name) || is_interpretable_text_file(x.name))
			//		this->internal_file_list.add_item(RGB(75, 200, 75), 2, x.name.c_str(), std::to_string(x.size).c_str());
			//	else
			//		this->internal_file_list.add_item(RGB(200, 75, 75), 2, x.name.c_str(), std::to_string(x.size).c_str());
			//}

			return true;
		}
		
		void main_form::initialize_menu()
		{
			HMENU file_menu = CreatePopupMenu();
			AppendMenu(file_menu, MF_STRING, IDM_LOAD_KOM, "Load .kom");
			//AppendMenu(file_menu, MF_STRING, IDM_SAVE_KOM, "Save .kom");
			
			HMENU dump_menu = CreatePopupMenu();
			AppendMenu(dump_menu, MF_STRING, IDM_DUMP_FROM_KOM, "Dump files from .kom");

			this->menu_bar = CreateMenu();
			AppendMenu(this->menu_bar, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(file_menu), "File");
			AppendMenu(this->menu_bar, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(dump_menu), "Dump");
			
			SetMenu(this->get_handle(), this->menu_bar);
		}
		
		bool main_form::load_kom_file()
		{
			char current_directory[MAX_PATH];
			memset(current_directory, 0, MAX_PATH);
			
			if (!GetModuleFileName(GetModuleHandle(NULL), current_directory, MAX_PATH))
				return false;

			*strrchr(current_directory, '\\') = 0;

			char target_file[MAX_PATH];
			memset(target_file, 0, MAX_PATH);

			OPENFILENAME open_file_name;
			memset(&open_file_name, 0, sizeof(OPENFILENAME));

			open_file_name.lStructSize = sizeof(OPENFILENAME);
			open_file_name.hwndOwner = this->get_handle();
			open_file_name.lpstrDefExt = ".kom";
			open_file_name.lpstrFile = target_file;
			open_file_name.nMaxFile = MAX_PATH;
			open_file_name.lpstrFilter = "KOM Files\0*.kom\0\0";
			open_file_name.lpstrInitialDir = current_directory;
			open_file_name.lpstrTitle = "Load .kom";
			open_file_name.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

			if (!GetOpenFileName(&open_file_name))
				return false;
			
			if (strlen(target_file) <= 4)
			{
				this->set_load_mode(load_mode::none);
				MessageBox(this->get_handle(), "Could not load .kom file.", "Error!", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
				return false;
			}

			this->set_load_mode(load_mode::kom);

			if (!kom::decompress_files(target_file, this->current_files))
			{
				this->set_load_mode(load_mode::none);
				MessageBox(this->get_handle(), "Could not load .kom file.", "Error!", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
				return false;
			}

			this->external_file_name.set_text(strrchr(target_file, '\\') + 1);

			for (kom::actual_file x : this->current_files)
			{
				if (is_interpretable_image_file(x.name) || is_interpretable_text_file(x.name))
					this->internal_file_list.add_item(RGB(75, 200, 75), 2, x.name.c_str(), std::to_string(x.size).c_str());
				else
					this->internal_file_list.add_item(RGB(200, 75, 75), 2, x.name.c_str(), std::to_string(x.size).c_str());
			}
		
			return true;
		}
		
		bool main_form::save_kom_file()
		{
			if (this->mode != load_mode::kom)
			{
				MessageBox(this->get_handle(), "Can only save .kom in 'KOM' mode.", "Error!", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
				return false;
			}

			char current_directory[MAX_PATH];
			memset(current_directory, 0, MAX_PATH);
			
			if (!GetModuleFileName(GetModuleHandle(NULL), current_directory, MAX_PATH))
			{
				return false;
			}

			*strrchr(current_directory, '\\') = 0;

			char target_file[MAX_PATH];
			memset(target_file, 0, MAX_PATH);

			OPENFILENAME open_file_name;
			memset(&open_file_name, 0, sizeof(OPENFILENAME));

			open_file_name.lStructSize = sizeof(OPENFILENAME);
			open_file_name.hwndOwner = this->get_handle();
			open_file_name.lpstrDefExt = ".kom";
			open_file_name.lpstrFile = target_file;
			open_file_name.nMaxFile = MAX_PATH;
			open_file_name.lpstrFilter = "KOM Files\0*.kom\0\0";
			open_file_name.lpstrInitialDir = current_directory;
			open_file_name.lpstrTitle = "Save .kom";

			if (!GetSaveFileName(&open_file_name))
			{
				return false;
			}
			
			if (strlen(target_file) <= 4)
			{
				MessageBox(this->get_handle(), "Could not save .kom file.", "Error!", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
				return false;
			}
			
			if (!kom::dump_compressed_files(target_file, this->current_files))
			{
				MessageBox(this->get_handle(), "Could not save .kom file.", "Error!", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
				return false;
			}

			MessageBox(this->get_handle(), "Succesfully saved .kom file.", "Success!", MB_OK | MB_ICONINFORMATION | MB_TOPMOST | MB_SETFOREGROUND);
			return true;
		}
		
		bool main_form::dump_from_kom_file()
		{
			if (this->mode != load_mode::kom)
			{
				MessageBox(this->get_handle(), "Can only dump files from .kom in 'KOM' mode.", "Error!", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
				return false;
			}
		
			char current_directory[MAX_PATH];
			memset(current_directory, 0, MAX_PATH);
			
			if (!GetModuleFileName(GetModuleHandle(NULL), current_directory, MAX_PATH))
			{
				MessageBox(this->get_handle(), "Could not dump .kom file.", "Error!", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
				return false;
			}

			*(strrchr(current_directory, '\\') + 1) = 0;

			std::string file_name = this->external_file_name.get_text();
			std::string directory_path = current_directory + file_name.substr(0, file_name.find_last_of('.'));

			if (!kom::dump_decompressed_files(directory_path, this->current_files))
			{
				MessageBox(this->get_handle(), "Could not dump .kom file.", "Error!", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
				return false;
			}

			MessageBox(this->get_handle(), "Succesfully dumped from .kom file.", "Success!", MB_OK | MB_ICONINFORMATION | MB_TOPMOST | MB_SETFOREGROUND);
			return true;
		}

		void main_form::set_load_mode(load_mode mode)
		{
			if (this->mode != mode)
			{
				this->current_files.clear();
				this->internal_file_list.clear_items();

				this->internal_file_content.set_text("");
				this->internal_file_content.set_read_only(true);

				this->mode = mode;
			}
		}

		bool main_form::register_class(std::string const& class_name, const char* icon_name)
		{
			WNDCLASSEX wcex;
			memset(&wcex, 0, sizeof(WNDCLASSEX));

			wcex.cbSize = sizeof(WNDCLASSEX);
			wcex.lpfnWndProc = main_form::window_proc;
			wcex.hInstance = this->get_instance_handle();
			wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
			wcex.hIcon = LoadIcon(this->get_instance_handle(), icon_name);
			wcex.hIconSm = LoadIcon(this->get_instance_handle(), icon_name);
			wcex.hbrBackground = CreateSolidBrush(this->get_background_color());
			wcex.lpszClassName = class_name.c_str();

			return (RegisterClassEx(&wcex) != NULL);
		}

		bool main_form::create_window(std::string const& class_name, std::string const& window_name, rectangle& rect)
		{
			RECT rc_window = { rect.get_x(), rect.get_y(), rect.get_width(), rect.get_height() };
			AdjustWindowRect(&rc_window, WS_OVERLAPPEDWINDOW, TRUE);
			
			rect.set_width_height(rc_window.right - rc_window.left, rc_window.bottom - rc_window.top);

			if (!rect.get_x() && !rect.get_y())
			{
				RECT rc;
				SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);

				rect.set_x_y((rc.right / 2) - (rect.get_width() / 2), (rc.bottom / 2) - (rect.get_height() / 2));
			}
			
			return this->create(class_name, window_name, rect, WS_OVERLAPPEDWINDOW);
		}
		
		void main_form::set_message_handlers()
		{
			this->add_message_handlers(8,
				message_pair(WM_CLOSE, [this](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
				{
					DestroyWindow(hWnd);
					return 0;
				}),
				message_pair(WM_DESTROY, [](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
				{
					PostQuitMessage(0);
					return 0;
				}),
				message_pair(WM_CTLCOLORLISTBOX, [this](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
				{
					return SendMessage(reinterpret_cast<HWND>(lParam), CUSTOM_CTLCOLOR, wParam, lParam);
				}),
				message_pair(WM_MEASUREITEM, [this](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
				{
					MEASUREITEMSTRUCT* measure_item = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);

					if (measure_item->CtlType == ODT_LISTVIEW)
					{
						reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->itemHeight = 24;
						return 0;
					}

					return DefWindowProc(hWnd, WM_MEASUREITEM, wParam, lParam);
				}),
				message_pair(WM_NOTIFY, [this](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
				{
					return SendMessage(reinterpret_cast<NMHDR*>(lParam)->hwndFrom, CUSTOM_NOTIFY, wParam, lParam);
				}),
				message_pair(WM_GETMINMAXINFO, [this](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
				{
					MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
					mmi->ptMinTrackSize.x = 900;
					mmi->ptMinTrackSize.y = 500;

					return DefWindowProc(hWnd, WM_GETMINMAXINFO, wParam, lParam);
				}),
				message_pair(WM_SIZE, [this](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
				{
					this->internal_file_list.set_size(size(330, HIWORD(lParam) - 10));
					
					this->internal_file_content.set_size(size(LOWORD(lParam) - 345, HIWORD(lParam) - 30));
					this->internal_file_image.set_size(size(LOWORD(lParam) - 345, HIWORD(lParam) - 30));

					return DefWindowProc(hWnd, WM_SIZE, wParam, lParam);
				}),
				message_pair(WM_COMMAND, [this](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
				{
					if (LOWORD(wParam) == IDM_LOAD_KOM)
					{
						return static_cast<LRESULT>(this->load_kom_file());
					}
					else if (LOWORD(wParam) == IDM_SAVE_KOM)
					{
						return static_cast<LRESULT>(this->save_kom_file());
					}
					else if (LOWORD(wParam) == IDM_DUMP_FROM_KOM)
					{
						return static_cast<LRESULT>(this->dump_from_kom_file());
					}
					
					return SendMessage(reinterpret_cast<HWND>(lParam), CUSTOM_COMMAND, wParam, lParam);
				})
			);
		}
	}
}