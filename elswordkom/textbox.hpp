#pragma once

#include "generic.hpp"
#include "widget.hpp"

#include <Richedit.h>

#include "crypto.hpp"
#include "kom.hpp"

#include "output.hpp"

namespace elswordkom
{
	namespace gui
	{
		typedef DWORD (CALLBACK* EditStreamCallback)(
		  _In_ DWORD_PTR dwCookie,
		  _In_ LPBYTE    pbBuff,
		  _In_ LONG      cb,
		  _In_ LONG      *pcb
		);

		typedef std::function<bool()> textbox_function_t;

		class textbox : public widget
		{
			void init()
			{
				this->format_reset = false;
				this->format_comment = false;
			}

		public:
			textbox(HWND hwnd_parent = NULL, HINSTANCE instance = NULL)
				: widget(hwnd_parent, instance)
			{
				this->init();
			}
			
			textbox(widget& parent_widget, rectangle& rect = rectangle())
				: widget(parent_widget.get_handle(), NULL)
			{
				this->init();

				if (!this->assemble(parent_widget, rect))
					throw std::exception("Failed to create 'textbox' widget");
			}

			~textbox()
			{

			}

			bool assemble(widget& parent_widget, rectangle& rect)
			{
				LoadLibrary("Msftedit.dll");

				if (!this->create("RICHEDIT50W", "", rect, WS_VISIBLE | WS_CHILD | ES_MULTILINE | WS_VSCROLL | WS_HSCROLL, NULL, parent_widget.get_handle()))
					return false;

				//unsigned int tabstops = 16;
				//SendMessage(this->hwnd, EM_SETTABSTOPS, 1, reinterpret_cast<LPARAM>(&tabstops));
				
				//SendMessage(this->hwnd, EM_SETBKGNDCOLOR, 0, RGB(30, 30, 30));

				this->set_message_handlers();
				return true;
			}
			
			bool set_read_only(bool read_only)
			{
				return (Edit_SetReadOnly(this->get_handle(), static_cast<BOOL>(read_only)) != FALSE);
			}
			
			std::string get_text()
			{
				int text_length = Edit_GetTextLength(this->get_handle());
				
				char* text_buffer = new char[text_length + 1];
				memset(text_buffer, 0, text_length + 1);
				
				text_length = Edit_GetText(this->get_handle(), text_buffer, text_length + 1);

				if (text_length <= 0)
				{
					delete[] text_buffer;
					return std::string("");
				}

				std::string text_string(text_buffer);

				delete[] text_buffer;
				return text_string;
			}
			
			bool set_text_rich(kom::actual_file& file)
			{
				int offset = crypto::has_special_header(file.file_data, file.size) ? 3 : 0;

				int event_mask = SendMessage(this->hwnd, EM_SETEVENTMASK, 0, 0);
				SendMessage(this->hwnd, WM_SETREDRAW, FALSE, 0);
				
				EditStreamCallback edit_stream_callback = [](DWORD_PTR dwCookie, LPBYTE pbBuffer, LONG cb, LONG* pcb) -> DWORD
				{
					std::wstring* data = reinterpret_cast<std::wstring*>(dwCookie);

					if (static_cast<int>(data->length() * sizeof(wchar_t)) < cb)
						*pcb = (data->length() * sizeof(wchar_t));
					else
						*pcb = cb;

					memcpy(pbBuffer, data->c_str(), *pcb);
					*data = data->substr((*pcb) / sizeof(wchar_t));
					return 0;
				};
				
				std::wstring utf = utf8_decode(std::string(reinterpret_cast<char*>(file.file_data.get()) + offset, file.size - offset));
				std::replace_if(utf.begin(), utf.end(), [](wchar_t w) -> bool { return (w > 0x1000); }, L'?');

				EDITSTREAM es;
				es.dwError = 0;
				es.pfnCallback = edit_stream_callback;
				es.dwCookie = reinterpret_cast<DWORD>(&utf);
				
				SendMessage(this->hwnd, EM_STREAMIN, SF_TEXT | SF_UNICODE, reinterpret_cast<LPARAM>(&es));
				
				//if (!this->parse_all_text())
				//	return false;
				
				SendMessage(this->hwnd, WM_SETREDRAW, TRUE, 0);
				InvalidateRect(this->hwnd, 0, TRUE);
				SendMessage(this->hwnd, EM_SETEVENTMASK, 0, event_mask);
				return true;
			}
		
