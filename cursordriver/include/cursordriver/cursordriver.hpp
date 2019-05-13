#pragma once

#include <cstddef>
#include <cstdint>

#include <cursor/cursor.hpp>

namespace cursor {

	/* Get image file as a cursor::Image.  It will automatically detect the image file format.  If there is an error, then caller should not call FreeImage(). */
	Image ReadImage(const unsigned char* buf, std::size_t size, const char*& error_out) noexcept;

	/* Free image */
	void FreeImage(Image img) noexcept;

	owning_span<std::byte> MakeCursor(std::size_t num_images, const unsigned char* const* bufs, const std::size_t* sizes, const Options* options, const char*& error_out) noexcept;
}