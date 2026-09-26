# TSP 模拟退火求解与可视化

本项目使用 C++17 求解 TSPLIB 的 ATT48 实例（美国本土 48 州首府），提供可实时关闭的逐步搜索可视化、实验记录、结果校验和论文图件生成工具。另保留一个使用 Python 标准库的教学实现。

问题建模、模拟退火原理、算法推导和实验分析见 [PDF 文档](output/pdf/tsp_simulated_annealing.pdf)。数据来源与距离单位见 [数据说明](data/README.md)。本 README 说明项目文件和使用方法。

## 文件结构

```text
TSP-Problem/
├── src/
│   ├── main.cpp                  # C++ 程序入口、命令行选项和结果导出
│   ├── tsp.hpp                   # TSPLIB 解析、距离计算和模拟退火求解
│   └── visualizer.hpp            # Windows 原生窗口及交互控制
├── tests/
│   ├── test_cpp.cpp              # C++ 算法、参考路线和回调行为测试
│   └── test_visualizer.cpp       # Windows 窗口暂停、单步和关闭测试
├── scripts/
│   ├── check_cli.py              # 命令行参数、异常输入和可复现性检查
│   ├── verify_results.py         # 独立校验保存的 ATT48 路线与费用
│   ├── make_figures.py           # 生成论文图件、数值宏和路线文本
│   └── prepare_map.py            # 从 Natural Earth 原始数据提取地图子集
├── data/
│   ├── att48.tsp                 # 求解器使用的原始 ATT48 数据
│   ├── att48.opt.tour            # 官方参考最优路线，仅用于验证
│   ├── att48_capitals.csv        # 节点编号、州名、首府和展示用经纬度
│   ├── us48_states.geojson       # 展示用美国本土州界
│   ├── SHA256SUMS                # 数据文件的 SHA-256 校验值
│   └── README.md                # 数据来源、处理方式和使用说明
├── results/att48/               # 论文使用的 20 次运行记录及汇总
├── figures/                     # 地图、路线和收敛图，含 PDF 与 PNG
├── output/pdf/                  # 编译后的论文 PDF
├── tsp_simulated_annealing.tex   # 论文主文件
├── att48_experiment.tex         # 主文件引入的 ATT48 实验部分
├── tsp_sa.py                    # Python 教学实现及十城市运行示例
├── test_tsp_sa.py               # Python 实现的单元测试
├── build.ps1                    # Windows 编译脚本，可选运行测试
├── CMakeLists.txt               # CMake 构建与 CTest 配置
├── requirements-figures.txt     # 绘图所需的 Python 依赖
├── build/                       # 本地编译产物，不纳入版本控制
└── tmp/                         # 中间文件与测试截图，不纳入版本控制
```

`prepare_map.py` 需要在 `tmp/` 中准备 `ne_10m_populated_places_simple.geojson` 和 `ne_110m_admin_1_states_provinces.geojson`，下载地址见数据说明。项目已包含提取后的地图数据，正常求解、绘图和编译论文无需运行该脚本。

## 环境要求

- **C++ 求解器**：支持 C++17 的编译器；Windows 下可使用 MinGW-w64 的 `g++`。
- **实时可视化**：仅支持 Windows，使用系统 GDI，无第三方图形库依赖。Linux/macOS 可运行无界面版本。
- **Python 示例、校验及测试脚本**：Python 3.10 或更高版本，仅使用标准库。
- **重新生成图件**：另需安装 `requirements-figures.txt` 中的 Matplotlib 和 NumPy。
- **编译论文**：安装包含 XeLaTeX、ctex 和 Fandol 字体的 TeX 发行版，如 TeX Live。

以下命令均在项目根目录执行。

## 编译与运行 C++ 程序

