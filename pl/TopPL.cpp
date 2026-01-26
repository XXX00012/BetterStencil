#include <hls_stream.h>
#include "config.h"

static axis_pkt make_pkt(data_t v,bool last){
    #pragma HLS inline 
    axis_pkt pkt;

    fp_unit conv;
    conv.data_float = v;
    pkt.data = (ap_uint<W>)conv.data_uint;

    pkt.keep = 0xF;
    pkt.strb = 0xF;
    pkt.last = last;
    return pkt;
}

static void load(const data_t* dataIN,hls::stream<data_t>& fifo_in,int N){
    data_t line_buf[2][IN_ROW];
    #pragma HLS array_partition variable=line_buf complete dim=1

    for (int i = 0; i < IN_ROW; i++){
        #pragma HLS PIPELINE II=1
        line_buf[0][i] = dataIN[i];
        line_buf[1][i] = dataIN[i+IN_ROW];
        
    }
    for(int r = 0;r<N; r ++){
        for (int i = 0; i <IN_ROW; i++){
            #pragma HLS PIPELINE II = 1
            data_t top = line_buf[0][i];
            data_t mid = line_buf[1][i];
            data_t bot = dataIN[(r+2)*IN_ROW+i];
            fifo_in.write(top);
            fifo_in.write(mid);
            fifo_in.write(bot);
            line_buf[0][i] = mid;
            line_buf[1][i] = bot;   
        }
    }
}

static void compute(hls::stream<data_t>& fifo_in,hls::stream<data_t>& fifo_out,
                    axis_pkt_stream& stencil_in,axis_pkt_stream& stencil_out ,int N){
        for (int r = 0; r<N ;r ++){
            for (int i = 0; i < IN_ROW*3; i ++){
                #pragama HLS PIPELINE II = 1
                bool last = (i == IN_ROW*3-1);
                stencil_in.write(make_pkt(fifo_in.read(),last));

            }
            for (int i = 0; i < ROW; i ++){
                #pragma HLS PIPELINE II = 1
                axis_pkt pkt = stencil_out.read();
                fp_uint conv;
                conv.data_uint = (unsigned int)pkt.data;
                fifo_out.write(conv.data_float);
            }

        }        
}

static void store(data_t* dataOut, hls::stream<data_t>& fifo_out, int N) {
    for (int r = 0; r < N; r++) {
        for (int i = 0; i < ROW; i++) {
#pragma HLS PIPELINE II=1
            dataOut[r * ROW + i] = fifo_out.read();
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

    hls::stream<data_t> fifo_in("fifo_in");
    hls::stream<data_t> fifo_out("fifo_out");
    // 深度应设大一点，防止函数间握手造成停顿
#pragma HLS STREAM variable=fifo_in  depth=108 
#pragma HLS STREAM variable=fifo_out depth=32

#pragma HLS DATAFLOW
    load(dataIn, fifo_in, N);
    compute(fifo_in, fifo_out, stencil_in, stencil_out, N);
    store(dataOut, fifo_out, N);
}