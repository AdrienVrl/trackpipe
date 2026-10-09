#ifndef TRACKPIPE_FRAME_H
#define TRACKPIPE_FRAME_H
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <vector>

#include "trackpipe/error.hpp"
#include "trackpipe/pixelformat.hpp"

namespace trackpipe {
class Frame {
 private:
  int width_;
  int height_;
  int stride_;
  PixelFormat pixel_format_;
  std::uint64_t sequence_number_;
  std::chrono::steady_clock::time_point timestamp_;
  std::vector<std::byte> buffer_;

 public:
  // Destructor
  ~Frame() = default;

  // Copy constructor
  Frame(const Frame& other) = delete;

  // Move constructor
  Frame(Frame&& other) noexcept = default;

  // Copy assignment operator
  Frame& operator=(const Frame& other) = delete;

  // Move assignment operator
  Frame& operator=(Frame&& other) noexcept = default;

  [[nodiscard]] static std::expected<Frame, Error> create(
      int width, int height, int stride, PixelFormat pixel_format,
      std::chrono::steady_clock::time_point timestamp, std::uint64_t sequence_number);

  [[nodiscard]] std::span<const std::byte> data() const noexcept;

  [[nodiscard]] std::span<std::byte> data() noexcept;

  [[nodiscard]] bool empty() const noexcept { return buffer_.empty(); }

  [[nodiscard]] int get_width() const noexcept { return width_; }

  [[nodiscard]] int get_height() const noexcept { return height_; }

  [[nodiscard]] int get_stride() const noexcept { return stride_; }

  [[nodiscard]] PixelFormat get_pixel_format() const noexcept { return pixel_format_; }

  [[nodiscard]] std::uint64_t get_sequence_number() const noexcept { return sequence_number_; }

  [[nodiscard]] std::chrono::steady_clock::time_point get_timestamp() const noexcept {
    return timestamp_;
  }

 private:
  // Constructor
  Frame(int width, int height, int stride, PixelFormat pixel_format,
        std::chrono::steady_clock::time_point timestamp, std::uint64_t sequence_number);
  void check_invariant() const;
};
}  // namespace trackpipe
#endif
