#include <gtest/gtest.h>

#include "trackpipe/version.hpp"

TEST(Version, IsNotEmpty) { EXPECT_FALSE(trackpipe::version().empty()); }
