#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <chrono>
#include <memory>
#include <algorithm> // for std::fill

#include <xrt/xrt_device.h>
#include <xrt/xrt_bo.h>
#include <xrt/xrt_kernel.h>
#include <experimental/xrt_graph.h>

#include "../pl/config.h"
#include "stencil.hpp"  // 依然保留，用于写输出文件

static constexpr const char* KERNEL_NAME = "TopPL:{TopPL_0}";
static constexpr const char* GRAPH_NAME  = "topStencil";

// 辅助函数：从单个文件循环读取数据填充 buffer
void load_data_from_single_file(const std::string& filename, float* buffer, size_t total_elems) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "[WARN] Cannot open " << filename << ". Using random data instead.\n";
        // 如果文件打不开，用随机数填充，防止程序挂掉
        for (size_t i = 0; i < total_elems; i++) buffer[i] = (float)(rand() % 100);
        return;
    }

    size_t count = 0;
    float val;
    while (count < total_elems) {
        if (infile >> val) {
            buffer[count++] = val;
        } else {
            // 文件读完了但 buffer 还没满：清除 EOF 标志，回到开头重新读
            infile.clear();
            infile.seekg(0, std::ios::beg);
            if (!(infile >> val)) break; // 防止死循环（空文件）
            buffer[count++] = val;
        }
    }
    std::cout << "Loaded " << count << " elements from " << filename << " (looped if needed).\n";
}

int main(int argc, char** argv) {
  try {
    if (argc < 2) { 
        std::cout << "Usage: " << argv[0] << " <xclbin> [N]\n";
        return 1; 
    }

    int N = (argc >= 3) ? std::atoi(argv[2]) : 1;
    if (N <= 0) N = 1;

    // 【修改点 1】只保留一个输入文件路径
    const std::string in_path  = "./data/stencil_in0.txt";
    const std::string out_path = "./data/houtput.txt";

    using host_t = data_t; 
    
    // 计算需要的总数据量：(N + 2行 Halo) * 每行宽度
    const size_t IN_ELEMS  = (size_t)(N + 2) * (size_t)IN_ROW;
    const size_t OUT_ELEMS = (size_t)N * (size_t)ROW;

    // 1. 初始化设备
    xrt::device device(0);
    auto uuid = device.load_xclbin(argv[1]);

    // 2. 获取内核
    xrt::kernel krnl(device, uuid, KERNEL_NAME);

    // 3. 启动 Graph
    // Graph 必须在整个运行期间保持存活
    std::unique_ptr<xrt::graph> graph;
    try {
      graph = std::make_unique<xrt::graph>(device, uuid, GRAPH_NAME);
      graph->reset();
      graph->run(-1); // 无限运行，等待数据
      std::cout << "Graph running: " << GRAPH_NAME << "\n";
    } catch (...) {
      std::cerr << "[WARN] graph start failed (maybe already running): " << GRAPH_NAME << "\n";
      // 不直接退出，尝试继续运行
    }

    // 4. 分配 DDR 内存 (Buffer Object)
    xrt::bo in_bo (device, IN_ELEMS  * sizeof(host_t), krnl.group_id(0));
    xrt::bo out_bo(device, OUT_ELEMS * sizeof(host_t), krnl.group_id(1)); // 建议用 group_id(1) 如果你的 PL 有两个接口

    // 5. 映射到主机内存
    host_t* in  = in_bo.map<host_t*>();
    host_t* out = out_bo.map<host_t*>();

    // 【修改点 2】不再调用 stencil_io::assemble...，而是直接读取单文件
    std::cout << "Loading input data for N=" << N << " (" << IN_ELEMS << " elements)...\n";
    load_data_from_single_file(in_path, reinterpret_cast<float*>(in), IN_ELEMS);

    // 6. 数据搬运：Host -> Device (DDR)
    in_bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    // 7. 启动 PL 内核
    std::cout << "Starting PL Kernel...\n";
    auto t0 = std::chrono::high_resolution_clock::now();
    
    // 这里的参数必须和 PL 的接口定义 TopPL(in, out, N) 严格对应
    auto run = krnl(in_bo, out_bo, N); 
    run.wait();
    
    auto t1 = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "Kernel execution completed in " << duration << " s\n";

    // 8. 数据搬运：Device (DDR) -> Host
    out_bo.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    // 9. 写出结果
    stencil_io::write_output(out_path, reinterpret_cast<float*>(out), N);
    std::cout << "Wrote output to " << out_path << "\n";

    // Graph 会在 unique_ptr析构时自动结束，或你可以手动 graph->end();
    return 0;

  } catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << "\n";
    return 1;
  }
}