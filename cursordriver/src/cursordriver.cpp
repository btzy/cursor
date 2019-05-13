#include "cursordriver/cursordriver.hpp"

#include <algorithm>
#include <csetjmp>
#include <utility>

#include <png.h>

namespace cursor {
	namespace {
		
	}
	owning_span<std::byte> MakeCursor(std::size_t num_images, const unsigned char* const* bufs, const std::size_t* sizes, const Options* options, const char*& error_out) noexcept {
		error_out = nullptr;
		Image* images = new Image[num_images];
		for (std::size_t i = 0; i != num_images; ++i) {
			images[i] = ReadImage(bufs[i], sizes[i], error_out);
			if (error_out) return owning_span<std::byte>();
		}
		return CompileCursor(num_images, images, options, nullptr, error_out);
	}
}