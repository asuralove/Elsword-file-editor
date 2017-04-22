#pragma once

#include "generic.hpp"
#include "widget.hpp"


//#include "bitmap.hpp"

namespace elswordkom
{
	namespace gui
	{
		class panel : public widget
		{
			#pragma pack(push, 1)
			// TGA file header structure.
			struct TgaHeader
			{
				BYTE id_len;
				BYTE map_t;
				BYTE img_t;
				WORD map_first_entry_index;
				WORD map_len;
				BYTE map_entry_size;
				WORD x_origin;
				WORD y_origin;
				WORD width;
				WORD height;
				BYTE depth;
				BYTE image_descriptor;
			};

			// TGA file footer structure.
			struct TgaFooter
			{
				LONG extension_offset;
				LONG developer_area_offset;
				BYTE signature[18];
			};
			#pragma pack(pop)

		public:
			panel(HWND hwnd_parent = NULL, HINSTANCE instance = NULL)
				: widget(hwnd_parent, instance)
			{
				this->bits = 0;
			}
			
			panel(widget& parent_widget, rectangle& rect = rectangle())
				: widget(parent_widget.get_handle(), NULL)
			{
				if (!this->assemble(parent_widget, rect))
					throw std::exception("Failed to create 'panel' widget");
			}

			~panel()
			{

			}
			
			bool assemble(widget& parent_widget, rectangle& rect = rectangle())
			{
				if (!this->create(WC_STATIC, "", rect, WS_VISIBLE | WS_CHILD | SS_OWNERDRAW, NULL, parent_widget.get_handle()))
					return false;

				this->set_message_handlers();
				return true;
			}

			void set_targa_image(unsigned char* buffer, unsigned int size)
			{
				this->load_targa_image(buffer, size);
				InvalidateRect(this->get_handle(), NULL, FALSE);
			}

