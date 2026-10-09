#ifndef TRACKPIPE_ERROR_HPP
#define TRACKPIPE_ERROR_HPP

#include <cstdint>
#include <string>
#include <string_view>

namespace trackpipe {

// Project-wide error type, returned through std::expected<T, Error>.
// Callers branch on `code`; `message` is for humans (logs, CLI output).
enum class ErrorCode : std::uint8_t {
  InvalidArgument,  // a parameter violates a precondition (e.g. stride too small)
  InvalidUri,       // a --source / --sink string could not be parsed
  Unsupported,      // valid request that this build or device cannot honour
  IoError,          // the OS refused (open, read, ioctl, socket...)
  DecodeError,      // the data arrived but could not be decoded
  EndOfStream,      // a finite source has no more frames
};

struct Error {
  ErrorCode code;
  std::string message;
};

[[nodiscard]] constexpr std::string_view to_string(ErrorCode code) noexcept {
  switch (code) {
    case ErrorCode::InvalidArgument:
      return "InvalidArgument";
    case ErrorCode::InvalidUri:
      return "InvalidUri";
    case ErrorCode::Unsupported:
      return "Unsupported";
    case ErrorCode::IoError:
      return "IoError";
    case ErrorCode::DecodeError:
      return "DecodeError";
    case ErrorCode::EndOfStream:
      return "EndOfStream";
  }
  return "Unknown";
}

}  // namespace trackpipe

#endif
