#pragma once
#include <adf.h>
#include "../Config.h"
#include "../ProcessUnit/stencil_kernel.h"
  // 你的 kernel.h
using namespace adf;

template<int N>
class StencilCoreGraph : public graph {
public:
    // 对外暴露的端口（以后 TopGraph/PL 都接这些）
    port<input>  in[3];
    port<output> out[N];
    kernel k1[N];

    StencilCoreGraph() {
        for (int i = 0; i < N; i++) {
            k1[i] = kernel::create(jacobi_2d3_i32);
            source(k1[i]) = "ProcessUnit/stencil_kernel.cc"; // 路径按你的工程调整
            runtime<ratio>(k1[i]) = 0.9;

            // 显式 window 大小
            connect<>(in[0], k1[i].in[0]);
            connect<>(in[1], k1[i].in[1]);
            connect<>(in[2], k1[i].in[2]);
            connect<>(k1[i].out[0], out[i]);
        }
    }
};
