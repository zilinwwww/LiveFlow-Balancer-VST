# LiveFlow Balancer 架构与设计规范 (Design & Architecture)

## 1. 项目定位与核心痛点
LiveFlow Balancer 是一款双向实时动态响度平衡器，专为现场演出、直播与播客等实时或后期内容制作而设计。

与标准的“侧链压缩” (Sidechain Compressor) 不同，LiveFlow Balancer 不是单纯地在人声过载时“压平”伴奏，而是采用了完全智能的**目标偏离感知算法 (Target Deviation Sensing)**。由于现场演出中，演唱者的麦克风发声点不可控，使得单纯的侧链由于无法感应“声音过小”的情况，会导致伴奏压迫人声。LiveFlow 提供了一个基于感官响度 LUFS 分析的“双向推子” (Bi-directional Fader)：

- **人声过大** -> 向下压低伴奏 (Ducking)
- **人声过小** -> 向上推起伴奏 (Boosting)

## 2. 核心 DSP 算法

### 2.1 LUFS 响度分析
所有输入被捕获并使用遵循 ITU-R BS.1770-4 标准的滤波后，求取平均功率与时间窗口内的均一感知响度，从而过滤由于尖锐敲击或其他峰值信号（Peak）引发的过度抽吸感（Pumping）。

### 2.2 自动增益计算与底层侦查 (Routing & Calculation)
LiveFlow 提供了严格的 Sidechain Routing 探查：侧链总线（Sidechain Bus）默认休眠（`Optional`），只有当宿主 DAW 实际启用并发来音频信号时，才会触发分析模块。
```
ΔRMS = LUFS_vocal - (LUFS_backing - BaseAnchor)
Target Gain Change = α × ΔRMS (受 Dynamics 模型缩放校调)
```

### 2.3 滤波与频谱避让 (Spectral Ducking)
除了宏观推子控制，底层管线还对伴奏通道施加了带有动态 Q 值的带通/带阻反向滤波器截取。
它只针对人声基频活跃范围（如 300Hz ~ 5000Hz）进行避让，这意味着贝斯的低频冲击力以及镲片的高频空气感在动态回避时依旧可以保持饱满。

### 2.4 智能音轨分析 (Smart Track Profiler) [v1.1 新增]
为了解决全频段增益避让带来的“沉闷感”，v1.1 引入了 Smart Track Profiler。它的核心思想是“离线分析，实时插值”：
1. **零分配录音 (Zero-Allocation Recording):** 用户点击 Listen 后，音频线程 (Audio Thread) 采用无锁 memcpy 方式将主通道与侧链通道的音频推入预分配的大容量环形缓冲区 (最大支持 10 分钟, 约 230MB)。
2. **后台 FFT 分析 (Offline FFT Analysis):** 录制结束后，主线程唤起 Backend Background Thread。采用 1024-point FFT 和汉宁窗提取逐帧频谱，根据伴奏 LUFS 将数据划分为数个能量级别（Zones，如主歌、副歌）。在每个 Zone 内计算伴奏与人声的频谱碰撞分数 (Collision Score)，从而提取出需要避让的最优目标频段 (Center Hz, Q, Depth)。
3. **多段动态 IIR 均衡器 (Multi-band Dynamic IIR EQ):** 在 Audio Thread 的处理阶段 (Stage 5.5)，引擎通过当前 LUFS 实时在不同的 Profile Zone 之间进行插值。利用带有 SIMD 优化的极轻量级 IIR 滤波器（Biquad），根据 `VoicePresence` 包络动态地对提取出的频段施加衰减。CPU 开销极低（3 个频段 CPU 占用增加不到 0.3%）。
4. **多态存储 (Dual Storage):** 分析生成的配方（Profile）既被编码为二进制大对象直接打包在 DAW 的保存工程中，也支持导出为 JSON 文件用于跨工程共享。

## 3. UI 界面与交互构架

使用了深度定制的 JUCE Component 构建了基于 OpenGL / 现代 2D 的渲染管线，摒弃了标准库死板的控件：

- **Gain Timeline (动态增益流)**：通过捕捉循环缓冲器实现在屏幕核心绘制随时间流逝的伴奏响度曲线，实时展示推子是被抬升了还是压暗了。
- **动态语言辞典与 OS 级母语嗅探**：使用底层编译级别的 UTF-8 内存挂载方案 `Translations.h` 彻底抽离多国语言（中文 / EN）。由于使用了 `juce::SystemStats::getUserLanguage` 动态嗅探系统区域，插件在装填初次启动时，会自动吸附至对应国家的界面，而无需配置编译两份不同的二进制分发包。

## 4. 线程安全性与开发规约

- **无锁无分配**：所有 Audio Thread（ProcessBlock）内的参数变更均使用 `std::atomic` 接待或 Lock-free Queue 通信机制。绝对禁止 `new`, `malloc`, 或可能抢占 OS 互斥锁的操作。
- **Look Ahead 时延上报**：为了精准捕捉峰值爆音，LiveFlow 取用最高滞后 50ms 的预读取缓存（环形数组队列）。这部分 Latency 需要如实上报给宿主 (Host) ，以让 DAW 完成延迟补偿 (PDC)。
- **GUI 解耦**：GUI 的重绘依赖于 Timer 对 Processor State 的固定间隔拍照（Snapshot提取），即便 GUI 长时间卡顿也不会干扰音频计算。

## 5. 持续交付与打包 (CD Pipeline)

依靠 CMake 的 `CPack` 管道，我们将 VST3 分发流程固化成了可编程脚本。
利用 NSIS 构建的 Windows `LiveFlow Balancer Setup.exe` 可穿透操作系统的 `Common Files\VST3` 安全策略实现纯净的一键安装，并在向导流全自动提供底层汉化支持。不仅隔离了厂商专属注册表，还能优雅地对接 Windows 系统的 "程序和功能" 面板实现可控卸载。
