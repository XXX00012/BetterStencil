#pragma once
#include <adf.h>
#include "../Config.h"
#include "../ProcessUnit/stencil_kernel.h"
  // 你的 kernel.h
using namespace adf;

template<int N>
class StencilCoreGraph : public adf::graph {
    public:
        port<input> in[1];
        port<output> out[N];
        kernel k1[N];

        StencilCoreGraph() {
            for (int i = 0; i < N; i++){
                k1[i] = kernel::create(jacobi_2d3_f32);
                source(k1[i]) = "ProcessUnit/stencil_kernel.cc";
                runtime<ratio>(k1[i]) = 0.9;

                connect<>(in[0],k1[i].in[0]);
                connect<>(k1[i].out[0],out[i]);

            }
        }
};