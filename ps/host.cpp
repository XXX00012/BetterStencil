#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <regex>
#include <chrono>
#include <cstdlib>
#include <algorithm>
#include <stdexcept>
#include <iterator>
#include <memory>

#include <xrt/xrt_device.h>
#include <xrt/xrt_bo.h>
#include <xrt/xrt_kernel.h>
#include <experimental/xrt_graph.h>

#include "../pl/config.h"
#include "stencil.hpp"   // ← 改名后的头文件

static constexpr const char* KERNEL_NAME = "TopPL:{TopPL_0}";
static constexpr const char* GRAPH_NAME  = "topStencil";

int main(int argc, char** argv) {
  try {
    if (argc < 2) { /* usage */ return 1; }
    int N = (argc >= 3) ? std::atoi(argv[2]) : 1;
    if (N <= 0) N = 1;

    const std::string in0_path = "./data/stencil_in0.txt";
    const std::string in1_path = "./data/stencil_in1.txt";
    const std::string in2_path = "./data/stencil_in2.txt";
    const std::string out_path = "./data/houtput.txt";

    using host_t = data_t; // 更稳（避免 data_t != float 时炸）
    const size_t IN_ELEMS  = (size_t)(N + 2) * (size_t)IN_ROW;
    const size_t OUT_ELEMS = (size_t)N * (size_t)ROW;

    xrt::device device(0);
    auto uuid = device.load_xclbin(argv[1]);

    xrt::kernel krnl(device, uuid, KERNEL_NAME);
    

    // ✅ graph 必须活到 kernel 结束
    std::unique_ptr<xrt::graph> graph;
    try {
      graph = std::make_unique<xrt::graph>(device, uuid, GRAPH_NAME);
      graph->reset();
      graph->run(-1);
      std::cout << "Graph running: " << GRAPH_NAME << "\n";
    } catch (...) {
      std::cerr << "[WARN] graph start failed: " << GRAPH_NAME << "\n";
      graph.reset();
    }

    xrt::bo in_bo (device, IN_ELEMS  * sizeof(host_t), krnl.group_id(0));
    xrt::bo out_bo(device, OUT_ELEMS * sizeof(host_t), krnl.group_id(1));

    host_t* in  = in_bo.map<host_t*>();
    host_t* out = out_bo.map<host_t*>();

    // 用你的 stencil_io 工具拼 dataIn
    stencil_io::assemble_dataIn_3rows_repeat(N, in0_path, in1_path, in2_path,
                                             reinterpret_cast<float*>(in));

    in_bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    auto t0 = std::chrono::high_resolution_clock::now();
    auto run = krnl(in_bo, out_bo, N);
    run.wait();
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << "Kernel time: "
              << std::chrono::duration<double>(t1 - t0).count() << " s\n";

    out_bo.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    stencil_io::write_output(out_path, reinterpret_cast<float*>(out), N);
    std::cout << "Wrote " << out_path << "\n";

    // graph 在这里才析构，生命周期正确
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << "\n";
    return 1;
  }
}
