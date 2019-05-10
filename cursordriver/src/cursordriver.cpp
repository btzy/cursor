#include "cursordriver/cursordriver.hpp"

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

		// Shields the indeterminate effects of setjmp from the caller.  The caller can then call delete/free with utmost confidence that the pointer isn't corrupted.
		template <typename F, typename... T>
		decltype(auto) indeterminate_value_shield(F f, T&&... t) noexcept {
			return std::move(f)(std::forward<T>(t)...);
		}
	}
	void GetImageDimensions(const unsigned char* buf, std::size_t size, const char*& error_out) noexcept {
		if (png_sig_cmp(buf, 0, size) == 0) {
			// buf is a PNG file

			png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

			indeterminate_value_shield([&]() noexcept {
				// error handling
				error_handler_args_t error_handler_args;
				error_handler_args.out = &error_out;
				if (setjmp(error_handler_args.jmp_buf) != 0) {
					return;
				}
				png_set_error_fn(png_ptr, static_cast<void*>(&error_handler_args), &png_user_error_fn, nullptr);
				});
			
			png_destroy_read_struct(&png_ptr, nullptr, nullptr);
		}
	}
}