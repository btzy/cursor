#pragma once

#include <cstddef>
#include <cstdint>

#include <cursor/cursor.hpp>

namespace cursor {

	/* Get dimensions of image file (as a byte array).  It will automatically detect the image file format. */
	void GetImageDimensions(std::byte* buf, std::size_t size) noexcept;
}