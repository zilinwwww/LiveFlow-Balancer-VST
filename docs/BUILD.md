# LiveFlow Balancer Build Notes

## Requirements

- CMake 3.24+
- A C++20 compiler
- JUCE is fetched automatically via `FetchContent` (No submodules needed)
- Windows: InnoSetup / NSIS depending on CMake packaging target.
- Linux: X11, Xrandr, Xinerama, Xcursor, Xext, Freetype, ALSA, JACK/PulseAudio development packages as needed by JUCE
- macOS: Xcode toolchain for `VST3` and `AU`

## Configure & Build

### macOS & Linux

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build VST3 (and AU on macOS)
cmake --build build --config Release
```

### Windows (CPack + NSIS Installer)

We have implemented an automated NSIS installer pipeline for Windows users to gracefully handle the VST3 deployment and Control Panel uninstaller registration.

```powershell
# Configure for MSVC focusing entirely on VST3 format
cmake -B build -G "Visual Studio 18 2026" -A x64 -DLIVEFLOW_ENABLE_AU=OFF -DLIVEFLOW_ENABLE_AAX=OFF -DLIVEFLOW_ENABLE_STANDALONE=OFF

# Automatically compile the C++ logic AND generate the installer
cmake --build build --target package --config Release
```

On macOS, `LiveFlowBalancer_AU` is also available when `LIVEFLOW_ENABLE_AU=ON`.

## Output Locations

- Windows Installer: `build-win/LiveFlow Balancer-v1.0.2604a1-win64.exe`
- Raw VST3 File: `build/LiveFlowBalancer_artefacts/Release/VST3/LiveFlow Balancer.vst3`

## Optional Targets

- `-DLIVEFLOW_ENABLE_AAX=ON -DAAX_SDK_PATH=/path/to/AAX_SDK`
- `-DLIVEFLOW_COPY_PLUGIN_AFTER_BUILD=ON` to install the plugin into the default user folder after a successful build
