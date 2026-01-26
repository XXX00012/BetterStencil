import numpy as np
import os
import re
import sys
import argparse

TOL = 1e-4


def load_numbers_loose(path, dtype=np.float32):
    """通用读取：把文件里所有数字抠出来（给 golden 用）"""
    if not os.path.exists(path):
        raise FileNotFoundError(path)
    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        text = f.read()
    nums = re.findall(r"[-+]?(?:\d+\.\d*|\.\d+|\d+)(?:[eE][-+]?\d+)?", text)
    if not nums:
        raise ValueError(f"No numeric data found in {path}")
    return np.array([float(x) for x in nums], dtype=dtype)


def load_aie_output(path, dtype=np.float32):
    """
    专门给 AIE 输出用的读取函数：
    - 如果是“时间戳 + 数值”交替：例如
        T 7.53258e+05 ps
        1.000000000e+01
      就会跳过以 T 开头的那一行，只读下面的数值
    - 如果是纯数值（每行一个数），也能正常读取
    """
    if not os.path.exists(path):
        raise FileNotFoundError(path)

    vals = []
    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            s = line.strip()
            if not s:
                continue

            # 典型时间戳行：'T 7.53e+05 ps'
            if s[0] in ("T", "t"):
                continue

            # 只取这一行里的第一个数字
            m = re.search(r"[-+]?(?:\d+\.\d*|\.\d+|\d+)(?:[eE][-+]?\d+)?", s)
            if m:
                vals.append(float(m.group(0)))

    if not vals:
        raise ValueError(f"No numeric data found in AIE output file {path}")

    return np.array(vals, dtype=dtype)


def report_diff(aie, gold, tol=TOL, max_print=10):
    if len(aie) != len(gold):
        print(f"[Warning] Size mismatch: AIE={len(aie)} vs GOLD={len(gold)}; compare on min length.")
        m = min(len(aie), len(gold))
        aie = aie[:m]
        gold = gold[:m]

    diff = np.abs(aie - gold)
    max_diff = float(np.max(diff))
    mean_diff = float(np.mean(diff))
    idx = int(np.argmax(diff))

    print("-" * 60)
    print(f"Samples checked        : {len(gold)}")
    print(f"Max absolute error     : {max_diff:.6g}")
    print(f"Mean absolute error    : {mean_diff:.6g}")
    print(f"Max error @ index      : {idx}")
    print(f"  GOLD[{idx}]          : {float(gold[idx]):.10g}")
    print(f"  AIE [{idx}]          : {float(aie[idx]):.10g}")
    print(f"  ABS DIFF             : {float(diff[idx]):.10g}")

    bad = np.where(diff > tol)[0]
    print(f"Mismatches (diff>tol)  : {bad.size} (tol={tol})")
    if bad.size > 0:
        print(f"First {min(max_print, bad.size)} mismatches:")
        for k in bad[:max_print]:
            print(f"  idx={int(k)}  gold={float(gold[k]):.10g}  aie={float(aie[k]):.10g}  diff={float(diff[k]):.10g}")
    print("-" * 60)

    return max_diff <= tol


def main():
    parser = argparse.ArgumentParser(
        description="Verify: compare stencil_output0.txt with golden output0.txt in the same data dir"
    )
    parser.add_argument("--gold", type=str, default="output0.txt",
                        help="golden file name (default: output0.txt)")
    parser.add_argument("--aie", type=str, default="stencil_output0.txt",
                        help="AIE output file name (default: stencil_output0.txt)")
    parser.add_argument("--tol", type=float, default=TOL, help="tolerance")
    args = parser.parse_args()

    data_dir = os.path.dirname(os.path.abspath(__file__))

    gold_path = os.path.join(data_dir, args.gold)
    aie_path = os.path.join(data_dir, args.aie)

    print("=== File-to-file Verification ===")
    print(f"GOLD : {gold_path}")
    print(f"AIE  : {aie_path}")
    print(f"TOL  : {args.tol}")

    gold = load_numbers_loose(gold_path, np.float32)
    aie = load_aie_output(aie_path, np.float32)

    passed = report_diff(aie, gold, args.tol)

    if passed:
        print("\033[92m[PASS]\033[0m AIE output matches GOLD within tolerance.")
        sys.exit(0)
    else:
        print("\033[91m[FAIL]\033[0m Output mismatch.")
        sys.exit(1)


if __name__ == "__main__":
    main()
