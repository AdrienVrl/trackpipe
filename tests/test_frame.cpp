#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "trackpipe/error.hpp"
#include "trackpipe/frame.hpp"

namespace {

using trackpipe::ErrorCode;
using trackpipe::Frame;
using trackpipe::PixelFormat;

constexpr int default_width = 640;
constexpr int default_height = 480;
constexpr int default_stride = default_width * 3;  // tightly packed RGB8

// Valid frame for tests that are not about validation; .value() throws
// (and fails the test) if create() unexpectedly returns an error.
Frame make_frame(std::uint64_t sequence_number = 1) {
  return Frame::create(default_width, default_height, default_stride, PixelFormat::RGB8,
                       std::chrono::steady_clock::now(), sequence_number)
      .value();
}

// Asserts that create() rejects the parameters with InvalidArgument.
void expect_invalid_argument(int width, int height, int stride, PixelFormat format) {
  auto result = Frame::create(width, height, stride, format, std::chrono::steady_clock::now(), 1);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, ErrorCode::InvalidArgument)
      << "got " << trackpipe::to_string(result.error().code);
}

}  // namespace

// --- Compile-time guarantees ------------------------------------------------

static_assert(!std::is_copy_constructible_v<Frame>);
static_assert(!std::is_copy_assignable_v<Frame>);
static_assert(std::is_nothrow_move_constructible_v<Frame> &&
              std::is_nothrow_move_assignable_v<Frame>);

static_assert(trackpipe::bytes_per_pixel(PixelFormat::BGR8) == 3);
static_assert(trackpipe::bytes_per_pixel(PixelFormat::RGB8) == 3);
static_assert(trackpipe::bytes_per_pixel(PixelFormat::GRAY8) == 1);
static_assert(trackpipe::bytes_per_pixel(PixelFormat::YUYV) == 2);

// --- create(): valid input --------------------------------------------------

TEST(FrameTest, createStoresMetadata) {
  const auto timestamp = std::chrono::steady_clock::now();
  auto result = Frame::create(default_width, default_height, default_stride, PixelFormat::RGB8,
                              timestamp, 42);
  ASSERT_TRUE(result.has_value());

  const Frame& frame = *result;
  EXPECT_EQ(frame.get_width(), default_width);
  EXPECT_EQ(frame.get_height(), default_height);
  EXPECT_EQ(frame.get_stride(), default_stride);
  EXPECT_EQ(frame.get_pixel_format(), PixelFormat::RGB8);
  EXPECT_EQ(frame.get_timestamp(), timestamp);
  EXPECT_EQ(frame.get_sequence_number(), 42U);
}

TEST(FrameTest, createAllocatesStrideTimesHeight) {
  const Frame frame = make_frame();
  EXPECT_FALSE(frame.empty());
  EXPECT_EQ(frame.data().size(),
            static_cast<std::size_t>(default_stride) * static_cast<std::size_t>(default_height));
}

TEST(FrameTest, createAcceptsStrideEqualToRowSize) {
  // Boundary: stride == width * bytes_per_pixel is the tightest valid layout.
  auto result = Frame::create(default_width, default_height, default_width * 2, PixelFormat::YUYV,
                              std::chrono::steady_clock::now(), 1);
  EXPECT_TRUE(result.has_value());
}

TEST(FrameTest, createAcceptsPaddedStride) {
  // Row padding (stride > row size) is common for aligned camera buffers.
  auto result = Frame::create(default_width, default_height, default_width + 64, PixelFormat::GRAY8,
                              std::chrono::steady_clock::now(), 1);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->data().size(), static_cast<std::size_t>(default_width + 64) *
                                       static_cast<std::size_t>(default_height));
}

// --- create(): invalid input returns an error, never throws -----------------

TEST(FrameTest, createRejectsStrideSmallerThanRow) {
  expect_invalid_argument(default_width, default_height, default_width * 3 - 1, PixelFormat::RGB8);
}

TEST(FrameTest, createRejectsNonPositiveWidth) {
  expect_invalid_argument(0, default_height, default_stride, PixelFormat::RGB8);
  expect_invalid_argument(-1, default_height, default_stride, PixelFormat::RGB8);
}

TEST(FrameTest, createRejectsNonPositiveHeight) {
  // A negative height must be rejected before any allocation is attempted.
  expect_invalid_argument(default_width, 0, default_stride, PixelFormat::RGB8);
  expect_invalid_argument(default_width, -480, default_stride, PixelFormat::RGB8);
}

TEST(FrameTest, createRejectsNonPositiveStride) {
  expect_invalid_argument(default_width, default_height, 0, PixelFormat::RGB8);
  expect_invalid_argument(default_width, default_height, -default_stride, PixelFormat::RGB8);
}

TEST(FrameTest, createDoesNotThrow) {
  EXPECT_NO_THROW({
    auto result = Frame::create(-1, -1, -1, PixelFormat::RGB8, std::chrono::steady_clock::now(), 1);
    (void)result;
  });
}

// --- Move semantics ---------------------------------------------------------

TEST(FrameTest, moveConstructorTransfersBuffer) {
  Frame frame1 = make_frame(1);
  const std::byte* buffer_before_move = frame1.data().data();

  Frame frame2(std::move(frame1));

  EXPECT_TRUE(frame1.empty());  // NOLINT(bugprone-use-after-move)
  EXPECT_FALSE(frame2.empty());
  EXPECT_EQ(frame2.data().data(), buffer_before_move);  // same block: nothing was copied
}

TEST(FrameTest, moveAssignmentTransfersBuffer) {
  Frame frame1 = make_frame(1);
  Frame frame2 = make_frame(2);
  const std::byte* buffer_before_move = frame1.data().data();

  frame2 = std::move(frame1);

  EXPECT_TRUE(frame1.empty());  // NOLINT(bugprone-use-after-move)
  EXPECT_FALSE(frame2.empty());
  EXPECT_EQ(frame2.data().data(), buffer_before_move);
  EXPECT_EQ(frame2.get_sequence_number(), 1U);
}
