#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

namespace cursor {
	using Pixel = std::uint32_t; // BGRA (LSB to MSB)
	struct Dimensions {
		std::size_t width, height;
	};
	struct Image {
		Dimensions dimensions;
		Pixel* buffer; // row-major, from bottom to top, then from left to right
	};
	struct Options {
		std::size_t hotspot_width, hotspot_height;
	};
	template <typename T>
	struct owning_span {
	private:
		T* _ptr;
		std::size_t _size;
	public:
		owning_span() noexcept :_ptr(nullptr), _size(0) {}
		owning_span(std::size_t size) :_ptr(new T[size]), _size(size) {}
		owning_span(const owning_span& other) = delete;
		owning_span& operator=(const owning_span& other) = delete;
		owning_span(owning_span&& other) noexcept :_ptr(std::exchange(other._ptr, nullptr)), _size(std::exchange(other._size, 0)) {}
		owning_span& operator=(owning_span&& other) noexcept {
			if (_ptr != nullptr) {
				delete[] _ptr;
			}
			_ptr = std::exchange(other._ptr, nullptr);
			_size = std::exchange(other._size, 0);
		}
		~owning_span() {
			if (_ptr != nullptr) {
				delete[] _ptr;
			}
		}
		T* get() noexcept {
			return _ptr;
		}
		const T* get() const noexcept {
			return _ptr;
		}
		std::size_t size() const noexcept {
			return _size;
		}
	};

	
	// and_mask[i] can be nullptr if no AND-mask is supplied. and_mask can be nullptr if all images don't want AND-masks.
	owning_span<std::byte> CompileCursor(std::size_t num_images, const Image* images, const Options* options, const bool* const* and_masks, const char*& error_out) noexcept;
}