		private:
			bool parse_all_text()
			{
				CHARRANGE old_charrange;
				SendMessage(this->hwnd, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&old_charrange));
				
				CHARRANGE new_charrange = { 0, -1 };
				SendMessage(this->hwnd, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&new_charrange));

				this->set_token_color(RGB(210, 210, 210));

				for (int position = 0, next = 1;; position = next)
				{
					if (position == (next = SendMessage(this->hwnd, EM_FINDWORDBREAK, WB_MOVEWORDRIGHT, position)))
						break;
					
					new_charrange = { position, next };
					SendMessage(this->hwnd, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&new_charrange));

					wchar_t* token = new wchar_t[next - position + 1];
					int length = SendMessage(this->hwnd, EM_GETSELTEXT, 0, reinterpret_cast<LPARAM>(token));
				
					this->parse_token(std::wstring(token, length), position, next);

					delete[] token;
				}
				
				SendMessage(this->hwnd, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&old_charrange));
				return true;
			}

			void parse_token(std::wstring token, int position, int next)
			{
				//if (!token.compare(L"<!-- "))
				//{
				//	std::size_t t = 
				//	printf("commenting..\n");
				//	this->format_comment = true;
				//}
				//
				//if (this->format_comment)
				//{
				//	this->set_token_color(RGB(102, 116, 123));

				//	if (!token.compare(L"-->"))
				//	{
				//		printf("uncommenting..\n");
				//		this->format_comment = false;
				//	}
				//}

				//if (wcscmp(token.c_str(), L"EventSceneData") == 0)
				//	this->set_token_color(RGB(255, 255, 0));
			}

			void set_token_color(COLORREF token_color, int group = SCF_SELECTION)
			{
				CHARFORMAT char_format;
				memset(&char_format, 0, sizeof(CHARFORMAT));

				char_format.cbSize = sizeof(CHARFORMAT);
				char_format.dwMask = CFM_COLOR;
				char_format.crTextColor = token_color;
				
				SendMessage(this->hwnd, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&char_format));
			}

			void set_message_handlers()
			{
				this->add_message_handlers(1,
					message_pair(CUSTOM_CTLCOLOR, [this](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
					{
						SetBkMode(reinterpret_cast<HDC>(wParam), TRANSPARENT);
						SetTextColor(reinterpret_cast<HDC>(wParam), RGB(0, 0, 0));
						//SetTextColor(reinterpret_cast<HDC>(wParam), RGB(240, 240, 240));
						SetDCBrushColor(reinterpret_cast<HDC>(wParam), RGB(255, 255, 255));
						return reinterpret_cast<LRESULT>(GetStockObject(DC_BRUSH));
					})
				);
			}

		private:
			std::string utf8_encode(std::wstring const& wstr)
			{
				if (wstr.empty()) 
					return std::string();

				int utf8_length = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);

				std::string utf8(utf8_length, 0);
				WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), &utf8[0], utf8_length, NULL, NULL);

				return utf8;
			}

			std::wstring utf8_decode(std::string const& str)
			{
				if (str.empty()) 
					return std::wstring();

				int utf8_length = MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()), NULL, 0);

				std::wstring utf8(utf8_length, 0);
				MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()), &utf8[0], utf8_length);

				return utf8;
			}

		private:
			bool format_reset;
			bool format_comment;
		};
	}
}