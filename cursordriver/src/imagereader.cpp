#include "cursordriver/cursordriver.hpp"

#include <algorithm>
#include <csetjmp>
#include <utility>

#include <png.h>

namespace cursor {
	namespace {
		struct error_handler_args_t {
			std::jmp_buf jmp_buf;
			const char** out;
		};
		void png_user_error_fn(png_structp png_ptr, png_const_charp error_msg) noexcept {
			error_handler_args_t* error_handler_args = static_cast<error_handler_args_t*>(png_get_error_ptr(png_ptr));
			*(error_handler_args->out) = error_msg;
			std::longjmp(error_handler_args->jmp_buf, 1);
		}
		struct read_data_args_t {
			const unsigned char* curr_ptr;
			std::size_t remaining_size;
		};
		void png_user_read_data(png_structp png_ptr, png_bytep data, std::size_t length) noexcept {
			read_data_args_t* read_data_args = static_cast<read_data_args_t*>(png_get_io_ptr(png_ptr));
			if (read_data_args->remaining_size < length) {
				png_error(png_ptr, "PNG file ended unexpectedly");
				return;
			}
			const unsigned char* const next_ptr = read_data_args->curr_ptr + length;
			std::copy(read_data_args->curr_ptr, next_ptr, data);
			read_data_args->curr_ptr = next_ptr;
			read_data_args->remaining_size -= length;
		}

		// Shields the indeterminate effects of setjmp from the caller.  The caller can then call delete/free with utmost confidence that the pointer isn't corrupted.
		template <typename F, typename... T>
		decltype(auto) indeterminate_value_shield(F f, T&&... t) noexcept {
			return std::move(f)(std::forward<T>(t)...);
		}
	}
	Image ReadImage(const unsigned char* buf, std::size_t size, const char*& error_out) noexcept {
		if (png_sig_cmp(buf, 0, size) == 0) {
			// buf is a PNG file

			// png_struct
			png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
			png_infop info_ptr = nullptr;

			// output image
			Image out_img;

			if (png_ptr) {
				indeterminate_value_shield([&]() noexcept {
					// setup error handling
					error_handler_args_t error_handler_args;
					error_handler_args.out = &error_out;
					if (setjmp(error_handler_args.jmp_buf) != 0) {
						return;
					}
					png_set_error_fn(png_ptr, static_cast<void*>(&error_handler_args), &png_user_error_fn, nullptr);

					// png_info
					info_ptr = png_create_info_struct(png_ptr);
					if (!info_ptr) {
						error_out = "Cannot create png_info";
						return;
					}

					// set the custom reader to read from buffer
					read_data_args_t read_data_args;
					read_data_args.curr_ptr = buf;
					read_data_args.remaining_size = size;
					png_set_read_fn(png_ptr, static_cast<void*>(&read_data_args), &png_user_read_data);

					// set some settings (PNG_ALPHA_PNG is the default)
					//png_set_alpha_mode(png_ptr, PNG_ALPHA_PNG,)

					// read the whole png file
					png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_SCALE_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_GRAY_TO_RGB | PNG_TRANSFORM_BGR, nullptr);
					// pixels will come out as BGRA (smallest index to largest index)

					png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
					png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
					std::size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
					if (width * 4 != rowbytes) {
						error_out = "PNG file has unsupported encoding";
						return;
					}

					// allocate memory for returned image
					png_bytepp row_ptrs = png_get_rows(png_ptr, info_ptr);
					out_img.dimensions.width = width;
					out_img.dimensions.height = height;
					out_img.buffer = new Pixel[width * height];

					// copy the image data over
					// have to flip rows, because rows are laid from top to bottom in PNG, but from bottom to top in Image
					Pixel* out_ptr = out_img.buffer;
					for (png_uint_32 i = height - 1; i != std::numeric_limits<png_uint_32>::max(); --i) {
						std::copy(row_ptrs[i], row_ptrs[i] + rowbytes, reinterpret_cast<unsigned char*>(out_ptr));
						out_ptr += width;
					}

					});

				png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

				return out_img;
			}
			else {
				error_out = "Cannot create png_struct";
				return out_img;
			}
		}
		else {
			// unrecognized image format
			error_out = "Unsupported image format";
			return Image();
		}
	}

	void FreeImage(Image img) noexcept {
		delete[] img.buffer;
	}
}