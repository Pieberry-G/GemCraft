# GemCraft 开发文档

---
## 开发环境

### 当前版本仅在以下环境进行测试：
|  | 型号 |
| ---------------- | ------------------------- |
| CPU | i5-12400f / R5-5600X |
| 显卡 | RTX3090 / RTX4070(至少8G以上显存)，显卡驱动 v566.03(至少支持CUDA 12.1) |
| 系统 | Windows11 |

### 安装步骤：
**1. 安装Visual Studio 2022**
**2. 安装CMake**
**3. 安装Anaconda**
**4. 运行GenerateProjects.bat构建项目，并自动创建conda环境**
- **GenerateProject.bat**会把代理设置到**Clash**的端口 http://127.0.0.1:7890 后再创建环境。如果conda环境安装出现错误，请仔细查看GenerateProjects.bat的内容，尝试删除已创建的conda环境并通过脚本重新创建。

**5. 构建完成后，build文件夹将会生成GenCraft.sln解决方案，使用**Release**模式编译整个项目得到快速的版本**

---
## 操作说明
1. 程序开始读取obj文件后，**自动进行区域选择**。选中的区域可以通过a/shift+a或者s/shift+s进行调整(目前不建议进行这个操作)。
2. **按下q**对选中的区域进行几何处理，并初始化测地距离属性。
3. 以下两种方式可以放置钻石(只支持前5种镶嵌方式)：
- **按下w**在钻石表面铺满钻石。
- **按下z**设置测地线起点，**按下x**设置测地线终点，**按下c**计算测地线，**按下v**沿着测地线放置钻石。
4. **按下e**做布尔操作。

注：目前是线性流程，不能逆向操作。

---
## 外部库(deps)
| Name | Purpose | Link |
| ---- | ---- | ---- |
| CGAL | CGAL是一个开源软件项目，以C++库的形式提供高效可靠的几何算法，适用于需要几何计算的各个领域。 | https://www.cgal.org/ |
| Geometry-central | Geometry-central是一个现代C++数据结构和算法库，用于几何处理，特别侧重于表面网格。 | https://geometry-central.net/ |
| Polyscope |  Polyscope 是一个 C++/Python 查看器和用户界面，用于查看网格和点云等 3D 数据。它允许您注册数据并快速生成信息丰富且美观的可视化效果。 | https://polyscope.run/ |
| SAM-Adapter | 把分割大模型SAM适配到下游项目，用于钻石区域检测。 | |
| spdlog | 高性能C++日志库。 | https://github.com/gabime/spdlog |
| tinygltf | 用于渲染的gltf/glb模型文件读取器。 | https://github.com/syoyo/tinygltf |
| tinyobjloader | 用于渲染的obj模型文件读取器。 | https://github.com/tinyobjloader/tinyobjloader |

---
## 源代码

### Core
- APP核心代码

| File | Explanation |
| ---- | ---- |
| EntryPoint.h       | main函数，程序入口，创建APP |
| **Application.h**  | App主干，初始化各个模块，创建场景 |
| **Scene.h**        | 场景类，放置场景中的各个模型 |
| Base.h             | 定义宏(assert) |
| Log.h              | 初始化spdlog，定义宏(print) |
| ResourceManager.h  | 预加载模型文件 |

### Mesh
- 关于Mesh的定义以及处理

| File | Explanation |
| ---- | ---- |
| **Mesh.h**         | GemCraft关于Mesh数据的定义 |
| MeshSubset.h       | Mesh的子集，比如说选定的子区域 |
| **FormatTool.h**   | GemCraft中定义的Mesh与CGAL、Geometry-central中定义的SurfaceMesh进行格式转换，方便几何库的调用 |
| GeodesicTool.h | 依赖Scene中的戒指进行初始化，这是戒指的测地距离属性，初始化后才能在戒指表面画线以及获得戒指表面的法线 |
| **RegionSelectionTool.h** | 区域选择步骤，目前包括：1) 渲染多视角图片；2) 调用Sam-Adapter预测分割钻石的Mask，注意我在Infer.py对预测的Mask做了过滤；3) 把分割结果逆投影回戒指表面； 4) 区域扩张算法，选中整片钻石区域 |
| **GeometryTool.h** | 几何处理步骤，目前包括删除选中的钻石区域以及填补空洞 |
| PlacementTool.h    | 在戒指表面放置钻石的工具 |
| BooleanTool.h      | 做布尔操作的工具 |
| GemSetting.h       | 定义各种钻石的镶嵌方式 |
| Path.h             | 多个点排成一条路径 |
| GemGroup.h          | 钻石组，存储放置的钻石的指针 |
| RegionGrowingUtils.h | 区域增长算法用到的工具包 |

### TinyRenderer
- 模型渲染器，包括读取模型、渲染图片、分割Mask、FaceID，依赖于polyscope的帧缓冲和着色器。
**注：这里解析Mesh文件的方式和几何处理解析Mesh文件的方式不一样。**

| File | Explanation |
| ---- | ---- |
| Mesh.h             | Mesh的定义 |
| Model.h            | 解析的模型文件包含Mesh以及材质，构成Model |
| Camera.h           | 一个简单的相机 |
| Image.h            | 纹理图像的载体 |
| OpenGLImage.h      | 纹理在OpenGL中的实现 |
| RenderTool.h       | 调用渲染器渲染图片，分割Mask，FaceID |

### Panels
| File | Explanation |
| ---- | ---- |
| CustomUI.h         | 自定义的UI界面 |

### Tools
| File | Explanation |
| ---- | ---- |
| DatasetBuilder.h   | 数据集处理，渲染3D数据集得到2D数据集 |

### EventSystem
- 事件系统，处理polyscope发出的事件。

| File | Explanation |
| ---- | ---- |
| Event.h            | 事件基类 |
| KeyEvent.h         | 键盘事件 |
| MouseEvent.h       | 鼠标事件 |
| ApplicationEvent.h | App事件 |
| Input.h            | 查询按键是否被按下 |
| KeyCodes.h         | GLFW的键盘映射 |
| MouseCodes.h       | GLFW的鼠标映射 |

### pch.h
- 预编译头文件，包含标准库函数

### Polyscope
- 在外部库中，只有Polyscope的代码是有改动的

| File | Explanation |
| ---- | ---- |
| **polyscope.h**    | 这里在300行左右有与事件系统有关的内容 |
| **state.h**        | 存放Polyscope的全局变量 |
| **custom_shaders.h** | 自定义的着色器 |
| **engine.h**       | Framebuffer的定义 |
| **gl_engine.h**    | Framebuffer的初始化 |
| surface_mesh.h     | polyscope对于mesh的定义 |