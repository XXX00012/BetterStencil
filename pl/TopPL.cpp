#include <hls_stream.h>
#include "config.h"

static axis_pkt make_pkt(data_t v, bool last) {
#pragma HLS inline 
    axis_pkt pkt;
    fp_uint conv;
    conv.data_float = v;
    pkt.data = (ap_uint<W>)conv.data_uint;
    pkt.keep = 0xF;
    pkt.strb = 0xF;
    pkt.last = last ? 1 : 0;
    return pkt;
}


static void load_send(const data_t* dataIN, axis_pkt_stream& stencil_in, int N) {
    
    for(int r = 0; r < N; r ++) {
        for (int i = 0; i < IN_ROW; i++) {
            #pragma HLS PIPELINE II=1

            data_t top = dataIN[(r + 0) * IN_ROW + i];
            data_t mid = dataIN[(r + 1) * IN_ROW + i];
            data_t bot = dataIN[(r + 2) * IN_ROW + i]; 

            stencil_in.write(make_pkt(top, false));
            stencil_in.write(make_pkt(mid, false));
            
            bool last = (i == IN_ROW - 1);
            stencil_in.write(make_pkt(bot, last));
        }
    }
}

static void recv_store(data_t* dataOut, axis_pkt_stream& stencil_out, int N) {
    for (int r = 0; r < N; r++) {
        for (int i = 0; i < ROW; i++) {
            #pragma HLS PIPELINE II=1
            axis_pkt pkt = stencil_out.read();
            fp_uint conv;
            conv.data_uint = (unsigned int)pkt.data;
            dataOut[r * ROW + i] = conv.data_float;
        }
    }
}

void TopPL(const data_t* dataIn,
           data_t* dataOut,
           const int N,
           axis_pkt_stream& stencil_in,
           axis_pkt_stream& stencil_out) {
#pragma HLS INTERFACE m_axi     port=dataIn  offset=slave bundle=gmem0 depth=16384
#pragma HLS INTERFACE m_axi     port=dataOut offset=slave bundle=gmem0 depth=16384
#pragma HLS INTERFACE s_axilite port=dataIn  bundle=control
#pragma HLS INTERFACE s_axilite port=dataOut bundle=control
#pragma HLS INTERFACE s_axilite port=N       bundle=control
#pragma HLS INTERFACE s_axilite port=return  bundle=control

#pragma HLS INTERFACE axis port=stencil_in
#pragma HLS INTERFACE axis port=stencil_out


#pragma HLS DATAFLOW
    load_send(dataIn, stencil_in, N);
    recv_store(dataOut, stencil_out, N);
}