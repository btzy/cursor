#include "cursor/cursor.hpp"

#include <iterator>

#include "bytewriter.hpp"

namespace cursor {

	namespace {
		// pixels and and_masks are in row-major order from bottom to top
		// dimensions[0] to dimensions[num_images-1], and options[0] to options[num_images-1] must be valid
		// returns 0 if there is an error
		std::size_t CalculateCursorSize(std::size_t num_images, const Image* images, const Options* options, const bool* const* and_masks, const char*& error_out) noexcept {
			if (num_images == 0) {
				error_out = "num_images must be non-zero";
				return 0;
			}
			if (num_images >= (1u << 16)) {
				error_out = "num_images must fit within 2-byte unsigned int";
				return 0;
			}
			std::size_t total_size = 6 /* ICONDIR */ + 2 /* padding */;
			for (std::size_t i = 0; i != num_images; ++i) {
				if (images[i].dimensions.width == 0 || images[i].dimensions.width > (1u << 8) || images[i].dimensions.height == 0 || images[i].dimensions.height > (1u << 8)) {
					error_out = "Each image must have height and width between 1 and 256 pixels inclusive";
					return 0;
				}
				// and_mask_byte_count is the number of bytes needed per row by the and_mask.
				// Each pixel needs one bit, and each row must be padded to a multiple of 4 bytes.
				std::size_t and_mask_byte_count = (and_masks == nullptr || and_masks[i] == nullptr) ? 0 : ((images[i].dimensions.width / 8 + 3) / 4 * 4);
				total_size += 16 /* ICONDIRENTRY */ + 40 /* DIB header */ + (images[i].dimensions.width * 4 + and_mask_byte_count) /* 4 bytes per pixel */ * images[i].dimensions.height;
			}
			return total_size;
		}
	}

	owning_span<std::byte> CompileCursor(std::size_t num_images, const Image* images, const Options* options, const bool* const* and_masks, const char*& error_out) noexcept {

		std::size_t cursor_size = CalculateCursorSize(num_images, images, options, and_masks, error_out);
		if (cursor_size == 0)return owning_span<std::byte>();

		owning_span<std::byte> out_span(cursor_size);

		ByteWriter writer(out_span.get());
		
		/*
		ICONDIR structure (6 bytes):
		2 bytes - Reserved, must be 0.
		2 bytes - Cursor image, must be 2.
		2 bytes - Number of images in file.
		*/
		writer.uint16(0);
		writer.uint16(2);
		writer.uint16(static_cast<std::uint16_t>(num_images));

		std::uint32_t image_offset = 6 /* ICONDIR */ + 2 /* padding */ + 16 /* ICONDIRENTRY */ * static_cast<std::uint32_t>(num_images);

		for (std::size_t i = 0; i != num_images; ++i) {
			/*
			ICONDIRENTRY structure (16 bytes):
			1 byte - Image width in pixels. Value 0 means image width is 256.
			1 byte - Image height in pixels. Value 0 means image height is 256.
			1 byte - Colour palette, set to 0 for us.
			1 byte - Reserved, must be 0.
			2 bytes - Hotspot x coordinate.
			2 bytes - Hotspot y coordinate.
			4 bytes - Size of image data in bytes.
			4 bytes - Offset of BMP data from beginning of ICO file.
			*/
			const Dimensions& dimension = images[i].dimensions;
			const Options& option = options[i];
			const bool* and_mask = (and_masks == nullptr) ? nullptr : and_masks[i];
			writer.uint8(dimension.width == (1u << 8) ? 0 : static_cast<uint8_t>(dimension.width));
			writer.uint8(dimension.height == (1u << 8) ? 0 : static_cast<uint8_t>(dimension.height));
			writer.uint8(0);
			writer.uint8(0);
			writer.uint16(static_cast<uint16_t>(option.hotspot_width));
			writer.uint16(static_cast<uint16_t>(option.hotspot_height));
			// and_mask_byte_count: see CalculateCursorSize() for explanation.
			std::size_t and_mask_byte_count = (and_mask == nullptr) ? 0 : ((dimension.width / 8 + 3) / 4 * 4);
			std::uint32_t bmp_byte_count = static_cast<std::uint32_t>(40 /* DIB header */ + (dimension.width * 4 + and_mask_byte_count) /* 4 bytes per pixel */ * dimension.height);
			writer.uint32(bmp_byte_count);
			writer.uint32(image_offset);
			image_offset += bmp_byte_count;
		}

		/* 2-byte padding so that the rest of the file is aligned to 4-byte boundary */
		writer.uint16(0);

		for (std::size_t i = 0; i != num_images; ++i) {
			/*
			BITMAPINFOHEADER structure (40 bytes):
			4 bytes - Size of this header, must be 40 for us.
			4 bytes - Image width in pixels.
			4 bytes - Twice of the image height in pixels.  Regardless of whether and_mask is present, this has to be twice of the real image height.
			2 bytes - Number of colour planes, 1 for us.
			2 bytes - Bits per pixel, 32 for us.
			4 bytes - Compression method, 0 (none) for us.
			4 bytes - Image size in bytes (including and_mask if present).
			4 bytes - Horizontal resolution, 0 for us.
			4 bytes - Vertical resolution, 0 for us.
			4 bytes - Number of colours in the colour palette, 0 for us.
			4 bytes - Number of important colours used, 0 for us.
			*/
			const Dimensions& dimension = images[i].dimensions;
			const Options& option = options[i];
			const Pixel* pixel = images[i].buffer;
			const bool* and_mask = (and_masks == nullptr) ? nullptr : and_masks[i];
			writer.uint32(40);
			writer.uint32(dimension.width);
			writer.uint32(dimension.height * 2);
			writer.uint16(1);
			writer.uint16(32);
			writer.uint32(0);
			// and_mask_byte_count: see CalculateCursorSize() for explanation.
			std::size_t and_mask_byte_count = (and_mask == nullptr) ? 0 : ((dimension.width / 8 + 3) / 4 * 4);
			std::uint32_t image_byte_count = static_cast<std::uint32_t>((dimension.width * 4 + and_mask_byte_count) /* 4 bytes per pixel */ * dimension.height);
			writer.uint32(image_byte_count);
			writer.uint32(0);
			writer.uint32(0);
			writer.uint32(0);
			writer.uint32(0);

			/* XOR mask (4 bytes per pixel) */
			writer.uint32(pixel, pixel + dimension.width * dimension.height);

			if (and_mask != nullptr) {
				/* AND mask (1 bit per pixel) */
				const bool* and_mask_ptr = and_mask;
				for (std::size_t i = 0; i != dimension.height; ++i) {
					std::size_t remaining_width = dimension.width;
					const std::byte* byte_start = writer.save();
					for (; remaining_width >= 8; remaining_width -= 8) {
						std::byte byte = std::byte(0);
						for (std::size_t i = 0; i != 8; ++i) {
							if (*and_mask_ptr++) {
								byte |= std::byte(1u << i);
							}
						}
						writer.byte(byte);
					}
					std::byte byte = std::byte(0);
					for (std::size_t i = 0; i != remaining_width; ++i) {
						if (*and_mask_ptr++) {
							byte |= std::byte(1u << i);
						}
					}
					writer.byte(byte);
					// Add padding to multiple of 4 bytes
					while (std::distance(byte_start, writer.save()) % 4 != 0) {
						writer.byte(std::byte(0));
					}
				}
			}
		}

		return out_span;
	}
}