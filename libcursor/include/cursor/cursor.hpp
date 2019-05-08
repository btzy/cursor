#pragma once

#include <cstddef>
#include <cstdint>

namespace cursor {
	using Pixel = std::uint32_t; // RGBA (LSB to MSB)
	struct Dimensions {
		std::size_t width, height;
	};
	struct Options {
		std::size_t hotspot_width, hotspot_height;
	};

	// pixels and and_masks are in row-major order from bottom to top
	// dimensions[0] to dimensions[num_images-1], and options[0] to options[num_images-1] must be valid
	// returns 0 if there is an error
	std::size_t CalculateCursorSize(std::size_t num_images, const Dimensions* dimensions, const Options* options, const Pixel* const* pixels, const bool* const* and_masks, const char*& error_out) noexcept;
	// and_mask[i] can be nullptr if no AND-mask is supplied. and_mask can be nullptr if all images don't want AND-masks.
	void CompileCursor(std::size_t num_images, const Dimensions* dimensions, const Options* options, const Pixel* const* pixels, const bool* const* and_masks, std::byte* out, const char*& error_out) noexcept;
}