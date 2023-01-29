// Copyright 2017 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "turbo/random/internal/randen_slow.h"

#include <cstring>

#include "gtest/gtest.h"
#include "turbo/base/endian.h"
#include "turbo/random/internal/randen_traits.h"

namespace {

using turbo::random_internal::RandenSlow;
using turbo::random_internal::RandenTraits;

TEST(RandenSlowTest, Default) {
  constexpr uint8_t kGolden[] = {
      0xee, 0xd3, 0xe6, 0x0e, 0x09, 0x34, 0x65, 0x6c, 0xc6, 0x33, 0x53, 0x9d,
      0x9b, 0x2b, 0x4e, 0x04, 0x77, 0x39, 0x43, 0x4e, 0x13, 0x4f, 0xc1, 0xc3,
      0xee, 0x10, 0x04, 0xd9, 0x7c, 0xf4, 0xa9, 0xdd, 0x10, 0xca, 0xd8, 0x7f,
      0x08, 0xf3, 0x7b, 0x88, 0x12, 0x29, 0xc7, 0x45, 0xf5, 0x80, 0xb7, 0xf0,
      0x9f, 0x59, 0x96, 0x76, 0xd3, 0xb1, 0xdb, 0x15, 0x59, 0x6d, 0x3c, 0xff,
      0xba, 0x63, 0xec, 0x30, 0xa6, 0x20, 0x7f, 0x6f, 0x60, 0x73, 0x9f, 0xb2,
      0x4c, 0xa5, 0x49, 0x6f, 0x31, 0x8a, 0x80, 0x02, 0x0e, 0xe5, 0xc8, 0xd5,
      0xf9, 0xea, 0x8f, 0x3b, 0x8a, 0xde, 0xd9, 0x3f, 0x5e, 0x60, 0xbf, 0x9c,
      0xbb, 0x3b, 0x18, 0x78, 0x1a, 0xae, 0x70, 0xc9, 0xd5, 0x1e, 0x30, 0x56,
      0xd3, 0xff, 0xb2, 0xd8, 0x37, 0x3c, 0xc7, 0x0f, 0xfe, 0x27, 0xb3, 0xf4,
      0x19, 0x9a, 0x8f, 0xeb, 0x76, 0x8d, 0xfd, 0xcd, 0x9d, 0x0c, 0x42, 0x91,
      0xeb, 0x06, 0xa5, 0xc3, 0x56, 0x95, 0xff, 0x3e, 0xdd, 0x05, 0xaf, 0xd5,
      0xa1, 0xc4, 0x83, 0x8f, 0xb7, 0x1b, 0xdb, 0x48, 0x8c, 0xfe, 0x6b, 0x0d,
      0x0e, 0x92, 0x23, 0x70, 0x42, 0x6d, 0x95, 0x34, 0x58, 0x57, 0xd3, 0x58,
      0x40, 0xb8, 0x87, 0x6b, 0xc2, 0xf4, 0x1e, 0xed, 0xf3, 0x2d, 0x0b, 0x3e,
      0xa2, 0x32, 0xef, 0x8e, 0xfc, 0x54, 0x11, 0x43, 0xf3, 0xab, 0x7c, 0x49,
      0x8b, 0x9a, 0x02, 0x70, 0x05, 0x37, 0x24, 0x4e, 0xea, 0xe5, 0x90, 0xf0,
      0x49, 0x57, 0x8b, 0xd8, 0x2f, 0x69, 0x70, 0xa9, 0x82, 0xa5, 0x51, 0xc6,
      0xf5, 0x42, 0x63, 0xbb, 0x2c, 0xec, 0xfc, 0x78, 0xdb, 0x55, 0x2f, 0x61,
      0x45, 0xb7, 0x3c, 0x46, 0xe3, 0xaf, 0x16, 0x18, 0xad, 0xe4, 0x2e, 0x35,
      0x7e, 0xda, 0x01, 0xc1, 0x74, 0xf3, 0x6f, 0x02, 0x51, 0xe8, 0x3d, 0x1c,
      0x82, 0xf0, 0x1e, 0x81,
  };

  alignas(16) uint8_t state[RandenTraits::kStateBytes];
  std::memset(state, 0, sizeof(state));

  RandenSlow::Generate(RandenSlow::GetKeys(), state);
  EXPECT_EQ(0, std::memcmp(state, kGolden, sizeof(state)));
}

}  // namespace
