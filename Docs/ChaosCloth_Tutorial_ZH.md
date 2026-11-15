# Chaos Cloth 布料资产 · 傻瓜式操作教程（UE 5.8）

> 适用：你已经用 **GarmentCloth 插件**生成了 `xxx_Cloth` 资产，但它现在是空的（只有骨架，约 5KB）。
> 本文从"空资产"一步步带你做到"人物身上跑布料"。
>
> 阅读方式：**照着做，不要跳步**。每一步都写了"你应该看到什么"，看不到就按后面的排错表处理。

---

## 目录

1. [开始前的自检](#1-开始前的自检)
2. [打开布料资产](#2-打开布料资产)
3. [认识 Cloth Asset 编辑器的界面](#3-认识-cloth-asset-编辑器的界面)
4. [连 Dataflow 图（核心步骤）](#4-连-dataflow-图核心步骤)
5. [点 Rebuild 并检查](#5-点-rebuild-并检查)
6. [刷布料权重（Max Distance）](#6-刷布料权重max-distance)
7. [调布料参数](#7-调布料参数)
8. [放进场景：让人物穿上它](#8-放进场景让人物穿上它)
9. [存成模板，以后真正一键](#9-存成模板以后真正一键)
10. [排错表](#10-排错表)
11. [最后检查清单](#11-最后检查清单)

---

## 1. 开始前的自检

在 Content Browser 里确认这几样东西都在（缺一样就回头补）：

| 需要什么 | 在哪 | 怎么确认 |
|---|---|---|
| 衣服高模 | `Content/Clothes/.../Female_Clothes_26_Player01/` | `SKM_Garment_clothes26_v6`，双击能打开、能看模型 |
| Sim 低模 | 同目录 | `SKM_Garment_clothes26_v6_Sim`，顶点数约 6 千（不是 62 万） |
| 骨架 | 同目录 | `SKM_Garment_clothes26_v6_Skeleton` |
| 布料资产 | 同目录 | `SKM_Garment_clothes26_v6_Cloth`（约 5KB，空的是正常的） |
| 插件 | 编辑 → 插件 | **Chaos Cloth Asset**、**Chaos Cloth Asset Dataflow Nodes**、**GarmentCloth** 都是勾选状态 |

> ⚠️ 如果 `..._Cloth` 不存在：在 Content Browser 里右键 `SKM_Garment_clothes26_v6` → **Garment: Create Chaos Cloth Asset**。

---

## 2. 打开布料资产

1. 在 Content Browser 里**双击** `SKM_Garment_clothes26_v6_Cloth`；
2. 会打开一个新标签页，标题是 `SKM_Garment_clothes26_v6_Cloth`；
3. 屏幕中间是**视口**（现在应该是空的/只有骨架），下方或右侧是 **Dataflow 图** 面板。

**你现在应该看到**：一个空白的图区域，可能只有一两个默认节点，或者完全空白。
**这很正常** —— 5.8 的布料几何必须在图里生成。

> 如果图里已经有一些节点（比如 Import + Make Cloth Asset），直接跳到第 4 步的"改网格"部分。

---

## 3. 认识 Cloth Asset 编辑器的界面

打开后你会用到 4 个区域，记住位置就行：

| 区域 | 长什么样 | 用来干什么 |
|---|---|---|
| **Dataflow 图** | 大片深色画布，可以右键 | 连节点，生成布料几何 |
| **视口** | 显示 3D 模型 | 看 Rebuild 后有没有衣服 |
| **Details / 属性面板** | 右侧或底部 | 选中节点后改参数（比如选网格） |
| **工具栏** | 顶部一排按钮 | **Rebuild** 在这里；权重绘制模式也在这里 |

找不到图面板？在顶部菜单 **Window** 里把 **Dataflow** 勾上。

---

## 4. 连 Dataflow 图（核心步骤）

目标形状（先看懂这个再动手）：

```
Import Skeletal Mesh ──► Render Mesh ──┐
        (高模)                          ├──► Make Cloth Asset（终端，必须有）
                     └──► Sim Mesh ─────┘
                          (低模 _Sim)
```

### 4.1 添加 Import 节点

1. 在**图空白处右键** → 弹出节点搜索菜单；
2. 搜索框输入 `Skeletal`（或 `Import`）；
3. 找到 **Import Skeletal Mesh**（有的版本叫 **Skeletal Mesh Import**）→ 单击添加；
4. 节点出现在鼠标位置，拖到左边空白处；
5. 选中这个节点 → 看**右侧 Details** 面板 → 找到 **Skeletal Mesh**（或 **Mesh**）属性；
6. 点下拉框 → 搜索 `SKM_Garment_clothes26_v6` → 选中**高模**（不是 _Sim）。

**你应该看到**：节点上出现了一个输出引脚（右侧的小圆点），Details 里显示你选的网格名。

> 如果 Details 里没有 Skeletal Mesh 属性：说明你加错了节点（可能是 `Import File` 之类），删掉重加。

### 4.2 添加 Render Mesh 节点

1. 右键空白处 → 搜索 `Render Mesh`；
2. 添加 **Render Mesh** 节点；
3. 选中它，Details 里如果有网格属性，选 `SKM_Garment_clothes26_v6`；
   - 如果**没有**网格属性，说明它会自动用上游的网格 —— 那就靠连线解决（下一步）；
4. 把它拖到 Import 节点右边，**从 Import 的输出引脚拖一条线到 Render Mesh 的输入引脚**。

### 4.3 添加 Sim Mesh 节点（**最容易漏、也最关键**）

1. 右键 → 搜索 `Sim Mesh`（也可能叫 `Simulation Mesh`）→ 添加；
2. 选中它 → Details 里找网格属性 → 选 `SKM_Garment_clothes26_v6_Sim`（**低模，5,966 顶点那个**）；
3. 同样从 **Import** 的输出引脚连一条线到 **Sim Mesh** 的输入引脚（和 Render Mesh 并联，不是串联）。

**为什么必须指 Sim**：不指的话它会拿 62 万顶点的高模去解算，UE 会直接卡死或者布料抖成面条。

### 4.4 添加终端节点 Make Cloth Asset

1. 右键 → 搜索 `Make Cloth Asset` → 添加；
2. 把它放在最右边；
3. 从 **Render Mesh** 的输出 → 连到 **Make Cloth Asset** 的输入；
4. 从 **Sim Mesh** 的输出 → 连到 **Make Cloth Asset** 的另一个输入（如果只有一个输入，就先接 Sim，再在 Details 里指定 Render）。

> `Make Cloth Asset` 是**终端节点**（terminal），图里必须有它，否则不会产出布料集合。

### 4.5（可选但强烈建议）挂顶点色权重

1. 右键 → 搜索 `Weight Map` → 添加 **Weight Map** 节点；
2. 它通常能在 Details 里选一个 **Name**（比如 `MaxDistance`）；
3. 把它插在 `Sim Mesh` 和 `Make Cloth Asset` 之间（或按节点提示接）；
4. 这样 Blender 导出的 `ClothMask` 顶点色就能直接驱动布料权重，省掉手刷。

如果这一步看不懂，**先跳过** —— 后面第 6 步可以手动画权重。

---

## 5. 点 Rebuild 并检查

1. 点顶部工具栏的 **Rebuild**（有些版本在终端节点右键菜单里也有 **Rebuild**）；
2. 等 3~10 秒（62 万顶点的高模第一次会比较久）；
3. 看**视口**：

| 你看到的 | 说明 | 下一步 |
|---|---|---|
| 衣服出现在视口里 | ✅ 成功 | 去第 6 步 |
| 视口还是空的 | 图没连对 | 检查 Make Cloth Asset 有没有接上、Import 有没有选网格 |
| 出现报错红字 | 看第 10 步排错表 | — |

4. 打开 **Window → Message Log**，如果有 Dataflow 报错，里面会写明是哪个节点的问题。

---

## 6. 刷布料权重（Max Distance）

布料不是"整件都能动"，必须告诉它**哪里固定、哪里自由**。

### 6.1 先理解两个值

| Max Distance | 含义 | 该刷在哪 |
|---|---|---|
| **0** | 完全固定，跟着骨骼硬走 | 领口、肩线、腰带一圈、袖口紧的地方 |
| **1** | 完全模拟，自由飘 | 裙摆、下摆、大袖口 |

### 6.2 怎么刷

1. 在 Cloth Asset 编辑器里切到 **Cloth Painting** 模式（工具栏上有个笔刷图标，或菜单 View → Cloth Painting）；
2. 左侧会出现笔刷设置：**Brush Size**、**Strength**、值；
3. 把值先设成 **0**，笔刷调大，把**领口、肩、腰带**刷一遍（这些地方要固定）；
4. 再把值设成 **1**，把**裙摆、袖口**刷一遍；
5. 交界处（比如腰部往下）用中间值（0.3~0.6）过渡。

### 6.3 参考 Blender 的 ClothMask

Blender 端已经给你画好了：

- 顶点色 `ClothMask` 的 **R 通道** = 可模拟（1 = 能动）
- **G 通道** = 固定（1 = 固定）

在 UE 的网格 Details 里能看到顶点色。照着它刷最省事：
**R 高的地方刷 1，G 高的地方刷 0。**

---

## 7. 调布料参数

在 Cloth Asset 的 Details / Cloth 配置面板里，参数太多不要慌，**只改这 5 个**：

| 参数 | 建议值 | 作用 |
|---|---|---|
| **Mass**（质量） | 0.3 ~ 0.6 | 越大越"沉"，太小会飘 |
| **Damping**（阻尼） | 0.5 | 越大越不抖 |
| **Bend**（弯曲刚度） | 0.3 | 越大越硬，像厚布料 |
| **Solver Frequency** | 60 ~ 120 | 越高越稳但越吃性能 |
| **Substeps** | 2 ~ 4 | 抖得厉害就加 |

### 碰撞设置

- **不要**用人物原始身体（32k 顶点）当碰撞体，会拖垮性能；
- 用人物自带的**简化碰撞**（Physics Asset 里的 capsule / sphere），或者只给关键部位（胸、腰、腿）加胶囊体；
- 在布料的 **Collision** 设置里勾上对应骨骼。

---

## 8. 放进场景：让人物穿上它

布料资产做好后，**它自己不会动**，需要在关卡里加组件。

### 8.1 最简单：直接加组件

1. 打开你的关卡（比如 `NewMap`）；
2. 工具栏 **Add → Chaos Cloth**（或 Add Component → 搜 `Chaos Cloth`）；
3. 选中新加的 **Chaos Cloth 组件**，在 Details 里：
   - **Cloth Asset** = `SKM_Garment_clothes26_v6_Cloth`
4. 播放（PIE）→ 应该能看到衣服跟着动、裙摆有物理。

### 8.2 正确姿势：让布料跟着人物动画走

如果你要把它穿在会动的角色身上（推荐做法）：

1. 选中 **Chaos Cloth 组件**；
2. 在 **Details → Attach** 里，把它 **Attach 到人物的 SkeletalMeshComponent**（比如 `SKM_MH_Boss04_Body` 的 Mesh）；
3. 勾上 **Use Attached Parent As Pose Component**（在 Cloth / Pose 设置里）；
4. 这样人物的动画驱动姿势，布料只负责物理模拟。

### 8.3 蓝图里动态换装

在角色蓝图里：

```
Set Cloth Asset (Target: Chaos Cloth Component, New Asset: 你的布料资产)
```

配合一个变量数组就能做"一键换衣服"。

### 8.4 常见错位问题

| 现象 | 原因 | 解决 |
|---|---|---|
| 衣服位置错开很远 | 组件没 attach，或者 transform 不对 | Attach 到 Mesh，Location 归零 |
| 衣服不跟着人物动 | 没勾 Use Attached Parent As Pose Component | 勾上 |
| 衣服穿模进身体 | 碰撞体没设 | 给胸/腰/腿加 capsule |
| 衣服抖/炸 | Substeps 太低 / Sim 网格用了高模 | 加 Substeps，确认 Sim 是 `_Sim` |

---

## 9. 存成模板，以后真正一键

你已经连过一次图了，把它存成模板，下次就省掉第 4 步。

1. 在 Dataflow 图面板里 **右键 → Save as / Export**（或文件菜单）把图另存为：
   ```
   /Game/ClothTemplates/DF_GarmentTemplate
   ```
2. 打开工程的 `Config/DefaultEditor.ini`，在文件末尾加：

```ini
[GarmentCloth]
TemplateDataflow=/Game/ClothTemplates/DF_GarmentTemplate
```

3. 重启编辑器；
4. 以后右键任何衣服 → **Garment: Create Chaos Cloth Asset**，插件会自动把这张图挂上去（日志里会显示 `Template Dataflow ... -> attached`）；
5. 你只需要在模板里把 Import 节点的网格换成新衣服，Rebuild 一下就完事。

> 模板里如果用的是"变量/参数"而不是写死的网格资产，改动会更少。

---

## 10. 排错表

| 现象 | 可能原因 | 怎么修 |
|---|---|---|
| 双击资产打开是空的、没有图面板 | 面板被关了 | Window → Dataflow |
| 右键搜不到 Import 节点 | Dataflow Nodes 插件没启用 | 编辑 → 插件 → 勾 **Chaos Cloth Asset Dataflow Nodes** → 重启 |
| Rebuild 后视口空的 | Make Cloth Asset 没接 / Import 没选网格 | 检查连线 |
| Rebuild 报 "Skeleton mismatch" | Sim 网格和衣服的骨架不一样 | 导入 Sim 时 Skeleton 要选同一个 |
| 布料抖成面条 | Sim 网格用了高模；Substeps 太低 | 指 `_Sim`，Substeps 加到 4 |
| 布料完全不动 | Max Distance 全 0 | 至少把裙摆刷成 1 |
| 布料穿过身体 | 没碰撞体 | 加 capsule 碰撞 |
| 帧率暴跌 | 高模在解算 | 确认 Sim Mesh |
| 衣服和人物错位 | 组件没 attach | Attach 到 Mesh 并归零 Location |
| Message Log 一堆红字 | 节点版本差异（5.8 里部分节点被 Deprecated） | 用最新版节点名；或从模板复制 |

---

## 11. 最后检查清单

做完请逐条打勾：

- [ ] `..._Cloth` 资产打开后能看到衣服（不是空的）
- [ ] Import 节点选的是**高模**，Sim Mesh 选的是 **`_Sim` 低模**
- [ ] 有 `Make Cloth Asset` 终端节点，且已连接
- [ ] 领口/肩/腰带 Max Distance = 0，裙摆/袖口 = 1
- [ ] Mass 0.3~0.6、Damping 0.5、Bend 0.3、Substeps ≥ 2
- [ ] 场景里加了 Chaos Cloth 组件，Cloth Asset 指对了
- [ ] 组件 Attach 到人物 Mesh，勾了 Use Attached Parent As Pose Component
- [ ] PIE 里衣服跟着人物动、裙摆有飘动、没有穿模
- [ ] 图已存成 `/Game/ClothTemplates/DF_GarmentTemplate`
- [ ] `Config/DefaultEditor.ini` 里 `[GarmentCloth] TemplateDataflow=...` 已写

---

## 附：一句话总结

> 插件负责**建资产 + 挂骨架**，你负责**连一次图（Import → Render/Sim → Make Cloth Asset → Rebuild）+ 刷权重 + 加组件**。
> 连完存模板，以后每件衣服就只剩"点一下 + 换网格"。