### Windows

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File build.ps1
.\build\tsp_sa.exe
```

执行策略参数只作用于本次 PowerShell 子进程。程序启动后询问 `Visualize every proposal? [y/N]`：输入 `y` 打开显示，回车关闭显示。也可以直接指定运行方式：

```powershell
.\build\tsp_sa.exe --visual
.\build\tsp_sa.exe --no-visual
.\build\tsp_sa.exe --help
```

### 其他构建方式

使用 CMake（需另行安装 CMake 和相应编译器）：

```bash
cmake -S . -B build/cmake
cmake --build build/cmake --config Release
```

可执行文件位于 `build/cmake/` 或其 `Release/` 子目录，取决于所用生成器。运行时仍以项目根目录为工作目录。

Linux/macOS 也可直接编译并运行无界面版本：

```bash
mkdir -p build
g++ -std=c++17 -O2 src/main.cpp -o build/tsp_sa
./build/tsp_sa --no-visual
```

### 常用参数

- `--input FILE`：TSPLIB 输入文件，默认 `data/att48.tsp`；支持 `ATT`、`EUC_2D` 坐标型实例。
- `--output DIR`：输出目录，默认 `results/latest/`。
- `--visual` / `--no-visual`：开启 / 关闭可视化。
- `--frame-ms N`：每帧等待毫秒数，默认 `16`；`0` 表示取消人为等待。
- `--seed N`：随机种子，默认 `42`。
- `--runs N`：独立运行次数，默认 `1`，使用连续种子；多次运行须关闭可视化。
- `--cooling X`：降温系数，默认 `0.98`。
- `--min-ratio X`：终温与初温的比值，默认 `0.0001`。
- `--steps N`：每个温度层的提案次数，默认城市数的 `100` 倍。
- `--max-levels N`：最大温度层数，默认 `2000`。

例如，将一次无界面运行的结果保存到独立目录：

```powershell
.\build\tsp_sa.exe --no-visual --seed 42 --output results/my_run
```

## 可视化操作

窗口在每一次候选提案判断后刷新当前路径，包括被拒绝的提案；同时显示提案次数、温度、当前费用和历史最好费用。搜索完成后显示最终最好路线，此时结果已经写入文件。

- **空格**：暂停 / 继续。
- **N**：暂停状态下前进一步。
- **+ / -**：增减帧间等待时间。
- **Esc / V / 窗口关闭按钮**：关闭可视化，继续完成求解并保存结果；暂停时也可关闭。

显示开关和等待不消耗求解器随机数，不改变固定种子下的搜索结果。

ATT48 默认预算有 2,188,800 次提案，每帧等待 16 ms 时，仅等待就约需 9.7 小时。可以观察一段后关闭显示，或用较小预算进行约两分钟的演示：

```powershell
.\build\tsp_sa.exe --visual --steps 50 --max-levels 150 --cooling 0.94
```

该演示使用的预算与论文实验不同，不保证得到同样的结果。

## 输出文件说明

每次运行在指定输出目录的 `seed_<seed>/` 子目录中保存：

- `result.json`：运行参数、初始费用、最好费用、路线、坐标、提案数、接受数、首次找到最好解的位置及耗时。
- `history.csv`：初始状态和每个温度层末的状态，字段为 `iteration, level, temperature, current, best, accepted`；提案数和接受数均为累计值。
- `best.tour`：TSPLIB TOUR 格式的最好路线，节点从 `1` 开始编号，返回起点的边隐含表示。

输出目录根部的 `runs.csv` 汇总本批所有运行；根部的 `result.json`、`history.csv` 和 `best.tour` 对应该批最好的一次运行，并列时取最先出现者。CSV 按温度层记录，实时窗口仍按每次提案刷新。

重复使用同一输出目录会覆盖同名文件；需要保留旧实验时，请更换 `--output`。ATT48 输出的费用以 ATT 单位计，不是公里；展示地图的经纬度不参与费用计算。

## 复现论文实验与图件

论文的运行记录已保存在 `results/att48/`。重新求解、校验并生成图件：

```powershell
.\build\tsp_sa.exe --no-visual --seed 42 --runs 20 --output results/att48
python scripts/verify_results.py
python -m pip install -r requirements-figures.txt
python scripts/make_figures.py
```

`make_figures.py` 专用于论文中约定的 ATT48 实验，会核对种子 42–61、运行参数及选中的种子 42，避免图件与正文不一致。它更新 `figures/` 下的图件，以及 `results/att48/numbers.tex` 和 `route.txt`。

已随项目提供图件和数值文件；仅重新编译论文时，无需重新求解或安装绘图依赖：

```powershell
xelatex -interaction=nonstopmode -halt-on-error -output-directory=output/pdf tsp_simulated_annealing.tex
xelatex -interaction=nonstopmode -halt-on-error -output-directory=output/pdf tsp_simulated_annealing.tex
```

生成文件为 [output/pdf/tsp_simulated_annealing.pdf](output/pdf/tsp_simulated_annealing.pdf)。编译两遍用于更新交叉引用。

## Python 教学实现

运行十城市示例：

```bash
python tsp_sa.py
```

也可在自己的代码中调用：

```python
from tsp_sa import euclidean_distances, simulated_annealing_tsp

points = [(0, 0), (1, 0), (1, 1), (0, 1)]
result = simulated_annealing_tsp(euclidean_distances(points), seed=42)
print(result.route + [result.route[0]])
print(result.length)
```

接口也接受有限、非负、严格对称且对角线为零的距离矩阵。Python 返回的城市编号从 `0` 开始；`route` 保存各城市一次，`length` 已包含返回起点的费用。该文件不提供实时窗口，论文中的 ATT48 实验由 C++ 程序完成。

## 测试与校验

Windows 下编译并运行 C++ 测试、Python 单元测试和命令行检查：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File build.ps1 -Test
```

单独运行 Python 测试或校验已有实验结果：

```powershell
python -m unittest -v
python scripts/verify_results.py
```

真实窗口集成测试需单独运行，会创建自身窗口并检查暂停、单步及三种关闭方式：

```powershell
g++ -std=c++17 -O2 -static tests/test_visualizer.cpp -o build/test_visualizer.exe -lgdi32 -luser32
.\build\test_visualizer.exe
```

如果使用 CMake 构建，可通过 `ctest --test-dir build/cmake -C Release --output-on-failure` 运行 C++ 核心测试。
