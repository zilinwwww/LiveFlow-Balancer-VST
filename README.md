# LiveFlow Balancer
A smart, bi-directional, LUFS-aware dynamic fader riding plugin. Designed to automatically balance backing tracks and vocals for live streaming, podcasting, and post-production.

![License](https://img.shields.io/badge/License-GPLv3-blue.svg)
![Version](https://img.shields.io/badge/Version-v1.0.2604a1-green.svg)

## 📌 Features
- **Smart Gain Riding (Auto-Fader)**: Bi-directional control. It lifts the backing track when the vocal is too quiet, and smoothly pushes the backing track down when the vocal hits hard.
- **Spectral Sidechain Ducking**: Preserves low-end punch (kick drums/bass) and high-end air (cymbals) when ducking, avoiding the "muddy" volume pumping artifacts associated with standard broadband sidechain compressors.
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
