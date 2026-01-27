#include <hls_stream.h>
#include "config.h"

// 辅助函数：打包
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

// 进程 1：【直读 + 直发】(Naive Load & Send)
// 逻辑：完全不使用 Line Buffer，也不预读。
// 每次循环都老老实实从 DDR 读 3 个数 (Top, Mid, Bot) 并发走。
// 这完全模拟了你以前 Buffer=3 时的读取行为，只是合并到了一个端口。
static void load_and_send_naive(const data_t* dataIN, axis_pkt_stream& stencil_in, int N) {
    // 移除了 line_buf 定义
    
    for(int r = 0; r < N; r ++) {
        for (int i = 0; i < IN_ROW; i++) {
            #pragma HLS PIPELINE II=1
            
            // 【修改核心】：直接从 DDR 计算地址并读取 3 行数据
            // 虽然浪费带宽，但对于 N=1，它不需要预热，启动最快
            data_t top = dataIN[(r + 0) * IN_ROW + i];
            data_t mid = dataIN[(r + 1) * IN_ROW + i];
            data_t bot = dataIN[(r + 2) * IN_ROW + i]; 

            // 串行发送：AIE 依然需要按 Top -> Mid -> Bot 顺序接收
            stencil_in.write(make_pkt(top, false));
            stencil_in.write(make_pkt(mid, false));
            
            // 标记一行的结束
            bool last = (i == IN_ROW - 1);
            stencil_in.write(make_pkt(bot, last));
        }
    }
}

// 进程 2：【直收 + 直写】(Receive & Store)
// 只要 AIE 有屁放，立马接住写回 DDR，防止死锁
static void recv_and_store(data_t* dataOut, axis_pkt_stream& stencil_out, int N) {
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
// 接口配置保持不变
#pragma HLS INTERFACE m_axi     port=dataIn  offset=slave bundle=gmem0 depth=16384
#pragma HLS INTERFACE m_axi     port=dataOut offset=slave bundle=gmem1 depth=16384
#pragma HLS INTERFACE s_axilite port=dataIn  bundle=control
#pragma HLS INTERFACE s_axilite port=dataOut bundle=control
#pragma HLS INTERFACE s_axilite port=N       bundle=control
#pragma HLS INTERFACE s_axilite port=return  bundle=control

#pragma HLS INTERFACE axis port=stencil_in
#pragma HLS INTERFACE axis port=stencil_out

    // 使用 DATAFLOW 确保发送和接收是并行的
#pragma HLS DATAFLOW
    load_and_send_naive(dataIn, stencil_in, N);
    recv_and_store(dataOut, stencil_out, N);
}