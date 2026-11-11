# StateBit

StateBit 枚举简单无向图中规模至少为 q 的全部极大 k-plex。每个解中，每个顶点至多与其他 k−1 个顶点不相邻；自身不计为缺边。输入要求 k ≥ 2、q ≥ 2k−1。

顺序与并行入口使用同一搜索核心。程序结合局部位表示、跨状态 SIMD 批处理、位宽选择及固定顶点折叠处理不同规模的搜索子问题。

## 环境

- Linux 或 WSL2，Python 3.10 及以上。
- 支持 C++14 的 GNU C++ 编译器；参考性能实验使用 Ubuntu g++ 11.4.0。
- x86-64 CPU，支持 SSE4.2、AVX2、BMI2 和 POPCNT。
- `taskset`，用于运行时绑定 CPU。

不需要第三方 Python 包。构建时优先使用 `g++-11`，也可通过 `CXX` 指定编译器。

## 构建和运行

```bash
bash build.sh
bash run.sh /path/to/graph.bin -k 3 -q 20
bash run.sh /path/to/graph.bin -k 3 -q 800 -t 16
```

单线程默认选择顺序入口，多线程默认选择并行入口。若需要并行入口的单线程运行：

```bash
bash run.sh /path/to/graph.bin -k 3 -q 20 --mode parallel -t 1
```

`--cpus 0,1,2,3` 可以指定与线程数对应的 CPU 列表。默认选择当前进程允许使用的前 t 个 CPU；不足时报告错误。小图预处理后任务不足时，内核可能使用少于请求数量的工作线程。

运行入口自动应用 `config/runtime.json` 的正式参数，不需要用户拼接内部内核开关。终端显示 case、开始时间、结束时间、结果数量、算法耗时及 wall 秒数。每次运行在 `results/` 下创建独立目录；原始程序输出保存在该目录中。

`bash run.sh --help` 显示完整的公开参数。

## 正确性检查

```bash
bash test.sh
```

检查包括：独立穷举 oracle 与完整解集合比较、极大性及重复输出、原顶点编号恢复、全部四顶点简单图、k=2到6的边界与相邻阈值、k=2/3/4/5/8/9/17的二跳临界构造、非法更小阈值拒绝、空图与完全剥离、固定顶点与多位宽域，以及并行入口的结果一致性。验证可执行文件由相同核心加验证宏单独构建，性能可执行文件不输出完整解集合。

测试图由测试程序生成，写入临时目录。测试并行正确性可以启动多于宿主可用 CPU 数的工作线程；这不是并行性能测量。

## 两例参考工作负载

`benchmarks/cases.csv` 定义以下两例，默认各运行一次：

| 数据集 | k | q | 入口 | 线程 | 期望数量 |
|---|---:|---:|---|---:|---:|
| soc-epinions | 3 | 20 | 顺序 | 1 | 548634119 |
| webbase-2001 | 3 | 800 | 并行 | 16 | 1785341050 |

```bash
DATASET_DIR=/path/to/Dataset bash run_all.sh
```

该入口自动构建、执行小图预检，并核对真实图的文件头、大小与完整 SHA256。参考计数与输入身份必须同时匹配。参考性能测量要求 g++ 11.4.0 和16个允许使用的 CPU，任务依次启动，默认不限时。

可通过环境变量设置：

- `REPEATS`：每例重复次数，默认1。
- `CPU_LIST`：16个互异的允许 CPU；顺序例使用其中第一个。
- `EXPORT_DIR`：结果 ZIP 的导出目录；默认使用 Windows 下载目录下的 `kplex_results`。
- `TIMEOUT_SEC`：单例超时秒数，默认0表示不限时。

```bash
# 仅查看工作负载，不构建也不运行
bash run_all.sh --list

# 每例三次，检查时间波动
DATASET_DIR=/path/to/Dataset REPEATS=3 bash run_all.sh
```

每例结束后保存 `raw.csv` 并导出结果 ZIP。只有结果目录 `status.json` 为 `COMPLETE`，才表示所有计划任务均通过。失败和中断保留已有记录，不自动重试或续跑。算法内部耗时与全进程 wall 分开记录；读图、编译和输入核对不能混入算法耗时。单次测量不代表稳定的性能分布。

## 文件布局

| 路径 | 职责 |
|---|---|
| `core/application.cpp` | 单一核心编译单元 |
| `core/search/` | 搜索上下文、各位宽内核、折叠与任务状态 |
| `core/preprocess/`、`core/runtime/`、`core/output/` | 图预处理、并行调度和结果输出 |
| `core/folding/`、`core/shared/`、`core/config/` | 固定基组件、通用原语和内核参数 |
| `Sequential/`、`Parallel/` | 顺序和并行入口 |
| `config/` | 正式编译选项和运行配置 |
| `scripts/` | 构建、单例运行和参考工作负载入口 |
| `tests/` | 当前实现的独立正确性与工具测试 |
| `benchmarks/` | 两例工作负载及输入身份 |
| `docs/DESIGN.md` | 最终实现的设计与配置说明 |
| `NOTICE.md` | 源码来源说明 |

构建产物位于 `.build/`，运行产物位于 `results/`，均不属于源码发布内容。