		protected:
			void set_message_handlers()
			{
				this->add_message_handlers(2,
					message_pair(WM_ERASEBKGND, [this](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
					{
						return 1;
					}),
					message_pair(OWNER_DRAWITEM, [this](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
					{
						return this->draw_item(hWnd, reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
					})
				);
			}
	
			LRESULT draw_item(HWND hwnd, DRAWITEMSTRUCT* panel_draw)
			{
				/* draw background */
				HBRUSH background_brush = CreateSolidBrush(this->get_widget(this->get_parent_handle()).get_background_color());
				FillRect(panel_draw->hDC, &panel_draw->rcItem, background_brush);
				DeleteObject(background_brush);

				/* draw bitmap */
				HDC memory_hdc = CreateCompatibleDC(panel_draw->hDC);
				HGDIOBJ old_bitmap = SelectObject(memory_hdc, this->bitmap);

				size panel_size = this->get_size();
				int x_offset = (panel_size.get_width() / 2) - (width / 2);
				int y_offset = (panel_size.get_height() / 2) - (height / 2);

				BitBlt(panel_draw->hDC, x_offset, y_offset, width, height, memory_hdc, 0, 0, SRCCOPY);

				SelectObject(memory_hdc, old_bitmap);
				DeleteDC(memory_hdc);
				return 0;
			}

		private:
			typedef struct 
			{
				unsigned char r;
				unsigned char g;
				unsigned char b;
				unsigned char a;
			} PIXEL;

			bool load_targa_image(unsigned char* buffer, unsigned int size)
			{
				int offset = 0;

				TgaHeader header;
				memcpy(&header, buffer, (offset += sizeof(header)));

				if (header.id_len > 0)
					offset += header.id_len;

				if (header.map_t != 0)
					offset += header.map_len;

				if (header.depth != 32 && header.depth != 24 && header.depth != 8)
					return false;
				
				if (!create_image(header.width, header.height))
					return false;
				
				unsigned int temp_pitch = header.width * (header.depth / 8);
				unsigned int buffer_size = temp_pitch * header.height;
				
				std::vector<unsigned char> temp_buffer(buffer + offset, buffer + offset + buffer_size);

				bool success = false;

				switch (header.img_t)
				{
				case 1: // Uncompressed, color-mapped images.
				case 2: // Uncompressed, RGB images.
				case 3: // Uncompressed, black and white images.
					success = decode_uncompressed(temp_buffer, header);
					break;

				case 10: // Runlength encoded RGB images.
					success = decode_compressed_rle(temp_buffer, header);
					break;

				default:
					break;
				}

				if (success)
				{
					if ((header.image_descriptor & 0x30) != 0x20)
					{
						int full_width = (header.width * sizeof(PIXEL));
						std::vector<unsigned char> temp_pixels(this->bits, this->bits + (header.height * full_width));

						for (unsigned int i = 0; i < header.height; i++)
							memcpy(&this->bits[(header.height - 1 - i) * full_width], &temp_pixels[i * full_width], full_width);
					}
				
					return true;
				}

				return false;
			}
			
			bool decode_uncompressed(std::vector<unsigned char>& temp_buffer, TgaHeader& header)
			{
				unsigned int offset = 0;

				PIXEL* pixels = reinterpret_cast<PIXEL*>(this->bits);
				unsigned int bytes_per_pixel = (header.depth / 8);

				for (int i = 0; i < header.height; i++)
				{
					for (int j = 0; j < header.width; j++)
					{
						PIXEL pixel;
						memset(&pixel, 0, sizeof(PIXEL));

						memcpy(&pixel, &temp_buffer[offset], bytes_per_pixel);
						offset += bytes_per_pixel;

						int n = (i * (this->pitch / sizeof(PIXEL))) + j;

						pixels[n].r = pixel.r;
						pixels[n].g = pixel.g;
						pixels[n].b = pixel.b;

						if (header.depth > 24)
							pixels[n].a = pixel.a;
					}
				}
				
				return true;
			}

			bool decode_compressed_rle(std::vector<unsigned char>& temp_buffer, TgaHeader& header)
			{
				unsigned int offset = 0;

				PIXEL* pixels = reinterpret_cast<PIXEL*>(this->bits);
				unsigned int bytes_per_pixel = (header.depth / 8);

				for (int n = 0; n < (header.width * header.height);)
				{
					unsigned char chunk_header = temp_buffer[offset++];
					unsigned char chunk_length = ((chunk_header & 0x7F) + 1);
					unsigned char chunk_encoded = (chunk_header & (1 << 7));

					PIXEL pixel;
					memset(&pixel, 0, sizeof(PIXEL));

					if (chunk_length > 0)
					{
						if (chunk_encoded) // RLE
						{
							memcpy(&pixel, &temp_buffer[offset], bytes_per_pixel);
							offset += bytes_per_pixel;
						}

						for (int i = 0; i < chunk_length; i++, n++)
						{
							if (!chunk_encoded)
							{
								memcpy(&pixel, &temp_buffer[offset], bytes_per_pixel);
								offset += bytes_per_pixel;
							}

							pixels[n].r = pixel.r;
							pixels[n].g = pixel.g;
							pixels[n].b = pixel.b;

							if (header.depth > 24)
								pixels[n].a = pixel.a;
						}
					}
				}

				return true;
			}

			bool create_image(int width_pixels, int height_pixels)
			{
				this->destroy_image();

				this->width = width_pixels;
				this->height = height_pixels;
				this->pitch = ((this->width * 32 + 31) & ~31) >> 3;
				this->hdc = CreateCompatibleDC(0);

				if (!this->hdc)
					return false;

				memset(&this->bitmap_info, 0, sizeof(BITMAPINFO));

				this->bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				this->bitmap_info.bmiHeader.biBitCount = 32;
				this->bitmap_info.bmiHeader.biWidth = this->width;
				this->bitmap_info.bmiHeader.biHeight = -this->height;
				this->bitmap_info.bmiHeader.biCompression = BI_RGB;
				this->bitmap_info.bmiHeader.biPlanes = 1;

				this->bitmap = CreateDIBSection(this->hdc, &this->bitmap_info, DIB_RGB_COLORS, 
					reinterpret_cast<void**>(&this->bits), 0, 0);

				if (!this->bitmap)
				{
					this->destroy_image();
					return false;
				}

				GdiFlush();
				return true;
			}

			void destroy_image()
			{
				if (this->bitmap)
				{
					DeleteObject(this->bitmap);
					this->bitmap = 0;
				}

				if (this->hdc)
				{
					DeleteDC(this->hdc);
					this->hdc = 0;
				}
				
				this->width = 0;
				this->height = 0;
				this->pitch = 0;
				this->bits = 0;
			}

		private:
			int width;
			int height;
			int pitch;
			HDC hdc;
			HBITMAP bitmap;
			BITMAPINFO bitmap_info;
			unsigned char* bits;
		};
	}
}