#include <iostream>

#include "trackpipe/version.hpp"

int main() {
  // <print> needs GCC 14; GCC 13 (Ubuntu 24.04 default) has <format> but not std::println.
  std::cout << "trackpipe " << trackpipe::version() << '\n';
  return 0;
}
