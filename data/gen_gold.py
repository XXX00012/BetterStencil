import numpy as np
import os
import re
import argparse

# ====== 必须与你的 stencil_kernel.h 一致 ======
ROW = 16
HALO = 1
IN_ROW = 18  # ROW + 2*HALO
# ============================================

def write_flat_txt(path: str, arr: np.ndarray, fmt: str = "{:.10g}"):
    """每个 sample 一行，写 golden 用"""
    # path 在同目录下时，dirname 可能是空字符串，这里做一下保护
    out_dir = os.path.dirname(path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    flat = arr.reshape(-1)
    with open(path, "w", encoding="utf-8") as f:
        for v in flat:
            f.write(fmt.format(float(v)) + "\n")

def load_numbers_loose(path: str, dtype=np.float32) -> np.ndarray:
    """
    读取 txt 中所有数字（哪怕夹杂 out: 之类文本也能读出来）
    """
    if not os.path.exists(path):
        raise FileNotFoundError(path)

    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        text = f.read()

    nums = re.findall(r"[-+]?(?:\d+\.\d*|\.\d+|\d+)(?:[eE][-+]?\d+)?", text)
    if not nums:
        raise ValueError(f"No numeric data found in {path}")

    return np.array([float(x) for x in nums], dtype=dtype)

def compute_golden(top: np.ndarray, mid: np.ndarray, bot: np.ndarray) -> np.ndarray:
    """
    严格对齐你的 kernel:
      out[j] = mid[j+1] + top[j+1] + bot[j+1] + mid[j] + mid[j+2]
    每行输入 18，输出 16。支持多行 frame：输入长度必须是 18 的整数倍
    返回：1D 向量，长度 = num_rows * ROW
    """
    if not (len(top) == len(mid) == len(bot)):
        raise ValueError(f"Input length mismatch: top={len(top)}, mid={len(mid)}, bot={len(bot)}")

    if len(top) % IN_ROW != 0:
        raise ValueError(f"Input length {len(top)} not divisible by IN_ROW={IN_ROW}")

    num_rows = len(top) // IN_ROW
    out = np.empty((num_rows * ROW,), dtype=np.float32)

    for r in range(num_rows):
        base = r * IN_ROW
        row_top = top[base:base + IN_ROW]
        row_mid = mid[base:base + IN_ROW]
        row_bot = bot[base:base + IN_ROW]

        for j in range(ROW):
            idx = j + HALO  # 1..16
            out[r * ROW + j] = (
                row_mid[idx] +
                row_top[idx] +
                row_bot[idx] +
                row_mid[idx - 1] +
                row_mid[idx + 1]
            )
    return out

def main():
    ap = argparse.ArgumentParser(
        description="gen_gold: read stencil_in*.txt and write output0.txt in the same folder as this script"
    )
    ap.add_argument("--in0-name", type=str, default="stencil_in0.txt")
    ap.add_argument("--in1-name", type=str, default="stencil_in1.txt")
    ap.add_argument("--in2-name", type=str, default="stencil_in2.txt")
    ap.add_argument("--gold-name", type=str, default="output0.txt")
    args = ap.parse_args()

    # 规则：脚本在哪个目录，数据就在哪个目录（你现在放在 templates/data）
    data_dir = os.path.dirname(os.path.abspath(__file__))

    in0_path  = os.path.join(data_dir, args.in0_name)
    in1_path  = os.path.join(data_dir, args.in1_name)
    in2_path  = os.path.join(data_dir, args.in2_name)
    gold_path = os.path.join(data_dir, args.gold_name)

    top = load_numbers_loose(in0_path, np.float32)
    mid = load_numbers_loose(in1_path, np.float32)
    bot = load_numbers_loose(in2_path, np.float32)

    gold = compute_golden(top, mid, bot)
    write_flat_txt(gold_path, gold)

    print("=== gen_gold done (inputs kept unchanged) ===")
    print(f"in0(top)     : {in0_path}  ({top.size} samples)")
    print(f"in1(mid)     : {in1_path}  ({mid.size} samples)")
    print(f"in2(bot)     : {in2_path}  ({bot.size} samples)")
    print(f"gold(output) : {gold_path} ({gold.size} samples)")

if __name__ == "__main__":
    main()
