#ifndef TRACKPIPE_PIXEL_FORMAT_H
#define TRACKPIPE_PIXEL_FORMAT_H

#include <cstdint>
#include <stdexcept>
namespace trackpipe {
enum class PixelFormat : std::uint8_t { BGR8, RGB8, GRAY8, YUYV };

constexpr int bytes_per_pixel(PixelFormat pixel_format) {
  switch (pixel_format) {
    case PixelFormat::BGR8:
    case PixelFormat::RGB8:
      return 3;
    case PixelFormat::GRAY8:
      return 1;
    case PixelFormat::YUYV:
      return 2;
  }
  throw std::invalid_argument("Unknown pixel format");
}
}  // namespace trackpipe

#endif
