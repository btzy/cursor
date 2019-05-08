#pragma once

#include <cstddef>
#include <cstdint>

namespace cursor {
	/*
	Little-endian byte writer
	*/
	class ByteWriter {
	private:
		std::byte* ptr;
	public:
		ByteWriter(std::byte* ptr) noexcept :ptr(ptr) {};
		const std::byte* save() noexcept {
			return ptr;
		}
		void byte(std::byte val) noexcept {
			*ptr++ = val;
		}
		void uint8(std::uint8_t val) noexcept {
			*ptr++ = std::byte(val);
		}
		void uint16(std::uint16_t val) noexcept {
			*ptr++ = std::byte(val & 0xFF);
			*ptr++ = std::byte(val >> 8);
		}
		void uint32(std::uint32_t val) noexcept {
			*ptr++ = std::byte(val & 0xFF);
			*ptr++ = std::byte((val >> 8) & 0xFF);
			*ptr++ = std::byte((val >> 16) & 0xFF);
			*ptr++ = std::byte(val >> 24);
		}
		void uint32(const std::uint32_t* begin, const std::uint32_t* end) noexcept {
			for (auto it = begin; it != end; ++it) {
				uint32(*it);
			}
		}
	};
}