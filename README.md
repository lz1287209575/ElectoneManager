# ElectoneManager

跨平台音乐管理框架，专为 Yamaha ELS-02C 电子管风琴设计。支持音色触发、MIDI 双向同步、B00 音色包导入，目标延迟 < 10ms。

## 功能特性

- **Flutter UI**：跨平台图形界面（macOS / iOS），8 槽位音色管理
- **XG 音色库**：200+ Yamaha XG 音色，按分类浏览 + 中英文搜索
- **MIDI 双向同步**：App → 钢琴发送 Bank/Program，钢琴 → App 实时更新 UI
- **B00 文件导入**：解析并发送 Yamaha 音色包 SysEx 到 ELS-02C
- **SysEx 支持**：动态缓冲区，支持任意大小的 SysEx 消息
- **低延迟音频**：Core Audio / WASAPI / ASIO / ALSA / Oboe 全平台支持

---

## 架构概览

```
ElectoneManager/
├── src/
│   ├── common/          # 跨平台抽象层（接口 + 核心算法）
│   │   ├── IMidiDevice.h / IAudioEngine.h
│   │   ├── RingBuffer.h              # Lock-free SPSC ring buffer
│   │   ├── MidiToChineseInstrumentMapper.h/.cpp
│   │   └── SamplerEngine.h/.cpp      # Looping sampler + EMA 低通 + ADSR
│   ├── platform/
│   │   ├── apple/       # Core MIDI + Core Audio（macOS / iOS）
│   │   ├── windows/     # WinMM MIDI + ASIO / WASAPI
│   │   ├── linux/       # ALSA Seq MIDI + ALSA PCM
│   │   └── android/     # AMidi NDK + Oboe
│   ├── ios/             # C 桥接层 + iOS Swift 应用
│   │   ├── em_bridge.h/.cpp          # 90+ extern "C" 函数
│   │   ├── Engine/EngineController.swift
│   │   ├── App/ElectoneManagerApp.swift
│   │   └── Views/                   # ContentView, VoicePage, ScorePage, RhythmPage
│   └── ui/              # FLTK 桌面 UI（macOS/Linux/Windows）
└── flutter_app/         # Flutter 跨平台 UI
    ├── lib/
    │   ├── bindings/    # FFI 桥接（em_bridge.dart）
    │   ├── models/      # 数据模型
    │   ├── providers/   # Riverpod 状态管理
    │   ├── screens/     # 页面（home, voice_editor 等）
    │   └── utils/       # XG 音色库（constants.dart）、B00 解析器
    └── macos/           # macOS Runner + 原生库框架
```

### 线程模型

| 线程 | 职责 |
|------|------|
| MIDI 线程（平台驱动） | 接收 MIDI 数据，推入 lock-free RingBuffer |
| Processing 线程 | 消费 RingBuffer → Mapper → 更新 SamplerEngine 参数 |
| Audio RT 线程（平台驱动） | `SamplerEngine::render()` → 输出 PCM |
| Flutter UI 线程 | 60Hz 轮询引擎状态，响应用户操作 |

---

## 构建：macOS（Flutter + C++ 引擎）

**依赖**：CMake ≥ 3.21、Flutter 3.41+、Xcode 15+

```bash
# 1. 编译 C++ 共享库
bash scripts/build_macos_lib.sh

# 2. 运行 Flutter 应用
cd flutter_app
flutter run -d macos
```

**或直接构建原生 C++ 可执行文件**：

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release \
      -DEM_BUFFER_SIZE=256 -DEM_SAMPLE_RATE=48000
cmake --build build -j$(sysctl -n hw.ncpu)
./build/ElectoneManager
```

---

## 构建：iOS（Swift + C++ 引擎）

**依赖**：Xcode 15+、iOS 17+、xcodegen

```bash
# 1. 编译 C++ 静态库（device 或 simulator）
bash scripts/build_ios_lib.sh

# 2. 生成 Xcode 项目
cd ios && xcodegen generate

# 3. 用 Xcode 打开并运行
open ios/ElectoneManager.xcodeproj
```

---

## 构建：Windows

**依赖**：CMake ≥ 3.21、Visual Studio 2022（MSVC C++17）  
**可选**：ASIO SDK（极限低延迟）→ 解压到 `third_party/asiosdk/`

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 `
      -DEM_BUFFER_SIZE=256 -DEM_SAMPLE_RATE=48000
cmake --build build --config Release
.\build\Release\ElectoneManager.exe
```

---

## 构建：Linux

```bash
sudo apt install cmake libasound2-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release \
      -DEM_BUFFER_SIZE=256 -DEM_SAMPLE_RATE=48000
cmake --build build -j$(nproc)
./build/ElectoneManager
```

> 建议直接使用 ALSA，避开 PulseAudio/PipeWire 混音延迟。

---

## 构建：Android

**依赖**：Android NDK r25+（API 29+）、Oboe 库

```bash
git submodule add https://github.com/google/oboe third_party/oboe
cmake -B build-android \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-29 \
  -DEM_ANDROID=1
cmake --build build-android -j$(nproc)
```

---

## 关键参数

| 参数 | 默认值 | CMake 变量 |
|------|--------|------------|
| 缓冲大小 | 256 frames（~5.3ms） | `-DEM_BUFFER_SIZE=256` |
| 采样率 | 48000 Hz | `-DEM_SAMPLE_RATE=48000` |

---

## MIDI 双向同步

### App → 钢琴
- 在 Flutter UI 选择音色后，自动发送 **Bank Select（CC#0 + CC#32）+ Program Change**
- 支持 200+ XG 音色，按 12 个分类（Piano / Strings / Brass 等）浏览

### 钢琴 → App
- 实时接收 8 个 MIDI 通道（对应 8 个槽位）的 Program Change
- UI 自动更新对应槽位的音色显示

### B00 文件导入
1. 点击首页右上角文件夹图标
2. 选择 `.B00` 文件（Yamaha 音色包）
3. 确认后自动解析并发送所有 SysEx 消息到 ELS-02C

---

## 核心算法

### 14-bit Pitch Bend 解析
```
raw   = (MSB << 7) | LSB        // 0–16383，中心 8192
value = (raw - 8192) / 8192.0   // -1.0 to +1.0
```

### 低通滤波器（SamplerEngine）
一阶 EMA：`output = α × input + (1-α) × prev`，截止频率由 CC#74（Brightness）控制。

---

## 平台注意事项

| 平台 | 注意 |
|------|------|
| macOS | `libem_engine.dylib` 需在 app bundle 内或 `DYLD_LIBRARY_PATH` 中 |
| Windows | ASIO SDK 需手动下载放到 `third_party/asiosdk/` |
| iOS | `config/Info.plist` 已包含麦克风 + 蓝牙 MIDI 权限 |
| Android | `config/AndroidManifest.xml` 已包含 RECORD_AUDIO + MIDI 权限 |
| Linux | 建议解锁 `RLIMIT_MEMLOCK` 或以 `CAP_SYS_NICE` 运行以允许 `SCHED_FIFO` |
