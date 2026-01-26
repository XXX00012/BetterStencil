#pragma once
#include <adf.h>
#include <string>
#include "ProcessGraph/StencilCoreGraph.h"

using namespace adf;

template<int N>
class TopStencilGraph : public graph{
public:
    StencilCoreGraph<N> core;

    input_plio in_plio[1];
    output_plio out_plio[N];

    TopStencilGraph(const std::string& graphID){

        const std::string base = 
        "/home/xianyijiang/large/BetterStencil/data/";

        in_plio[0] = input_plio::create(
            graphID + "_in0",
            plio_32_bits,a
            base + graphID + "_in0.txt"
        );

        for (int i = 0; i < N; i++){
            output_plio[i] = output_plio::create(
                graphID + "_out" + std::to_string(i),
                plio_32_bits,
                base + graphID + "_out" + std::to_string(i) + ".txt"
            );
        }

        connect<>(in_plio[0].out[0],core.in[0]);
        for (int i = 0; i < N; i++){
            connect<>(core.out[i], output_plio[i].in[0]);
        }
    }
};