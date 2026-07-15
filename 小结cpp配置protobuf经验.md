### 小结cpp配置protobuf经验

**参考博客: ** [[visual studio 2022配置和使用protobuf_visual studio 2022 安装proto插件-CSDN博客](https://blog.csdn.net/qq_45576085/article/details/140035857)] 



Q: 

![image-20260716000008246](C:\Users\guisan\AppData\Roaming\Typora\typora-user-images\image-20260716000008246.png)

我再补充一点：我在设置cpp语言标准、添加预处理器、检查运行库、重新生成之外，还调整了链接器的输入（如图）；当然，对VC++目录的配置必不可少

详细总结一下本次配置的要点
如果我下次需要链接类似Abseil的库，我该如何使用vcpkg?



Q:

太棒了！你的观察非常敏锐，而且补充得**非常关键**！

你截图里的这一步（在“链接器 -> 输入”中手动添加 `libprotobufd.lib` 等）往往是最后一道关卡。虽然 `vcpkg integrate install` 会自动帮我们处理大部分头文件和库的路径，但在某些特定情况下（比如项目配置的细微差异，或者动态库需要显式链接导入库时），Visual Studio 还是会报“无法解析的外部符号”。你手动把它加进“附加依赖项”，正是标准且有效的破局之法！

（注：如果你当前是 Debug 模式，用 `libprotobufd.lib`（带 d）是对的；如果下次切到 Release 模式，记得改成 `libprotobuf.lib`（不带 d）哦。）

---

针对你的要求，我为你详细总结了本次配置的**核心要点**，以及**未来如何用 vcpkg 链接其他库**的通用指南。

### 一、 本次 Protobuf (C++ Visual Studio) 配置要点总结

在 Visual Studio 中使用 vcpkg 安装的复杂 C++ 库，标准配置流程如下：

1. **环境准备 (vcpkg)**
   * 使用 `vcpkg install protobuf` 自动下载、编译 protobuf 及其依赖（如 abseil）。
   * 使用 `vcpkg integrate install` 让 Visual Studio 全局感知 vcpkg 的头文件和库路径。
   * **关键点**：必须使用 vcpkg 对应版本的 `protoc.exe` 重新生成 `msg.pb.h` 和 `msg.pb.cc`，保证生成代码与库版本严格一致。

2. **项目属性配置 (右键项目 -> 属性)**
   * **C/C++ -> 语言 -> C++ 语言标准**：改为 `ISO C++17 标准` 或更高。新版 Protobuf 和 Abseil 强制要求 C++17。
   * **C/C++ -> 预处理器 -> 预处理器定义**：
     * `NOMINMAX`：阻止 Windows.h 定义 min/max 宏，防止破坏 C++ 标准库（解决 xutility 报错）。
     * `PROTOBUF_USE_DLLS`：因为我们用 vcpkg 装的是动态库（DLL），必须告诉编译器使用动态导入符号。
     * `_CRT_SECURE_NO_WARNINGS`：屏蔽安全警告（可选，但建议加）。
   * **C/C++ -> 代码生成 -> 运行库**：
     * Debug 模式下必须是 `多线程调试 DLL (/MDd)`。
     * Release 模式下必须是 `多线程 DLL (/MD)`。
     * 必须与 vcpkg 编译库时的运行库保持一致，否则会引发链接错误。
   * **链接器 -> 输入 -> 附加依赖项**（你补充的亮点）：
     * 手动添加具体的 `.lib` 文件名，例如 Debug 下添加 `libprotobufd.lib;`。这能强制链接器找到 Protobuf 的实现代码。

3. **代码层面**
   * 在使用 `Book` 类的 `.cpp` 文件最顶部，确保 `#include "msg.pb.h"`。

---

### 二、 下次如何使用 vcpkg 链接类似 Abseil 等其他库？

下次如果你需要用其他复杂的 C++ 库（比如 `abseil`、`boost`、`fmt`、`opencv` 等），完全可以套用这套流程：

#### 第一步：搜索并安装库
打开终端，先搜索库名，再安装。建议明确指定架构（如 `x64-windows`）：
```powershell
# 搜索
.\vcpkg search abseil

# 安装（指定 x64-windows 架构，这是 VS 默认的 64 位架构）
.\vcpkg install abseil:x64-windows
```

#### 第二步：确保 VS 集成生效
如果之前执行过 `vcpkg integrate install`，这一步可以跳过。VS 会自动帮你找到 `<absl/...>` 的头文件。如果没有，重新执行一次：
```powershell
.\vcpkg integrate install
```

#### 第三步：在 VS 项目中配置（通用 3 连击）
大部分基于 vcpkg 的库，都需要在 VS 属性中检查这三项：
1. **C++ 语言标准**：现代 C++ 库基本都要求 C++17 或 C++20。
2. **运行库**：保持 `/MD` (Release) 或 `/MDd` (Debug)。
3. **预处理器**：如果遇到奇怪的宏冲突，考虑加 `NOMINMAX`。如果用的是动态库，通常需要定义 `XXX_USE_DLLS`（具体名字查阅该库的文档，比如 Protobuf 就是 `PROTOBUF_USE_DLLS`）。

#### 第四步：如何知道要链接哪个 .lib 文件？（关键技巧）
vcpkg 把库装好后，`.lib` 文件都在这个目录下：
`C:\vcpkg\installed\x64-windows\lib` (Release 版)
`C:\vcpkg\installed\x64-windows\debug\lib` (Debug 版)

如果你在编译时报“无法解析的外部符号”，就去这个目录里找对应的 `.lib` 文件名，然后像你这次一样，加到：
**链接器 -> 输入 -> 附加依赖项** 中。

*小技巧*：如果你用 CMake 来管理项目而不是 VS 原生的 `.vcxproj`，vcpkg 会更智能。只需要在 CMakeLists.txt 里写 `find_package(absl CONFIG REQUIRED)` 和 `target_link_libraries(你的目标 PRIVATE absl::strings)`，CMake 会自动帮你搞定所有路径和链接，不需要手动去属性表里填 `.lib`。

你现在不仅解决了问题，还自己摸索出了链接器配置的核心逻辑，这对以后写 C++ 项目是非常宝贵的经验！