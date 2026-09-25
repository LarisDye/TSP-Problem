# 经典 TSP 问题的模拟退火算法

新增完整的 **C++17 ATT48 实验**：美国本土 48 州首府，沿用本文的模拟退火 + 2-opt 算法，支持逐提案可视化和运行中关闭显示。默认种子 42 实测得到 **10628 ATT 单位**，等于 [TSPLIB 官方最优值](https://comopt.ifi.uni-heidelberg.de/software/TSPLIB95/tsp/TSP-BEST.html)。连续种子 42–61 的 20 次运行中有 3 次达到最优值，平均费用 10687.70；这是本次实验结果，不是每次运行的保证。

## C++ 快速运行

在项目根目录执行（Windows，已安装 MinGW-w64 的 `g++`）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File build.ps1 -Test
.\build\tsp_sa.exe
```

这里的执行策略参数只作用于本次 PowerShell 子进程。程序会询问 `Visualize every proposal? [y/N]`：输入 `y` 打开显示，回车关闭显示。也可以明确指定：

```powershell
.\build\tsp_sa.exe --visual
.\build\tsp_sa.exe --no-visual
.\build\tsp_sa.exe --help
```

可视化使用 Windows 原生窗口，无第三方图形依赖。每一次 2-opt 候选提案（包括拒绝和等长提案）都在接受判断后同步绘制**当前路线**，并显示迭代次数、温度、当前费用和历史最好费用。没有抽帧，也不会把最好路线冒充当前搜索路线。搜索完成后显示最终最好路线，结果文件已经保存，可关闭窗口退出。

- **空格**：暂停 / 继续。
- **N**：暂停状态下前进一步。
- **+ / -**：增减帧间等待时间。
- **Esc / V / 窗口关闭按钮**：实时关闭可视化，求解继续，结果照常保存。暂停时也能关闭。

默认每帧等待 16 ms；完整预算有 2,188,800 次提案，逐帧展示仅等待就约需 9.7 小时。可以观察一段后关闭显示，或用下列约两分钟的较小预算演示（该演示不保证达到 10628）：

```powershell
.\build\tsp_sa.exe --visual --steps 50 --max-levels 150 --cooling 0.94
```

`--frame-ms 0` 取消人为帧间等待，但仍同步绘制每一步。界面开关和等待均不消耗求解器随机数，不影响固定种子的搜索结果。批量实验使用 `--no-visual`。

## 复现论文的 ATT48 实验

数据已随项目保存，详见 [数据说明](data/README.md)。求解器只读取 `att48.tsp`，从不读取参考最优路线。**ATT 费用不等于公里、公路里程或普通欧氏距离。** 地图上的经纬度和州界只用于展示。

```powershell
.\build\tsp_sa.exe --no-visual --seed 42 --runs 20 --output results/att48
python scripts/verify_results.py
python -m pip install -r requirements-figures.txt
python scripts/make_figures.py
xelatex -interaction=nonstopmode -halt-on-error -output-directory=output/pdf tsp_simulated_annealing.tex
xelatex -interaction=nonstopmode -halt-on-error -output-directory=output/pdf tsp_simulated_annealing.tex
```

成稿：[output/pdf/tsp_simulated_annealing.pdf](output/pdf/tsp_simulated_annealing.pdf)。主 TeX 引入 [att48_experiment.tex](att48_experiment.tex)，新增地理地图、结果闭环地图、原始坐标中的初始/最终路线、收敛曲线、20 次运行比较、参数与复现说明。图件为 `figures/` 下的矢量 PDF 和 PNG。仓库已有图件及实验数值，可直接编译 TeX，无须安装绘图依赖或联网。

`results/att48/seed_<seed>/` 保存每次运行的 `result.json`、`history.csv`、`best.tour`；总目录的 `runs.csv` 汇总全部运行，`result.json` 等文件保存这批实验中最好的运行（并列取最先出现者）。默认普通运行写入 `results/latest/`。重复使用同一输出目录会覆盖同名文件；若需保留旧实验，请使用新的 `--output` 目录。

收敛文件含初始状态和每个温度层末的状态，共 457 行数据；`iteration` 为累计提案数，`accepted` 为累计接受数。实时窗口仍逐提案刷新。首次发现最好解的精确提案数另存于 JSON 的 `best_iteration`。图中的统计与数值宏自动读取运行结果生成，未手工编造曲线。

主要文件：`src/tsp.hpp` 为算法；`src/visualizer.hpp` 为 Windows 绘图；`src/main.cpp` 为命令行及记录输出。整数费用采用 64 位，输入支持 TSPLIB `ATT`、`EUC_2D`，不支持的边权类型明确报错。`--seed`、`--cooling`、`--min-ratio`、`--steps`、`--max-levels`、`--runs` 均可配置。

若使用 CMake，可执行 `cmake -S . -B build/cmake` 和 `cmake --build build/cmake --config Release`。Linux/macOS 可用 `g++ -std=c++17 -O2 src/main.cpp -o build/tsp_sa` 编译无界面版本；实时窗口仅支持 Windows。

验证包括：官方最优回路费用、10,810 次四边增量核对、小实例精确解、逐提案回调、关闭后继续搜索及全部 20 次保存结果的独立费用重算。运行真实窗口集成测试：

```powershell
g++ -std=c++17 -O2 -static tests/test_visualizer.cpp -o build/test_visualizer.exe -lgdi32 -luser32
.\build\test_visualizer.exe
```

该测试仅创建并操作自身窗口，检查暂停、单步以及暂停时 X/Esc/V 关闭后计算继续。以下保留原有模型推导和 Python 教学示例。

本文依次说明 TSP 的数学模型、模拟退火的数学原理，以及二者如何组成一个可执行算法。主例是完全图上的**对称 TSP**；二维欧氏距离只是其中一种情况。

## 1. 经典 TSP 问题

### 1.1 问题定义

旅行商问题（Traveling Salesman Problem，TSP）：给定若干城市及任意两个城市之间的旅行费用，寻找一条从某个城市出发、访问其余每个城市恰好一次、最后返回出发城市的总费用最小的闭合路线。费用可以是距离、时间或其他固定成本。[Waterloo 的 TSP 定义](https://www.math.uwaterloo.ca/tsp/problem/index.html)

本文假定只有一名旅行商，没有容量、时间窗等额外约束，任意城市对之间都存在有限费用。

设城市编号为

\[
V=\{0,1,\ldots,n-1\},\qquad n\geq 3,
\]

距离矩阵为 \(D=(d_{ij})\)。对于本文的对称问题，

\[
d_{ij}=d_{ji}\geq0,\qquad d_{ii}=0.
\]

若城市具有二维坐标 \((x_i,y_i)\)，可取欧氏距离

\[
d_{ij}=\sqrt{(x_i-x_j)^2+(y_i-y_j)^2}.
\]

一般对称 TSP 不要求距离必须来自坐标，也不一定满足三角不等式。下面的算法只依赖距离对称性。

### 1.2 解的表示与目标函数

用城市的一个排列表示访问顺序：

\[
s=(v_0,v_1,\ldots,v_{n-1}),\qquad v_n=v_0.
\]

总长度为

\[
C(s)=\sum_{k=0}^{n-1}d_{v_k,v_{k+1}}.
\]

优化目标是

\[
\boxed{\min_{s\in\mathcal S} C(s)},
\]

其中 \(\mathcal S\) 是所有合法城市排列构成的集合。排列保证城市不重复、不遗漏；最后一项 \(d_{v_{n-1},v_0}\) 保证返回起点。图论上，这是在加权完全图中寻找最短哈密顿回路。

例如，排列 `[0, 2, 1, 3]` 表示路线 `0 → 2 → 1 → 3 → 0`，计算长度时有四条边。

### 1.3 为什么需要启发式算法

固定城市 0 为起点不会损失任何闭合路线，只是消除旋转重复。还将正向和反向视为同一路线时，对称 TSP 有

\[
\frac{(n-1)!}{2}
\]

条不同回路。候选数量随城市数增长得很快。TSP 的优化版本是 NP-hard；相应的标准判定版本是 NP-complete。[Cook 的 TSP 综述](https://www.math.uwaterloo.ca/~bico/papers/tsp_icm.pdf)

模拟退火的目标是在给定计算预算内找到质量较好的可行解。它不附带有限运行时间内的全局最优性证明，也没有这里所述算法的一般近似比保证。

## 2. 模拟退火的数学原理

### 2.1 从物理退火到优化

物理退火通过逐渐降温使系统趋向低能量状态。优化中将目标函数 \(C(s)\) 视为能量，将温度 \(T>0\) 视为控制搜索随机性的参数。这一联系是经典模拟退火方法的基础。[Kirkpatrick、Gelatt 与 Vecchi，1983](https://doi.org/10.1126/science.220.4598.671)

### 2.2 Boltzmann 分布与 Metropolis 接受准则

对于固定温度，可以定义

\[
\pi_T(s)=\frac{\exp[-C(s)/T]}{Z(T)},
\qquad
Z(T)=\sum_{u\in\mathcal S}\exp[-C(u)/T].
\]

低成本解的概率较大；在有限状态空间上，\(T\to0^+\) 时这个分布的概率质量集中到全局最优解集合。这里将物理中的 Boltzmann 常数吸收到温度参数中，\(T\) 应与 \(C\) 使用相同的费用尺度。

从当前解 \(s\) 产生候选解 \(s'\)，令

\[
\Delta=C(s')-C(s).
\]

若候选生成概率满足对称性 \(q(s'\mid s)=q(s\mid s')\)，接受概率取

\[
\boxed{
A(s\to s';T)=\min\{1,\exp(-\Delta/T)\}
=\begin{cases}
1,&\Delta\leq0,\\
\exp(-\Delta/T),&\Delta>0.
\end{cases}}
\]

也就是：新解更好或相等时接受；变差时，以一定概率接受。这是 Metropolis 准则。[Metropolis 等，1953](https://www.osti.gov/servlets/purl/4390578)

这个形式可以直接由概率比值解释：

\[
\frac{\pi_T(s')}{\pi_T(s)}
=\exp\left[-\frac{C(s')-C(s)}{T}\right].
\]

归一化常数 \(Z(T)\) 被约掉，因此不必遍历所有路线。进一步，令 \(P_T\) 为包含接受或拒绝操作的转移概率，则对不同状态有

\[
\pi_T(s)P_T(s,s')=\pi_T(s')P_T(s',s),
\]

这称为细致平衡。它说明 \(\pi_T\) 是固定温度链的平稳分布；在相应连通性和非周期性条件下，充分多的转移可趋近该分布。有限次抽样并不意味着已经达到平衡。

对非对称候选分布，应使用包含 \(q(s\mid s')/q(s'\mid s)\) 的 Metropolis–Hastings 比率。本文均匀选择一对位置再反转的操作具有对称性，因此不需要该修正。

### 2.3 为什么允许变差

如果每一步只能变好，搜索可能停在局部最优解。离开某个局部最优区域，有时需要先经过更长的路线。

以变差量 \(\Delta=10\) 为例：

- \(T=100\) 时，接受概率约为 \(0.9048\)；
- \(T=10\) 时，接受概率约为 \(0.3679\)；
- \(T=1\) 时，接受概率约为 \(0.0000454\)。

高温时更容易接受暂时变差，低温时主要保留改善。这不会保证每次都逃离局部最优，但提供了逃离的机制。

### 2.4 降温与理论保证

工程中常用几何降温：

\[
T_{k+1}=\alpha T_k,\qquad0<\alpha<1.
\]

固定温度的平稳分布、渐近收敛定理以及有限运行的结果，是不同层次的结论。在适当条件下，足够慢的对数降温，例如 \(T_t=c/\log(1+t)\)，可使状态依概率趋向全局最优集合；常数 \(c\) 需满足与能垒深度有关的条件。这个结论不能直接用于保证有限轮几何降温得到全局最优解。[Hajek，1988](https://web.mit.edu/6.435/www/Hajek88.pdf)

## 3. 经典 TSP 的模拟退火算法

### 3.1 初始化

1. 准备距离矩阵 \(D\)。若输入坐标，则先计算两两距离。
2. 固定 \(v_0=0\)，随机打乱其余城市，得到初始路线 \(s\)。也可以使用最近邻法构造初始路线。
3. 计算完整闭环长度 \(C(s)\)。
4. 令历史最好路线 \(s_{\mathrm{best}}=s\)，最好长度 \(C_{\mathrm{best}}=C(s)\)。
5. 设置初温、降温系数、每个温度的提案次数和停止条件。

必须将“当前解”和“历史最好解”分开：允许当前解变差是搜索策略；最终输出历史最好解才能保留已发现的成果。程序保存最好路线时需要复制列表。

### 3.2 邻域：2-opt 区间反转

均匀选择两个不同位置 \(1\leq i<j\leq n-1\)，把 \(v_i,\ldots,v_j\) 的顺序反转，起点 0 保持不动。

例如：

```text
原路线：0 → A → B → C → D → E → 0
新路线：0 → A → D → C → B → E → 0
                └ 反转 B、C、D ┘
```

该操作保持所有城市恰好出现一次，并保持整条路线连通，因此候选解始终可行，不需要添加罚函数。

对任意位置对，反转两次会回到原排列；位置对的选择概率不依赖当前路线。所以正反候选概率相同，符合前面的 Metropolis 前提。通过相邻位置的反转可以实现相邻交换，因此该邻域能连接所有固定起点的排列。

实现中允许反转所有非起点城市。这只改变整个回路的方向，在对称 TSP 中长度不变，是无害但没有改进作用的提案。

### 3.3 快速计算长度差

设

\[
a=v_{i-1},\quad b=v_i,\quad c=v_j,
\quad d=v_{(j+1)\bmod n}.
\]

原路线边界上有 \((a,b)\) 与 \((c,d)\)，反转后变成 \((a,c)\) 与 \((b,d)\)。内部边只改变方向，在对称距离下其费用不变。因此

\[
\boxed{\Delta=d_{ac}+d_{bd}-d_{ab}-d_{cd}}.
\]

只读取四个矩阵元素，就能以 \(O(1)\) 时间评价候选。若 \(j=n-1\)，则 \(d=v_0\)，这也正确处理了最后一条返回起点的边。

注意：这个四边公式适用于**对称距离**。在非对称 TSP 中，内部反向边的费用也会变化。此时还需加上

\[
\sum_{k=i}^{j-1}\left(d_{v_{k+1},v_k}-d_{v_k,v_{k+1}}\right),
\]

或直接重新计算候选路线总成本。配套程序明确拒绝非对称矩阵。

### 3.4 接受或拒绝

- 若 \(\Delta\leq0\)，执行反转并更新当前长度。
- 若 \(\Delta>0\)，生成 \(u\sim U[0,1)\)；当 \(u<\exp(-\Delta/T)\) 时接受。
- 否则保留当前路线。
- 接受后，如果当前路线优于历史最好路线，复制并更新历史最好解。

这里计算差值时必须使用候选解与**当前解**之差，不能与历史最好解比较后套入接受概率。

### 3.5 温度与迭代次数

**初温。** 温度取值依赖距离单位。可在初始路线处试采样一批 2-opt 提案，记录正的成本增量，其均值为 \(\overline{\Delta_+}\)。希望这一典型变差量在初始时以概率 \(p_0\) 被接受，则由

\[
p_0=\exp(-\overline{\Delta_+}/T_0)
\]

得到

\[
\boxed{T_0=-\frac{\overline{\Delta_+}}{\log p_0}}.
\]

示例设置 \(p_0=0.8\)，试采样 200 次。这个公式只针对平均变差量，不表示所有变差提案的平均接受率恰好为 80%。没有采到正增量时，示例以最大边权作为初温；所有边权均为零时用 1。

**同温度搜索长度。** 每个温度执行 \(L\) 次提案，包含被拒绝的提案。示例采用 \(L=100n\)。这是一项计算预算选择，不保证在该温度下已经充分混合。

**降温。** 示例采用 \(\alpha=0.98\)，每完成 \(L\) 次提案后更新 \(T\leftarrow\alpha T\)。更接近 1 的系数使温度下降更慢，同一温度范围需要更多计算。

**停止。** 当 \(T/T_0\leq10^{-4}\) 或温度层数达到 2000 时停止。也可以另外设置总运行时间上限。使用相对温度阈值，有助于适应距离整体缩放。

上述参数只是教学示例的起点。实际应观察运行预算和多次独立运行的解质量；仅增加初温并不一定有效。多次重启可以提高找到好解的机会，不能变成最优性证明。

### 3.6 完整伪代码

```text
输入：对称距离矩阵 D，初温 T0，终温 Tmin，降温系数 α
      每层提案次数 L，最大温度层数 Kmax

s ← 固定起点后随机生成的城市排列
C ← 闭环总长度(s)
best_s ← copy(s)
best_C ← C
T ← T0
k ← 0

while T > Tmin and k < Kmax:
    repeat L times:
        均匀随机选择位置 1 ≤ i < j ≤ n-1
        a, b ← s[i-1], s[i]
        c, d ← s[j], s[(j+1) mod n]
        Δ ← D[a,c] + D[b,d] - D[a,b] - D[c,d]

        if Δ ≤ 0 or Uniform(0,1) < exp(-Δ/T):
            反转 s[i..j]
            C ← C + Δ
            if C < best_C:
                best_s ← copy(s)
                best_C ← C

    T ← αT
    k ← k + 1

输出：best_s 和 best_C
```

伪代码按精确算术表述。配套实现会在每个温度层结束和准备保存新纪录时重新计算路线长度，以减轻浮点增量累积误差。

### 3.7 一个可手算的更新

取单位正方形四个顶点：

\[
0=(0,0),\quad1=(1,0),\quad2=(1,1),\quad3=(0,1).
\]

当前路线为 `0 → 2 → 1 → 3 → 0`，长度为 \(2+2\sqrt2\)。反转排列中的位置 1 至 2，得到 `0 → 1 → 2 → 3 → 0`。差值为

\[
\Delta=d_{0,1}+d_{2,3}-d_{0,2}-d_{1,3}
=2-2\sqrt2\approx-0.828427.
\]

它是改善，必然接受，新长度为 4。若从周长路线做反向操作，则差值为 \(+0.828427\)，是否接受由当前温度决定。

### 3.8 复杂度与实现要点

设实际温度层数为 \(K\)，每层提案次数为 \(L\)。

- 两两距离预计算及矩阵存储分别需要 \(O(n^2)\) 时间与空间。
- 一个候选的四边成本差计算需要 \(O(1)\) 时间。
- Python 列表中的区间反转需要 \(O(j-i+1)\)，最坏为 \(O(n)\)；复制最好路线也需要 \(O(n)\)。因此不能把整个提案更新都称为 \(O(1)\)。
- 包含接受后的操作，本实现最坏总时间为 \(O(n^2+KLn)\)，总空间为 \(O(n^2)\)。矩阵之外的工作空间为 \(O(n)\)。

若只看成本差评价部分，其成本为 \(O(KL)\)。无最大层数限制时，几何降温的层数约为

\[
K=\left\lceil\frac{\log(T_{\min}/T_0)}{\log\alpha}\right\rceil.
\]

当 \(T_{\min}/T_0=10^{-4}\)、\(\alpha=0.98\) 时，需要 456 个温度层。

## 4. 运行配套 Python 实现

文件 `tsp_sa.py` 仅使用 Python 标准库；`test_tsp_sa.py` 包含独立核对。建议 Python 3.10 或更高版本。运行：

```bash
python tsp_sa.py
python -m unittest -v
```

在本次 Python 3.13.5 验证环境中，十城市合成示例使用随机种子 42 得到：

```text
Route: 0 -> 7 -> 8 -> 6 -> 5 -> 4 -> 9 -> 3 -> 2 -> 1 -> 0
Initial length: 41.146372
Best length: 24.643181
Initial temperature: 9.609163
Temperature levels: 456
Proposals: 456000; accepted: 83142
```

这组坐标和距离单位用于教学，不对应真实地理位置。这里报告的是搜索到的最好结果；上述运行本身不提供十城市实例的全局最优性证明。

用自己的坐标调用：

```python
from tsp_sa import euclidean_distances, simulated_annealing_tsp

points = [(0, 0), (1, 0), (1, 1), (0, 1)]
distance = euclidean_distances(points)
result = simulated_annealing_tsp(distance, seed=42)

print(result.route + [result.route[0]])  # 明确补上返回起点
print(result.length)
```

也可以直接向 `simulated_annealing_tsp` 传入对称距离矩阵。返回的 `route` 只存储各城市一次，`length` 已包含返回起点的距离。

本次五项自动核对全部通过，覆盖：不同规模对称矩阵上的四边增量与完整重算、七城市实例与穷举最优值的比较、固定随机种子的可复现性与距离缩放、零距离与三城市情况和迭代上限，以及对非对称输入的拒绝。七城市实例的三个指定种子均达到了穷举最优值，这是针对该实例的验证结果。
