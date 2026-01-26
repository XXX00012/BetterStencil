#pragma once
#include <adf.h>
#include "aie_api/aie.hpp"
#include <aie_api/aie_adf.hpp>
#include "aie_api/utils.hpp"
#include "../Config.h"

void jacobi_2d3_f32(
    adf::input_buffer<float,adf::extents<3*IN_ROW>>& __restrict in0,
    adf::output_buffer<float,adf::extents<ROW>>& __restrict out0
);
