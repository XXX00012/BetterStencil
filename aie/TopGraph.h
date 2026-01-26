#pragma once
#include <adf.h>
#include <string>
#include "ProcessGraph/StencilCoreGraph.h"

using namespace adf;

template<int N>
class TopStencilGraph : public graph {
public:
    StencilCoreGraph<N> core;

    input_plio  in_plio[3];
    output_plio out_plio[N];

    TopStencilGraph(const std::string& graphID) {
        // 绝对路径，后面文件名拼在这后面
        const std::string base =
            "/home/xianyijiang/large/SVD/templates/data/";

        // 3 路输入
        in_plio[0] = input_plio::create(
            graphID + "_in0",
            plio_32_bits,
            base + graphID + "_in0.txt"
        );

        in_plio[1] = input_plio::create(
            graphID + "_in1",
            plio_32_bits,
            base + graphID + "_in1.txt"
        );

        in_plio[2] = input_plio::create(
            graphID + "_in2",
            plio_32_bits,
            base + graphID + "_in2.txt"
        );

        // N 路输出
        for (int i = 0; i < N; i++) {
            out_plio[i] = output_plio::create(
                graphID + "_out" + std::to_string(i),
                plio_32_bits,
                base + graphID + "_outputaie" + std::to_string(i) + ".txt"
            );
        }

        // 连接
        connect(in_plio[0].out[0], core.in[0]);
        connect(in_plio[1].out[0], core.in[1]);
        connect(in_plio[2].out[0], core.in[2]);

        for (int i = 0; i < N; i++) {
            connect(core.out[i], out_plio[i].in[0]);
        }
    }
};
