#pragma once

#include "generic.hpp"
#include "widget.hpp"

#include "resource.hpp"

#include "label.hpp"
#include "listview.hpp"
#include "panel.hpp"
#include "textbox.hpp"

#include "kom.hpp"

namespace elswordkom
{
	WPARAM execute();
	
	namespace gui
	{
		#define COLOR_RED		RGB(255, 0, 0)
		#define COLOR_YELLOW	RGB(255, 255, 0)
		#define COLOR_WHITE		RGB(255, 255, 255)
		
		enum class load_mode
		{
			none,
			kom,
			custom
		};

		class main_form : public widget
		{
		public:
			static main_form& get_instance()
			{
				static main_form instance("sparta_window_class", "Sparta Editor", MAKEINTRESOURCE(IDI_MAIN_ICON));
				return instance;
			}
			
		private:
			main_form(std::string const& class_name, std::string const& window_name, const char* icon_name = IDI_APPLICATION);
			~main_form();

			bool initialize();
			void initialize_menu();

			bool load_kom_file();
			bool save_kom_file();

			bool dump_from_kom_file();

			void set_load_mode(load_mode mode);

			bool register_class(std::string const& class_name, const char* icon_name);
			bool create_window(std::string const& class_name, std::string const& window_name, rectangle& rect);

			void set_message_handlers();

		private:
			HMENU menu_bar;
			std::vector<std::unique_ptr<widget>> widget_container;
			
			listview internal_file_list;

			label external_file_name;
			label internal_file_name;
			
			textbox internal_file_content;
			panel internal_file_image;

			load_mode mode;
			std::vector<kom::actual_file> current_files;
		};
	}
}