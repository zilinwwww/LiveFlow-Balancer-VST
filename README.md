# LiveFlow Balancer

<details open>
<summary><strong>🇺🇸 English (Click to collapse/expand)</strong></summary>

<br>

A smart, bi-directional, LUFS-aware dynamic fader riding plugin. Designed to automatically balance backing tracks and vocals for live streaming, podcasting, and post-production.

![License](https://img.shields.io/badge/License-GPLv3-blue.svg)
![Version](https://img.shields.io/badge/Version-v1.0.2604a1-green.svg)

## 📌 Features
- **Smart Gain Riding (Auto-Fader)**: Bi-directional control. It lifts the backing track when the vocal is too quiet, and smoothly pushes the backing track down when the vocal hits hard.

- **Real-Time Visualization**: Features a FabFilter-style rolling gain timeline, providing immediate feedback on tracking hysteresis, voice presence detection, and fader movements.
- **Internationalization & Locale Detection**: Complete, zero-dependency English and Simplified Chinese user manual/UI toggling builtin. The plugin features **OS-locale Auto-Sensing**, instantly switching to the native UI based on your operating system region out-of-the-box.
- **Expert Mode Configuration**: Fine-tune look-ahead latency (up to 50ms), boost ceilings, duck floors, noise gates, and hysteresis release variables to achieve transparent broadcast-ready mixes.

## 🚀 Installation & Building

The project is built around modern CMake (>= 3.24) and natively leverages `FetchContent` to download the [JUCE 8.x](https://github.com/juce-framework/JUCE) framework dependency automatically. **No recursive Git submodule clones are needed.**

#### Windows
1. **Install Prerequisites**: 
   - Download and install [Visual Studio Community](https://visualstudio.microsoft.com/). Make sure to check **"Desktop development with C++"** during installation.
   - Download and install [CMake](https://cmake.org/download/).
2. **Open Terminal** (PowerShell or Git Bash) and run:
```powershell
git clone https://github.com/zilinwwww/LiveFlow-Balancer-VST.git
cd LiveFlow-Balancer-VST

# Configure the project for MSVC (it will automatically find your Visual Studio)
cmake -B build -DLIVEFLOW_ENABLE_STANDALONE=OFF

# Compile and automatically generate the .exe installer
cmake --build build --target package --config Release
```
*(The generated `.exe` installer will be located inside the `build` folder)*

#### macOS
1. **Install Prerequisites**: 
   - Open Terminal and install Apple compiler tools: `xcode-select --install`
   - Install CMake: `brew install cmake` (if you have Homebrew) or download it from their [website](https://cmake.org/download/).
2. **Build**:
```bash
git clone https://github.com/zilinwwww/LiveFlow-Balancer-VST.git
cd LiveFlow-Balancer-VST

# Configure the project
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Compile VST3 and AU formats
cmake --build build --config Release
```
*(Your compiled `.vst3` and `.component` plugins will be placed in `build/LiveFlowBalancer_artefacts/Release/`)*

#### Linux (Ubuntu/Debian)
1. **Install Prerequisites**: 
Open Terminal and install the required development libraries for the JUCE GUI and audio backend:
```bash
sudo apt-get update
sudo apt-get install -y cmake g++ libasound2-dev libx11-dev libxinerama-dev libxext-dev libfreetype6-dev libglu1-mesa-dev libxcursor-dev libxrender-dev libcurl4-openssl-dev libxrandr-dev libxcomposite-dev
# Depending on your Ubuntu version, install webkit (try both, one will work):
sudo apt-get install -y libwebkit2gtk-4.1-dev || sudo apt-get install -y libwebkit2gtk-4.0-dev
```
2. **Build**:
```bash
git clone https://github.com/zilinwwww/LiveFlow-Balancer-VST.git
cd LiveFlow-Balancer-VST
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## 🎛️ Usage Quick Start
1. Place **LiveFlow Balancer** on your backing track / instrumental channel.
2. Route your vocal channel into the plugin's **Sidechain Input**.
3. Adjust the `Target Balance` (e.g., +3dB) to establish the focal relationship between vocal and instrumental.
4. Set the `Anchor` to `Auto` to let LiveFlow monitor your baseline LUFS, or switch to `Manual` to point it steadily at the thickest part of your vocal dynamically.

## 📖 Architecture & Design
Refer to the `docs/Design.md` document for deeper architectural insights on multithreading parameters, lock-free queues, and DSP pipeline logic.

## 📜 License
This plugin is licensed under the GPLv3 License, subject to the inherent licensing obligations carried by the JUCE framework in open-source utilization. Please check [JUCE Licensing](https://juce.com/) for commercial deployment policies.

</details>

<details>
<summary><strong>🇨🇳 简体中文 (点击展开/折叠)</strong></summary>

<br>

一款智能、双向、感知 LUFS 响度的动态推子调节插件（Fader Riding）。专为直播、播客和后期制作自动平衡伴奏与人声音量而设计。

![License](https://img.shields.io/badge/License-GPLv3-blue.svg)
![Version](https://img.shields.io/badge/Version-v1.0.2604a1-green.svg)

## 📌 核心功能
- **智能增益调节 (Auto-Fader)**：双向控制。当人声太小，它会自动提升伴奏音量进行托底；当人声爆发时，又会平滑地将伴奏避让压低。

- **实时可视化**：提供类似 FabFilter 风格的滚动增益时间线，让你可以随时直观地掌控追踪滞后、人声检测阈值以及推子的动态游走状态。
- **国际化与多语言侦测**：内置零依赖的完全纯净的英文/简体中文使用手册与 UI 切换引擎。插件支持**操作系统语言自适应 (OS-locale Auto-Sensing)**，开箱即用，会根据操作系统的地区自动切换到你熟悉的母语界面。
- **专家模式 (Expert Mode)**：支持深度微调最多 50ms 的预读延迟 (Look-ahead)、推高天花板、避让地板、噪声门阈值以及迟滞释放等参数，助力打造广电级的透明混音作品。

## 🚀 编译与安装

项目基于现代化的 CMake (>= 3.24) 构建，并原生使用 `FetchContent` 自动全网拉取 [JUCE 8.x](https://github.com/juce-framework/JUCE) 框架依赖。**无需进行繁琐的 Git Submodule 递归克隆。**

#### Windows
1. **环境准备**: 
   - 下载并安装 [Visual Studio Community](https://visualstudio.microsoft.com/)。请确保在安装时勾选了 **"使用 C++ 的桌面开发 (Desktop development with C++)"**。
   - 下载并安装 [CMake](https://cmake.org/download/)。
2. **打开终端** (PowerShell 或 Git Bash) 然后运行:
```powershell
git clone https://github.com/zilinwwww/LiveFlow-Balancer-VST.git
cd LiveFlow-Balancer-VST

# 为 MSVC 配置项目 (它将自动寻找你的 Visual Studio)
cmake -B build -DLIVEFLOW_ENABLE_STANDALONE=OFF

# 开始编译并全自动打包出 .exe 安装包
cmake --build build --target package --config Release
```
*(生成的 `.exe` 傻瓜安装包将会由于前面写的配置而直接出现在 `build` 文件夹内)*

#### macOS
1. **环境准备**: 
   - 打开终端并安装苹果官方编译工具链: `xcode-select --install`
   - 安装 CMake: `brew install cmake` (如果你有 Homebrew)，或者直接到[官网](https://cmake.org/download/)下载安装。
2. **构建**:
```bash
git clone https://github.com/zilinwwww/LiveFlow-Balancer-VST.git
cd LiveFlow-Balancer-VST

# 配置项目引擎
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 编译生成 VST3 和 AU 格式
cmake --build build --config Release
```
*(编译好的 `.vst3` 和 `.component` 插件将被投放在 `build/LiveFlowBalancer_artefacts/Release/` 目录下)*

#### Linux (Ubuntu/Debian)
1. **环境准备**: 
打开终端，为 JUCE 引擎底层的图形 GUI 接口和音频底层安装专用的开发依赖库：
```bash
sudo apt-get update
sudo apt-get install -y cmake g++ libasound2-dev libx11-dev libxinerama-dev libxext-dev libfreetype6-dev libglu1-mesa-dev libxcursor-dev libxrender-dev libcurl4-openssl-dev libxrandr-dev libxcomposite-dev
# 根据你的 Ubuntu 内核版本，安装并适配 WebKit 接口 (以下两个尝试一个即可):
sudo apt-get install -y libwebkit2gtk-4.1-dev || sudo apt-get install -y libwebkit2gtk-4.0-dev
```
2. **构建**:
```bash
git clone https://github.com/zilinwwww/LiveFlow-Balancer-VST.git
cd LiveFlow-Balancer-VST
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## 🎛️ 快速上手
1. 将 **LiveFlow Balancer** 挂载在你的伴奏 / 乐器总轨上。
2. 把你的人声轨道作为信号发送进入该插件的 **Sidechain (侧链) 输入端**。
3. 调节 `Target Balance` (例如设置为 +3dB) 来确立人声和你期望的伴奏音量之间的绝对统治力焦点关系。
4. 将 `Anchor (锚点)` 设为 `Auto (自动)` 让 LiveFlow 自行侦测接管你的底噪 LUFS，或者拨到 `Manual (手动)` 将探测点极其稳固地指向你人声爆发最厚实的地方。

## 📖 架构设计
如果有兴趣深究的话，请查阅 `docs/Design.md` 以获取关于多线程无锁队列安全、参数传递以及 DSP 管线逻辑的终极架构视角剖析。

## 📜 开源协议 / 授权
本项目基于 GPLv3 协议开源发布。遵循 JUCE 框架在开源使用情形下的内在附带许可义务准则。如果你需要将本架构用于商业发行目的，请查阅 [JUCE 商业授权条例](https://juce.com/)。

</details>
