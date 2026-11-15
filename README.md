<div align="center">

# UE Chaos Clothes Render

**UE 5.8 editor plugin — right-click a garment SkeletalMesh → one-click create a Chaos Cloth Asset.**
**UE 5.8 编辑器插件 — 右键衣服骨骼网格 → 一键生成 Chaos Cloth 资产。**

[简体中文](#简体中文) · [English](#english)

</div>

---

<a id="简体中文"></a>

## 简体中文

### 这个插件做什么

在 Content Browser 里右键一件**衣服的 SkeletalMesh**，菜单里会多出一项：

> **Garment: Create Chaos Cloth Asset**

点它，插件会：

1. 创建 `UChaosClothAsset`（资产名 = `<衣服名>_Cloth`，同目录）；
2. 自动把衣服的 **Skeleton** 挂上（先走 `Skeleton` 属性，失败再调 `SetSkeleton()`）；
3. 自动找同目录的 `<衣服名>_Sim`（或任意 `*_Sim`）低模网格，并在日志里提示；
4. 如果配置了模板 Dataflow，自动 `SetDataflow()` 挂载；
5. 保存资产并**自动打开** Cloth Asset 编辑器；
6. 整个过程逐步打印在 **Output Log**（`[GarmentCloth]` 前缀）。

另外提供控制台命令，方便自动化：

```
GarmentCloth.CreateFrom /Game/Clothes/.../SKM_Garment
GarmentCloth.CreateFrom SKM_Garment        （按名字模糊查找）
```

### 安装

1. 把本仓库放到你的工程里（推荐直接 git clone 到 `Plugins/`）：
   ```
   YourProject/
   ├── Content/
   ├── Plugins/
   │   └── UEChaosClothesRender/     ← 本仓库
   └── YourProject.uproject
   ```
2. 编辑 `.uproject`（或在 编辑 → 插件 里勾选），启用：
   - **Chaos Cloth Asset**（5.8 默认不启用，必须手动开）
   - **Chaos Cloth Asset Dataflow Nodes**（建布料图要用）
   - **GarmentCloth**（本插件）
   - **EditorScriptingUtilities**（本插件依赖，一般默认已启用）
3. 编译 Editor 目标：
   ```powershell
   & "<UE>\Engine\Build\BatchFiles\Build.bat" YourProjectEditor Win64 Development `
       -project="D:\Path\YourProject.uproject" -waitmutex
   ```
4. 启动编辑器，右键任意 SkeletalMesh 即可看到菜单。

> 纯蓝图工程第一次加 C++ 插件时，UE 会提示生成 `Source/`（Target.cs + Build.cs），按提示生成后编译即可。

### 从 Blender 到 UE 的完整流程

本插件配套 Blender 端插件 `Garment Fit`（衣服贴合/蒙皮/布料预处理），流程：

| 步骤 | 在哪做 | 产物 |
|---|---|---|
| 1. 衣服贴合人物 + 蒙皮 | Blender（Garment Fit） | 衣服 SkeletalMesh |
| 2. 布料预处理（焊接碎块 + 生成 `ClothMask` 顶点色 + 低模 Sim 网格） | Blender（Garment Fit → 布料优化） | `SKM_Garment` + `SKM_Garment_Sim` |
| 3. 导出 FBX（**两个文件**：`SKM_Garment.fbx` / `SKM_Garment_Sim.fbx`） | Blender | 2 个 FBX，共用同一套骨架 |
| 4. 导入 UE | UE | 2 个 SkeletalMesh + 1 个 Skeleton |
| 5. 右键衣服 → **Create Chaos Cloth Asset** | 本插件 | `SKM_Garment_Cloth` |
| 6. 连 Dataflow 图（一次性，可存模板） | UE Cloth Asset 编辑器 | 可用的布料资产 |
| 7. 刷 Max Distance（照 `ClothMask` 顶点色） | UE | 布料权重 |

> ⚠️ **为什么 Sim 网格要单独导一个 FBX**：UE 的骨骼网格 FBX 导入一次只吃一个 mesh，两个 mesh 放在同一个 FBX 里只会导入第一个。

### 已经在 UE 里生成之后，还需要做什么（重要）

5.8 的 Chaos Cloth Asset 几何来自 **Dataflow 图**，而 Dataflow 节点类的头文件在引擎的
`ChaosClothAssetDataflowNodes/.../Internal/` 下，**第三方插件拿不到**，所以这一步不能全自动。
第一次手动做一遍（约 2 分钟），之后存成模板就完全一键：

1. 打开生成的 `SKM_Garment_Cloth`；
2. 加 **Import Skeletal Mesh** 节点 → 选 `SKM_Garment`；
3. 加 **Render Mesh / Sim Mesh** 节点 → Sim 选 `SKM_Garment_Sim`；
4. 连到 **Make Cloth Asset** → 点 **Rebuild**；
5. 用 **Max Distance** 刷布料权重（参考顶点色 `ClothMask`：R=1 可模拟，G=1 固定）；
6. 参数起点：Mass 0.3~0.6 / Damping 0.5 / Bend 0.3 / Solver Frequency 60~120 / Substeps 2~4；
7. 碰撞体用人物身体的简化碰撞（不要用原始 32k 顶点身体）。

**存成模板（强烈建议）**：把这个图另存为 `/Game/ClothTemplates/DF_GarmentTemplate`，然后在
`Config/DefaultEditor.ini` 里加：

```ini
[GarmentCloth]
TemplateDataflow=/Game/ClothTemplates/DF_GarmentTemplate
```

之后右键任何衣服，插件会连模板一起挂好 —— 只需在模板里换一次网格参数。

### 面板 / 菜单说明

| 功能 | 位置 | 说明 |
|---|---|---|
| Create Chaos Cloth Asset | Content Browser → 右键 SkeletalMesh | 主功能 |
| `GarmentCloth.CreateFrom <path\|name>` | 控制台 | 自动化/脚本用 |
| 模板 Dataflow | `Config/DefaultEditor.ini` | 让后续一键完成 |

### 已验证的环境

| 项 | 值 |
|---|---|
| UE | 5.8（源码 `D:\UE\UE_5.8`） |
| 编译 | UBT + VS 2022（MSVC 14.44 / SDK 22621） |
| 测试资产 | 62 万顶点衣服 + 5,966 顶点 Sim 网格 |
| 结果 | `SKM_Garment_clothes26_v6_Cloth` 创建成功，Skeleton 已挂 |

实测日志：

```
[GarmentCloth] Source mesh: /Game/.../SKM_Garment_clothes26_v6
[GarmentCloth] Cloth asset class: /Script/ChaosClothAssetEngine.ChaosClothAsset
[GarmentCloth] Created: /Game/.../SKM_Garment_clothes26_v6_Cloth
[GarmentCloth] Skeleton SKM_Garment_clothes26_v6_Skeleton -> set
[GarmentCloth] Sim mesh found: SKM_Garment_clothes26_v6_Sim
[GarmentCloth] === RESULT: SUCCESS ===
```

### 技术说明（5.8 源码核对过）

| 结论 | 依据 |
|---|---|
| 布料资产类 = `/Script/ChaosClothAssetEngine.ChaosClothAsset`（**不是** `ChaosClothAsset` 模块） | `ChaosClothAsset/Source/ChaosClothAssetEngine/Public/ChaosClothAsset/ClothAsset.h` |
| 有 `Skeleton` 属性（Setter=SetSkeleton）、`PhysicsAsset`、公开 `SetDataflow()` | 同上 |
| **没有** Cloth Asset 的 `UFactory` → `CreateAsset(..., nullptr)` | 在 `ChaosClothAsset*` 插件里搜 `: public UFactory` 无结果 |
| 旧版 clothing 已移除（`ClothingAssetCommon.h` 不存在） | 引擎源码 |
| Dataflow 节点头在 `Internal/`，外部插件不可用 | `ChaosClothAssetDataflowNodes/.../Internal/` |

因为上述限制，本插件所有 Chaos 相关调用都走**反射**（`FindObject` / `FindFProperty` / `FindFunction`），
版本变化只会降级 + 打日志，不会编译失败。

### 排错

| 现象 | 处理 |
|---|---|
| 右键没有菜单 | 插件没编译/没启用；或选中的不是 SkeletalMesh |
| "ChaosClothAsset not found" | 没启用 **Chaos Cloth Asset** 插件 |
| 日志显示 `No template Dataflow configured` | 正常，见上面"还需要做什么" |
| 编译报 `ColorVertexBuffer` | 本版本已默认关闭该检查（`#define GARMENTCLOTH_CHECK_VERTEXCOLOR 0`） |
| 编辑器下次启动要求重建模块 | 正常，插件是 C++ 的；按提示 rebuild 即可 |

---

<a id="english"></a>

## English

### What it does

Right-click a **garment SkeletalMesh** in the Content Browser:

> **Garment: Create Chaos Cloth Asset**

The plugin will:

1. create a `UChaosClothAsset` named `<mesh>_Cloth` next to the source mesh;
2. assign the garment's **Skeleton** (property first, then `SetSkeleton()`);
3. find the matching `<mesh>_Sim` (or any `*_Sim`) low-poly mesh in the same folder;
4. attach a template Dataflow if one is configured;
5. save the asset and **open the Cloth Asset editor**;
6. log every step to the Output Log with a `[GarmentCloth]` prefix.

Console command for automation:

```
GarmentCloth.CreateFrom /Game/Clothes/.../SKM_Garment
```

### Install

1. Put this repository under your project's `Plugins/` (clone it directly):
   ```
   YourProject/Plugins/UEChaosClothesRender/
   ```
2. Enable in `.uproject`: **Chaos Cloth Asset**, **Chaos Cloth Asset Dataflow Nodes**,
   **GarmentCloth**, **EditorScriptingUtilities**.
3. Build the editor target:
   ```powershell
   & "<UE>\Engine\Build\BatchFiles\Build.bat" YourProjectEditor Win64 Development -project="..." -waitmutex
   ```

### Pipeline

Blender (Garment Fit) → fit + skin → cloth prep (weld shells, `ClothMask` vertex color, Sim mesh) →
export **two FBX files** (mesh + `_Sim`, same skeleton) → import into UE → right-click → create cloth asset →
build the Dataflow graph once → paint Max Distance.

> UE's skeletal FBX importer only takes **one mesh per file**, that's why the Sim mesh is exported separately.

### Remaining manual step (important)

In 5.8 the cloth geometry comes from a **Dataflow graph**, and the node headers live under
`Internal/` in the engine — a third-party plugin cannot build that graph. Do it once:

1. open the generated `..._Cloth`;
2. add **Import Skeletal Mesh** (pick the garment) and **Render/Sim Mesh** nodes (Sim = `*_Sim`);
3. connect to **Make Cloth Asset** → **Rebuild**;
4. paint **Max Distance** using the `ClothMask` vertex color (R = simulate, G = pin);
5. starting values: Mass 0.3–0.6 / Damping 0.5 / Bend 0.3 / Frequency 60–120 / Substeps 2–4.

Save the graph as `/Game/ClothTemplates/DF_GarmentTemplate` and add to `Config/DefaultEditor.ini`:

```ini
[GarmentCloth]
TemplateDataflow=/Game/ClothTemplates/DF_GarmentTemplate
```

From then on it is genuinely one click.

### Verified on

UE 5.8 + VS 2022 (MSVC 14.44), 622k-vertex garment + 5,966-vertex Sim mesh →
`SKM_Garment_clothes26_v6_Cloth` created with the correct class and skeleton.

### Notes

Everything Chaos-related is resolved through reflection (`FindObject` / `FindFProperty` / `FindFunction`),
so a version change degrades gracefully with logs instead of breaking the build.

### License

MIT.
