#pragma once

#include "../SystemConfig.h"
#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <hls_print.h>

const int W = 32;
const int DDR_WIDTH = 128;

const int ELEM_NUM     = W / 32;
const int DDR_ELEM_NUM = DDR_WIDTH / 32;

typedef ap_axiu<W, 0, 0, 0> axis_pkt;
typedef hls::stream<axis_pkt> axis_pkt_stream;

typedef hls::stream<ap_uint<1>, 4> axis_stream_sig;
typedef hls::stream<float, 4>      axis_stream_float;

const int START = 1;
const int END   = 0;

typedef union {
  float        data_float;
  unsigned int data_uint;
} fp_uint;
