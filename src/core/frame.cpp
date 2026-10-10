#include "trackpipe/frame.hpp"

#include <cassert>
#include <chrono>
#include <expected>
#include <format>

#include "trackpipe/error.hpp"
#include "trackpipe/pixelformat.hpp"

namespace trackpipe {

Frame::Frame(int width, int height, int stride, PixelFormat pixel_format,
             std::chrono::steady_clock::time_point timestamp, std::uint64_t sequence_number)
    : width_(width),
      height_(height),
      stride_(stride),
      pixel_format_(pixel_format),
      sequence_number_(sequence_number),
      timestamp_(timestamp),
      buffer_(static_cast<std::size_t>(height) * static_cast<std::size_t>(stride)) {}

std::expected<Frame, Error> Frame::create(int width, int height, int stride,
                                          PixelFormat pixel_format,
                                          std::chrono::steady_clock::time_point timestamp,
                                          std::uint64_t sequence_number) {
  if (width <= 0 || height <= 0) {
    return std::unexpected(Error{ErrorCode::InvalidArgument,
                                 std::format("Invalid frame dimensions: {}x{}", width, height)});
  }

  if (stride <= 0) {
    return std::unexpected(
        Error{ErrorCode::InvalidArgument, std::format("Invalid frame stride: {}", stride)});
  }

  if (static_cast<std::size_t>(stride) <
      static_cast<std::size_t>(width) * static_cast<std::size_t>(bytes_per_pixel(pixel_format))) {
    return std::unexpected(Error{ErrorCode::InvalidArgument,
                                 std::format("Incompatible width, stride and pixel format: stride "
                                             "{} < width {} * bytes_per_pixel({})",
                                             stride, width, bytes_per_pixel(pixel_format))});
  }

  Frame frame(width, height, stride, pixel_format, timestamp, sequence_number);
  frame.check_invariant();
  return frame;
}

std::span<const std::byte> Frame::data() const noexcept { return buffer_; }

std::span<std::byte> Frame::data() noexcept { return buffer_; }

void Frame::check_invariant() const {
  assert(empty() ||
         (static_cast<std::size_t>(stride_) >=
              static_cast<std::size_t>(width_) *
                  static_cast<std::size_t>(bytes_per_pixel(pixel_format_)) &&
          buffer_.size() >= static_cast<std::size_t>(stride_) * static_cast<std::size_t>(height_)));
}

}  // namespace trackpipe
