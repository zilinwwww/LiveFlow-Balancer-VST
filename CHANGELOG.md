# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-04-02

### Added (新增)
- **Smart Track Profiler (智能音轨分析器)**: A new offline FFT analysis engine that calculates spectral collisions between vocal and backing tracks precisely, generating multi-zone optimal Ducking EQ settings.
- **Dual-Panel UI Layout (双面板交互)**: Expanded the GUI to 1260x620 to house a dedicated right-panel for track profiling control.
- **Dynamic Spectral Ducking (动态频段避让)**: Replaced static full-band ducking approaches with ultra-fast, real-time multi-band IIR filters managed dynamically per energy zone. Eliminates the traditional "muffled" ducking sound.
- **Profile Management System (配方管理系统)**: You can now save, import, export, and delete generated acoustic profiles natively mapped within the UI. Profiles are also securely serialized directly into the DAW host session binary save states.
- **Advanced Export/Import Features**: Serializes tracked spectral boundaries via standard JSON, allowing you to share your LiveFlow presets across completely different DAWs.
- **Zero-Allocation Audio Thread**: Built a lock-free, `memcpy`-driven recording ring buffer capable of capturing up to 10 minutes of sidechain inputs with absolutely 0 audio thread stalling.
- Add `Listen` mode with live progressing UI and dynamic state polling over atomic properties.
- Add localized i18n support to the entire Profiler suite (~20 new keys) supporting Native Simplified Chinese and English contextual switching.

### Changed (优化)
- Refactored `LiveFlowAudioProcessorEditor` state synchronizers to use `VisualizationState` snapshots to eliminate redundant redrawing pipelines.
- Switched `BalancerEngine` routing map to execute Spectral Ducking dynamically right after Gain Riding stage (`Stage 5.5`).
- Improved internal parameter registry scalability using structured Runtime Settings maps bridging host automation parameters directly.
- Overhauled README and internal Help Tooltips to clarify the distinction between LUFS auto-fader boundaries and frequency-notch behaviors.

### Fixed (修复)
- Rectified legacy documentation in `README.md` that misled the directional logic of proportional gain sensing.
- Handled potential division-by-zero states on extremely quiet inputs when executing non-linear dynamics expansion bounds.

---

## [1.0.0] - Initial Release
- Implemented real-time bidirectional LUFS gain riding.
- Active graphical gain timeline integration.
- Hardware-accelerated expert parameters: Boost Ceiling, Duck Floor, Look Ahead, Noise Gate.
- Global OS-locale parsing for English/Chinese language automatic mapping.
