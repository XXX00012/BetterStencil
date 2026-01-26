#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include "../pl/config.h"   // 需要 data_t / IN_ROW / ROW 等，路径按你工程改

namespace stencil_io {

// 宽松提取所有数字：支持科学计数法、容忍混杂字符
inline std::vector<float> load_numbers_loose(const std::string& path) {
  std::ifstream ifs(path);
  if (!ifs) throw std::runtime_error("Cannot open file: " + path);

  std::string text((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  static const std::regex re(R"([-+]?(?:\d+\.\d*|\.\d+|\d+)(?:[eE][-+]?\d+)?)");

  std::sregex_iterator it(text.begin(), text.end(), re), end;
  std::vector<float> nums;
  for (; it != end; ++it) nums.push_back(std::stof(it->str()));
  return nums;
}

// 读一个 row：期望 n 个 float，不足补0，超出截断
inline void read_row_n(const std::string& path, float* dst, size_t n) {
  auto nums = load_numbers_loose(path);
  if (nums.empty()) throw std::runtime_error("No numeric data found in: " + path);

  if (nums.size() != n) {
    std::cerr << "[WARN] " << path << " numbers=" << nums.size()
              << " expected=" << n << " (truncate/pad)\n";
  }
  size_t m = std::min(n, nums.size());
  for (size_t i = 0; i < m; ++i) dst[i] = nums[i];
  for (size_t i = m; i < n; ++i) dst[i] = 0.0f;
}

// 把三路 18-float 输入拼成 (N+2)*IN_ROW 的 DDR 输入（row-major）
// - N=1 时：就是 row0,row1,row2
// - N>1 时：这里默认循环复用三行来填充（跑通链路用）。你也可以改成读取更多文件。
inline void assemble_dataIn_3rows_repeat(int N,
                                        const std::string& in0,
                                        const std::string& in1,
                                        const std::string& in2,
                                        float* dataIn /* size=(N+2)*IN_ROW */) {
  std::vector<float> r0(IN_ROW), r1(IN_ROW), r2(IN_ROW);
  read_row_n(in0, r0.data(), IN_ROW);
  read_row_n(in1, r1.data(), IN_ROW);
  read_row_n(in2, r2.data(), IN_ROW);

  for (int r = 0; r < N + 2; ++r) {
    float* row = dataIn + (size_t)r * IN_ROW;
    const float* src = (r % 3 == 0) ? r0.data() : (r % 3 == 1) ? r1.data() : r2.data();
    std::copy(src, src + IN_ROW, row);
  }
}

// 写输出：一行一个 float（最稳，便于 AIE/PLIO/脚本解析）
inline void write_output(const std::string& path, const float* dataOut, int N) {
  std::ofstream ofs(path);
  if (!ofs) throw std::runtime_error("Cannot write file: " + path);
  ofs.setf(std::ios::fixed);
  ofs.precision(6);

  for (int r = 0; r < N; ++r) {
    for (int i = 0; i < ROW; ++i) {
      ofs << dataOut[(size_t)r * ROW + i] << "\n";
    }
  }
}

// 可选：对比输出与 gold（容忍 gold 文件里混杂字符）
inline int compare_with_gold(const float* out, size_t n,
                             const std::string& gold_path,
                             float tol = 1e-4f,
                             size_t max_report = 10) {
  auto nums = load_numbers_loose(gold_path);
  if (nums.empty()) throw std::runtime_error("No numeric data found in: " + gold_path);

  int bad = 0;
  size_t m = std::min(n, nums.size());
  for (size_t i = 0; i < m; ++i) {
    float diff = std::fabs(out[i] - nums[i]);
    if (diff > tol) {
      if ((size_t)bad < max_report) {
        std::cerr << "[DIFF] idx=" << i << " out=" << out[i] << " gold=" << nums[i]
                  << " |diff|=" << diff << "\n";
      }
      bad++;
    }
  }
  if (nums.size() != n) {
    std::cerr << "[WARN] gold count=" << nums.size() << " out count=" << n
              << " (compared " << m << " elements)\n";
  }
  return bad;
}

} // namespace stencil_io
