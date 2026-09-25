# ATT48 数据来源

- `att48.tsp`：TSPLIB 官方原始输入，48 个美国本土州首府，`EDGE_WEIGHT_TYPE: ATT`。
  下载地址：https://comopt.ifi.uni-heidelberg.de/software/TSPLIB95/tsp/att48.tsp.gz
- `att48.opt.tour`：官方参考最优回路，仅用于测试，不被 C++ 搜索器读取。
  下载地址：https://comopt.ifi.uni-heidelberg.de/software/TSPLIB95/tsp/att48.opt.tour.gz
- 公开最优费用：10628（ATT 单位）。
  官方结果：https://comopt.ifi.uni-heidelberg.de/software/TSPLIB95/tsp/TSP-BEST.html
- ATT 距离规则：https://comopt.ifi.uni-heidelberg.de/software/TSPLIB95/tsp/tsp95.pdf ，第 2.5 节。

`att48_capitals.csv` 仅用于地理展示：按州名英文字母顺序，将美国相连 48 州首府对应到 ATT48 的 1–48 号节点。经纬度来自 Natural Earth 的首府点；排除 Alaska、Hawaii 和 District of Columbia。**这些经纬度不用于计算目标函数。** 原始 ATT 坐标及地理经纬度不是同一个坐标系。

`us48_states.geojson` 为 Natural Earth 1:110m 概化州界的本土 48 州子集。地理图采用经纬度坐标，并按北纬 38 度校正显示纵横比。直线连接仅表示访问顺序，不代表公路或航线。

Natural Earth 原始数据（公有领域）：

- https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/geojson/ne_10m_populated_places_simple.geojson
- https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/geojson/ne_110m_admin_1_states_provinces.geojson
- 使用条款：https://www.naturalearthdata.com/about/terms-of-use/

数据于 2026-09-25 获取，`SHA256SUMS` 固定本次输入文件的 SHA-256 校验值。`scripts/prepare_map.py` 从保存在 `tmp/` 的上述两个 GeoJSON 原文件抽取地图子集；已随项目提供抽取结果，复现求解和绘图不需要联网。
