#pragma once
#include <cstddef>

using data_t = float;

constexpr int ROW   = 16;
constexpr int HALO  = 1;
constexpr int IN_ROW = ROW + 2 * HALO;   // 18
constexpr int w     = 8;

constexpr int IN_BYTES  = IN_ROW * sizeof(data_t);  // 72
constexpr int OUT_BYTES = ROW    * sizeof(data_t);  // 64

static_assert(ROW % w == 0, "ROW must be multiple of w");
